#include "irdomedriver.h"
#include "oemmodulehead.h"
#include "ptzpserialtransport.h"
#include "irdomepthead.h"
#include "debug.h"

#include <QFile>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <errno.h>

IRDomeDriver::IRDomeDriver(QObject *parent)
	: PtzpDriver(parent)
{
	defaultPTHead = NULL;
	defaultModuleHead = NULL;
	state = INIT;
}

PtzpHead *IRDomeDriver::getHead(int index)
{
	if (index == 0)
		return defaultModuleHead;
	else if (index == 1)
		return defaultPTHead;
	return NULL;
}

/**
 * @brief IRDomeDriver::setTarget
 * @param targetUri: This API using for initialize Serial Devices.
 * First parameter must contains Camera module serial console info.
 * Because sometimes callback flows need to close the Module serial console
 * (first initialized console).
 * Second and last parameter must contains PT serial console info.
 * This API can do two different consoles initialize, But it can't do for three or more consoles.
 * TargetUri Example-1: ttyS0?baud=9600 // Ekinoks cams
 * TargetUri Example-2: ttyS0?baud=9600;ttyTHS2?baud=9600 // USB-Aselsan
 * @return
 */
int IRDomeDriver::setTarget(const QString &targetUri)
{
	ptSupport = false;
	headModule = new OemModuleHead;
	headDome = new IRDomePTHead;
	QStringList fields = targetUri.split(";");
	if (fields.size() == 1) {
		tp = new PtzpSerialTransport();
		headModule->setTransport(tp);
		headModule->enableSyncing(true);
		headDome->setTransport(tp);
		headDome->enableSyncing(true);

		defaultPTHead = headDome;
		defaultModuleHead = headModule;

		tp->setMaxBufferLength(50);
		if (tp->connectTo(fields[0]))
			return -EPERM;
	} else {
		tp = new PtzpSerialTransport();
		headModule->setTransport(tp);
		headModule->enableSyncing(true);

		defaultModuleHead = headModule;
		if (tp->connectTo(fields[0]))
			return -EPERM;

		if (fields[1] != "null") {
			ptSupport = true;
			tp1 = new PtzpSerialTransport();
			headDome->setTransport(tp1);
			headDome->enableSyncing(true);

			defaultPTHead = headDome;
			if (tp1->connectTo(fields[1]))
				return -EPERM;

			SpeedRegulation sreg = getSpeedRegulation();
			sreg.enable = true;
			sreg.ipol = SpeedRegulation::LINEAR;
			sreg.minSpeed = 0.01;
			sreg.minZoom = 0;
			sreg.maxZoom = 16384;
			sreg.zoomHead = headModule;
			setSpeedRegulation(sreg);
		}
	}

	QStringList zoomLines, hLines, vLines;
	QFile f("/etc/smartstreamer/Zoom_value.txt");
	if (f.open(QIODevice::ReadOnly)) {
		zoomLines = QString::fromUtf8(f.readAll()).split("\n");
		f.close();
	}
	f.setFileName("/etc/smartstreamer/Hfov.txt");
	if (f.open(QIODevice::ReadOnly)) {
		hLines = QString::fromUtf8(f.readAll()).split("\n");
		f.close();
	}
	f.setFileName("/etc/smartstreamer/Vfov.txt");
	if (f.open(QIODevice::ReadOnly)) {
		vLines = QString::fromUtf8(f.readAll()).split("\n");
		f.close();
	}
	if (zoomLines.size() && zoomLines.size() == vLines.size() && zoomLines.size() == hLines.size()) {
		mDebug("Valid zoom mapping files found, parsing");
		std::vector<float> hmap, vmap;
		std::vector<int> zooms;
		for (int i = 0; i < zoomLines.size(); i++) {
			zooms.push_back(zoomLines[i].toInt());
			hmap.push_back(hLines[i].toFloat());
			vmap.push_back(vLines[i].toFloat());
		}
		headModule->getRangeMapper()->setLookUpValues(zooms);
		headModule->getRangeMapper()->addMap(hmap);
		headModule->getRangeMapper()->addMap(vmap);
	}

	return 0;
}

