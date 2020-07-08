#include "oem4kmodulehead.h"

#include <ptzptransport.h>
#include <gpiocontroller.h>
#include "debug.h"

enum CommandsList {

	C_SET_ZOOM,
	C_SET_BRIGHTNESS,
	C_SET_CONTRAST,
	C_SET_HUE,
	C_SET_SATURATION,
	C_SET_SHARPNESS,

	C_SET_ZOOM_IN,
	C_SET_ZOOM_OUT,
	C_SET_ZOOM_STOP,
	C_SET_FOCUS_IN,
	C_SET_FOCUS_OUT,
	C_SET_FOCUS_STOP,

	C_SET_CAMMODE,

	C_GET_CAMMODE,
	C_GET_BRIGHTNESS,
	C_GET_CONTRAST,
	C_GET_HUE,
	C_GET_SATURATION,
	C_GET_SHARPNESS,
	C_GET_ZOOM,
	C_COUNT

};

static QStringList createCommandList()
{
	QStringList cmdList;
	cmdList << QString("/cgi-bin/ptz.cgi?action=start&channel=0&code=PositionABS&arg1=1&arg2=1&arg3=%1");
	cmdList << QString("/cgi-bin/configManager.cgi?action=setConfig&VideoColor[0][0].Brightness=%1");
	cmdList << QString("/cgi-bin/configManager.cgi?action=setConfig&VideoColor[0][0].Contrast=%1");
	cmdList << QString("/cgi-bin/configManager.cgi?action=setConfig&VideoColor[0][0].Hue=%1");
	cmdList << QString("/cgi-bin/configManager.cgi?action=setConfig&VideoColor[0][0].Saturation=%1");
	cmdList << QString("/cgi-bin/configManager.cgi?action=setConfig&VideoInSharpness[0][0].Sharpness=%1");

	cmdList << QString("/cgi-bin/ptz.cgi?action=start&channel=0&code=ZoomTele&arg1=0&arg2=0&arg3=0&arg4=0");
	cmdList << QString("/cgi-bin/ptz.cgi?action=start&channel=0&code=ZoomWide&arg1=0&arg2=0&arg3=0&arg4=0");
	cmdList << QString("/cgi-bin/ptz.cgi?action=stop&channel=0&code=ZoomTele&arg1=0&arg2=0&arg3=0&arg4=0");
	cmdList << QString("/cgi-bin/ptz.cgi?action=start&channel=0&code=FocusNear&arg1=1&arg2=1&arg3=0&arg4=0");
	cmdList << QString("/cgi-bin/ptz.cgi?action=start&channel=0&code=FocusFar&arg1=1&arg2=1&arg3=0&arg4=0");
	cmdList << QString("/cgi-bin/ptz.cgi?action=stop&channel=0&code=FocusFar&arg1=1&arg2=1&arg3=0&arg4=0");

	cmdList << QString("/cgi-bin/configManager.cgi?action=setConfig&VideoInDayNight[0][0].Mode=%1");

	cmdList << QString("/cgi-bin/configManager.cgi?action=getConfig&name=VideoInDayNight[0][0].Mode");
	cmdList << QString("/cgi-bin/configManager.cgi?action=getConfig&name=VideoColor[0][0].Brightness");
	cmdList << QString("/cgi-bin/configManager.cgi?action=getConfig&name=VideoColor[0][0].Contrast");
	cmdList << QString("/cgi-bin/configManager.cgi?action=getConfig&name=VideoColor[0][0].Hue");
	cmdList << QString("/cgi-bin/configManager.cgi?action=getConfig&name=VideoColor[0][0].Saturation");
	cmdList << QString("/cgi-bin/configManager.cgi?action=getConfig&name=VideoInSharpness[0][0].Sharpness");
	cmdList << QString("/cgi-bin/ptz.cgi?action=getStatus");
	return cmdList;
}

