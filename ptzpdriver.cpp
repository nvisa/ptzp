#include "ptzpdriver.h"
#include "debug.h"
#include "net/remotecontrol.h"

#include <drivers/patternng.h>
#include <ecl/ptzp/ptzphead.h>
#include <ecl/ptzp/ptzptransport.h>

#include <QTimer>
#include <QThread>

#include <errno.h>

#ifdef HAVE_PTZP_GRPC_API
#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/channel.h>
#include <grpc++/create_channel.h>
#include <grpc++/client_context.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/credentials.h>

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
	GRpcThread(PtzpDriver *driver)
	{
		service = driver;
	}

	void run()
	{
		qDebug() << "GRPC Starting @ 50052" ;
		string ep("0.0.0.0:50052");
		ServerBuilder builder;
		builder.AddListeningPort(ep, grpc::InsecureServerCredentials());
		builder.RegisterService(service);
		std::unique_ptr<Server> server(builder.BuildAndStart());
		server->Wait();
	}
	int port;
	PtzpDriver *service;
};

#endif

PtzpDriver::PtzpDriver(QObject *parent)
	: QObject(parent)
{
	timer = new QTimer(this);
	time = new QElapsedTimer();
	timeSettingsLoad = new QElapsedTimer();
	connect(timer, SIGNAL(timeout()), SLOT(timeout()));
	timer->start(10);
	time->start();
	timeSettingsLoad->start();
	defaultPTHead = NULL;
	defaultModuleHead = NULL;
	ptrn = new PatternNg(this);
}

int PtzpDriver::getHeadCount()
{
	int count = 0;
	while (getHead(count++)) {}
	return count - 1;
}

void PtzpDriver::configLoad(const QString filename)
{
	Q_UNUSED(filename);
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
	Q_UNUSED(port);
	GRpcThread *thr = new GRpcThread(this);
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
	}
	return "almost_there";
	return QVariant();
}

int PtzpDriver::set(const QString &key, const QVariant &value)
{
	mInfo("Set func: %s %d", qPrintable(key), value.toInt());
	if (defaultPTHead == NULL || defaultModuleHead == NULL)
		return -ENODEV;

	if (key == "ptz.cmd.pan_left") {
		ptrn->commandUpdate(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(),
							defaultModuleHead->getZoom(),C_PAN_LEFT, value.toFloat(),0);
		defaultPTHead->panLeft(value.toFloat());
	} else if (key == "ptz.cmd.pan_right") {
		ptrn->commandUpdate(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(),
							defaultModuleHead->getZoom(),C_PAN_RIGHT, value.toFloat(),0);
		defaultPTHead->panRight(value.toFloat());
	} else if (key == "ptz.cmd.tilt_down") {
		ptrn->commandUpdate(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(),
							defaultModuleHead->getZoom(),C_TILT_DOWN, value.toFloat(),0);
		defaultPTHead->tiltDown(value.toFloat());
	} else if (key == "ptz.cmd.tilt_up") {
		ptrn->commandUpdate(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(),
							defaultModuleHead->getZoom(),C_TILT_UP, value.toFloat(),0);
		defaultPTHead->tiltUp(value.toFloat());
	} else if (key == "ptz.cmd.pan_stop") {
		ptrn->commandUpdate(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(),
							defaultModuleHead->getZoom(),C_PAN_TILT_STOP, value.toInt(),0);
		defaultPTHead->panTiltAbs(0, 0);
	} else if (key == "ptz.cmd.pan_tilt_abs") {
		const QStringList &vals = value.toString().split(";");
		if (vals.size() < 2)
			return -EINVAL;
		float pan = vals[0].toFloat();
		float tilt = vals[1].toFloat();
		defaultPTHead->panTiltAbs(pan, tilt);
	} else if (key == "ptz.cmd.zoom_in") {
		ptrn->commandUpdate(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(),
							defaultModuleHead->getZoom(),C_ZOOM_IN, value.toInt(),0);
		defaultModuleHead->startZoomIn(value.toInt());
	} else if (key == "ptz.cmd.zoom_out") {
		ptrn->commandUpdate(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(),
							defaultModuleHead->getZoom(),C_ZOOM_OUT, value.toInt(),0);
		defaultModuleHead->startZoomOut(value.toInt());
	} else if (key == "ptz.cmd.zoom_stop") {
		ptrn->commandUpdate(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(),
							defaultModuleHead->getZoom(),C_ZOOM_STOP, value.toInt(),0);
		defaultModuleHead->stopZoom();
	} else if (key == "pattern_start")
		ptrn->start(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(), defaultModuleHead->getZoom());
	else if (key == "pattern_stop")
		ptrn->stop(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(), defaultModuleHead->getZoom());
	else if (key == "pattern_save")
		ptrn->save(value.toString());
	else if (key == "pattern_load")
		ptrn->load(value.toString());
	else if (key == "pattern_replay")
		ptrn->replay();
	else if (key == "preset_save")
		ptrn->addPreset(value.toString(),defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(), defaultModuleHead->getZoom());
	else if (key == "preset_delete")
		ptrn->deletePreset(value.toString());
	else if (key == "preset_goto")
		ptrn->goToPreset(value.toString());
	else if (key == "patrol_save"){
		QStringList str = value.toString().split(".");
		ptrn->addPatrol(str[0], str[1], str[2]);
	} else if (key == "patrol_delete")
		ptrn->deletePatrol(value.toString());
	else if (key == "patrol_run")
		ptrn->runPatrol(value.toString());
	else if (key == "patrol_stop")
		ptrn->stopPatrol(value.toString());
	else
		return -ENOENT;

	return 0;
}

