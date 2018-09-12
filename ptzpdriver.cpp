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
	connect(timer, SIGNAL(timeout()), SLOT(timeout()));
	timer->start(10);

	defaultPTHead = NULL;
	defaultModuleHead = NULL;
	ptrn = new PatternNg;

	MAX_PRESET = 256;
	MAX_PATROL = 10;
#ifdef HAVE_PTZP_GRPC_API
	presetList.fill(Preset(),MAX_PRESET);
	patrolList.fill(Patrol(),MAX_PATROL);
#endif
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
	if (defaultPTHead == NULL || defaultModuleHead == NULL)
		return -ENODEV;

	if (key == "ptz.cmd.pan_left") {
		ptrn->commandUpdate(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(),
							defaultModuleHead->getZoom(),0, value.toInt(),NULL);
		defaultPTHead->panLeft(value.toFloat());
	} else if (key == "ptz.cmd.pan_right") {
		ptrn->commandUpdate(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(),
							defaultModuleHead->getZoom(),1, value.toInt(),NULL);
		defaultPTHead->panRight(value.toFloat());
	} else if (key == "ptz.cmd.tilt_down") {
		ptrn->commandUpdate(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(),
							defaultModuleHead->getZoom(),2, value.toInt(),NULL);
		defaultPTHead->tiltDown(value.toFloat());
	} else if (key == "ptz.cmd.tilt_up") {
		ptrn->commandUpdate(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(),
							defaultModuleHead->getZoom(),3, value.toInt(),NULL);
		defaultPTHead->tiltUp(value.toFloat());
	} else if (key == "ptz.cmd.pan_stop") {
		ptrn->commandUpdate(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(),
							defaultModuleHead->getZoom(),9, value.toInt(),NULL);
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
							defaultModuleHead->getZoom(),4, value.toInt(),NULL);
		defaultModuleHead->startZoomIn(value.toInt());
	} else if (key == "ptz.cmd.zoom_out") {
		ptrn->commandUpdate(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(),
							defaultModuleHead->getZoom(),5, value.toInt(),NULL);
		defaultModuleHead->startZoomOut(value.toInt());
	} else if (key == "ptz.cmd.zoom_stop") {
		ptrn->commandUpdate(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(),
							defaultModuleHead->getZoom(),6, value.toInt(),NULL);
		defaultModuleHead->stopZoom();
	} else if (key == "ptz.cmd.pattern_start")
		ptrn->start(defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(), defaultModuleHead->getZoom());
	else
		return -ENOENT;

	return 0;
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

	presetGo(request->preset_id());

	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PresetDelete(grpc::ServerContext *context, const ptzp::PresetCmd *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	presetDelete(request->preset_id());

	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PresetSave(grpc::ServerContext *context, const ptzp::PresetCmd *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);

	Preset p(QString::fromStdString(request->preset_name()),request->pt_id(),request->z_id());

	presetSave(request->preset_id(),p);

	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PresetGetList(grpc::ServerContext *context, const ptzp::PresetCmd *request, ptzp::PresetList *response)
{

	QJsonArray arr;

	for(int i=0;i<presetList.size();i++)
		if (!(presetList.at(i).empty)) arr.append(presetList.at(i).toJson(i));

	auto doc = QJsonDocument(arr);
	response->set_list(doc.toJson().constData());

	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatrolSave(grpc::ServerContext *context, const ptzp::PatrolCmd *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	// TODO: this is merely a placeholder and will be deleted.
	patrolSave(request->patrol_id(),Patrol(QString::fromStdString(request->patrol_name()),commaToList(QString::fromStdString(request->preset_list())),commaToList(QString::fromStdString(request->interval_list()))));
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatrolRun(grpc::ServerContext *context, const ptzp::PatrolCmd *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	Q_UNUSED(response);
	patrolRun(request->patrol_id());
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatrolDelete(grpc::ServerContext *context, const ptzp::PatrolCmd *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	Q_UNUSED(response);

	patrolDelete(request->patrol_id());
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatrolStop(grpc::ServerContext *context, const ptzp::PatrolCmd *request, ptzp::PtzCommandResult *response)
{
	Q_UNUSED(context);
	patrolStop();
	return grpc::Status::OK;
}

grpc::Status PtzpDriver::PatrolGetList(grpc::ServerContext *context, const ptzp::PatrolCmd *request, ptzp::PresetList *response)
{
	QJsonArray arr;

	for(int i=0;i<patrolList.size();i++)
		if (!(patrolList.at(i).empty)) arr.append(patrolList.at(i).toJson(i));

	auto doc = QJsonDocument(arr);

	response->set_list(doc.toJson().constData());

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

void PtzpDriver::timeout()
{

}
#endif


QVariant PtzpDriver::headInfo(const QString &key, PtzpHead *head)
{
	if (key == "status")
		return head->getHeadStatus();
	return QVariant();
}

#ifdef HAVE_PTZP_GRPC_API

int PtzpDriver::presetGo(int id)
{
	Preset preset = presetList.at(id);

	if (preset.empty)
		return -1;

	PtzpHead *ptHead = getHead(preset.pt_id), *zHead = getHead(preset.zoom_id);
	if(ptHead != NULL) ptHead->panTiltGoPos(preset.pan,preset.tilt);
	if(zHead != NULL) zHead->setZoom(preset.zoom);

	return 0;
}

int PtzpDriver::presetDelete(int id)
{
	presetList[id].empty = true;
	return 0;
}

int PtzpDriver::presetSave(int id, Preset preset)
{
	PtzpHead *ptHead = getHead(preset.pt_id), *zHead = getHead(preset.zoom_id);

	if(ptHead != NULL)
	{
		preset.pan = ptHead->getPanAngle();
		preset.tilt = ptHead->getTiltAngle();
	}

	if(zHead != NULL) preset.zoom = zHead->getZoom();

	preset.empty = false;
	presetList[id] = preset;

	return 0;
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

int PtzpDriver::patrolSave(int id, PtzpDriver::Patrol patrol)
{

	for(int a : patrol.presetSequence){
		if (presetList.at(a).empty)
			return -1;
	}

	patrolList[id] = patrol;
	return 0;
}

int PtzpDriver::patrolDelete(int id)
{
	patrolList[id].empty = true;
	return 0;
}

int PtzpDriver::patrolRun(int id)
{
	patrolTimer = new QTimer(this);
	connect(patrolTimer,SIGNAL(timeout()),this,SLOT(patrolTick()));

	patrolStop();
	if(id < 0 && id > patrolList.size())
		return -1;

	runningPatrol = patrolList.at(id);
	if (runningPatrol.empty)
		return -1;

	int i = runningPatrol.intervals.size() , s = runningPatrol.presetSequence.size();
	if (i!=s)
		return -1;

	isRunning = true;
	patrolTimer->start(runningPatrol.intervals[runningPatrol.currentIndex]*1000);

	return 0;
}

int PtzpDriver::patrolStop()
{
	isRunning = false;
	if(patrolTimer->isActive()) patrolTimer->stop();

	return 0;
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

void PtzpDriver::patrolTick()
{
	if(!isRunning)
	{
		patrolStop();
		return;
	}

	presetGo(runningPatrol.presetSequence[runningPatrol.currentIndex]);
	runningPatrol.inc();
	patrolTimer->setInterval(runningPatrol.intervals[runningPatrol.currentIndex]*1000);
}
#endif
