#include "ptzpdriver.h"
#include "debug.h"
#include "net/remotecontrol.h"

#include "drivers/patrolng.h"
#include "drivers/patternng.h"
#include "drivers/presetng.h"

#include "ptzp/ptzphead.h"
#include "ptzp/ptzptransport.h"

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <QTimer>
#include <errno.h>

#ifdef HAVE_PTZP_GRPC_API
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc/grpc.h>

#include <grpc++/security/server_credentials.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerWriter;
using grpc::Status;
using namespace std;

class GRpcThread : public QThread
{
public:
	GRpcThread(int port, PtzpDriver *driver)
	{
		ep = QString("0.0.0.0:%1").arg(port);
		service = driver;
	}

	void run()
	{
		mDebug("Grpc starting with %s", qPrintable(ep));
		string ep(this->ep.toStdString());
		ServerBuilder builder;
		builder.AddListeningPort(ep, grpc::InsecureServerCredentials());
		builder.RegisterService(service);
		std::unique_ptr<Server> server(builder.BuildAndStart());
		server->Wait();
	}
	QString ep;
	PtzpDriver *service;
};
#endif

PtzpDriver::PtzpDriver(QObject *parent)
	: QObject(parent), gpiocont(new GpioController)
{
	doStartupProcess = true;
	driverEnabled = true;
	sreg.enable = false;
	sreg.zoomHead = NULL;
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), SLOT(timeout()));
	timer->start(10);
	defaultPTHead = NULL;
	defaultModuleHead = NULL;
	ptrn = new PatternNg(this);
	elaps = new QElapsedTimer();
	elaps->start();
	regsavet = new QElapsedTimer();
	setRegisterSaving(false, 60000);
	changeOverlay = true;
}

int PtzpDriver::getHeadCount()
{
	int count = 0;
	while (getHead(count++)) {}
	return count - 1;
}

/**
 * @brief PtzpDriver::startSocketApi
 * Kamerada soket bağlantısı için kullanılacak port numarasının
 * bildirildiği metod.Bu projede bağlantı için genel olarak "8945"
 * numaralı port kullanılmaktadır.
 * @param port
 */

void PtzpDriver::startSocketApi(quint16 port)
{
	RemoteControl *rcon = new RemoteControl(this, this);
	rcon->listen(QHostAddress::Any, port);
}

int PtzpDriver::startGrpcApi(quint16 port)
{
#ifdef HAVE_PTZP_GRPC_API
	GRpcThread *thr = new GRpcThread(port, this);
	thr->start();
	return 0;
#else
	return -ENOENT;
#endif
}

QVariant PtzpDriver::get(const QString &key)
{
	mInfo("Get func: %s", qPrintable(key));
	if (defaultPTHead == NULL || defaultModuleHead == NULL)
		return QVariant();

	if (key == "ptz.all_pos")
		return QString("%1,%2,%3")
			.arg(defaultPTHead->getPanAngle())
			.arg(defaultPTHead->getTiltAngle())
			.arg(defaultModuleHead->getZoom());
	else if (key == "ptz.head.count")
		return getHeadCount();
	/*
	 * head status kullanımı;
	 * ptz.head.head_index(0,1,2 etc.).status
	 */
	else if (key.startsWith("ptz.head.")) {
		QStringList fields = key.split(".");
		if (fields.size() <= 2)
			return -EINVAL;
		int index = fields[2].toInt();
		fields.removeFirst();
		fields.removeFirst();
		fields.removeFirst();
		if (index < 0 || index >= getHeadCount())
			return -ENONET;
		return headInfo(fields.join("."), getHead(index));
	} else if (key == "preset_list")
		return PresetNg::getInstance()->getList();
	else if (key == "patrol_list")
		return PatrolNg::getInstance()->getList();
	else if (key == "pattern_list")
		return ptrn->getList();
	return "almost_there";
	return QVariant();
}

