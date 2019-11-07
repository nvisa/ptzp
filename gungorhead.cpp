#include "gungorhead.h"
#include "debug.h"
#include "ptzptransport.h"

#include <QFile>
#include <QTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "errno.h"

#define MAX_CMD_LEN 10

enum Commands {
	C_SET_OPEN,
	C_SET_CLOSE,
	C_SET_ZOOM_TELE,
	C_SET_ZOOM_WIDE,
	C_SET_ZOOM_INC_START,
	C_SET_ZOOM_STOP,
	C_SET_ZOOM_DEC_START,
	C_SET_ZOOM,
	C_SET_FOCUS_INC_START,
	C_SET_FOCUS_STOP,
	C_SET_FOCUS_DEC_START,
	C_SET_FOCUS,
	C_SET_AUTO_FOCUS_MODE,
	C_SET_AUTO_FOCUS_OFF,
	C_SET_DIGI_ZOOM_ON,
	C_SET_DIGI_ZOOM_OFF,
	C_SET_FOCUS_STEPPER,
	C_GET_ZOOM,
	C_GET_FOCUS,
	C_GET_CHIP_VERSION,
	C_GET_SOFTWARE_VERSION,
	C_GET_HARDWARE_VERSION,
	C_GET_DIGI_ZOOM,

	C_SET_FOG_MODE,

	C_SET_EXTENDER_MODE,
	C_FOV,
	C_COUNT,
	R_FACTORY
};

static unsigned char protoBytes[C_COUNT][MAX_CMD_LEN] = {
	{0x05, 0xF1, 0x08, 0x00, 0x00, 0x07},
	{0x05, 0xF1, 0x09, 0x00, 0x00, 0x06},
	{0x05, 0xF1, 0x0F, 0x00, 0x00, 0xF5},
	{0x05, 0xF1, 0x0E, 0x00, 0x00, 0xF5},
	{0x05, 0xF1, 0x1A, 0x00, 0x00, 0xF5},
	{0x05, 0xF1, 0x1B, 0x00, 0x00, 0xF4},
	{0x05, 0xF1, 0x1C, 0x00, 0x00, 0xF3},
	{0x05, 0xF1, 0x4A, 0x00, 0x00, 0x19},
	{0x05, 0xF1, 0x1E, 0x00, 0x00, 0xF1},
	{0x05, 0xF1, 0x1F, 0x00, 0x00, 0xF0},
	{0x05, 0xF1, 0x2A, 0x00, 0x00, 0xE5},
	{0x05, 0xF1, 0x4C, 0x00, 0x00, 0xF2},
	{0x05, 0xF1, 0x74, 0x00, 0x00, 0x9B},
	{0x05, 0xF1, 0x74, 0x00, 0x00, 0x99}, //af_off
	{0x05, 0xF1, 0x78, 0x00, 0x00, 0xEB},
	{0x05, 0xF1, 0x80, 0x00, 0x00, 0x8F},
	{0x05, 0xF1, 0x29, 0xEA, 0x60, 0x9C},
	{0x05, 0xF1, 0x0A, 0x00, 0x00, 0x05}, // GET
	{0x05, 0xF1, 0x0B, 0x00, 0x00, 0x04},
	{0x05, 0xF1, 0xA4, 0x00, 0x00, 0x6B},
	{0x05, 0xF1, 0x91, 0x00, 0x00, 0x7E},
	{0x05, 0xF1, 0x92, 0x00, 0x00, 0x7D},
	{0x05, 0xF1, 0x82, 0x00, 0x00, 0x8D},

	{0x05, 0xF1, 0x37, 0x00, 0x00, 0xD7}, // data1: 0x01 on,  0x00 off

	{0x05, 0xF1, 0x79, 0x00, 0x00, 0x96}, // 00 00 X1,  01 00 X2
	{0x05, 0xF1, 0x4A, 0x00, 0x00, 0x19}, // same with set_zoom
	// x00 back_zoom, x01 tele, x02 wide , x03 random1, x04 random2
};

static uchar chksum(const uchar *buf, int len)
{
	uint sum = 0;
	for (int i = 0; i < len; i++)
		sum += buf[i];
	return (~(sum) + 0x01) & 0xFF;
}