QVariant IRDomeDriver::get(const QString &key)
{
	mInfo("Get func: %s", qPrintable(key));
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
				.arg(headModule->getProperty(0));
	else if (key == "ptz.get_gain_value")
		return QString("%1")
				.arg(headModule->getProperty(1));
	else if (key == "ptz.get_exp_compmode")
		return QString("%1")
				.arg(headModule->getProperty(3));
	else if (key == "ptz.get_exp_compval")
		return QString("%1")
				.arg(headModule->getProperty(4));
	else if (key == "ptz.get_gain_lim")
		return QString("%1")
				.arg(headModule->getGainLimit());
	else if (key == "ptz.get_shutter")
		return QString("%1")
				.arg(headModule->getProperty(6));
	else if (key == "ptz.get_noise_reduct")
		return QString("%1")
				.arg(headModule->getProperty(7));
	else if (key == "ptz.get_wdrstat")
		return QString("%1")
				.arg(headModule->getProperty(8));
	else if (key == "ptz.get_gamma")
		return QString("%1")
				.arg(headModule->getProperty(9));
	else if (key == "ptz.get_awb_mode")
		return QString("%1")
				.arg(headModule->getProperty(10));
	else if (key == "ptz.get_defog_mode")
		return QString("%1")
				.arg(headModule->getProperty(11));
	else if (key == "ptz.get_digi_zoom_stat")
		return QString("%1")
				.arg(headModule->getProperty(12));
	else if (key == "ptz.get_zoom_type")
		return QString("%1")
				.arg(headModule->getProperty(13));
	else if (key == "ptz.get_focus_mode")
		return QString("%1")
				.arg(headModule->getProperty(14));
	else if (key == "ptz.get_zoom_trigger")
		return QString("%1")
				.arg(headModule->getProperty(15));
	else if (key == "ptz.get_blc_stattus")
		return QString("%1")
				.arg(headModule->getProperty(16));
	else if (key == "ptz.get_ircf_status")
		return QString("%1")
				.arg(headModule->getProperty(17));
	else if (key == "ptz.get_auto_icr")
		return QString("%1")
				.arg(headModule->getProperty(18));
	else if (key == "ptz.get_program_ae_mode")
		return QString("%1")
				.arg(headModule->getProperty(19));
	else if (key == "ptz.get_flip")
		return QString("%1")
				.arg(headModule->getProperty(20));
	else if (key == "ptz.get_mirror")
		return QString("%1")
				.arg(headModule->getProperty(21));
	else if (key == "ptz.get_display_rot")
		return QString("%1")
				.arg(headModule->getProperty(22));
	else if (key == "ptz.get_digi_zoom_pos")
		return QString("%1")
				.arg(headModule->getProperty(23));
	else if (key == "ptz.get_optic_zoom_pos")
		return QString("%1")
				.arg(headModule->getProperty(24));
	else if (key == "ptz.get_device_definition"){
//		HAzırlanacak
	} else if (key == "ptz.get_pt_speed")
		return QString("%1")
				.arg(headModule->getProperty(25));
	else if (key == "ptz.get_zoom_speed")
		return QString("%1")
				.arg(headModule->getProperty(26));
	else if (key == "ptz.get_device_definiton")
		return QString("%1")
				.arg(headModule->getDeviceDefinition());
	else if (key == "ptz.get _zoom_ratio")
		return QString("%1")
				.arg(headModule->getZoomRatio());
	else if (key == "ptz.get_exposure_target")
		return QString("%1")
				.arg(headModule->getProperty(27));
	else if (key == "ptz.get_shutter_lim")
		return QString("%1")
				.arg(headModule->getShutterLimit());
	else if (key == "ptz.get_iris_lim")
		return QString("%1")
				.arg(headModule->getIrisLimit());
	else if (key == "ptz.get_ir_led_level")
		return QString("%1")
				.arg(headModule->getIRLed());
	else return PtzpDriver::get(key);

	return QVariant();
}