int PtzpDriver::set(const QString &key, const QVariant &value)
{
	PatrolNg *ptrl = PatrolNg::getInstance();
	PresetNg *prst = PresetNg::getInstance();
	mInfo("Set func: %s %d", qPrintable(key), value.toInt());
	if (defaultPTHead == NULL || defaultModuleHead == NULL)
		return -ENODEV;

	if (key == "ptz.cmd.pan_left") {
		commandUpdate(PtzControlInterface::C_PAN_LEFT, value.toFloat(), 0);
		defaultPTHead->panLeft(value.toFloat());
	} else if (key == "ptz.cmd.pan_right") {
		commandUpdate(PtzControlInterface::C_PAN_RIGHT, value.toFloat(), 0);
		defaultPTHead->panRight(value.toFloat());
	} else if (key == "ptz.cmd.tilt_down") {
		commandUpdate(PtzControlInterface::C_TILT_DOWN, value.toFloat(), 0);
		defaultPTHead->tiltDown(value.toFloat());
	} else if (key == "ptz.cmd.tilt_up") {
		commandUpdate(PtzControlInterface::C_TILT_UP, value.toFloat(), 0);
		defaultPTHead->tiltUp(value.toFloat());
	} else if (key == "ptz.cmd.pan_stop") {
		commandUpdate(PtzControlInterface::C_PAN_TILT_STOP, value.toInt(), 0);
		defaultPTHead->panTiltAbs(0, 0);
	} else if (key == "ptz.cmd.pan_tilt_abs") {
		const QStringList &vals = value.toString().split(";");
		if (vals.size() < 2)
			return -EINVAL;
		float pan = vals[0].toFloat();
		float tilt = vals[1].toFloat();
		defaultPTHead->panTiltAbs(pan, tilt);
	} else if (key == "ptz.cmd.zoom_in") {
		commandUpdate(PtzControlInterface::C_ZOOM_IN, value.toFloat(), 0);
		defaultModuleHead->startZoomIn(value.toInt());
	} else if (key == "ptz.cmd.zoom_out") {
		commandUpdate(PtzControlInterface::C_ZOOM_OUT, value.toFloat(), 0);
		defaultModuleHead->startZoomOut(value.toInt());
	} else if (key == "ptz.cmd.zoom_stop") {
		commandUpdate(PtzControlInterface::C_ZOOM_STOP, value.toFloat(), 0);
		defaultModuleHead->stopZoom();
	} else if (key == "pattern_start")
		ptrn->start(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(),
					defaultModuleHead->getZoom());
	else if (key == "pattern_stop")
		ptrn->stop(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(),
				   defaultModuleHead->getZoom());
	else if (key == "pattern_save")
		ptrn->save(value.toString());
	else if (key == "pattern_load")
		ptrn->load(value.toString());
	else if (key == "pattern_replay")
		ptrn->replay();
	else if (key == "pattern_delete")
		ptrn->deletePattern(value.toString());
	else if (key == "preset_save")
		prst->addPreset(value.toString(), defaultPTHead->getPanAngle(),
						defaultPTHead->getTiltAngle(),
						defaultModuleHead->getZoom());
	else if (key == "preset_delete")
		prst->deletePreset(value.toString());
	else if (key == "preset_goto") {
		QStringList pos = prst->getPreset(value.toString());
		if (!pos.isEmpty())
			goToPosition(pos.at(0).toFloat(), pos.at(1).toFloat(),
						 pos.at(2).toInt());
		else
			return -EINVAL;
	} else if (key == "patrol_list") { // kerim@p1,p2,p3
		QString presets = value.toString();
		if (presets.contains("@")) {
			QString name = presets.split("@").first().remove(" ");
			presets.remove(QString("%1@").arg(name));
			if (presets.contains(","))
				ptrl->addPatrol(name, presets.split(","));
		}
	} else if (key == "patrol_interval") {
		QString intervals = value.toString();
		if (intervals.contains("@")) {
			QString name = intervals.split("@").first().remove(" ");
			intervals.remove(QString("%1@").arg(name));
			if (intervals.contains(","))
				ptrl->addInterval(name, intervals.split(","));
		}
	} else if (key == "patrol_delete") {
		ptrl->deletePatrol(value.toString());
	} else if (key == "patrol_state_run") {
		ptrl->setPatrolStateRun(value.toString());
	} else if (key == "patrol_state_stop") {
		ptrl->setPatrolStateStop(value.toString());
	} else
		return -ENOENT;

	return 0;
}

void PtzpDriver::goToPosition(float p, float t, int z)
{
	if (defaultModuleHead && defaultPTHead) {
		defaultModuleHead->setZoom(z);
		defaultPTHead->panTiltGoPos(p, t);
	}
}

float PtzpDriver::getPanAngle()
{
	return defaultPTHead->getPanAngle();
}

float PtzpDriver::getTiltAngle()
{
	return defaultPTHead->getTiltAngle();
}

void PtzpDriver::sendCommand(int c, float par1, float par2)
{
	Q_UNUSED(par2);
	if (defaultPTHead) {
		if (c == PtzControlInterface::C_PAN_LEFT)
			defaultPTHead->panLeft(par1);
		else if (c == PtzControlInterface::C_PAN_RIGHT)
			defaultPTHead->panRight(par1);
		else if (c == PtzControlInterface::C_TILT_DOWN)
			defaultPTHead->tiltDown(par1);
		else if (c == PtzControlInterface::C_TILT_UP)
			defaultPTHead->tiltUp(par1);
		else if (c == PtzControlInterface::C_PAN_TILT_STOP)
			defaultPTHead->panTiltStop();
		else if (c == PtzControlInterface::C_PAN_TILT_ABS_MOVE)
			defaultPTHead->panTiltAbs(par1, par2);
	}
	if (defaultModuleHead) {
		if (c == PtzControlInterface::C_ZOOM_IN)
			defaultModuleHead->startZoomIn((int)par1);
		else if (c == PtzControlInterface::C_ZOOM_OUT)
			defaultModuleHead->startZoomOut((int)par1);
		else if (c == PtzControlInterface::C_ZOOM_STOP)
			defaultModuleHead->stopZoom();
	}
}

void PtzpDriver::setSpeedRegulation(PtzpDriver::SpeedRegulation r)
{
	sreg = r;
}

PtzpDriver::SpeedRegulation PtzpDriver::getSpeedRegulation()
{
	return sreg;
}

void PtzpDriver::enableDriver(bool value)
{
	driverEnabled = value;
}

