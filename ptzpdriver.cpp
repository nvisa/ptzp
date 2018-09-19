#include "ptzpdriver.h"
#include "debug.h"
#include "net/remotecontrol.h"

#include <drivers/patternng.h>
#include <drivers/presetng.h>
#include <ecl/ptzp/ptzphead.h>
#include <ecl/ptzp/ptzptransport.h>

#include <QTimer>

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

#include <QThread>

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
		string ep("0.0.0.0:50052");
		ServerBuilder builder;
		builder.AddListeningPort(ep, grpc::InsecureServerCredentials());
		builder.RegisterService(service);
		std::unique_ptr<Server> server(builder.BuildAndStart());
		server->Wait();
	}

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
	PatrolNg *ptrl = PatrolNg::getInstance();
	PresetNg *prst = PresetNg::getInstance();
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
		prst->addPreset(value.toString(),defaultPTHead->getPanAngle(), defaultPTHead->getTiltAngle(), defaultModuleHead->getZoom());
	else if (key == "preset_delete")
		prst->deletePreset(value.toString());
	else if (key == "preset_goto") {
		QStringList pos = prst->getPreset(value.toString());
		if (!pos.isEmpty())
			goToPosition(pos.at(0).toFloat(), pos.at(1).toFloat(), pos.at(2).toInt());
		else return -EINVAL;
	}
	else if (key == "patrol_list") {
		QString presets = value.toString();
		if (presets.contains(","))
			ptrl->addPatrol(presets.split(","));
	} else if (key == "patrol_interval") {
		QString intervals = value.toString();
		if (intervals.contains(","))
			ptrl->addInterval(intervals.split(","));
	} else if (key == "patrol_ind") {
		ptrl->setPatrolIndex(value.toInt());
	} else if (key == "patrol_delete") {
		ptrl->deletePatrol();
	} else if (key == "patrol_state_run") {
		ptrl->setPatrolStateRun(value.toInt());
	} else if (key == "patrol_state_stop") {
		ptrl->setPatrolStateStop(value.toInt());
	}
	else
		return -ENOENT;

	return 0;
}

void PtzpDriver::goToPosition(float p, float t, int z)
{
	defaultModuleHead->setZoom(z);
	defaultPTHead->panTiltGoPos(p, t);
}

void PtzpDriver::sendCommand(int c, float par1, int par2)
{
	qDebug() << "command"  << c << par1;
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
	int count = 0;
	PtzpHead *head;
	while ((head = getHead(count))) {
		ptzp::PtzHead *h = response->add_list();
		//h->set_capabilities(); //TODO: convert PtzpHead capabilities
		h->set_head_id(count);
		h->set_status((ptzp::PtzHead_Status)head->getHeadStatus()); //TODO: proto is not in sync with code
		count++;
	}
	return grpc::Status::OK;
}
#endif

void PtzpDriver::timeout()
{
	ptrn->positionUpdate(defaultPTHead->getPanAngle(),
						 defaultPTHead->getTiltAngle(),
						 defaultModuleHead->getZoom());
	PatrolNg *ptrl = PatrolNg::getInstance();
	if (ptrl->getCurrentPatrol().state != 0) { // patrol
		static QElapsedTimer elaps;
		static int listPos = 0;
		PatrolNg::PatrolInfo *patrol = &ptrl->getCurrentPatrol();
		if (patrol->list.isEmpty()) {
			ptrl->setPatrolStateStop(patrol->patrolId);
			return;
		}
		QPair<QString, int> pp = patrol->list[listPos];
		QString preset = pp.first;
		int waittime = pp.second;
		if (elaps.elapsed() >= waittime) {
			listPos ++;
			if (listPos == patrol->list.size())
				listPos = 0;
			pp = patrol->list[listPos];
			preset = pp.first;
			waittime = pp.second;
			elaps.restart();
			PresetNg *prst = PresetNg::getInstance();
			QStringList pos = prst->getPreset(preset);
			goToPosition(pos.at(0).toFloat(), pos.at(1).toFloat(), pos.at(2).toInt());
		}
	}
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
