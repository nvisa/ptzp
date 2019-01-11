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
	headModule = new OemModuleHead;
	headDome = new IRDomePTHead;
	state = INIT;
	defaultPTHead = headDome;
	defaultModuleHead = headModule;
	configLoad("config.json");
}

PtzpHead *IRDomeDriver::getHead(int index)
{
	if (index == 0)
		return headModule;
	else if (index == 1)
		return headDome;
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
	QStringList fields = targetUri.split(";");
	if (fields.size() == 1) {
		PtzpTransport *tp = new PtzpSerialTransport();
		headModule->setTransport(tp);
		headModule->enableSyncing(true);
		headDome->setTransport(tp);
		headDome->enableSyncing(true);
		tp->setMaxBufferLength(50);
		tp->enableQueueFreeCallbacks(true);
		if (tp->connectTo(fields[0]))
			return -EPERM;
	} else {
		PtzpTransport *tp1 = new PtzpSerialTransport();
		headModule->setTransport(tp1);
		headModule->enableSyncing(true);
		tp1->enableQueueFreeCallbacks(true);
		if (tp1->connectTo(fields[0]))
			return -EPERM;
		PtzpTransport *tp2 = new PtzpSerialTransport();
		headDome->setTransport(tp2);
		headDome->enableSyncing(true);
		tp2->enableQueueFreeCallbacks(true);
		if (tp2->connectTo(fields[1]))
			return -EPERM;
	}
	return 0;
}

QVariant IRDomeDriver::get(const QString &key)
{
	mInfo("Get func: %s", qPrintable(key));
	if (sleep)
		return 0;
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
	else if (key == "camera.model")
		return QString("%1")
				.arg(config.model);
	else if (key == "camera.type")
		return QString("%1")
				.arg(config.type);
	else if (key == "camera.cam_module")
		return QString("%1")
				.arg(config.cam_module);
	else if (key == "camera.pan_tilt_support")
		return QString("%1")
				.arg(config.ptSupport);
	else if (key == "camera.ir_led_support")
		return QString("%1")
				.arg(config.irLedSupport);
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

	return "almost_there";
	return QVariant();
}

int IRDomeDriver::set(const QString &key, const QVariant &value)
{
	mInfo("Set func: %s %d", qPrintable(key), value.toInt());
	if (key == "ptz.command.control")
		sleepMode(value.toBool());
	if (sleep)
		return 0;
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
	} else if (key == "mask.cmd.apply"){
		QStringList str = value.toString().split(".");
		headModule ->maskSet(str[0].toUInt(), str[1].toInt(), str[2].toInt());
	} else if (key == "mask.cmd.display") {
		QStringList str = value.toString().split(".");
		headModule-> maskDisplay(str[0].toUInt(), str[1].toInt());
	} else if (key == "ptz.cmd.ir_led"){
		headModule->setIRLed(value.toInt());
	} else if (key == "ptz.cmd.device_defintion" )
		headModule->setDeviceDefinition(value.toString());
	else if (key == "ptz.cmd.focus")
		headModule->setProperty(14, value.toUInt());
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
void IRDomeDriver::configLoad(const QString filename)
{
	if (!QFile::exists(filename)) {
		// create default
		QJsonDocument doc;
		QJsonObject o;
		o.insert("model",QString("Ekinoks"));
		if (getHeadCount() > 0) {
			o.insert("type" , QString("moving"));
			o.insert("pan_tilt_support", 1);
			o.insert("ir_led_support", 1);
		}
		else {
			o.insert("type" , QString("fixed"));
			o.insert("pan_tilt_support", 0);
			o.insert("ir_led_support", 0);
		}
		o.insert("cam_module", QString("PV8430_F2D"));
		doc.setObject(o);
		QFile f(filename);
		f.open(QIODevice::WriteOnly);
		f.write(doc.toJson());
		f.close();
	}

	QFile f(filename);
	if (!f.open(QIODevice::ReadOnly))
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
			headDome->syncRegisters();
		}
		break;
	case SYNC_HEAD_DOME:
		if (headDome->getHeadStatus() == PtzpHead::ST_NORMAL) {
			state = NORMAL;
			headModule->loadRegisters("oemmodule.json");
			timer->setInterval(1000);
		}
		break;
	case NORMAL:
		if(time->elapsed() >= 10000) {
			headModule->saveRegisters("oemmodule.json");
			time->restart();
		}
		break;
	}

	PtzpDriver::timeout();
}

