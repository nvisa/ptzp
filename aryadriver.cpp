#include "aryadriver.h"
#include "gungorhead.h"
#include "aryapthead.h"
#include "mgeothermalhead.h"
#include "ptzptcptransport.h"
#include "debug.h"

#include <QFile>

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

	netman = new NetworkAccessManager();
	olay.pos = LEFT_UP;
	olay.posx = 0;
	olay.posy = 0;
	olay.textSize = 24;
	olay.disabled = false;

	connect(netman, SIGNAL(finished()), SLOT(overlayFinished()));
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
			thermal->loadRegisters("thermal.json");
			state = SYNC_THERMAL_MODULES;
		} else if (gungor->getSystemStatus() == 2) {
			gungor->syncRegisters();
			gungor->loadRegisters("gungor.json");
			state = SYNC_GUNGOR_MODULES;
		}
		checker->restart();
		break;
	case SYNC_THERMAL_MODULES:
		if(thermal->getHeadStatus() == PtzpHead::ST_NORMAL) {
			state = SYNC_GUNGOR_MODULES;
			gungor->syncRegisters();
			gungor->loadRegisters("gungor.json");
		}
		break;
	case SYNC_GUNGOR_MODULES:
		if(gungor->getHeadStatus() == PtzpHead::ST_NORMAL) {
			gungor->setFocusStepper();
			state = NORMAL;
		}
		break;
	case NORMAL:
		if (getChangeOverlayState()) {
			setZoomOverlay();
			setChangeOverlayState(false);
		}
		manageRegisterSaving();
		break;
	}
	PtzpDriver::timeout();
}

void AryaDriver::overlayFinished()
{
	if (netman->getLastError() != 0)
		mDebug("Overlay process got an error. Error code %d, %s", netman->getLastError(), qPrintable(netman->getLastErrorString()));
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
	else if (key == "video.overlay")
		return QString("%1;%2;%3;%4")
				.arg(olay.pos).arg(olay.posx).arg(olay.posy).arg(olay.textSize);
	else if (key == "video.overlay.disable")
		return olay.disabled;

	return PtzpDriver::get(key);
	return QVariant();
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
	else if (key == "video.overlay") {
		QString v = value.toString();
		if (v.contains(";")) {
			QStringList flds = v.split(";");
			if (flds.size() != 4)
				return -EINVAL;
			olay.pos = (OverlayPos)flds.at(0).toInt();
			olay.posx = flds.at(1).toInt();
			olay.posy = flds.at(2).toInt();
			olay.textSize = flds.at(3).toInt();
		}
	} else if (key == "video.overlay.disable")
		olay.disabled = value.toBool();

	else PtzpDriver::set(key, value);
	return 0;
}

QString AryaDriver::setZoomOverlayString(overlayForHead head)
{
	QString config = "ctoken=osdcfg01&action=setconfig";
	QString type = "type=0";
	QString display;
	if (olay.disabled)
		display = "display=0";
	else display = "display=2";
	QString position = QString("position=%1&posx=%2&posy=%3").arg(olay.pos).arg(olay.posx).arg(olay.posy);
	QString bgspan = "bgspan=0";
	QString textSize = QString("textsize=%1").arg(olay.textSize);
	QString dateTimeFormat = "datetimeformat=0";
	QString showDate = "showdate=0";
	QString showTime = "showtime=0";
	QString text = QString("text=%1").arg(QString("ZOOM %1x").arg(thermal->getZoom()));
	if (head == DAY)
		text = QString("text=%1").arg(QString("ZOOM %1x").arg(gungor->getZoom()));
	QString overlayData = config + "&"+
			type + "&" +
			display + "&" +
			position + "&" +
			bgspan + "&" +
			textSize + "&" +
			dateTimeFormat + "&" +
			showDate + "&" +
			showTime + "&" +
			text;
	return overlayData;
}

int AryaDriver::setZoomOverlay()
{
	QString overlayDataThermal = setZoomOverlayString(THERMAL);
	netman->post("50.23.169.211", "admin", "moxamoxa", "/moxa-cgi/imageoverlay.cgi", overlayDataThermal);

	QString overlayDataDay = setZoomOverlayString(DAY);
	netman->post("50.23.169.212", "admin", "moxamoxa", "/moxa-cgi/imageoverlay.cgi", overlayDataDay);
	return 0;
}

int AryaDriver::setOverlay(const QString data)
{
	if (vdParams.ip.isEmpty()) {
		mDebug("Video Device parameter missing.");
		return -1;
	}
	netman->post(vdParams.ip, vdParams.uname, vdParams.pass, "/moxa-cgi/imageoverlay.cgi", data);
	return 0;
}

void AryaDriver::setVideoDeviceParams(const QString &ip, const QString &uname, const QString &pass)
{
	vdParams.ip = ip;
	vdParams.pass = pass;
	vdParams.uname = uname;
}
