#include "aryadriver.h"
#include "gungorhead.h"
#include "aryapthead.h"
#include "mgeothermalhead.h"
#include "ptzptcptransport.h"
#include "debug.h"

#include <QFile>

AryaDriver::AryaDriver(QObject *parent)
	: PtzpDriver(parent)
{
	aryapt = new AryaPTHead;
	thermal = new MgeoThermalHead;
	gungor = new MgeoGunGorHead;
	tcp1 = new PtzpTcpTransport(PtzpTransport::PROTO_STRING_DELIM);
	tcp2 = new PtzpTcpTransport(PtzpTransport::PROTO_BUFFERED);
	tcp3 = new PtzpTcpTransport(PtzpTransport::PROTO_BUFFERED);
	state = INIT;
	defaultPTHead = aryapt;
	defaultModuleHead = thermal;
	configLoad("config.json");
}

int AryaDriver::setTarget(const QString &targetUri)
{
	aryapt->setTransport(tcp1);
	thermal->setTransport(tcp2);
	gungor->setTransport(tcp3);
	int err = tcp1->connectTo(QString("%1:4001").arg(targetUri));
	if (err)
		return err;
	err = tcp2->connectTo(QString("%1:4002").arg(targetUri));
	if (err)
		return err;
	err = tcp3->connectTo(QString("%1:4003").arg(targetUri));
	if (err)
		return err;
	tcp1->enableQueueFreeCallbacks(true);
	tcp2->enableQueueFreeCallbacks(true);
	tcp3->enableQueueFreeCallbacks(true);
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
	case INIT:
		state = SYNC_ALL_MODULES;
		thermal->syncRegisters();
		gungor->syncRegisters();
		break;
	case SYNC_ALL_MODULES:
		if (thermal->getHeadStatus() == PtzpHead::ST_NORMAL
				|| gungor->getHeadStatus() == PtzpHead::ST_NORMAL) {
			state = NORMAL;
		}
		break;
	case NORMAL:
		static int once = 1;
		if (!once) {
			once = 1;
			thermal->startZoomIn(1);
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
	return  QVariant();
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
	else if (key == "ptz.cmd.gungor.zoom")
		gungor->setProperty(5, value.toUInt());
	else if (key == "ptz.cmd.gungor.focus_in")
		gungor->setProperty(6, value.toUInt());
	else if (key == "ptz.cmd.gungor.focus_stop")
		gungor->setProperty(7, value.toUInt());
	else if (key == "ptz.cmd.gungor.focus_out")
		gungor->setProperty(8, value.toUInt());
	else if (key == "ptz.cmd.gungor.focus")
		gungor->setProperty(9, value.toUInt());
	else if (key == "ptz.cmd.gungor.auto_focus")
		gungor->setProperty(10, value.toUInt());
	else if (key == "ptz.cmd.gungor.digi_zoom")
		gungor->setProperty(12, value.toUInt());

	else PtzpDriver::set(key, value);
	return 0;
}

void AryaDriver::configLoad(const QString filename)
{
	QFile f(filename);
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
		return ;
	const QByteArray &json = f.readAll();
	f.close();
	const QJsonDocument &doc = QJsonDocument::fromJson(json);

	QJsonObject root = doc.object();
	config.model = root["model"].toString();
	config.type = root["type"].toString();
	config.cam_module = root["cam_module"].toString();
	config.ptSupport = root["pan_tilt_support"].toInt();
	config.irLedSupport = root["ir_led_support"].toInt();
}
