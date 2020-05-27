#include "ptzpdriver.h"
#include "debug.h"

#include "patrolng.h"
#include "patternng.h"
#include "presetng.h"

#include "ptzphead.h"
#include "ptzptransport.h"
#include "mgeothermalhead.h"

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <QTimer>
#include <errno.h>

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

int PtzpDriver::startGrpcApi(quint16 port)
{
	GRpcThread *thr = new GRpcThread(port, this);
	thr->start();
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
	if (defaultPTHead)
		return defaultPTHead->getPanAngle();
	else
		return 999999.99;
}

float PtzpDriver::getTiltAngle()
{
	if (defaultPTHead)
		return defaultPTHead->getTiltAngle();
	else
		return 999999.99;
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

		head->fillCapabilities(h);
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

	PtzpHead *head = findHead(ptzp::PtzHead_Capability_PAN, idx);
	if (head == NULL || !head->hasCapability(ptzp::PtzHead_Capability_PAN)) {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}
	if (sreg.enable && sreg.zoomHead)
		speed = regulateSpeed(speed, sreg.zoomHead->getZoom());

	float maxSpeed = head->getMaxPatternSpeed();
	if (ptrn->isRecording() && maxSpeed != 1.0) {
		if (speed > maxSpeed)
			speed = maxSpeed;
	}
	/*
	 * [CR] [fo] pattern recording aktif olmasa bile alttaki satır çalışmaz mı?
	 * Gereksiz yere bu metoda gidip işlem yapacak.
	 * Bir üztteki if scope'una taşınması daha doğru gibi.
	 * Eğer doğru bir kullanımsa sebebine dair buraya açıklama getirlmeli.
	 */
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

	PtzpHead *head = findHead(ptzp::PtzHead_Capability_PAN, idx);
	if (head == NULL || !head->hasCapability(ptzp::PtzHead_Capability_PAN)) {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}
	if (sreg.enable && sreg.zoomHead)
		speed = regulateSpeed(speed, sreg.zoomHead->getZoom());

	float maxSpeed = head->getMaxPatternSpeed();
	if (ptrn->isRecording() && maxSpeed != 1.0) {
		if (speed > maxSpeed)
			speed = maxSpeed;
	}
	/*
	 * [CR] [fo] Aynı "ptrn->isRecording()" sorunu.
	 */
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

	PtzpHead *head = findHead(ptzp::PtzHead_Capability_PAN, idx);
	if (head == NULL || !head->hasCapability(ptzp::PtzHead_Capability_PAN)) {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}
	/*
	 * [CR] [fo] Aynı "ptrn->isRecording()" sorunu.
	 */
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

	PtzpHead *head = findHead(ptzp::PtzHead_Capability_ZOOM, idx);

	if (head == NULL || !head->hasCapability(ptzp::PtzHead_Capability_ZOOM)) {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}
	/*
	 * [CR] [fo] Aynı "ptrn->isRecording()" sorunu.
	 */
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

	PtzpHead *head = findHead(ptzp::PtzHead_Capability_ZOOM, idx);

	if (head == NULL || !head->hasCapability(ptzp::PtzHead_Capability_ZOOM)) {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}
	/*
	 * [CR] [fo] Aynı "ptrn->isRecording()" sorunu.
	 */
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

	PtzpHead *head = findHead(ptzp::PtzHead_Capability_ZOOM, idx);
	if (head == NULL || !head->hasCapability(ptzp::PtzHead_Capability_ZOOM)) {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}
	/*
	 * [CR] [fo] Aynı "ptrn->isRecording()" sorunu.
	 */
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

	PtzpHead *head = findHead(ptzp::PtzHead_Capability_ZOOM, idx);
	if (head == NULL || !head->hasCapability(ptzp::PtzHead_Capability_TILT)) {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}

	if (sreg.enable && sreg.zoomHead)
		speed = regulateSpeed(speed, sreg.zoomHead->getZoom());

	float maxSpeed = head->getMaxPatternSpeed();
	if (ptrn->isRecording() && maxSpeed != 1.0) {
		if (speed > maxSpeed)
			speed = maxSpeed;
	}

	/*
	 * [CR] [fo] Aynı "ptrn->isRecording()" sorunu.
	 */
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

	PtzpHead *head = findHead(ptzp::PtzHead_Capability_TILT, idx);
	if (head == NULL || !head->hasCapability(ptzp::PtzHead_Capability_TILT)) {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}

	if (sreg.enable && sreg.zoomHead)
		speed = regulateSpeed(speed, sreg.zoomHead->getZoom());

	float maxSpeed = head->getMaxPatternSpeed();
	if (ptrn->isRecording() && maxSpeed != 1.0) {
		if (speed > maxSpeed)
			speed = maxSpeed;
	}

	/*
	 * [CR] [fo] Aynı "ptrn->isRecording()" sorunu.
	 */
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

	PtzpHead *head = findHead(ptzp::PtzHead_Capability_TILT, idx);
	if (head == NULL || !head->hasCapability(ptzp::PtzHead_Capability_TILT)) {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}
	/*
	 * [CR] [fo] Aynı "ptrn->isRecording()" sorunu.
	 */
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

	PtzpHead *head = findHead(ptzp::PtzHead_Capability_TILT, idx);
	if (!head)
		return grpc::Status::CANCELLED;
	if (head == NULL || !head->hasCapability(ptzp::PtzHead_Capability_TILT)
			|| !head->hasCapability(ptzp::PtzHead_Capability_PAN)) {
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}
	/*
	 * [CR] [fo] Aynı "ptrn->isRecording()" sorunu.
	 */
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

	PtzpHead *head = findHead(ptzp::PtzHead_Capability_TILT, idx);
	if (head == NULL || !head->hasCapability(ptzp::PtzHead_Capability_TILT)
			|| !head->hasCapability(ptzp::PtzHead_Capability_PAN)) {
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
	if (ptrn->isRecording() && maxSpeed != 1.0) {
		if (pan > maxSpeed)
			pan = maxSpeed;
		if (tilt > maxSpeed)
			tilt = maxSpeed;
	}

	pan *= signPan;
	tilt *= signTilt;

	/*
	 * [CR] [fo] Aynı "ptrn->isRecording()" sorunu.
	 */
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

	response->set_pan_pos(0);
	response->set_tilt_pos(0);
	response->set_zoom_pos(0);
	response->set_pan_pos(findHead(ptzp::PtzHead_Capability_PAN, idx)->getPanAngle());
	response->set_tilt_pos(findHead(ptzp::PtzHead_Capability_TILT, idx)->getTiltAngle());
	{
		auto head = findHead(ptzp::PtzHead_Capability_ZOOM, idx);
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
	QVariantMap normMap;
	int ret = normalizeValues(request->head_id(), map, &normMap);
	if (ret < 0)
		return grpc::Status::CANCELLED;
	head->setSettings(normMap);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::FocusIn(grpc::ServerContext *context,
								 const ptzp::PtzCmdPar *request,
								 ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	response->set_err(-1);
	PtzpHead *head = findHead(ptzp::PtzHead_Capability_FOCUS, request->head_id());
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
	PtzpHead *head = findHead(ptzp::PtzHead_Capability_FOCUS, request->head_id());
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
	PtzpHead *head = findHead(ptzp::PtzHead_Capability_FOCUS, request->head_id());
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
		zooms[0] = sreg.zoomHead->getAngle();
		// Check it: zooms[1] just used on mgeothermal
		zooms[1] = ((MgeoThermalHead*)defaultModuleHead)->getFovMax();
		return sreg.interFunc(raw,zooms);
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
	if (defaultPTHead && defaultModuleHead)
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

QString PtzpDriver::getCapString(ptzp::PtzHead_Capability cap)
{
	return QString();
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

grpc::Status PtzpDriver::GetAdvancedControl(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response, ptzp::PtzHead_Capability cap)
{
	Q_UNUSED(context);

	PtzpHead *head = findHeadNull(cap, request->head_id());
	if (head == nullptr)
		return grpc::Status::CANCELLED;

	if(cap == ptzp::PtzHead_Capability_ZOOM){
		GetZoom(context,request,response);
	}

	float normalized;
	QVariant var = head->getCapabilityValues(cap);
	int ret = normalizeValue(request->head_id(), getCapString(cap), var, normalized);
	if (ret < 0)
		return grpc::Status::CANCELLED;

	response->set_value(normalized);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::SetAdvancedControl(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response, ptzp::PtzHead_Capability cap)
{
	Q_UNUSED(context);

	PtzpHead *head = findHeadNull(cap, request->head_id());
	if (head == nullptr)
		return grpc::Status::CANCELLED;

	if(cap == ptzp::PtzHead_Capability_ZOOM){
		SetZoom(context,request,response);
	}

	QVariant var;
	int ret = denormalizeValue(request->head_id(), getCapString(cap), request->new_value(), var);
	if (ret < 0)
		return grpc::Status::CANCELLED;

	head->setCapabilityValues(cap, var.toUInt());
	return grpc::Status::OK;
}

/*
 * Call that after head initialized...
 * */
int PtzpDriver::createHeadMaps()
{
	if (QFile::exists("headmaps.json"))
		return 0;
	QJsonObject mainObj;
	for (int i = 0; i < getHeadCount(); i++) {
		PtzpHead *head = getHead(i);
		if (head == NULL) {
			mDebug("Head shot !!! Please before initialize heads..");
			return -2;
		}
		QJsonArray arr;
		foreach (QString key, head->getSettings().keys()) {
			QJsonObject obj;
			obj.insert("dmax", 0);
			obj.insert("dmin", 0);
			QJsonObject subObj;
			subObj.insert(key, obj);
			arr.append(subObj);
		}
		mainObj.insert(QString("head%1").arg(i), arr);
	}
	mainObj.insert("normalize_value", false);
	mainObj.insert("normalize_value_max", 1);
	mainObj.insert("normalize_value_min", -1);
	QJsonDocument doc;
	doc.setObject(mainObj);
	QFile f("headmaps.json");
	if (!f.open(QIODevice::WriteOnly)) {
		mDebug("file opening error '%s'", qPrintable(f.fileName()));
		return -1;
	}
	mDebug("Exporting settings of heads.");
	f.write(doc.toJson());
	f.close();
	return 0;
}

QJsonObject PtzpDriver::readHeadMaps()
{
	if (!QFile::exists("headmaps.json"))
		createHeadMaps();
	QFile f("headmaps.json");
	if (!f.open(QIODevice::ReadOnly))
		return QJsonObject();
	QByteArray ba = f.readAll();
	f.close();
	QJsonObject obj = QJsonDocument::fromJson(ba).object();
	if (obj.isEmpty())
		return QJsonObject();
	return obj;
}


int PtzpDriver::denormalizeValue(int head, const QString &key, const float &value, QVariant &resMap)
{
	int headIndex = findHeadIndex(key, head);
	float resp = value;
	resMap.setValue(resp);
	QJsonObject obj = readHeadMaps();
	if (obj.isEmpty())
		return -1;
	if (!obj.value("normalize_value").toBool()){
		mLog("Normalization options disabled!!!");
		return 0;
	}
	float valmin = obj.value("normalize_value_min").toDouble();
	float valmax = obj.value("normalize_value_max").toDouble();

	QJsonArray arr = obj.value(QString("head%1").arg(headIndex)).toArray();
	QJsonObject dobj;
	for (int i = 0; i < arr.size(); i++) {
		QJsonObject setsObj = arr[i].toObject();
		if (setsObj.keys()[0] == key) {
			dobj = setsObj.value(key).toObject();
			break;
		}
	}
	float dmin = dobj.value("dmin").toDouble();
	float dmax = dobj.value("dmax").toDouble();
	if (valmin > valmax)
		return -3;
	if (value < valmin)
		return -4;
	if (value > valmax)
		return -5;
	resp = (resp - valmin) / (valmax - valmin);
	resp = (dmax - dmin) * resp + dmin;
	mLog("denormalize processing, '%f' to '%f'", value, resp);
	resMap.setValue(resp);
	return 0;
}

int PtzpDriver::normalizeValue(int head, const QString &key, const QVariant &value, float &normalized)
{
	int headIndex = findHeadIndex(key, head);
	normalized = value.toFloat();
	QJsonObject obj = readHeadMaps();
	if (obj.isEmpty())
		return -1;
	if (!obj.value("normalize_value").toBool()){
		mLog("Normalization options disabled!!!");
		return 0;
	}
	float valmin = obj.value("normalize_value_min").toDouble();
	float valmax = obj.value("normalize_value_max").toDouble();

	QJsonArray arr = obj.value(QString("head%1").arg(headIndex)).toArray();
	QJsonObject dobj;
	for (int i = 0; i < arr.size(); i++) {
		QJsonObject setsObj = arr[i].toObject();
		if (setsObj.keys()[0] == key) {
			dobj = setsObj.value(key).toObject();
			break;
		}
	}
	float dmin = dobj.value("dmin").toDouble();
	float dmax = dobj.value("dmax").toDouble();
	if (value.toFloat() < dmin)
		return -3;
	if (value.toFloat() > dmax)
		return -4;
	if (dmin > dmax)
		return -5;
	float resp = value.toFloat() / (dmax - dmin);
	if (valmin < 0)
		resp = (valmax + qAbs(valmin)) * resp + valmin;
	else resp = (valmax - valmin) * resp;
	mLog("normalize processing, '%f' to '%f'", value.toFloat(), resp);
	normalized = resp;
	return 0;
}

int PtzpDriver::normalizeValues(int head, const QVariantMap &map, QVariantMap *resMap)
{
	*resMap = map;
	QJsonObject headMaps = readHeadMaps();
	if (headMaps.value("normalize_value").toBool()) {
		QMapIterator<QString, QVariant> mi(map);
		while (mi.hasNext()) {
			mi.next();
			bool validateRange = true;
			float valmin = headMaps.value("normalize_value_min").toDouble();
			float valmax = headMaps.value("normalize_value_max").toDouble();
			QJsonObject dvalues;
			QJsonArray arr = headMaps.value(QString("head%1").arg(head)).toArray();
			for (int i = 0; i < arr.size(); i++) {
				QJsonObject setsObj = arr[i].toObject();
				if (setsObj.keys()[0] == mi.key()) {
					dvalues = setsObj.value(mi.key()).toObject();
					break;
				}
			}
			float dmin = dvalues.value("dmin").toDouble();
			float dmax = dvalues.value("dmax").toDouble();
			float val = mi.value().toFloat();
			if (valmin > valmax)
				return -1;
			if (validateRange && val < valmin)
				return -1;
			if (validateRange && val > valmax)
				return -1;
			/*
			 * For instance:
			 *
			 *    brightness driver range: 20-300
			 *
			 *    float dmin = 20;
			 *    float dmax = 300;
			 *    float valmin = 0;
			 *    flaot valmax = 1;
			 */
			// change to absolute normalize value 0-100, -1 - +1, 0-1 etc...
			val = (val - valmin) / (valmax - valmin);
			float val2 = (dmax - dmin) * val + dmin;
			mLog("normalize processing, '%f' to '%f'", mi.value().toFloat(), val2);
			resMap->insert(mi.key(), val2);
		}
	}
	return 0;
}

PtzpHead *PtzpDriver::findHead(ptzp::PtzHead_Capability cap, int id)
{
	auto h = findHeadNull(cap, id);
	if (h)
		return h;
	return getHead(0);
}

PtzpHead *PtzpDriver::findHeadNull(ptzp::PtzHead_Capability cap, int id)
{
	if (id >= 0)
		return getHead(id);

	for (int i = 0; i < getHeadCount(); i++) {
		auto head = getHead(i);
		if (head->hasCapability(cap))
			return head;
	}

	return nullptr;
}

int PtzpDriver::findHeadIndex(QString capstring, int id)
{
	if (id >= 0)
		return id;

	for (int i = 0; i < getHeadCount(); i++) {
		auto head = getHead(i);
		if (head->settings.contains(capstring))
			return i;
	}

	return 0;
}

grpc::Status PtzpDriver::GetExposure(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return GetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_EXPOSURE);
}

grpc::Status PtzpDriver::SetExposure(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return SetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_EXPOSURE);
}

grpc::Status PtzpDriver::GetWhiteBalance(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return GetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_WHITE_BALANCE);
}

grpc::Status PtzpDriver::SetWhiteBalance(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return SetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_WHITE_BALANCE);
}

grpc::Status PtzpDriver::GetSaturation(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return GetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_SATURATION);
}

grpc::Status PtzpDriver::SetSaturation(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return SetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_SATURATION);
}

