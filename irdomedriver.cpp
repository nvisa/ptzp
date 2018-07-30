#include "irdomedriver.h"
#include "oemmodulehead.h"
#include "ptzpserialtransport.h"
#include "irdomepthead.h"
#include "debug.h"

#include <QTimer>
#include <errno.h>

IRDomeDriver::IRDomeDriver(QObject *parent)
	: PtzpDriver(parent)
{
	headModule = new OemModuleHead;
	headDome = new IRDomePTHead;
	transport = new PtzpSerialTransport;
	state = INIT;
	defaultPTHead = headDome;
	defaultModuleHead = headModule;
}

PtzpHead *IRDomeDriver::getHead(int index)
{
	if (index == 0)
		return headModule;
	else if (index == 1)
		return headDome;
	return NULL;
}

int IRDomeDriver::setTarget(const QString &targetUri)
{
	headModule->setTransport(transport);
	headDome->setTransport(transport);
	int err = transport->connectTo(targetUri);
	if (err)
		return err;
	return 0;
}

QVariant IRDomeDriver::get(const QString &key)
{
	if (key.startsWith("ptz.module.reg.")) {
		uint reg = key.split(".").last().toUInt();
		return QString("%1").arg(headModule->getProperty(reg));
	}
	else if (key == "ptz.get_pan_angle")
		return QString("%1")
				.arg(headDome->getPanAngle());
	else if (key == "ptz.get_til_tangle")
		return QString("%1")
				.arg(headDome->getTiltAngle());
	else if (key == "ptz.get_zoom")
		return QString("%1")
				.arg(headModule->getZoom());
	else if (key == "ptz.get_exposure")
		return QString("%1")
				.arg(headModule->getProperty(3));
	else if (key == "ptz.get_gain_value")
		return QString("%1")
				.arg(headModule->getProperty(4));
	else if (key == "ptz.get_exp_compmode")
		return QString("%1")
				.arg(headModule->getProperty(5));
	else if (key == "ptz.get_exp_compval")
		return QString("%1")
				.arg(headModule->getProperty(6));
	else if (key == "ptz.get_gain_lim")
		return QString("%1")
				.arg(headModule->getProperty(7));
	else if (key == "ptz.get_shutter")
		return QString("%1")
				.arg(headModule->getProperty(8));
	else if (key == "ptz.get_noise_reduct")
		return QString("%1")
				.arg(headModule->getProperty(9));
	else if (key == "ptz.get_wdrstat")
		return QString("%1")
				.arg(headModule->getProperty(10));
	else if (key == "ptz.get_gamma")
		return QString("%1")
				.arg(headModule->getProperty(11));
	else if (key == "ptz.get_awb_mode")
		return QString("%1")
				.arg(headModule->getProperty(12));
	else if (key == "ptz.get_defog_mode")
		return QString("%1")
				.arg(headModule->getProperty(13));
	else if (key == "ptz.get_digi_zoom_stat")
		return QString("%1")
				.arg(headModule->getProperty(14));
	else if (key == "ptz.get_zoom_type")
		return QString("%1")
				.arg(headModule->getProperty(15));
	else if (key == "ptz.get_focus_mode")
		return QString("%1")
				.arg(headModule->getProperty(16));
	else if (key == "ptz.get_zoom_trigger")
		return QString("%1")
				.arg(headModule->getProperty(17));
	else if (key == "ptz.get_blc_stattus")
		return QString("%1")
				.arg(headModule->getProperty(18));
	else if (key == "ptz.get_ircf_status")
		return QString("%1")
				.arg(headModule->getProperty(19));
	else if (key == "ptz.get_auto_icr")
		return QString("%1")
				.arg(headModule->getProperty(20));
	else if (key == "ptz.get_program_ae_mode")
		return QString("%1")
				.arg(headModule->getProperty(21));
	else if (key == "ptz.get_flip")
		return QString("%1")
				.arg(headModule->getProperty(22));
	else if (key == "ptz.get_mirror")
		return QString("%1")
				.arg(headModule->getProperty(23));
	else if (key == "ptz.get_display_rot")
		return QString("%1")
				.arg(headModule->getProperty(24));
	else if (key == "ptz.get_digi_zoom_pos")
		return QString("%1")
				.arg(headModule->getProperty(25));
	else if (key == "ptz.get_optic_zoom_pos")
		return QString("%1")
				.arg(headModule->getProperty(26));
	else if (key == "ptz.get_device_definition"){
//		HAzırlanacak
	} else if (key == "ptz.get_pt_speed")
		return QString("%1")
				.arg(headModule->getProperty(27));
	else if (key == "ptz.get_zoom_speed")
		return QString("%1")
				.arg(headModule->getProperty(28));
	return PtzpDriver::get(key);

	return "almost_there";
	return QVariant();
}

