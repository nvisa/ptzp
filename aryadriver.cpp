#include "aryadriver.h"
#include "aryapthead.h"

#include "debug.h"
#include "gungorhead.h"
#include "moxacontrolhead.h"
#include "mgeothermalhead.h"
#include "ptzptcptransport.h"
#include "ptzphttptransport.h"

#include <QProcess>
#include <QCoreApplication>
#include <math.h>

static float speedRegulateThermal(float speed, float zooms[])
{
	float zoomVal = zooms[0];
	float zoomMax = zooms[1];
	return speed * (pow(zoomVal / zoomMax, 2));
}

static float speedRegulateDay(float speed, float zooms[])
{
	float stepSize = 1085.0;
	float zoomVal = zooms[0];
	float stepAmount = zoomVal / stepSize;
	float correction = pow((13.751 - stepAmount * 0.229) / 13.751, 2);
	float result = speed * correction;
	if (result < 0.003)
		result = 0.003;
	return result;
}

AryaDriver::AryaDriver(QObject *parent) : PtzpDriver(parent)
{
	headType = "tip3";
	apiEnable = false;
	gungorInterval = 100;
	thermalInterval = 1000;
	tcp1 = NULL;
	tcp2 = NULL;
	tcp3 = NULL;
	aryapt = NULL;
	gungor = NULL;
	thermal = NULL;
	moxaDay = NULL;
	moxaThermal = NULL;

	defaultPTHead = NULL;
	defaultModuleHead = NULL;
	state = INIT;
	connectLaps.start();

	overlayLaps.start();
	overlayInterval = 1000;
}

int AryaDriver::setTarget(const QString &targetUri)
{
	connectPT(targetUri);
	connectThermal(targetUri);
	connectDay(targetUri);

	setRegulateSettings();
	return 0;
}

void AryaDriver::reboot()
{
	mDebug("Application going to restart.");
	QStringList args = QCoreApplication::instance()->arguments();
	QString appName = args.takeFirst();
	if (appName.contains("/"))
		appName = appName.split("/").last();
	QProcess::execute(QString("killall %1").arg(appName));
}

int AryaDriver::connectPT(const QString &target)
{
	tcp1 = new PtzpTcpTransport(PtzpTransport::PROTO_STRING_DELIM);
	tcp1->connectTo(QString("%1:4001").arg(target));
	tcp1->enableQueueFreeCallbacks(true);
	aryapt = new AryaPTHead;
	aryapt->setTransport(tcp1);
	defaultPTHead = aryapt;
	return 0;
}

int AryaDriver::connectThermal(const QString &target)
{
	tcp2 = new PtzpTcpTransport(PtzpTransport::PROTO_BUFFERED);
	tcp2->connectTo(QString("%1:4002").arg(target));
	tcp2->setTimerInterval(thermalInterval);
	thermal = new MgeoThermalHead(headType);
	thermal->setTransport(tcp2);
	return 0;
}

int AryaDriver::connectDay(const QString &target)
{
	tcp3 = new PtzpTcpTransport(PtzpTransport::PROTO_BUFFERED);
	tcp3->connectTo(QString("%1:4003").arg(target));
	tcp3->setTimerInterval(gungorInterval);
	gungor = new MgeoGunGorHead;
	gungor->setTransport(tcp3);
	defaultModuleHead = gungor;
	return 0;
}

int AryaDriver::setMoxaControl(const QString &targetThermal, const QString &targetDay)
{
	httpThermal = new PtzpHttpTransport(PtzpTransport::PROTO_BUFFERED);
	httpThermal->setContentType(PtzpHttpTransport::TextPlain);
	httpThermal->setAuthorizationType(PtzpHttpTransport::Digest);
	int ret = httpThermal->connectTo(targetThermal);
	if (ret)
		return ret;

	httpDay = new PtzpHttpTransport(PtzpTransport::PROTO_BUFFERED);
	httpDay->setContentType(PtzpHttpTransport::TextPlain);
	httpDay->setAuthorizationType(PtzpHttpTransport::Digest);
	ret = httpDay->connectTo(targetDay);
	if (ret)
		return ret;

	moxaDay = new MoxaControlHead();
	moxaDay->setTransport(httpDay);

	moxaThermal = new MoxaControlHead();
	moxaThermal->setTransport(httpThermal);
	return 0;
}

