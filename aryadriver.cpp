#include "aryadriver.h"
#include "mgeothermalhead.h"
#include "ptzptcptransport.h"
#include "aryapthead.h"
#include "debug.h"

AryaDriver::AryaDriver(QObject *parent)
	: PtzpDriver(parent)
{
	aryapt = new AryaPTHead;
	thermal = new MgeoThermalHead;
	tcp1 = new PtzpTcpTransport(PtzpTransport::PROTO_STRING_DELIM);
	tcp2 = new PtzpTcpTransport(PtzpTransport::PROTO_BUFFERED);
	state = INIT;
	defaultPTHead = aryapt;
	defaultModuleHead = thermal;
}

int AryaDriver::setTarget(const QString &targetUri)
{
	aryapt->setTransport(tcp1);
	thermal->setTransport(tcp2);
	int err = tcp1->connectTo(QString("%1:4001").arg(targetUri));
	if (err)
		return err;
	err = tcp2->connectTo(QString("%1:4002").arg(targetUri));
	if (err)
		return err;
	return 0;
}

PtzpHead *AryaDriver::getHead(int index)
{
	if (index == 0)
		return aryapt;
	return thermal;
}

void AryaDriver::timeout()
{
	switch (state) {
	case INIT:
		state = SYNC_THERMAL_MODULE;
		thermal->syncRegisters();
		break;
	case SYNC_THERMAL_MODULE:
		if (thermal->getHeadStatus() == PtzpHead::ST_NORMAL) {
			state = NORMAL;
		}
		break;
	case NORMAL:
		//ffDebug() << "running";
		static int once = 1;
		if (!once) {
			once = 1;
			//aryapt->panTiltStop();
			thermal->startZoomIn(1);
		}
		break;
	}

	PtzpDriver::timeout();
}

QVariant AryaDriver::get(const QString &key)
{
	if (key == "ptz.get_cooled_down")
		return QString("%1")
			.arg(thermal->getProperty(0));
	else if (key == "ptz.get_fov_change")
		return QString("%1")
			.arg(thermal->getProperty(1));
	else if (key == "ptz.get_zoom")
		return QString("%1")
				.arg(thermal->getProperty(2));
	else if (key == "ptz.get_focus")
		return QString("%1")
			.arg(thermal->getProperty(3));
	else if (key == "ptz.get_angle")
		return QString("%1")
			.arg(thermal->getProperty(4));
	else if (key == "ptz.get_nuc_table")
		return QString("%1")
			.arg(thermal->getProperty(5));
	else if (key == "ptz.get_polarity")
		return QString("%1")
				.arg(thermal->getProperty(6));
	else if (key == "ptz.get_reticle")
		return QString("%1")
				.arg(thermal->getProperty(7));
	else if (key == "ptz.get_digital_zoom")
		return QString("%1")
				.arg(thermal->getProperty(8));
	else if (key == "ptz.get_image_freeze")
		return QString("%1")
				.arg(thermal->getProperty(9));
	else if (key == "ptz.get_agc")
		return QString("%1")
				.arg(thermal->getProperty(10));
	else if (key == "ptz.get_brightness")
		return QString("%1")
				.arg(thermal->getProperty(11));
	else if (key== "ptz.get_contrast")
		return QString("%1")
				.arg((thermal->getProperty(12)));
	else if (key == "ptz.get_intensity")
		return QString("%1")
				.arg(thermal->getProperty(13));
	else if (key == "ptz.get_nuc")
		return QString("%1")
				.arg(thermal->getProperty(14));
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
	else if (key == "ptz.get_image_update")
		return QString("%1")
				.arg(thermal->getProperty(19));
	else PtzpDriver::get(key);
	return  QVariant();
}

int AryaDriver::set(const QString &key, const QVariant &value)
{
	if (key == "ptz.cmd.cooled_down")
		thermal->setProperty(0, value.toUInt());
	else if (key == "ptz.cmd.contrast")
		thermal->setProperty(1, value.toUInt());
	else if (key == "ptz.cmd.fov")
		thermal->setProperty(2, value.toUInt());
	else if (key == "ptz.cmd.cont_zoom")
		thermal->setProperty(3, value.toUInt());
	else if (key == "ptz.cmd.get_zoom_focus")
		thermal->setProperty(4, value.toUInt());
	else if (key == "ptz.cmd.focus")
		thermal->setProperty(5, value.toUInt());
	else if (key == "ptz.cmd.nuc_select")
		thermal->setProperty(6, value.toUInt());
	else if (key == "ptz.cmd.pol_change")
		thermal->setProperty(7, value.toUInt());
	else if (key == "ptz.cmd.reticle_onoff")
		thermal->setProperty(8, value.toUInt());
	else if (key == "ptz.cmd.digital_zoom")
		thermal->setProperty(9, value.toUInt());
	else if (key == "ptz.cmd.freeze_image")
		thermal->setProperty(10, value.toUInt());
	else if (key == "ptz.cmd.agc_select")
		thermal->setProperty(11, value.toUInt());
	else if (key == "ptz.cmd.brightness_change")
		thermal->setProperty(12, value.toUInt());
	else if (key == "ptz.cmd.contrast_change")
		thermal->setProperty(13, value.toUInt());
	else if (key == "ptz.cmd.reticle_change")
		thermal->setProperty(14, value.toUInt());
	else if (key == "ptz.cmd.nuc")
		thermal->setProperty(15, value.toUInt());
	else if (key == "ptz.cmd.ipm_change")
		thermal->setProperty(16, value.toUInt());
	else if (key == "ptz.cmd.hpf_gain_change")
		thermal->setProperty(17, value.toUInt());
	else if (key == "ptz.cmd.hpf_spatial_change")
		thermal->setProperty(18, value.toUInt());
	else if (key == "ptz.cmd.flip")
		thermal->setProperty(19, value.toUInt());
	else if (key == "ptz.cmd.image_update_speed")
		thermal->setProperty(20, value.toUInt());
	else PtzpDriver::set(key, value);
	return 0;
}