grpc::Status PtzpDriver::GetAntiFlicker(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return GetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_ANTI_FLICKER);
}

grpc::Status PtzpDriver::SetAntiFlicker(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return SetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_ANTI_FLICKER);
}

grpc::Status PtzpDriver::GetGain(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return GetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_GAIN);
}

grpc::Status PtzpDriver::SetGain(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return SetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_GAIN);
}

grpc::Status PtzpDriver::GetShutterSpeed(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return GetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_SHUTTER);
}

grpc::Status PtzpDriver::SetShutterSpeed(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return SetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_SHUTTER);
}

grpc::Status PtzpDriver::GetIris(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return GetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_IRIS);
}

grpc::Status PtzpDriver::SetIris(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return SetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_IRIS);
}

grpc::Status PtzpDriver::GetSharpness(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return GetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_SHARPNESS);
}

grpc::Status PtzpDriver::SetSharpness(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return SetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_SHARPNESS);
}

grpc::Status PtzpDriver::GetBrightness(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return GetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_BRIGHTNESS);
}

grpc::Status PtzpDriver::SetBrightness(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return SetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_BRIGHTNESS);
}

grpc::Status PtzpDriver::GetContrast(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return GetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_CONTRAST);
}

grpc::Status PtzpDriver::SetContrast(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return SetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_CONTRAST);
}