void PtzpDriver::goToPosition(float p, float t, int z)
{
	defaultModuleHead->setZoom(z) ;
	defaultPTHead->panTiltGoPos(p, t);
}

void PtzpDriver::sendCommand(int c, float par1, int par2)
{
	if (c == C_PAN_LEFT)
		defaultPTHead->panLeft(par1);
	else if (c == C_PAN_RIGHT)
		defaultPTHead->panRight(par1);
	else if (c == C_TILT_DOWN)
		defaultPTHead->tiltDown(par1);
	else if (c == C_TILT_UP)
		defaultPTHead->tiltUp(par1);
	else if (c == C_ZOOM_IN)
		defaultModuleHead->startZoomIn((int)par1);
	else if (c == C_ZOOM_OUT)
		defaultModuleHead->startZoomOut((int)par1);
	else if (c == C_ZOOM_STOP)
		defaultModuleHead->stopZoom();
	else if (c == C_PAN_TILT_STOP)
		defaultPTHead->panTiltStop();
}

#ifdef HAVE_PTZP_GRPC_API
grpc::Status PtzpDriver::GetHeads(grpc::ServerContext *context, const google::protobuf::Empty *request, ptzp::PtzHeadInfo *response)
{
	Q_UNUSED(context);
	Q_UNUSED(request);

	int count = 0;
	PtzpHead *head;
	while ((head = getHead(count))) {
		ptzp::PtzHead *h = response->add_list();
		h->set_head_id(count);
		h->set_status((ptzp::PtzHead_Status)head->getHeadStatus()); //TODO: proto is not in sync with code

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

grpc::Status PtzpDriver::PanLeft(grpc::ServerContext *context, const::ptzp::PtzCmdPar *request, ::ptzp::PtzCommandResult *response){

	Q_UNUSED(context);
	int idx = request->head_id();
	float speed = request->pan_speed();

	PtzpHead *head = getHead(idx);
	if (head==NULL && ((head->getCapabilities() & PtzpHead::CAP_PAN) != PtzpHead::CAP_PAN) )
	{
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}

	head->panLeft(speed);

	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PanRight(grpc::ServerContext *context, const::ptzp::PtzCmdPar *request, ::ptzp::PtzCommandResult *response){

	Q_UNUSED(context);

	int idx = request->head_id();
	float speed = request->pan_speed();

	PtzpHead *head = getHead(idx);
	if (head==NULL && ((head->getCapabilities() & PtzpHead::CAP_PAN) != PtzpHead::CAP_PAN) )
	{
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}
	head->panRight(speed);

	return grpc::Status::OK;

}

grpc::Status PtzpDriver::PanStop(grpc::ServerContext *context, const::ptzp::PtzCmdPar *request, ::ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	int idx = request->head_id();

	PtzpHead *head = getHead(idx);
	if (head==NULL && ((head->getCapabilities() && PtzpHead::CAP_PAN) != PtzpHead::CAP_PAN) )
	{
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}

	head->panTiltStop();

	return grpc::Status::OK;
}

grpc::Status PtzpDriver::ZoomIn(grpc::ServerContext *context, const::ptzp::PtzCmdPar *request, ::ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	int idx = request->head_id();
	float speed = request->zoom_speed();

	PtzpHead *head = getHead(idx);

	if (head==NULL && ((head->getCapabilities() & PtzpHead::CAP_ZOOM) != PtzpHead::CAP_ZOOM) )
	{
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}

	head->startZoomIn(speed);

	return grpc::Status::OK;
}

grpc::Status PtzpDriver::ZoomOut(grpc::ServerContext *context, const::ptzp::PtzCmdPar *request, ::ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	int idx = request->head_id();
	float speed = request->zoom_speed();

	PtzpHead *head = getHead(idx);

	if (head==NULL && ((head->getCapabilities() & PtzpHead::CAP_ZOOM) != PtzpHead::CAP_ZOOM) )
	{
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}

	head->startZoomOut(speed);

	return grpc::Status::OK;
}

grpc::Status PtzpDriver::ZoomStop(grpc::ServerContext *context, const::ptzp::PtzCmdPar *request, ::ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	int idx = request->head_id();

	PtzpHead *head = getHead(idx);
	if (head==NULL && ((head->getCapabilities() & PtzpHead::CAP_ZOOM) != PtzpHead::CAP_ZOOM) )
	{
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}

	head->stopZoom();

	return grpc::Status::OK;
}

grpc::Status PtzpDriver::TiltUp(grpc::ServerContext *context, const ptzp::PtzCmdPar *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	int idx = request->head_id();
	float speed = request->tilt_speed();

	PtzpHead *head = getHead(idx);
	if (head==NULL && ((head->getCapabilities() & PtzpHead::CAP_TILT) != PtzpHead::CAP_TILT) )
	{
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}

	head->tiltUp(speed);

	return grpc::Status::OK;
}

grpc::Status PtzpDriver::TiltDown(grpc::ServerContext *context, const ptzp::PtzCmdPar *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	int idx = request->head_id();
	float speed = request->tilt_speed();

	PtzpHead *head = getHead(idx);
	if (head==NULL && ((head->getCapabilities() & PtzpHead::CAP_TILT) != PtzpHead::CAP_TILT) )
	{
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}

	head->tiltDown(speed);

	return grpc::Status::OK;
}

grpc::Status PtzpDriver::TiltStop(grpc::ServerContext *context, const ptzp::PtzCmdPar *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	int idx = request->head_id();

	PtzpHead *head = getHead(idx);
	if (head==NULL && ((head->getCapabilities() & PtzpHead::CAP_TILT) != PtzpHead::CAP_TILT) ){
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}

	head->panTiltStop();

	return grpc::Status::OK;
}


grpc::Status PtzpDriver::PanTiltAbs(grpc::ServerContext *context, const ptzp::PtzCmdPar *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	int idx = request->head_id();
	float tilt = request->tilt_abs();
	float pan = request->pan_abs();

	PtzpHead *head = getHead(idx);
	int cap = head->getCapabilities();
	if (head==NULL && ( ((cap & PtzpHead::CAP_PAN) != PtzpHead::CAP_PAN)|| ((cap & PtzpHead::CAP_TILT) != PtzpHead::CAP_TILT)) )
	{
		response->set_err(-1);
		return grpc::Status::CANCELLED;
	}

	head->panTiltAbs(pan,tilt);

	return grpc::Status::OK;
}

grpc::Status PtzpDriver::GetPTZPosInfo(grpc::ServerContext *context, const ptzp::PtzCmdPar *request, ptzp::PTZPosInfo *response)
{
	Q_UNUSED(context);

	int idx = request->head_id();

	PtzpHead *head = getHead(idx);
	
	int cap = head->getCapabilities();
	if (head==NULL && ( ((cap & PtzpHead::CAP_PAN) != PtzpHead::CAP_PAN)
						|| ((cap & PtzpHead::CAP_TILT) != PtzpHead::CAP_TILT)
						|| ((cap & PtzpHead::CAP_ZOOM) != PtzpHead::CAP_ZOOM)) )
	{
		return grpc::Status::CANCELLED;
	}

	response->set_pan_pos(head->getPanAngle());
	response->set_tilt_pos(head->getTiltAngle());
	response->set_zoom_pos(head->getZoom());

	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PresetGo(grpc::ServerContext *context, const ptzp::PresetCmd *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PresetDelete(grpc::ServerContext *context, const ptzp::PresetCmd *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PresetSave(grpc::ServerContext *context, const ptzp::PresetCmd *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PresetGetList(grpc::ServerContext *context, const ptzp::PresetCmd *request, ptzp::PresetList *response)
{

//	QJsonArray arr;

//	for(int i=0;i<presetList.size();i++)
//		if (!(presetList.at(i).empty)) arr.append(presetList.at(i).toJson(i));

//	auto doc = QJsonDocument(arr);
//	response->set_list(doc.toJson().constData());

	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatrolSave(grpc::ServerContext *context, const ptzp::PatrolCmd *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatrolRun(grpc::ServerContext *context, const ptzp::PatrolCmd *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	Q_UNUSED(response);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatrolDelete(grpc::ServerContext *context, const ptzp::PatrolCmd *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	Q_UNUSED(response);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatrolStop(grpc::ServerContext *context, const ptzp::PatrolCmd *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatrolGetList(grpc::ServerContext *context, const ptzp::PatrolCmd *request, ptzp::PresetList *response)
{
//	QJsonArray arr;
//	for(int i=0;i<patrolList.size();i++)
//		if (!(patrolList.at(i).empty)) arr.append(patrolList.at(i).toJson(i));
//	auto doc = QJsonDocument(arr);
//	response->set_list(doc.toJson().constData());
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatternRun(grpc::ServerContext *context, const ptzp::PatternCmd *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatternStop(grpc::ServerContext *context, const ptzp::PatternCmd *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatternStartRecording(grpc::ServerContext *context, const ptzp::PatternCmd *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatternStopRecording(grpc::ServerContext *context, const ptzp::PatternCmd *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::GetSettings(grpc::ServerContext *context, const ptzp::Settings *request, ptzp::Settings *response){	
	Q_UNUSED(context);

	PtzpHead *head = getHead(request->head_id());
	if (head==NULL)
		return grpc::Status::CANCELLED;

	QByteArray settings = mapToJson(head->getSettings());
	response->set_json(settings);
	response->set_head_id(request->head_id());

	return grpc::Status::OK;
}

grpc::Status PtzpDriver::SetSettings(grpc::ServerContext *context, const ptzp::Settings *request, ptzp::Settings *response){

	Q_UNUSED(context);

	PtzpHead *head = getHead(request->head_id());
	if (head==NULL)
		return grpc::Status::CANCELLED;

	QVariantMap map = jsonToMap(request->json().c_str());
	head->setSettings(map);
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::FocusIn(grpc::ServerContext *context, const ptzp::PtzCmdPar *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	PtzpHead *head = getHead(request->head_id());
	if (head==NULL)
		return grpc::Status::CANCELLED;
	if (!head->settings.contains("focus_in"))
		return grpc::Status::CANCELLED;

	// focus not implemented yet so empty for now
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::FocusOut(grpc::ServerContext *context, const ptzp::PtzCmdPar *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	PtzpHead *head = getHead(request->head_id());
	if (head==NULL)
		return grpc::Status::CANCELLED;
	if (!head->settings.contains("focus_out"))
		return grpc::Status::CANCELLED;

	// focus not implemented yet so empty for now
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::FocusStop(grpc::ServerContext *context, const ptzp::PtzCmdPar *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	PtzpHead *head = getHead(request->head_id());
	if (head==NULL)
		return grpc::Status::CANCELLED;
	if (!head->settings.contains("focus_stop"))
		return grpc::Status::CANCELLED;

	// focus not implemented yet so empty for now
	return grpc::Status::OK;
}

QList<int> PtzpDriver::commaToList(const QString& comma)
{
	QList<int> ret;
	for (QString item : comma.split(",")){
		ret.append(item.toInt());
	}
	return ret;
}

QString PtzpDriver::listToComma(const QList<int> &list)
{
	QString ret = "";
	for(int item : list){
		ret+=QString::number(item) + ",";
	}

	return ret;
}


QVariantMap PtzpDriver::jsonToMap(const QByteArray& arr)
{
	QJsonDocument doc = QJsonDocument::fromJson(arr);
	if(doc.isEmpty())
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
	ptrn->positionUpdate(defaultPTHead->getPanAngle(),
						 defaultPTHead->getTiltAngle(),
						 defaultModuleHead->getZoom());
	if(time->elapsed() >= 10000) {
		defaultModuleHead->saveRegisters();
		time->restart();
	}
	if(timeSettingsLoad->elapsed() <= 50)
		defaultModuleHead->loadRegisters();
}

QVariant PtzpDriver::headInfo(const QString &key, PtzpHead *head)
{
	if (key == "status")
		return head->getHeadStatus();
	return QVariant();
}