MgeoGunGorHead::MgeoGunGorHead()
{
	for (int i = C_GET_ZOOM; i <= C_GET_DIGI_ZOOM; i++)
		syncList << i;
	nextSync = syncList.size();
	syncTimer.start();
#ifdef HAVE_PTZP_GRPC_API
	settings = {
		{"focus", {C_SET_FOCUS_INC_START, R_FOCUS}},
		{"chip_version", {NULL, R_CHIP_VERSION}},
		{"digi_zoom", {NULL, R_DIGI_ZOOM}},
		{"cam_status", {C_SET_OPEN, R_CAM_STATUS}},
		{"auto_focus_mode", {C_SET_AUTO_FOCUS_MODE, R_AUTO_FOCUS_STATUS}},
		{"digi_zoom", {C_SET_DIGI_ZOOM_ON, R_DIGI_ZOOM_STATUS}},
		{"fog_mode", {C_SET_FOG_MODE, R_FOG_STATUS}},
		{"ext_mode", {C_SET_EXTENDER_MODE, R_EXT_STATUS}},
		{"fov_mode", {C_FOV, R_FOV_STATUS}},
		{"factory_settings", {R_FACTORY, R_FACTORY}},
	};
#endif
}

int MgeoGunGorHead::getCapabilities()
{
	return CAP_ZOOM;
}

void MgeoGunGorHead::setProperty(uint r, uint x)
{
	if (r == C_SET_OPEN) {
		if (x == 1)
			sendCommand(C_SET_OPEN);
		else if (x == 0)
			sendCommand(C_SET_CLOSE);
		setRegister(R_CAM_STATUS, x);
	} else if (r == C_SET_FOCUS) {
		sendCommand(r, (x & 0xff00) >> 8, (x & 0x00ff));
	} else if (r == C_SET_AUTO_FOCUS_MODE) {
		sendCommand(C_SET_AUTO_FOCUS_MODE, x);
		setRegister(R_AUTO_FOCUS_STATUS, x);
	} else if (r == C_SET_DIGI_ZOOM_ON) {
		if (x == 0)
			sendCommand(C_SET_DIGI_ZOOM_OFF);
		else
			sendCommand(C_SET_DIGI_ZOOM_ON, x);
		setRegister(R_DIGI_ZOOM_STATUS, x);
	} else if (r == C_SET_FOG_MODE) {
		sendCommand(r, x);
		setRegister(R_FOG_STATUS, x);
	} else if (r == C_SET_EXTENDER_MODE) {
		sendCommand(r, x);
		setRegister(R_EXT_STATUS, x);
	} else if (r == C_FOV) {// 0x00, 0x01, 0x02, 0x03, 0x04
		if (x == 0x00)
			sendCommand(r);
		else if (x == 0x01)
			sendCommand(C_SET_ZOOM_TELE);
		else if (x == 0x02)
			sendCommand(C_SET_ZOOM_WIDE);
		else if (x == 0x03)
			sendCommand(r, 0x55, 0x55);
		else if (x == 0x04)
			sendCommand(r, 0xAA, 0xAA);
	}
}

uint MgeoGunGorHead::getProperty(uint r)
{
	return getRegister(r);
}

void MgeoGunGorHead::setFocusStepper()
{
	sendCommand(C_SET_FOCUS_STEPPER, 0xEA, 0x60);
}

float MgeoGunGorHead::getAngle()
{
	return getZoom();
}

QJsonObject MgeoGunGorHead::factorySettings(const QString &file)
{
	QJsonObject obj = PtzpHead::factorySettings(file);
	if (obj.contains("fov"))
		setProperty(C_FOV, obj.value("fov").toInt());
	if (obj.contains("extender"))
		setProperty(C_SET_EXTENDER_MODE, obj.value("extender").toInt());
	if (obj.contains("fog"))
		setProperty(C_SET_FOG_MODE, obj.value("fog").toInt());
	if (obj.contains("digi_zoom"))
		setProperty(C_SET_DIGI_ZOOM_ON, obj.value("digi_zoom").toInt());
	return obj;
}

void MgeoGunGorHead::initHead()
{
	nextSync = 0;
	QTimer::singleShot(1000, this, SLOT(timeout()));
}

void MgeoGunGorHead::timeout()
{
	if (nextSync != syncList.size()) {
		syncRegisters();
		mInfo("Syncing.. '%d'.step, total step '%d'",
				nextSync, syncList.size());
		QTimer::singleShot(1000, this, SLOT(timeout()));
	} else {
		mDebug("Day register sync completed");
		loadRegisters("head2.json");
		transport->enableQueueFreeCallbacks(true);
	}
}

