#include "oem4kmodulehead.h"

#include <ecl/ptzp/ptzptransport.h>
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
	syncList << C_GET_ZOOM
			 << C_GET_BRIGHTNESS
			 << C_GET_CONTRAST
			 << C_GET_HUE
			 << C_GET_SATURATION
			 << C_GET_SHARPNESS;
	commandList = createCommandList();
	settings = {
		{"brightness", {C_SET_BRIGHTNESS,R_BRIGHTNESS}},
		{"contrast", {C_SET_CONTRAST,R_CONTRAST}},
		{"hue", {C_SET_HUE,R_HUE}},
		{"saturation", {C_SET_SATURATION,R_SATURATION}},
		{"sharpness", {C_SET_SHARPNESS,R_SHARPNESS}},
		{"zoomvalue", {C_SET_ZOOM,R_ZOOM}},
		{"focus", {NULL, NULL}}
	};
	nextSync = 0;
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
	head->add_capabilities(ptzp::PtzHead_Capability_REBOOT);
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
	nextSync = C_SET_FOCUS_STOP;
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
	if (nextSync != C_COUNT) {
		/* we are in sync mode, let's sync next */
		if (++nextSync == C_COUNT) {
			ffDebug() << "Oem4k register syncing completed, activating auto-sync";
		} else
			return commandList.at(nextSync).toUtf8();
	}
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
		if(settings.contains(key.toLower())) {
			if (settings[key.toLower()].second == R_ZOOM)
				setRegister(R_ZOOM, mapZoom(line.split("=").last().toInt()));
			else
				setRegister(settings[key.toLower()].second, line.split("=").last().toInt());
		}
	}
	return len;
}

uint Oem4kModuleHead::mapZoom(uint x)
{
	return (x - 10) * 5.12 + 1;
}
