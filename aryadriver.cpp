#include "aryadriver.h"
#include "gungorhead.h"
#include "aryapthead.h"
#include "moxacontrolhead.h"
#include "mgeothermalhead.h"
#include "ptzptcptransport.h"
#include "ptzphttptransport.h"
#include "debug.h"

static float speedRegulateThermal(float speed, float zooms[]) {
	float stepSize = 1575.0;
	float zoomVal = zooms[0];
	float stepAmount = zoomVal / stepSize;
	float correction = pow((16.8 - stepAmount * 2) / 16.8,2);
	float result = speed * correction;
	if (result < 0.003)
		result = 0.003;
	return result;
}

static float speedRegulateDay(float speed, float zooms[]) {
	float stepSize = 1085.0;
	float zoomVal = zooms[1];
	float stepAmount = zoomVal / stepSize;
	float correction = pow((13.751 - stepAmount * 0.229) / 13.751,2);
	float result = speed * correction;
	if (result < 0.003)
		result = 0.003;
	return result;
}

static float speedRegulateArya(float speed, float zooms[]) {
	float speedThermal = speedRegulateThermal(speed,zooms);
	float speedDay = speedRegulateDay(speed,zooms);
	return speedThermal < speedDay ? speedThermal : speedDay;
}

AryaDriver::AryaDriver(QObject *parent)
	: PtzpDriver(parent)
{
	aryapt = new AryaPTHead;
	gungor = new MgeoGunGorHead;
	thermal = new MgeoThermalHead;
	tcp1 = new PtzpTcpTransport(PtzpTransport::PROTO_STRING_DELIM);
	tcp2 = new PtzpTcpTransport(PtzpTransport::PROTO_BUFFERED);
	tcp3 = new PtzpTcpTransport(PtzpTransport::PROTO_BUFFERED);
	state = SYSTEM_CHECK;

	defaultPTHead = aryapt;
	defaultModuleHead = gungor;

	checker = new QElapsedTimer();
	checker->start();

	overlayLaps.start();
	overlayInterval = 1000;

}

int AryaDriver::setTarget(const QString &targetUri)
{
	aryapt->setTransport(tcp1);
	thermal->setTransport(tcp2);
	gungor->setTransport(tcp3);

	SpeedRegulation sreg = getSpeedRegulation();
	sreg.enable = true;
	sreg.ipol = SpeedRegulation::ARYA;
	sreg.interFunc = speedRegulateArya;
	sreg.zoomHead = thermal;
	sreg.secondZoomHead = gungor;
	setSpeedRegulation(sreg);

	int err = tcp1->connectTo(QString("%1:4001").arg(targetUri));
	if (err)
		return err;
	err = tcp2->connectTo(QString("%1:4002").arg(targetUri));
	if (err)
		return err;
	err = tcp3->connectTo(QString("%1:4003").arg(targetUri));
	if (err)
		return err;
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
	mLog("Driver state: %d", state);
	switch (state) {
	case SYSTEM_CHECK:
		if (checker->elapsed() > 5000) {
			if (aryapt->getSystemStatus() == 2)
				tcp1->enableQueueFreeCallbacks(true);
			if (gungor->getSystemStatus() == 2)
				tcp3->enableQueueFreeCallbacks(true);
			if (thermal->getSystemStatus() == 2) {
				tcp2->enableQueueFreeCallbacks(true);
			}
			state = INIT;
		}

		if (aryapt->getSystemStatus() != 2) {
			aryapt->headSystemChecker();
		}
		if (gungor->getSystemStatus() != 2) {
			gungor->headSystemChecker();
		}
		if (thermal->getSystemStatus() != 2) {
			thermal->headSystemChecker();
		}
		break;
	case INIT:
		if (thermal->getSystemStatus() == 2) {
			thermal->syncRegisters();
			thermal->loadRegisters("head1.json");
			state = SYNC_THERMAL_MODULES;
		} else if (gungor->getSystemStatus() == 2) {
			gungor->syncRegisters();
			gungor->loadRegisters("head2.json");
			state = SYNC_GUNGOR_MODULES;
		}
		checker->restart();
		break;
	case SYNC_THERMAL_MODULES:
		if(thermal->getHeadStatus() == PtzpHead::ST_NORMAL) {
			state = SYNC_GUNGOR_MODULES;
			gungor->syncRegisters();
			gungor->loadRegisters("head2.json");
		}
		break;
	case SYNC_GUNGOR_MODULES:
		if(gungor->getHeadStatus() == PtzpHead::ST_NORMAL) {
			gungor->setFocusStepper();
			state = NORMAL;
		}
		break;
	case NORMAL:
		updateZoomOverlay(thermal->getZoom(), gungor->getZoom());
		manageRegisterSaving();
		if (doStartupProcess) {
			doStartupProcess = false;
			getStartupProcess();
		}
		break;
	}
	PtzpDriver::timeout();
}