void AryaDriver::updateZoomOverlay(int thermal, int day)
{
	if (!moxaThermal || !moxaDay) {
		mDebug("Moxa Thermal and Day device don't ready!!!");
		return;
	}
	thermal = this->thermal->getFovMax() / thermal;
	day = this->gungor->getZoom() / 1000.0;
	day = day ? day : 1; // because getting zero !.
	if (overlayLaps.elapsed() > overlayInterval) {
		moxaDay->setZoomShow(day);
		moxaThermal->setZoomShow(thermal);
		overlayLaps.restart();
	}
	return;
}

void AryaDriver::setOverlayInterval(int ms)
{
	if (ms != 0)
		overlayInterval = ms;
}

void AryaDriver::setThermalInterval(int ms)
{
	thermalInterval = ms;
}

void AryaDriver::setGungorInterval(int ms)
{
	gungorInterval = ms;
}

void AryaDriver::setHeadType(QString type)
{
	if (!type.isEmpty())
		headType = type;
}

grpc::Status AryaDriver::GetSettings(grpc::ServerContext *context, const ptzp::Settings *request, ptzp::Settings *response)
{
	if (!apiEnable) {
		int headID = request->head_id();
		if (headID == 1)
			defaultModuleHead = thermal;
		else if (headID == 2)
			defaultModuleHead = gungor;
		setRegulateSettings();
	}
	return PtzpDriver::GetSettings(context, request, response);
}

grpc::Status AryaDriver::SetSettings(grpc::ServerContext *context, const ptzp::Settings *request, ptzp::Settings *response)
{
	if (request->json() == "change_head") {
		apiEnable = true;
		int headID = request->head_id();
		if (headID == 1)
			defaultModuleHead = thermal;
		else if (headID == 2)
			defaultModuleHead = gungor;
		setRegulateSettings();
		return grpc::Status::OK;
	}
	return PtzpDriver::SetSettings(context, request, response);
}

void AryaDriver::setRegulateSettings()
{
	SpeedRegulation sreg = getSpeedRegulation();
	sreg.enable = true;
	sreg.ipol = SpeedRegulation::ARYA;
	if (defaultModuleHead == gungor) {
		sreg.interFunc = speedRegulateDay;
		sreg.zoomHead = gungor;
	}
	else {
		sreg.interFunc = speedRegulateThermal;
		sreg.zoomHead = thermal;
	}
	setSpeedRegulation(sreg);
}

PtzpHead *AryaDriver::getHead(int index)
{
	if (index == 0)
		return aryapt;
	else if (index == 1)
		return thermal;
	else if (index == 2)
		return gungor;
	return NULL;
}

void AryaDriver::timeout()
{
	if (thermal->getHeadStatus() != PtzpHead::ST_NORMAL
		||	gungor->getHeadStatus() != PtzpHead::ST_NORMAL
		||	aryapt->getHeadStatus() != PtzpHead::ST_NORMAL) {
		mLog("System heads didn't initialize...System will go to restart;\n"
			  "Thermal State %d, Gungor State %d, PanTilt State %d.",
			   thermal->getHeadStatus(), gungor->getHeadStatus(), aryapt->getHeadStatus());
		if (connectLaps.elapsed() > 30000)
			reboot();
	} else
		connectLaps.restart();
	mLog("Driver state: %d", state);
	/*
	 * [CR] [yca] Bu sinifin state dongusu daha robust
	 * hale getirilebilir mi? Kafalardan herhangi birisi
	 * calismasa bile sistem bunu hata olarak belirtip digerlerini
	 * yonetebilir hale getirilebilir mi?
	 */
	switch (state) {
	case INIT:
		thermal->syncRegisters();
		thermal->loadRegisters("head1.json");
		state = SYNC_THERMAL_MODULES;
		break;
	case SYNC_THERMAL_MODULES:
		if (thermal->getHeadStatus() == PtzpHead::ST_NORMAL) {
			state = SYNC_GUNGOR_MODULES;
			gungor->syncRegisters();
			gungor->loadRegisters("head2.json");
		}
		break;
	case SYNC_GUNGOR_MODULES:
		if (gungor->getHeadStatus() == PtzpHead::ST_NORMAL) {
			gungor->setFocusStepper();
			tcp2->enableQueueFreeCallbacks(true);
			tcp3->enableQueueFreeCallbacks(true);
			state = NORMAL;
		}
		break;
	case NORMAL:
		updateZoomOverlay(thermal->getAngle(), gungor->getAngle());
		manageRegisterSaving();
		if (doStartupProcess) {
			doStartupProcess = false;
			getStartupProcess();
		}
		break;
	}
	PtzpDriver::timeout();
}

