#include "flirmodulehead.h"

#include "debug.h"
#include <assert.h>
#include <ecl/ptzp/ptzptransport.h>

/*
 * [CR] [fo] Hangi enum kullanılacak?
 */
enum Commands {
	CGI_SET_FOCUS_INC,
	CGI_SET_FOCUS_DEC,
	CGI_SET_FOCUS_POS,
	CGI_SET_FOCUS_STOP,
	CGI_SET_FOCUS_MODE,

	CGI_GET_FOCUS_MODE,
	CGI_GET_FOCUS_POS,

	CGI_COUNT
};

enum ComandList {
	C_ZOOM_STOP,
	C_ZOOM_IN,
	C_ZOOM_OUT,
	C_SET_MANUAL_FOCUS,
	C_SET_AUTO_FOCUS,
	C_FOCUS_INC,
	C_FOCUS_DEC,
	C_FOCUS_STOP,
	C_GET_CAMPOS,
	C_SET_FOCUS_POS,
	C_GET_CAM_MODES,
	C_GET_LASER_STATE,
	C_COUNT,
};

static QStringList createCommandList()
{
	QStringList cmdList;
	cmdList << QString("/cgi-bin/"
					   "control.cgi?action=update&group=PTZCTRL&channel=0&"
					   "PTZCTRL.action=40&nRanId=96325");
	cmdList << QString("/cgi-bin/"
					   "control.cgi?action=update&group=PTZCTRL&channel=0&"
					   "PTZCTRL.action=13&PTZCTRL.speed=%1&nRanId=341425");
	cmdList << QString("/cgi-bin/"
					   "control.cgi?action=update&group=PTZCTRL&channel=0&"
					   "PTZCTRL.action=14&PTZCTRL.speed=%1&nRanId=341425");
	cmdList << QString(
		"/cgi-bin/"
		"param.cgi?action=list&group=CAM&channel=0&CAM.autoFocusMode=%1");
	cmdList << QString("/cgi-bin/"
					   "param.cgi?action=list&group=CAM&channel=0&CAM."
					   "autoFocusMode=%1"); // dene bunu auto focus 1
	cmdList << QString("/cgi-bin/"
					   "control.cgi?action=update&group=PTZCTRL&channel=0&"
					   "PTZCTRL.action=15&PTZCTRL.speed=%1&nRanId=341425");
	cmdList << QString("/cgi-bin/"
					   "control.cgi?action=update&group=PTZCTRL&channel=0&"
					   "PTZCTRL.action=16&PTZCTRL.speed=%1&nRanId=341425");
	cmdList << QString("/cgi-bin/"
					   "control.cgi?action=update&group=PTZCTRL&channel=0&"
					   "PTZCTRL.action=41&PTZCTRL.speed=%1&nRanId=341425");
	cmdList << QString("/cgi-bin/param.cgi?action=list&group=CAMPOS&channel=0");
	cmdList << QString(
		"/cgi-bin/"
		"param.cgi?action=update&group=CAMPOS&channel=0&CAMPOS.focuspos=%1");
	cmdList << QString("/cgi-bin/param.cgi?action=list&group=CAM&channel=0");
	cmdList << QString("/cgi-bin/control.cgi?action=list&group=LASER&LASER.cmdType=100");
	return cmdList;
}

FlirModuleHead::FlirModuleHead()
{
	setHeadName("module_head");
	syncList << C_GET_CAMPOS << C_GET_LASER_STATE;
	next = 0;
	zoomPos = 0;
	focusPos = 0;
	settings = {
		{"focus", {NULL, NULL}},
		{"focus_pos", {CGI_SET_FOCUS_POS, CGI_GET_FOCUS_POS}},
		{"focus_mode", {CGI_SET_FOCUS_MODE, CGI_GET_FOCUS_MODE}},
	};
	commandList = createCommandList();
	assert(commandList.size() == C_COUNT);
	laserTimer.restart();
	laserCIT = true;
}

int FlirModuleHead::getLaserState()
{
	return sendCommand(commandList.at(C_GET_LASER_STATE));
}

bool FlirModuleHead::getLaserCIT()
{
	return laserCIT;
}

int FlirModuleHead::getLaserTimer()
{
	return laserTimer.elapsed();
}