QVariant AryaDriver::get(const QString &key)
{
	mInfo("Get func: %s", qPrintable(key));
	if (key == "ptz.get_cooled_down")
		return QString("%1")
			.arg(thermal->getProperty(0));
	else if (key == "ptz.get_brightness")
		return QString("%1")
				.arg(thermal->getProperty(1));
	else if (key== "ptz.get_contrast")
		return QString("%1")
				.arg((thermal->getProperty(2)));
	else if (key == "ptz.get_fov_change")
		return QString("%1")
			.arg(thermal->getProperty(3));
	else if (key == "ptz.get_thermal_zoom")
		return QString("%1")
				.arg(thermal->getProperty(4));
	else if (key == "ptz.get_thermal_focus")
		return QString("%1")
			.arg(thermal->getProperty(5));
	else if (key == "ptz.get_thermal_angle")
		return QString("%1")
			.arg(thermal->getProperty(6));
	else if (key == "ptz.get_nuc_table")
		return QString("%1")
			.arg(thermal->getProperty(7));
	else if (key == "ptz.get_polarity")
		return QString("%1")
				.arg(thermal->getProperty(8));
	else if (key == "ptz.get_reticle")
		return QString("%1")
				.arg(thermal->getProperty(9));
	else if (key == "ptz.get_thermal_digital_zoom")
		return QString("%1")
				.arg(thermal->getProperty(10));
	else if (key == "ptz.get_image_freeze")
		return QString("%1")
				.arg(thermal->getProperty(11));
	else if (key == "ptz.get_agc")
		return QString("%1")
				.arg(thermal->getProperty(12));
	else if (key == "ptz.get_intensity")
		return QString("%1")
				.arg(thermal->getProperty(13));
	else if (key == "ptz.get_ibit")
		return QString("%1")
				.arg(thermal->getProperty(15));
	else if (key == "ptz.get_ipm")
		return QString("%1")
				.arg(thermal->getProperty(16));
	else if (key == "ptz.get_hpf_gain")
		return QString("%1")
				.arg(thermal->getProperty(17));
	else if (key == "ptz.get_hpf_spatial")
		return QString("%1")
				.arg(thermal->getProperty(18));
	else if (key == "ptz.get_thermal_flip")
		return QString("%1")
				.arg(thermal->getProperty(19));
	else if (key == "ptz.get_image_update")
		return QString("%1")
				.arg(thermal->getProperty(20));
	else if (key == "ptz.head.2.zoom")
		return QString("%1")
				.arg(gungor->getZoom());
	else if (key == "ptz.head.2.focus")
		return QString("%1")
				.arg(gungor->getProperty(1));
	else if (key == "ptz.head.2.chip") {
		QString vrsn = QString::number(gungor->getProperty(2));
		vrsn = "V0" + vrsn[0] + "." +vrsn[1] +vrsn[2];
		return vrsn;
	} else if (key == "ptz.head.2.digi_zoom")
		return QString("%1")
				.arg(gungor->getProperty(5));
	else if (key == "ptz.head.2.cam_status")
		return QString("%1")
				.arg(gungor->getProperty(6));
	else if (key == "ptz.head.2.auto_focus_status")
		return QString("%1")
				.arg(gungor->getProperty(7));
	else if (key == "ptz.head.2.digi_zoom_status")
		return QString("%1")
				.arg(gungor->getProperty(8));

	return PtzpDriver::get(key);
}