int IRDomeDriver::set(const QString &key, const QVariant &value)
{
	mInfo("Set func: %s %d", qPrintable(key), value.toInt());
	if (key == "ptz.cmd.exposure_val"){
		headModule->setProperty(0,value.toUInt());
	} else if (key == "ptz.cmd.exposure_target")
		headModule->setProperty(1,value.toUInt());
	else if (key == "ptz.cmd.gain_value"){
		headModule->setProperty(2,value.toUInt());
	} else if (key == "ptz.cmd.exp_compmode") {
		headModule->setProperty(4,value.toUInt());
	} else if (key == "ptz.cmd.exp_compvalue") {
		headModule->setProperty(5,value.toUInt());
	} else if (key == "ptz.cmd.gain_lim") {
		QStringList str = value.toString().split(",");
		headModule->setGainLimit(str[0].toUInt(), str[1].toInt());
	} else if (key == "ptz.cmd.noise_reduct") {
		headModule->setProperty(8,value.toUInt());
	} else if (key == "ptz.cmd.shutter"){
		headModule->setProperty(7,value.toUInt());
	} else if (key == "ptz.cmd.wdr_stat"){
		headModule->setProperty(9,value.toUInt());
	} else if (key == "ptz.cmd.gamma") {
		headModule->setProperty(10,value.toUInt());
	} else if (key == "ptz.cmd.awb_mode"){
		headModule->setProperty(11,value.toUInt());
	} else if (key == "ptz.cmd.defog_mode"){
		headModule->setProperty(12,value.toUInt());
	} else if (key == "ptz.cmd.digi_zoom"){
		headModule->setProperty(13,value.toUInt());
	} else if (key == "ptz.cmd.zoom_type"){
		headModule->setProperty(14,value.toUInt());
	} else if (key == "ptz.cmd.focus_mode"){
		headModule->setProperty(16,value.toUInt());
	} else if (key == "ptz.cmd.zoom_trig"){
		headModule->setProperty(17,value.toUInt());
	} else if (key == "ptz.cmd.blc_stat"){
		headModule->setProperty(18,value.toUInt());
	} else if (key == "ptz.cmd.ircf_stat"){
		headModule->setProperty(19,value.toUInt());
	} else if (key == "ptz.cmd.auto_icr"){
		headModule->setProperty(20,value.toUInt());
	} else if (key == "ptz.cmd.program_ae_mode"){
		headModule->setProperty(21,value.toUInt());
	} else if (key == "ptz.cmd.flip"){
		headModule->setProperty(22,value.toUInt());
	} else if (key == "ptz.cmd.mirror"){
		headModule->setProperty(23,value.toUInt());
	} else if (key == "ptz.cmd.one_push_af"){
			headModule->setProperty(24,value.toUInt());
	} else if (key == "ptz.cmd.display_rot"){
		if (value.toInt() == 0){
			headModule->setProperty(22, 1);
			headModule->setProperty(23, 0);
		} else if (value.toInt() == 1){
			headModule->setProperty(22, 0);
			headModule->setProperty(23, 1);
		} else if (value.toInt() == 2){
			headModule->setProperty(22, 1);
			headModule->setProperty(23, 1);
		} else if (value.toInt() == 3){
			headModule->setProperty(22, 0);
			headModule->setProperty(23, 0);
		}
	} else if (key == "ptz.cmd.zoom_point") {
		headModule->setZoom(value.toUInt());
	} else if (key == "ptz.cmd.ir_led"){
		headModule->setIRLed(value.toInt());
	} else if (key == "ptz.cmd.device_defintion" )
		headModule->setDeviceDefinition(value.toString());
	else if (key == "ptz.cmd.focus")
		headModule->setProperty(15, value.toUInt());
	else if (key == "ptz.cmd.shutter_top_bot_lim"){
		QStringList str = value.toString().split(",");
		headModule->setShutterLimit(str[0].toUInt(), str[1].toInt());
	} else if (key == "ptz.cmd.iris_top_bot_lim"){
		QStringList str = value.toString().split(",");
		headModule->setIrisLimit(str[0].toUInt(), str[1].toInt());
	}
	else if (key == "camera.clock_invert")
			headModule->clockInvert(value.toBool());
	else return PtzpDriver::set(key,value);

	return 0;
}

void IRDomeDriver::timeout()
{
//	mLog("Driver state: %d", state);
	switch (state) {
	case INIT:
		headModule->syncRegisters();
		state = SYNC_HEAD_MODULE;
		break;
	case SYNC_HEAD_MODULE:
		if (headModule->getHeadStatus() == PtzpHead::ST_NORMAL) {
			state = SYNC_HEAD_DOME;
			if(ptSupport == 0) {
				if (registerSavingEnabled)
					state = LOAD_MODULE_REGISTERS;
				else
					state = NORMAL;
			} else
				headDome->syncRegisters();
			tp->enableQueueFreeCallbacks(true);
		}
		break;
	case SYNC_HEAD_DOME:
		if (headDome->getHeadStatus() == PtzpHead::ST_NORMAL) {
			if (registerSavingEnabled)
				state = LOAD_MODULE_REGISTERS;
			else
				state = NORMAL;
			//headModule->loadRegisters("oemmodule.json");
			timer->setInterval(1000);
			tp1->enableQueueFreeCallbacks(true);
		}
		break;
	case LOAD_MODULE_REGISTERS:
		headModule->loadRegisters("head0.json");
		state = NORMAL;
		break;
	case NORMAL: {
		manageRegisterSaving();
		if (doStartupProcess) {
			doStartupProcess = false;
			getStartupProcess();
		}
		break;
	}
	}

	PtzpDriver::timeout();
}