int IRDomeDriver::set(const QString &key, const QVariant &value)
{
	if (key == "ptz.cmd.exposure_val"){
		headModule->setProperty(0,value.toUInt());
	} else if (key == "ptz.cmd.gain_value"){
		headModule->setProperty(1,value.toUInt());
	} else if (key == "ptz.cmd.exp_compmode") {
		headModule->setProperty(3,value.toUInt());
	} else if (key == "ptz.cmd.exp_compvalue") {
		headModule->setProperty(4,value.toUInt());
	} else if (key == "ptz.cmd.gain_lim") {
		headModule->setProperty(5,value.toUInt());
	} else if (key == "ptz.cmd.noise_reduct") {
		headModule->setProperty(7,value.toUInt());
	} else if (key == "ptz.cmd.shutter"){
		headModule->setProperty(6,value.toUInt());
	} else if (key == "ptz.cmd.wdr_stat"){
		headModule->setProperty(8,value.toUInt());
	} else if (key == "ptz.cmd.gamma") {
		headModule->setProperty(9,value.toUInt());
	} else if (key == "ptz.cmd.awb_mode"){
		headModule->setProperty(10,value.toUInt());
	} else if (key == "ptz.cmd.defog_mode"){
		headModule->setProperty(11,value.toUInt());
	} else if (key == "ptz.cmd.digi_zoom"){
		headModule->setProperty(12,value.toUInt());
	} else if (key == "ptz.cmd.zoom_type"){
		headModule->setProperty(13,value.toUInt());
	} else if (key == "ptz.cmd.focus_mode"){
		headModule->setProperty(14,value.toUInt());
	} else if (key == "ptz.cmd.zoom_trig"){
		headModule->setProperty(15,value.toUInt());
	} else if (key == "ptz.cmd.blc_stat"){
		headModule->setProperty(16,value.toUInt());
	} else if (key == "ptz.cmd.ırcf_stat"){
		headModule->setProperty(17,value.toUInt());
	} else if (key == "ptz.cmd.auto_icr"){
		headModule->setProperty(18,value.toUInt());
	} else if (key == "ptz.cmd.program_ae_mode"){
		headModule->setProperty(19,value.toUInt());
	} else if (key == "ptz.cmd.flip"){
		headModule->setProperty(20,value.toUInt());
	} else if (key == "ptz.cmd.mirror"){
		headModule->setProperty(21,value.toUInt());
	} else if (key == "ptz.cmd.one_push_af"){
			headModule->setProperty(22,value.toUInt());
	} else if (key == "ptz.cmd.display_rot"){
	if (value.toInt() == 0){
		headModule->setProperty(20, 1);
		headModule->setProperty(21, 0);
	} else if (value.toInt() == 1){
		headModule->setProperty(20, 0);
		headModule->setProperty(21, 1);
	} else if (value.toInt() == 2){
		headModule->setProperty(20, 1);
		headModule->setProperty(21, 1);
	} else if (value.toInt() == 3){
		headModule->setProperty(20, 0);
		headModule->setProperty(21, 0);
	}
	} else if (key == "ptz.cmd.setPtSpeed"){
		headModule->setProperty(23,value.toUInt());
	} else if (key == "ptz.cmd.setZoomSpeed"){
		headModule->setProperty(24,value.toUInt());
	} else if (key == "ptz.cmd.device_definition"){
//		Hazırlanacak
	}
	return PtzpDriver::set(key,value);

	return 0;
}

void IRDomeDriver::timeout()
{
	switch (state) {
	case INIT:
		headModule->syncRegisters();
		state = SYNC_HEAD_MODULE;
		break;
	case SYNC_HEAD_MODULE:
		if (headModule->getHeadStatus() == PtzpHead::ST_NORMAL) {
			state = SYNC_HEAD_DOME;
			headDome->syncRegisters();
		}
		break;
	case SYNC_HEAD_DOME:
		if (headDome->getHeadStatus() == PtzpHead::ST_NORMAL) {
			state = NORMAL;
			transport->enableQueueFreeCallbacks(true);
			timer->setInterval(1000);
		}
		break;
	case NORMAL:

		break;
	}

	PtzpDriver::timeout();
}