Oem4kModuleHead::Oem4kModuleHead()
{
	commandList = createCommandList();
	settings = {
		{"brightness", {C_SET_BRIGHTNESS,R_BRIGHTNESS}},
		{"contrast", {C_SET_CONTRAST,R_CONTRAST}},
		{"hue", {C_SET_HUE,R_HUE}},
		{"saturation", {C_SET_SATURATION,R_SATURATION}},
		{"sharpness", {C_SET_SHARPNESS,R_SHARPNESS}},
		{"zoom_pos_level", {C_SET_ZOOM,R_ZOOM}},
		{"focus", {NULL, NULL}},
		{"choose_cam", {C_SET_CAMMODE, R_CAMMODE}}
	};

	camModes = {
		{ "Brightness", 0 }, //auto
		{ "Color", 1 },		 //day
		{ "BlackWhite", 2 }, //night
	};

	_mapCap = {
		{ptzp::PtzHead_Capability_BRIGHTNESS, {C_SET_BRIGHTNESS,R_BRIGHTNESS}},
		{ptzp::PtzHead_Capability_CONTRAST, {C_SET_CONTRAST,R_CONTRAST}},
		{ptzp::PtzHead_Capability_HUE, {C_SET_HUE,R_HUE}},
		{ptzp::PtzHead_Capability_SATURATION, {C_SET_SATURATION,R_SATURATION}},
		{ptzp::PtzHead_Capability_SHARPNESS, {C_SET_SHARPNESS,R_SHARPNESS}},
		{ptzp::PtzHead_Capability_DAY_NIGHT, {C_SET_CAMMODE, R_CAMMODE}}
	};

	nextSync = 0;

	switchCmd = false;
	tpIRC = new PtzpSerialTransport();
	tpIRC->setMaxBufferLength(20);
	tpIRC->connectTo("ttyUSB0?baud=115200");
}

void Oem4kModuleHead::fillCapabilities(ptzp::PtzHead *head)
{
	head->add_capabilities(ptzp::PtzHead_Capability_ZOOM);
	head->add_capabilities(ptzp::PtzHead_Capability_FOCUS);
	head->add_capabilities(ptzp::PtzHead_Capability_BRIGHTNESS);
	head->add_capabilities(ptzp::PtzHead_Capability_CONTRAST);
	head->add_capabilities(ptzp::PtzHead_Capability_HUE);
	head->add_capabilities(ptzp::PtzHead_Capability_SATURATION);
	head->add_capabilities(ptzp::PtzHead_Capability_SHARPNESS);
	head->add_capabilities(ptzp::PtzHead_Capability_DAY_NIGHT);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_BRIGHTNESS);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_CONTRAST);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_HUE);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_SATURATION);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_SHARPNESS);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_ZOOM);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_FOCUS);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_DAY_VIEW);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_MULTI_ROI);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_DETECTION);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_JOYSTICK_CONTROL);
}

QVariant Oem4kModuleHead::getCapabilityValues(ptzp::PtzHead_Capability c)
{
	return getRegister(_mapCap[c].second);
}

void Oem4kModuleHead::setCapabilityValues(ptzp::PtzHead_Capability c, uint val)
{
	setProperty(_mapCap[c].first, val);
}

int Oem4kModuleHead::focusIn(int speed)
{

	Q_UNUSED(speed);
	sendCommand(commandList.at(C_SET_FOCUS_IN));
	return 0;
}

int Oem4kModuleHead::focusOut(int speed)
{
	Q_UNUSED(speed);
	sendCommand(commandList.at(C_SET_FOCUS_OUT));
	return 0;
}

int Oem4kModuleHead::focusStop()
{
	sendCommand(commandList.at(C_SET_FOCUS_STOP));
	return 0;
}

int Oem4kModuleHead::startZoomIn(int speed)
{
	Q_UNUSED(speed);
	sendCommand(commandList.at(C_SET_ZOOM_IN));
	return 0;
}

int Oem4kModuleHead::startZoomOut(int speed)
{
	Q_UNUSED(speed);
	sendCommand(commandList.at(C_SET_ZOOM_OUT));
	return 0;
}

int Oem4kModuleHead::stopZoom()
{
	sendCommand(commandList.at(C_SET_ZOOM_STOP));
	return 0;
}