int MgeoGunGorHead::getHeadStatus()
{
	if (nextSync != syncList.size())
		return ST_SYNCING;
	if (pingTimer.elapsed() < 1500)
		return ST_NORMAL;
	return ST_ERROR;
}

int MgeoGunGorHead::syncRegisters()
{
	if (!transport)
		return -ENOENT;
	return syncNext();
}

int MgeoGunGorHead::startZoomIn(int speed)
{
	Q_UNUSED(speed);
	return sendCommand(C_SET_ZOOM_INC_START);
}

int MgeoGunGorHead::startZoomOut(int speed)
{
	Q_UNUSED(speed);
	return sendCommand(C_SET_ZOOM_DEC_START);
}

int MgeoGunGorHead::stopZoom()
{
	return sendCommand(C_SET_ZOOM_STOP);
}

int MgeoGunGorHead::getZoom()
{
	return getRegister(R_ZOOM);
}

int MgeoGunGorHead::setZoom(uint pos)
{
	return sendCommand(C_SET_ZOOM, (pos & 0xff00) >> 8, (pos & 0x00ff));
}

int MgeoGunGorHead::focusIn(int speed)
{
	Q_UNUSED(speed);
	return sendCommand(C_SET_FOCUS_INC_START);
}

int MgeoGunGorHead::focusOut(int speed)
{
	Q_UNUSED(speed);
	return sendCommand(C_SET_FOCUS_DEC_START);
}

int MgeoGunGorHead::focusStop()
{
	return sendCommand(C_SET_FOCUS_STOP);
}

int MgeoGunGorHead::sendCommand(uint index, uchar data1, uchar data2)
{
	unsigned char *cmd = protoBytes[index];
	int cmdlen = cmd[0];
	cmd++;
	cmd[2] = data1;
	cmd[3] = data2;
	cmd[cmdlen - 1] = chksum(cmd, cmdlen - 1);
	return transport->send((const char *)cmd, cmdlen);
}

int MgeoGunGorHead::dataReady(const unsigned char *bytes, int len)
{
	if (bytes[0] != 0xF1)
		return len;
	if (len < 5)
		return len;

	if (nextSync != syncList.size())
		nextSync++;

	pingTimer.restart();
	uchar opcode = bytes[1];
	const uchar *p = bytes + 2;

	if (opcode == 0x0a) {
		if (systemChecker == 0)
			systemChecker = 1;
		setRegister(R_ZOOM, ((p[0] & 0xFE) << 8));
	}
	else if (opcode == 0x0b)
		setRegister(R_FOCUS, (p[0] << 8 | p[1]));
	else if (opcode == 0xa4)
		setRegister(R_CHIP_VERSION, ((p[0] * 100) + p[1]));
	else if (opcode == 0x82)
		setRegister(R_DIGI_ZOOM, p[0]);
	return 5;
}

QByteArray MgeoGunGorHead::transportReady()
{
	if (syncTimer.elapsed() > 700) {
		syncTimer.restart();
		sendCommand(C_GET_ZOOM);
	}
	return QByteArray();
}

int MgeoGunGorHead::syncNext()
{
	uint cmd = syncList[nextSync];
	if (cmd == C_GET_ZOOM)
		return sendCommand(cmd);
	else if (cmd == C_GET_FOCUS)
		return sendCommand(cmd);
	else if (cmd == C_GET_CHIP_VERSION)
		return sendCommand(cmd);
	else if (cmd == C_GET_SOFTWARE_VERSION)
		return sendCommand(cmd);
	else if (cmd == C_GET_HARDWARE_VERSION)
		return sendCommand(cmd);
	else if (cmd == C_GET_DIGI_ZOOM)
		return sendCommand(cmd);
	return -ENODATA;
}

QJsonValue MgeoGunGorHead::marshallAllRegisters()
{
	QJsonObject json;
	json.insert(QString("reg0"), (int)getRegister(6));
	json.insert(QString("reg10"), (int)getRegister(7));
	json.insert(QString("reg13"), (int)getRegister(8));

	return json;
}

void MgeoGunGorHead::unmarshallloadAllRegisters(const QJsonValue &node)
{
	QJsonObject root = node.toObject();
	QString key = "reg%1";
	foreach (key, root.keys()) {
		if (!key.startsWith("reg"))
			continue;
		int ind = key.remove("reg").toInt();
		key = (QString) "reg" + key;
		setProperty(ind, root[key].toInt());
	}
}