void PtzpDriver::manageRegisterSaving()
{
	if (!registerSavingEnabled)
		return;
	if (regsavet->elapsed() < registerSavingIntervalMsecs)
		return;
	for (int i = 0; i < getHeadCount(); i++)
		getHead(i)->saveRegisters(QString("head%1.json").arg(i));
	regsavet->restart();
}

void PtzpDriver::setRegisterSaving(bool enabled, int intervalMsecs)
{
	if (!intervalMsecs)
		intervalMsecs = 60000;
	regsavet->restart();
	registerSavingEnabled = enabled;
	registerSavingIntervalMsecs = intervalMsecs;
}

void PtzpDriver::setPatternHandler(PatternNg *p)
{
	if (ptrn)
		delete ptrn;
	ptrn = p;
}

int PtzpDriver::setZoomOverlay()
{
	return 0;
}

int PtzpDriver::setOverlay(QString data)
{
	Q_UNUSED(data);
	return 0;
}

QJsonObject PtzpDriver::doExtraDeviceTests()
{
	return QJsonObject();
}

void PtzpDriver::getStartupProcess()
{
	QString filename = "startup_process.json";
	QFile f(filename);
	if (!f.open(QIODevice::ReadOnly)) {
		mDebug("File opening error %s", qPrintable(f.fileName()));
		return;
	}
	QByteArray ba = f.readAll();
	f.close();
	QJsonObject obj;
	QJsonDocument doc = QJsonDocument::fromJson(ba);
	obj = doc.object();
	if (!obj.contains("startup"))
		return;
	QString type = obj.value("startup").toObject().value("type").toString();
	QString name = obj.value("startup").toObject().value("name").toString();
	mDebug("Startup process definated, %s, %s", qPrintable(type),
		   qPrintable(name));
	if (type == "pattern") {
		mDebug("Pattern type founded starting...");
		if (!ptrn->getList().contains(name)) {
			mDebug("Pattern undefined");
			return;
		}
		ptrn->load(name);
		ptrn->replay();
	} else if (type == "patrol") {
		PatrolNg *ptrl = PatrolNg::getInstance();
		if (!ptrl->getList().contains(name)) {
			mDebug("Patrol undefined");
			return;
		}
		runPatrol(name);
	} else if (type == "preset") {

	} else {
		mDebug("Startup process type(%s) cannot found", qPrintable(type));
	}
	return;
}

void PtzpDriver::addStartupProcess(const QString &type, const QString name)
{
	removeStartupProcess();
	QString filename = "startup_process.json";
	QFile f(filename);
	if (!f.open(QIODevice::WriteOnly)) {
		mDebug("File opening error %s", qPrintable(f.fileName()));
		return;
	}
	QJsonObject obj;
	QJsonObject startObj;
	startObj.insert("type", type);
	startObj.insert("name", name);
	obj.insert("startup", startObj);
	QJsonDocument doc;
	doc.setObject(obj);
	f.write(doc.toJson());
	f.close();
	return;
}

void PtzpDriver::removeStartupProcess()
{
	QDir d;
	d.remove("startup_process.json");
}