int Oem4kModuleHead::getZoom()
{
	return getRegister(R_ZOOM);
}

int Oem4kModuleHead::setZoom(uint pos)
{
	sendCommand(commandList.at(C_SET_ZOOM).arg(pos));
	setRegister(R_ZOOM, pos);
	return 0;
}

void Oem4kModuleHead::setProperty(uint r, uint x)
{
	if(r == C_SET_CAMMODE) {
		setRegister(R_CAMMODE, x);
		sendCommand(commandList.at(C_SET_CAMMODE).arg(camModes.key(x)));
	}

	if(r == C_SET_ZOOM)
		setRegister(R_ZOOM, x);
	if(r == C_SET_BRIGHTNESS)
		setRegister(R_BRIGHTNESS, x);
	if(r == C_SET_CONTRAST)
		setRegister(R_CONTRAST, x);
	if(r == C_SET_HUE)
		setRegister(R_HUE, x);
	if(r == C_SET_SATURATION)
		setRegister(R_SATURATION, x);
	if(r == C_SET_SHARPNESS)
		setRegister(R_SHARPNESS, x);
	sendCommand(commandList.at(r).arg(x));
}

uint Oem4kModuleHead::getProperty(uint r)
{
	return getRegister(r);
}

int Oem4kModuleHead::syncRegisters()
{
	if (!transport)
		return -ENOENT;
	nextSync = C_GET_CAMMODE;
	return 0;
}

int Oem4kModuleHead::getHeadStatus()
{
	if (nextSync == C_COUNT)
		return ST_NORMAL;
	return ST_SYNCING;
}

int Oem4kModuleHead::sendCommand(const QString &key)
{
	return transport->send(key.toUtf8());
}

QByteArray Oem4kModuleHead::transportReady()
{
	QByteArray ba;
	if(nextSync != C_COUNT) {
		ba = commandList.at(nextSync).toUtf8();
		if(++nextSync == C_COUNT)
			ffDebug() << "Oem4k register syncing completed, activating auto-sync";
		return ba;
	}

	switchCmd = !switchCmd;
	if(switchCmd)
		return commandList.at(C_GET_CAMMODE).toUtf8();
	else
		return commandList.at(C_GET_ZOOM).toUtf8();
}

int Oem4kModuleHead::dataReady(const unsigned char *bytes, int len)
{
	QString data = QString::fromUtf8((const char *) bytes, len);
	QStringList lines = data.split("\n");

	/* update registers */
	foreach (QString line, lines) {
		line = line.remove("\r");
		line = line.split(".").last();
		QString key = line.split("=").first();
		if(key == "ZoomValue")
			setRegister(R_ZOOM, mapZoom(line.split("=").last().toInt()));
		else if(key == "Mode")
			setRegister(R_CAMMODE, camModes.value(line.split("=").last()));
		else if(settings.contains(key.toLower()))
			setRegister(settings[key.toLower()].second, line.split("=").last().toInt());
		pingTimer.restart();
	}

	manageIRC();
	return len;
}

uint Oem4kModuleHead::mapZoom(uint x)
{
	uint z = (x - 10) * 6.4;
	return z > 128 ? 128 : z;
}

static unsigned char ircInit[20] = {
	0x3a, 0xff, 0xb1, 0x00, 0xa3, 0x00, 0x09, 0x5e,
	0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x5c
};

void Oem4kModuleHead::manageIRC()
{
	unsigned char pb[20];
	memcpy((void *) pb, (void *) ircInit, sizeof(ircInit));

	int zoomlvl = getRegister(R_ZOOM) / 43;

	if(getRegister(R_CAMMODE) != camModes.value("BlackWhite")) {
		zoomlvl = -1;
	}

	switch(zoomlvl){
	case 2:
		pb[11] = 0xff;
	case 1:
		pb[10] = 0xff;
	case 0:
		pb[9] = 0xff;
		break;
	default:
		break;
	}

	int chksum = 0;
	for(size_t i = 1; i < 17; i++) {
		chksum += pb[i];
	}
	pb[18] = chksum;

	tpIRC->send((const char*) pb, 20);
}