int AryaDriver::set(const QString &key, const QVariant &value)
{
	mInfo("Set func: %s %d", qPrintable(key), value.toInt());
	if (key == "ptz.cmd.brightness") {
		thermal->setProperty(0, value.toUInt());
	} else if (key == "ptz.cmd.contrast")
		thermal->setProperty(1, value.toUInt());
	else if (key == "ptz.cmd.fov_change")
		thermal->setProperty(2, value.toUInt());
	else if (key == "ptz.cmd.cont_zoom")
		thermal->setProperty(3, value.toUInt());
	else if (key == "ptz.cmd.get_zoom_focus")
		thermal->setProperty(4, value.toUInt());
	else if (key == "ptz.cmd.focus")
		thermal->setProperty(5, value.toUInt());
	 else if (key == "ptz.cmd.nuc_table")
		thermal->setProperty(6, value.toUInt());
	else if (key == "ptz.cmd.polarity")
		thermal->setProperty(7, value.toUInt());
	else if (key == "ptz.cmd.reticle_onoff")
		thermal->setProperty(8, value.toUInt());
	else if (key == "ptz.cmd.thermal_digital_zoom")
		thermal->setProperty(9, value.toUInt());
	else if (key == "ptz.cmd.image_freeze")
		thermal->setProperty(10, value.toUInt());
	else if (key == "ptz.cmd.agc_select")
		thermal->setProperty(11, value.toUInt());
	else if (key == "ptz.cmd.brightness_change")
		thermal->setProperty(12, value.toUInt());
	else if (key == "ptz.cmd.contrast_change")
		thermal->setProperty(13, value.toUInt());
	else if (key == "ptz.cmd.reticle_intensity")
		thermal->setProperty(14, value.toUInt());
	else if (key == "ptz.cmd.nuc")
		thermal->setProperty(15, value.toUInt());
	else if (key == "ptz.cmd.ibit")
		thermal->setProperty(16, value.toUInt());
	else if (key == "ptz.cmd.ipm_change")
		thermal->setProperty(17, value.toUInt());
	else if (key == "ptz.cmd.hpf_gain_change")
		thermal->setProperty(18, value.toUInt());
	else if (key == "ptz.cmd.hpf_spatial_change")
		thermal->setProperty(19, value.toUInt());
	else if (key == "ptz.cmd.flip")
		thermal->setProperty(20, value.toUInt());
	else if (key == "ptz.cmd.image_update_speed")
		thermal->setProperty(21, value.toUInt());
	else if (key == "ptz.cmd.focus_stop")
		thermal->setProperty(5, value.toUInt());
	else if (key == "ptz.cmd.gungor.cam_stat")
		gungor->setProperty(0, value.toUInt());
	else if (key == "ptz.cmd.gungor.zoom_in")
		gungor->startZoomIn(value.toUInt());
	else if (key == "ptz.cmd.gungor.zoom_stop")
		gungor->stopZoom();
	else if (key == "ptz.cmd.gungor.zoom_out")
		gungor->startZoomOut(value.toUInt());
	else if (key == "ptz.cmd.gungor.zoom_position")
		gungor->setZoom(value.toUInt());
	else if (key == "ptz.cmd.gungor.focus_in")
		gungor->setProperty(6, value.toUInt());
	else if (key == "ptz.cmd.gungor.focus_stop")
		gungor->setProperty(7, value.toUInt());
	else if (key == "ptz.cmd.gungor.focus_out")
		gungor->setProperty(8, value.toUInt());
	else if (key == "ptz.cmd.gungor.focus_position")
		gungor->setProperty(9, value.toUInt());
	else if (key == "ptz.cmd.gungor.auto_focus")
		gungor->setProperty(10, value.toUInt());
	else if (key == "ptz.cmd.gungor.digi_zoom")
		gungor->setProperty(12, value.toUInt());
	else
		return PtzpDriver::set(key, value);
	return 0;
}