#ifdef HAVE_PTZP_GRPC_API
grpc::Status PtzpDriver::GetHeads(grpc::ServerContext *context,
								  const google::protobuf::Empty *request,
								  ptzp::PtzHeadInfo *response)
{
	Q_UNUSED(context);
	Q_UNUSED(request);

	int count = 0;
	PtzpHead *head;
	while ((head = getHead(count))) {
		ptzp::PtzHead *h = response->add_list();
		h->set_head_id(count);
		h->set_status(
			(ptzp::PtzHead_Status)
				head->getHeadStatus()); // TODO: proto is not in sync with code

		int cap = head->getCapabilities();
		if ((cap & PtzpHead::CAP_PAN) == PtzpHead::CAP_PAN)
			h->add_capabilities(ptzp::PtzHead_Capability_PAN);
		if ((cap & PtzpHead::CAP_TILT) == PtzpHead::CAP_TILT)
			h->add_capabilities(ptzp::PtzHead_Capability_TILT);
		if ((cap & PtzpHead::CAP_ZOOM) == PtzpHead::CAP_ZOOM)
			h->add_capabilities(ptzp::PtzHead_Capability_ZOOM);
		if ((cap & PtzpHead::CAP_ADVANCED) == PtzpHead::CAP_ADVANCED)
			h->add_capabilities(ptzp::PtzHead_Capability_ADVANCED);

		ptzp::Settings *s = h->mutable_head_settings();
		s->set_json(mapToJson(head->getSettings()));
		s->set_head_id(count);

		count++;
	}

	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PanLeft(grpc::ServerContext *context,
								 const ::ptzp::PtzCmdPar *request,
								 ::ptzp::PtzCommandResult *response)
{

	Q_UNUSED(context);
	int idx = request->head_id();
	float speed = request->pan_speed();

	PtzpHead *head = getHead(idx);
	if (head == NULL ||
		((head->getCapabilities() & PtzpHead::CAP_PAN) != PtzpHead::CAP_PAN)) {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}
	if (sreg.enable && sreg.zoomHead)
		speed = regulateSpeed(speed, sreg.zoomHead->getZoom());

	float maxSpeed = head->getMaxPatternSpeed();
	if (ptrn->isRecording() && maxSpeed != -EINVAL) {
		if (speed > maxSpeed)
			speed = maxSpeed;
	}

	commandUpdate(PtzControlInterface::C_PAN_LEFT, speed, 0);
	head->panLeft(speed);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PanRight(grpc::ServerContext *context,
								  const ::ptzp::PtzCmdPar *request,
								  ::ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	int idx = request->head_id();
	float speed = request->pan_speed();

	PtzpHead *head = getHead(idx);
	if (head == NULL ||
		((head->getCapabilities() & PtzpHead::CAP_PAN) != PtzpHead::CAP_PAN)) {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}
	if (sreg.enable && sreg.zoomHead)
		speed = regulateSpeed(speed, sreg.zoomHead->getZoom());

	float maxSpeed = head->getMaxPatternSpeed();
	if (ptrn->isRecording() && maxSpeed != -EINVAL) {
		if (speed > maxSpeed)
			speed = maxSpeed;
	}

	commandUpdate(PtzControlInterface::C_PAN_RIGHT, speed, 0);
	head->panRight(speed);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PanStop(grpc::ServerContext *context,
								 const ::ptzp::PtzCmdPar *request,
								 ::ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	int idx = request->head_id();

	PtzpHead *head = getHead(idx);
	if (head == NULL ||
		((head->getCapabilities() && PtzpHead::CAP_PAN) != PtzpHead::CAP_PAN)) {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}
	commandUpdate(PtzControlInterface::C_PAN_TILT_STOP, 0, 0);
	head->panTiltStop();

	return grpc::Status::OK;
}

grpc::Status PtzpDriver::ZoomIn(grpc::ServerContext *context,
								const ::ptzp::PtzCmdPar *request,
								::ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	int idx = request->head_id();
	float speed = request->zoom_speed();

	PtzpHead *head = getHead(idx);

	if (head == NULL || ((head->getCapabilities() & PtzpHead::CAP_ZOOM) !=
						 PtzpHead::CAP_ZOOM)) {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}
	commandUpdate(PtzControlInterface::C_ZOOM_IN, speed, 0);
	head->startZoomIn(speed);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::ZoomOut(grpc::ServerContext *context,
								 const ::ptzp::PtzCmdPar *request,
								 ::ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	int idx = request->head_id();
	float speed = request->zoom_speed();

	PtzpHead *head = getHead(idx);

	if (head == NULL || ((head->getCapabilities() & PtzpHead::CAP_ZOOM) !=
						 PtzpHead::CAP_ZOOM)) {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}
	commandUpdate(PtzControlInterface::C_ZOOM_OUT, speed, 0);
	head->startZoomOut(speed);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::ZoomStop(grpc::ServerContext *context,
								  const ::ptzp::PtzCmdPar *request,
								  ::ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	int idx = request->head_id();

	PtzpHead *head = getHead(idx);
	if (head == NULL || ((head->getCapabilities() & PtzpHead::CAP_ZOOM) !=
						 PtzpHead::CAP_ZOOM)) {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}
	commandUpdate(PtzControlInterface::C_ZOOM_STOP, 0, 0);
	head->stopZoom();
	setChangeOverlayState(true);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::TiltUp(grpc::ServerContext *context,
								const ptzp::PtzCmdPar *request,
								ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	int idx = request->head_id();
	float speed = request->tilt_speed();

	PtzpHead *head = getHead(idx);
	if (head == NULL || ((head->getCapabilities() & PtzpHead::CAP_TILT) !=
						 PtzpHead::CAP_TILT)) {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}

	if (sreg.enable && sreg.zoomHead)
		speed = regulateSpeed(speed, sreg.zoomHead->getZoom());

	float maxSpeed = head->getMaxPatternSpeed();
	if (ptrn->isRecording() && maxSpeed != -EINVAL) {
		if (speed > maxSpeed)
			speed = maxSpeed;
	}

	commandUpdate(PtzControlInterface::C_TILT_UP, speed, 0);
	head->tiltUp(speed);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::TiltDown(grpc::ServerContext *context,
								  const ptzp::PtzCmdPar *request,
								  ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	int idx = request->head_id();
	float speed = request->tilt_speed();

	PtzpHead *head = getHead(idx);
	if (head == NULL || ((head->getCapabilities() & PtzpHead::CAP_TILT) !=
						 PtzpHead::CAP_TILT)) {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}

	if (sreg.enable && sreg.zoomHead)
		speed = regulateSpeed(speed, sreg.zoomHead->getZoom());

	float maxSpeed = head->getMaxPatternSpeed();
	if (ptrn->isRecording() && maxSpeed != -EINVAL) {
		if (speed > maxSpeed)
			speed = maxSpeed;
	}

	commandUpdate(PtzControlInterface::C_TILT_DOWN, speed, 0);
	head->tiltDown(speed);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::TiltStop(grpc::ServerContext *context,
								  const ptzp::PtzCmdPar *request,
								  ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	int idx = request->head_id();

	PtzpHead *head = getHead(idx);
	if (head == NULL || ((head->getCapabilities() & PtzpHead::CAP_TILT) !=
						 PtzpHead::CAP_TILT)) {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}
	commandUpdate(PtzControlInterface::C_PAN_TILT_STOP, 0, 0);
	head->panTiltStop();
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PanTilt2Pos(grpc::ServerContext *context,
									 const ptzp::PtzCmdPar *request,
									 ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	int idx = request->head_id();
	float tilt = request->tilt_abs();
	float pan = request->pan_abs();

	PtzpHead *head = getHead(idx);
	if (!head)
		return grpc::Status::CANCELLED;
	int cap = head->getCapabilities();
	if (head == NULL || (((cap & PtzpHead::CAP_PAN) != PtzpHead::CAP_PAN) ||
						 ((cap & PtzpHead::CAP_TILT) != PtzpHead::CAP_TILT))) {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}
	commandUpdate(PtzControlInterface::C_PAN_TILT_GOTO_POS, 0, 0);
	head->panTiltGoPos(pan, tilt);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PanTiltAbs(grpc::ServerContext *context,
									const ptzp::PtzCmdPar *request,
									ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	int idx = request->head_id();
	float tilt = request->tilt_abs();
	float pan = request->pan_abs();

	PtzpHead *head = getHead(idx);
	int cap = head->getCapabilities();
	if (head == NULL || (((cap & PtzpHead::CAP_PAN) != PtzpHead::CAP_PAN) ||
						 ((cap & PtzpHead::CAP_TILT) != PtzpHead::CAP_TILT))) {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}

	float signPan = (pan > 0) ? 1.0 : -1.0;
	float signTilt = (tilt > 0) ? 1.0 : -1.0;
	pan = qAbs(pan);
	tilt = qAbs(tilt);

	if (sreg.enable && sreg.zoomHead) {
		pan = regulateSpeed(pan, sreg.zoomHead->getZoom());
		tilt = regulateSpeed(tilt, sreg.zoomHead->getZoom());
	}

	float maxSpeed = head->getMaxPatternSpeed();
	if (ptrn->isRecording() && maxSpeed != -EINVAL) {
		if (pan > maxSpeed)
			pan = maxSpeed;
		if (tilt > maxSpeed)
			tilt = maxSpeed;
	}

	pan *= signPan;
	tilt *= signTilt;

	commandUpdate(PtzControlInterface::C_PAN_TILT_ABS_MOVE, pan, tilt);
	head->panTiltAbs(pan, tilt);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::GetPTZPosInfo(grpc::ServerContext *context,
									   const ptzp::PtzCmdPar *request,
									   ptzp::PTZPosInfo *response)
{
	Q_UNUSED(context);

	int idx = request->head_id();

	PtzpHead *head = getHead(idx);

	if (head == NULL)
		return grpc::Status::CANCELLED;

	int cap = head->getCapabilities();

	response->set_pan_pos(0);
	response->set_tilt_pos(0);
	response->set_zoom_pos(0);
	if ((cap & PtzpHead::CAP_PAN) == PtzpHead::CAP_PAN)
		response->set_pan_pos(head->getPanAngle());
	if ((cap & PtzpHead::CAP_TILT) == PtzpHead::CAP_TILT)
		response->set_tilt_pos(head->getTiltAngle());
	if ((cap & PtzpHead::CAP_ZOOM) == PtzpHead::CAP_ZOOM) {
		response->set_zoom_pos(head->getZoom());
		float fovh = -1, fovv = -1;
		head->getFOV(fovh, fovv);
		response->set_fovh(fovh);
		response->set_fovv(fovv);
	}

	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PresetGo(grpc::ServerContext *context,
								  const ptzp::PresetCmd *request,
								  ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	Q_UNUSED(response);
	stopAnyProcess();
	QStringList li = PresetNg::getInstance()->getPreset(
		QString::fromStdString(request->preset_name()));
	if (li.isEmpty())
		return grpc::Status::CANCELLED;
	goToPosition(li[0].toFloat(), li[1].toFloat(), li[2].toInt());
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PresetDelete(grpc::ServerContext *context,
									  const ptzp::PresetCmd *request,
									  ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	int err = PresetNg::getInstance()->deletePreset(
		QString::fromStdString(request->preset_name()));
	response->set_err(err);
	if (err < 0)
		return grpc::Status::CANCELLED;
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PresetSave(grpc::ServerContext *context,
									const ptzp::PresetCmd *request,
									ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	Q_UNUSED(response);
	stopAnyProcess();
	if (defaultPTHead && defaultModuleHead)
		PresetNg::getInstance()->addPreset(
			QString::fromStdString(request->preset_name()),
			defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(),
			defaultModuleHead->getZoom());
	else
		return grpc::Status::CANCELLED;
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PresetGetList(grpc::ServerContext *context,
									   const ptzp::PresetCmd *request,
									   ptzp::PresetList *response)
{
	Q_UNUSED(context);
	Q_UNUSED(request);
	response->set_list(PresetNg::getInstance()->getList().toStdString());
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatrolSave(grpc::ServerContext *context,
									const ptzp::PatrolCmd *request,
									ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	Q_UNUSED(response);
	stopAnyProcess();
	auto patrolNg = PatrolNg::getInstance();
	int err = patrolNg->addPatrol(
		QString::fromStdString(request->patrol_name()),
		commaToList(QString::fromStdString(request->preset_list())));
	if (err < 0)
		return grpc::Status::CANCELLED;
	patrolNg->addInterval(
		QString::fromStdString(request->patrol_name()),
		commaToList(QString::fromStdString(request->interval_list())));

	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatrolRun(grpc::ServerContext *context,
								   const ptzp::PatrolCmd *request,
								   ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	stopAnyProcess();
	addStartupProcess("patrol", QString::fromStdString(request->patrol_name()));
	int ret = runPatrol(QString::fromStdString(request->patrol_name()));
	response->set_err(ret);
	if (ret < 0)
		return grpc::Status::CANCELLED;
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatrolDelete(grpc::ServerContext *context,
									  const ptzp::PatrolCmd *request,
									  ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	int err = PatrolNg::getInstance()->deletePatrol(
		QString::fromStdString(request->patrol_name()));
	response->set_err(err);
	if (err < 0)
		return grpc::Status::CANCELLED;
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatrolStop(grpc::ServerContext *context,
									const ptzp::PatrolCmd *request,
									ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	removeStartupProcess();
	PatrolNg::getInstance()->setPatrolStateStop(
		QString::fromStdString(request->patrol_name()));
	response->set_err(0);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatrolGetList(grpc::ServerContext *context,
									   const ptzp::PatrolCmd *request,
									   ptzp::PresetList *response)
{
	Q_UNUSED(context);
	Q_UNUSED(request);
	response->set_list(PatrolNg::getInstance()->getList().toStdString());
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatrolGetDetails(grpc::ServerContext *context,
										  const ptzp::PatrolCmd *request,
										  ptzp::PatrolDefinition *response)
{
	Q_UNUSED(context);
	QString name = QString::fromStdString(request->patrol_name());
	PatrolNg::patrolType patrol = PatrolNg::getInstance()->getPatrolDef(name);
	for (int i = 0; i < patrol.size(); i++) {
		const QPair<QString, int> preset = patrol[i];
		std::string *pname = response->add_presets();
		pname->append(preset.first.toStdString());
		response->add_intervals(preset.second);
	}
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatternRun(grpc::ServerContext *context,
									const ptzp::PatternCmd *request,
									ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	stopAnyProcess();
	if (ptrn->load(QString::fromStdString(request->pattern_name())) == 0) {
		addStartupProcess("pattern",
						  QString::fromStdString(request->pattern_name()));
		int ret = ptrn->replay();
		response->set_err(ret);
		return grpc::Status::OK;
	}
	response->set_err(ENODEV);
	return grpc::Status::CANCELLED;
}

grpc::Status PtzpDriver::PatternStop(grpc::ServerContext *context,
									 const ptzp::PatternCmd *request,
									 ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	Q_UNUSED(request);
	if (defaultPTHead && defaultModuleHead) {
		removeStartupProcess();
		ptrn->stop(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(),
				   defaultModuleHead->getZoom());
	} else {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}
	response->set_err(0);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatternStartRecording(grpc::ServerContext *context,
											   const ptzp::PatternCmd *request,
											   ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	Q_UNUSED(request);
	stopAnyProcess();
	if (defaultPTHead && defaultModuleHead)
		ptrn->start(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(),
					defaultModuleHead->getZoom());
	else {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}
	response->set_err(0);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatternStopRecording(grpc::ServerContext *context,
											  const ptzp::PatternCmd *request,
											  ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	response->set_err(-1);
	if (defaultPTHead && defaultModuleHead)
		ptrn->stop(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(),
				   defaultModuleHead->getZoom());
	else
		return grpc::Status::CANCELLED;
	if (ptrn->save(QString::fromStdString(request->pattern_name())) == 0) {
		response->set_err(0);
		return grpc::Status::OK;
	}
	return grpc::Status::CANCELLED;
}

grpc::Status PtzpDriver::PatternDelete(grpc::ServerContext *context,
									   const ptzp::PatternCmd *request,
									   ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	int err =
		ptrn->deletePattern(QString::fromStdString(request->pattern_name()));
	response->set_err(err);
	if (err < 0)
		return grpc::Status::CANCELLED;
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatternGetList(grpc::ServerContext *context,
										const ptzp::PatternCmd *request,
										ptzp::PresetList *response)
{
	Q_UNUSED(context);
	Q_UNUSED(request);
	response->set_list(ptrn->getList().toStdString());
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::GetSettings(grpc::ServerContext *context,
									 const ptzp::Settings *request,
									 ptzp::Settings *response)
{
	Q_UNUSED(context);
	PtzpHead *head = getHead(request->head_id());
	if (head == NULL)
		return grpc::Status::CANCELLED;

	QByteArray settings = mapToJson(head->getSettings());
	response->set_json(settings);
	response->set_head_id(request->head_id());
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::SetSettings(grpc::ServerContext *context,
									 const ptzp::Settings *request,
									 ptzp::Settings *response)
{
	Q_UNUSED(context);
	Q_UNUSED(response);

	PtzpHead *head = getHead(request->head_id());
	if (head == NULL)
		return grpc::Status::CANCELLED;
	QVariantMap map = jsonToMap(request->json().c_str());
	head->setSettings(map);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::FocusIn(grpc::ServerContext *context,
								 const ptzp::PtzCmdPar *request,
								 ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	response->set_err(-1);
	PtzpHead *head = getHead(request->head_id());
	float speed = request->zoom_speed();
	if (head == NULL)
		return grpc::Status::CANCELLED;
	if (!head->settings.contains("focus"))
		return grpc::Status(grpc::StatusCode::CANCELLED, "not support");
	head->focusIn(speed);
	response->set_err(0);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::FocusOut(grpc::ServerContext *context,
								  const ptzp::PtzCmdPar *request,
								  ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	response->set_err(-1);
	PtzpHead *head = getHead(request->head_id());
	float speed = request->zoom_speed();
	if (head == NULL)
		return grpc::Status::CANCELLED;
	if (!head->settings.contains("focus"))
		return grpc::Status(grpc::StatusCode::CANCELLED, "not support");
	head->focusOut(speed);
	response->set_err(0);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::FocusStop(grpc::ServerContext *context,
								   const ptzp::PtzCmdPar *request,
								   ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	response->set_err(-1);
	PtzpHead *head = getHead(request->head_id());
	if (head == NULL)
		return grpc::Status::CANCELLED;
	if (!head->settings.contains("focus"))
		return grpc::Status(grpc::StatusCode::CANCELLED, "not support");
	head->focusStop();
	response->set_err(0);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::GetIO(grpc::ServerContext *context,
							   const ptzp::IOCmdPar *request,
							   ptzp::IOCmdPar *response)
{
	Q_UNUSED(context);
	if (request->name_size() != request->state_size())
		return grpc::Status::CANCELLED;

	for (int i = 0; i < request->name_size(); i++) {
		std::string name = request->name(i);
		int gpio = gpiocont->getGpioNo(QString::fromStdString(name));
		if (gpio < 0)
			continue;
		if (gpiocont->getDirection(gpio) != GpioController::INPUT)
			continue;
		response->add_name(name);
		ptzp::IOState *state = response->add_state();
		state->set_direction(ptzp::IOState_Direction_INPUT);
		if (gpiocont->getGpioValue(gpio))
			state->set_value(ptzp::IOState_OutputValue_HIGH);
		else
			state->set_value(ptzp::IOState_OutputValue_LOW);
	}
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::SetIO(grpc::ServerContext *context,
							   const ptzp::IOCmdPar *request,
							   ptzp::IOCmdPar *response)
{
	Q_UNUSED(context);
	if (request->name_size() != request->state_size())
		return grpc::Status::CANCELLED;

	for (int i = 0; i < request->name_size(); i++) {
		std::string name = request->name(i);
		int gpio = gpiocont->getGpioNo(QString::fromStdString(name));
		if (gpio < 0)
			continue;
		if (gpiocont->getDirection(gpio) != GpioController::OUTPUT)
			continue;
		response->add_name(name);
		const ptzp::IOState &sin = request->state(i);
		ptzp::IOState *sout = response->add_state();
		sout->set_direction(ptzp::IOState_Direction_OUTPUT);
		sout->set_value(sin.value());
		if (sin.value() == ptzp::IOState_OutputValue_HIGH)
			gpiocont->setGpio(gpio, 1);
		else
			gpiocont->setGpio(gpio, 0);
	}
	return grpc::Status::OK;
}

float PtzpDriver::regulateSpeed(float raw, int zoom)
{
	if (!sreg.enable)
		return raw;
	if (sreg.ipol == SpeedRegulation::ARYA) {
		float zooms[2];
		zooms[0] = sreg.zoomHead->getZoom();
		zooms[1] = sreg.secondZoomHead->getZoom();
		return sreg.interFunc(raw, zooms);
	}
	if (sreg.ipol == SpeedRegulation::CUSTOM) {
		float fovs[2];
		sreg.zoomHead->getFOV(fovs[0], fovs[1]);
		return sreg.interFunc(raw, fovs);
	}
	if (sreg.minZoom < sreg.maxZoom) {
		if (zoom < sreg.minZoom)
			return raw;
		if (zoom > sreg.maxZoom)
			return sreg.minSpeed;
		if (sreg.ipol == SpeedRegulation::LINEAR) {
			// when zoom == minZoom => zoomr = 1, when zoom == maxZoom => zoomr
			// = 0
			float zoomr = 1.0 - (zoom - sreg.minZoom) /
									(float)(sreg.maxZoom - sreg.minZoom);
			raw *= zoomr;
		}
		if (raw < sreg.minSpeed)
			return sreg.minSpeed;
	} else {
		if (zoom < sreg.maxZoom)
			return raw;
		if (zoom > sreg.minZoom)
			return sreg.minSpeed;
		if (sreg.ipol == SpeedRegulation::LINEAR) {
			// when zoom == minZoom => zoomr = 1, when zoom == maxZoom => zoomr
			// = 0
			float zoomr =
				(zoom - sreg.maxZoom) / (float)(sreg.minZoom - sreg.maxZoom);
			raw *= zoomr;
		}
		if (raw < sreg.minSpeed)
			return sreg.minSpeed;
	}
	return raw;
}

QStringList PtzpDriver::commaToList(const QString &comma)
{
	QStringList ret;
	for (QString item : comma.split(",")) {
		ret.append(item);
	}
	return ret;
}

QString PtzpDriver::listToComma(const QStringList &list)
{
	QString ret = "";
	for (QString item : list) {
		ret += item + ",";
	}

	return ret;
}

QVariantMap PtzpDriver::jsonToMap(const QByteArray &arr)
{
	QJsonDocument doc = QJsonDocument::fromJson(arr);
	if (doc.isEmpty())
		return QVariantMap();
	QVariantMap sMap = doc.toVariant().toMap();
	return sMap;
}

QByteArray PtzpDriver::mapToJson(const QVariantMap &map)
{
	return QJsonDocument::fromVariant(map).toJson();
}
#endif

void PtzpDriver::timeout()
{
	if (defaultPTHead && defaultModuleHead) {
		ptrn->positionUpdate(defaultPTHead->getPanAngle(),
							 defaultPTHead->getTiltAngle(),
							 defaultModuleHead->getZoom());
		PatrolNg *ptrl = PatrolNg::getInstance();
		if (ptrl->getCurrentPatrol()->state != PatrolNg::STOP) { // patrol
			PatrolNg::PatrolInfo *patrol = ptrl->getCurrentPatrol();
			if (patrol->list.isEmpty()) {
				ptrl->setPatrolStateStop(patrol->patrolName);
				return;
			}
			QPair<QString, int> pp = patrol->list[patrolListPos];
			QString preset = pp.first;
			int waittime = pp.second;
			mInfo("Patrol state %d", ptrl->state);
			switch (ptrl->state) {
			case PatrolNg::PS_WAIT:
				if (elaps->elapsed() >= waittime) {
					patrolListPos++;
					if (patrolListPos == patrol->list.size())
						patrolListPos = 0;
					ptrl->state = PatrolNg::PS_GO_POS;
				}
				break;
			case PatrolNg::PS_GO_POS: {
				pp = patrol->list[patrolListPos];
				preset = pp.first;
				PresetNg *prst = PresetNg::getInstance();
				QStringList pos = prst->getPreset(preset);
				if (!pos.isEmpty())
					goToPosition(pos.at(0).toFloat(), pos.at(1).toFloat(),
								 pos.at(2).toInt());
				ptrl->state = PatrolNg::PS_WAIT_FOR_POS;
				break;
			}
			case PatrolNg::PS_WAIT_FOR_POS: {
				PresetNg *prst = PresetNg::getInstance();
				QStringList pos = prst->getPreset(preset);
				int diff = qAbs(pos.at(0).toFloat() - getPanAngle());
				int tdiff = qAbs(pos.at(1).toFloat() - getTiltAngle());
				if ((diff < 4) && (tdiff < 4)) {
					elaps->restart();
					ptrl->state = PatrolNg::PS_WAIT;
				}
				break;
			}
			default:
				break;
			}
		}
	}
}

QVariant PtzpDriver::headInfo(const QString &key, PtzpHead *head)
{
	if (key == "status")
		return head->getHeadStatus();
	return QVariant();
}

void PtzpDriver::commandUpdate(int c, float arg1, float arg2)
{
	stopAnyProcess();
	if (defaultPTHead && defaultModuleHead && ptrn->isRecording())
		ptrn->commandUpdate(defaultPTHead->getPanAngle(),
							defaultPTHead->getTiltAngle(),
							defaultModuleHead->getZoom(), c, arg1, arg2);
}

int PtzpDriver::runPatrol(QString name)
{
	mDebug("Running patrol %s", qPrintable(name));
	int err = PatrolNg::getInstance()->setPatrolStateRun(name);
	if (err < 0)
		return err;
	PatrolNg::PatrolInfo *patrol = PatrolNg::getInstance()->getCurrentPatrol();
	if (!patrol->list.isEmpty()) {
		patrolListPos = 0;
		PatrolNg::getInstance()->state = PatrolNg::PS_GO_POS;
	} else
		return -1;
	return 0;
}

/* TODO: remove this function, it is not a general one */
void PtzpDriver::setChangeOverlayState(bool state)
{
	changeOverlay = state;
}

/* TODO: remove this function, it is not a general one */
bool PtzpDriver::getChangeOverlayState()
{
	return changeOverlay;
}

bool PtzpDriver::isReady()
{
	return true;
}

void PtzpDriver::stopAnyProcess(StopProcess stop)
{
	if (stop == PATROL) {
		PatrolNg *ptrl = PatrolNg::getInstance();
		if (ptrl->getCurrentPatrol()->state != PatrolNg::STOP) {
			QString ptrlName = ptrl->getCurrentPatrol()->patrolName;
			ptrl->setPatrolStateStop(ptrlName);
			removeStartupProcess();
		}
	} else if (stop == PATTERN) {
		if (ptrn->isReplaying()) {
			removeStartupProcess();
			ptrn->stop(0, 0, 0);
		}
	} else if (stop == ANY) {
		stopAnyProcess(PATROL);
		stopAnyProcess(PATTERN);
	}
}