void FlirModuleHead::fillCapabilities(ptzp::PtzHead *head)
{
	head->add_capabilities(ptzp::PtzHead_Capability_ZOOM);
}

int FlirModuleHead::startZoomIn(int speed)
{
	return sendCommand(commandList.at(C_ZOOM_IN).arg(speed));
}

int FlirModuleHead::startZoomOut(int speed)
{
	return sendCommand(commandList.at(C_ZOOM_OUT).arg(speed));
}

int FlirModuleHead::stopZoom()
{
	return sendCommand(commandList.at(C_ZOOM_STOP));
}

int FlirModuleHead::getZoom()
{
	return zoomPos;
}

int FlirModuleHead::getFocus()
{
	return focusPos;
}

int FlirModuleHead::focusIn(int speed)
{
	sendCommand(commandList.at(C_SET_MANUAL_FOCUS).arg(0));
	return sendCommand(commandList.at(C_FOCUS_INC).arg(speed));
}

int FlirModuleHead::focusOut(int speed)
{
	sendCommand(commandList.at(C_SET_MANUAL_FOCUS).arg(0));
	return sendCommand(commandList.at(C_FOCUS_DEC).arg(speed));
}

int FlirModuleHead::focusStop()
{
	return sendCommand(commandList.at(C_FOCUS_STOP));
}

/*[CR] [fo] Neden ayrı fonksiyonlarda?
 * Kullanım esnasında bunlar içinde api yazılması gerekmez mi?
 * setProperty() zaten bu kullanımlar için var.
 * (setFocusMode, setFocusValue, getFocusMode)
 */
int FlirModuleHead::setFocusMode(uint x)
{
	return sendCommand(commandList.at(C_SET_MANUAL_FOCUS).arg(x));
}

int FlirModuleHead::setFocusValue(uint x)
{
	return sendCommand(commandList.at(C_SET_FOCUS_POS).arg(x));
}

int FlirModuleHead::getFocusMode()
{
	if (camModes.contains("focusMode"))
		return camModes.value("focusMode").toInt();
	return -1;
}

void FlirModuleHead::setProperty(uint r, uint x)
{
	if (r == CGI_SET_FOCUS_POS)
		setFocusValue(x);
	else if (r == CGI_SET_FOCUS_MODE)
		setFocusMode(x);
}

uint FlirModuleHead::getProperty(uint r)
{
	if (r == CGI_GET_FOCUS_POS)
		return focusPos;
	else if (r == CGI_GET_FOCUS_MODE)
		if (camModes.contains("focusMode"))
			return camModes.value("focusMode").toUInt();
	return -1;
}

int FlirModuleHead::sendCommand(const QString &key)
{
	return transport->send(key.toUtf8());
}

int FlirModuleHead::getNext()
{
	if (next == 0)
		next = syncList.size();
	next--;
	return syncList.at(next);
}

QByteArray FlirModuleHead::transportReady()
{
	return commandList.at(getNext()).toUtf8();
}

int FlirModuleHead::dataReady(const unsigned char *bytes, int len)
{
	pingTimer.restart();
	QString data = QString::fromUtf8((const char *)bytes, len);
	if (data.contains("root.ERR.no=4") ||
		data.size() > 1000) // unnecessary message
		return len;
	QStringList lines = data.split("\n");
	foreach (QString line, lines) {
		if (line.isEmpty())
			continue;
		line.remove("\r");
		if (line.contains("LASER.current")) {
			line = line.remove(" ");
			QString value = line.split("=").last();
			if (value.toInt() == 64) {
				laserCIT = true;
				laserTimer.restart();
				return len;
			}
		}
		if (line.contains("root.CAMPOS.")) {
			line.remove("root.CAMPOS.");
			if (line.contains("zoompos"))
				zoomPos = line.split("=").last().toInt();
			if (line.contains("focuspos"))
				focusPos = line.split("=").last().toInt();
		}
		if (line.contains("root.CAM.")) {
			line.remove("root.CAM.");
			QString key = line.split("=").first();
			QString value = line.split("=").last();
			camModes.insert(key, value);
		}
	}
	mDebug("Zoom position %d, Focus position %d, Laser timer %d",
			zoomPos, focusPos,  getLaserTimer());
	return len;
}