grpc::Status PtzpDriver::GetHue(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return GetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_HUE);
}

grpc::Status PtzpDriver::SetHue(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return SetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_HUE);
}

grpc::Status PtzpDriver::GetDefog(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return GetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_DEFOG);
}

grpc::Status PtzpDriver::SetDefog(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return SetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_DEFOG);
}

grpc::Status PtzpDriver::GetVideoRotate(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return GetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_VIDEO_ROTATE);
}

grpc::Status PtzpDriver::SetVideoRotate(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return SetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_VIDEO_ROTATE);
}

grpc::Status PtzpDriver::GetBacklight(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return GetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_BACKLIGHT);
}

grpc::Status PtzpDriver::SetBacklight(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return SetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_BACKLIGHT);
}

grpc::Status PtzpDriver::GetDayNight(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return GetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_DAY_NIGHT);
}

grpc::Status PtzpDriver::SetDayNight(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return SetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_DAY_NIGHT);
}

grpc::Status PtzpDriver::GetFocus(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return GetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_FOCUS);
}

grpc::Status PtzpDriver::SetFocus(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return SetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_FOCUS);
}

grpc::Status PtzpDriver::GetZoom(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	PtzpHead *head = findHeadNull(ptzp::PtzHead_Capability_ZOOM , request->head_id());
	if (head == nullptr)
		return grpc::Status::CANCELLED;
	float var = head->getZoom();
	response->set_value(var);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::SetZoom(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	PtzpHead *head = findHeadNull(ptzp::PtzHead_Capability_ZOOM, request->head_id());
	if (head == nullptr)
		return grpc::Status::CANCELLED;
	QVariant var = head->setZoom(request->new_value());
	return grpc::Status::OK;

}

grpc::Status PtzpDriver::RebootSystem(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return GetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_REBOOT);
}

grpc::Status PtzpDriver::PoweroffSystem(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	return SetAdvancedControl(context, request, response, ptzp::PtzHead_Capability_REBOOT);
}

grpc::Status PtzpDriver::ScreenClick(grpc::ServerContext *context, const ptzp::ClickParameter *request, google::protobuf::Empty *response)
{
	defaultModuleHead-> screenClick(request->pt().x(), request->pt().y(), request->value());
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::GetCapabilityValues(grpc::ServerContext *context, const ptzp::CapabilityValuesReq *request, ptzp::CapabilityValuesResponse *response)
{
	auto cap = request->caps(0);
	auto req = request->reqs(0);
	response->add_caps(cap);
	auto resp = response->add_values();
	return GetAdvancedControl(context, &req, resp, cap);
}

grpc::Status PtzpDriver::SetCapabilityValues(grpc::ServerContext *context, const ptzp::CapabilityValuesReq *request, ptzp::CapabilityValuesResponse *response)
{
	auto cap = request->caps(0);
	auto req = request->reqs(0);
	response->add_caps(cap);
	auto resp = response->add_values();
	return SetAdvancedControl(context, &req, resp, cap);
}
