#include "gungorhead.h"
#include "debug.h"
#include "ptzptransport.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include "errno.h"

#define MAX_CMD_LEN 10

enum Commands {
	C_SET_OPEN,
	C_SET_CLOSE,
//	C_SET_ZOOM_TELE,
//	C_SET_ZOOM_WIDE,
	C_SET_ZOOM_INC_START,
	C_SET_ZOOM_STOP,
	C_SET_ZOOM_DEC_START,
	C_SET_ZOOM,
	C_SET_FOCUS_INC_START,
	C_SET_FOCUS_STOP,
	C_SET_FOCUS_DEC_START,
	C_SET_FOCUS,
	C_SET_AUTO_FOCUS_ON,
	C_SET_AUTO_FOCUS_OFF,
	C_SET_DIGI_ZOOM_ON,
	C_SET_DIGI_ZOOM_OFF,
	C_GET_ZOOM,
	C_GET_FOCUS,
	C_GET_CHIP_VERSION,
	C_GET_SOFTWARE_VERSION,
	C_GET_HARDWARE_VERSION,
	C_GET_DIGI_ZOOM,

	C_COUNT
};

static unsigned char protoBytes[C_COUNT][MAX_CMD_LEN] = {
	{0x05, 0xF1, 0x08, 0x00, 0x00, 0x07},
	{0x05, 0xF1, 0x09, 0x00, 0x00, 0x06},
//	{0x05, 0xF1, 0x0F, 0x00, 0x00, 0xF5},
//	{0x05, 0xF1, 0x0E, 0x00, 0x00, 0xF5},
	{0x05, 0xF1, 0x1A, 0x00, 0x00, 0xF5},
	{0x05, 0xF1, 0x1B, 0x00, 0x00, 0xF4},
	{0x05, 0xF1, 0x1C, 0x00, 0x00, 0xF3},
	{0x05, 0xF1, 0x4A, 0x00, 0x00, 0x19},
	{0x05, 0xF1, 0x1E, 0x00, 0x00, 0xF1},
	{0x05, 0xF1, 0x1F, 0x00, 0x00, 0xF0},
	{0x05, 0xF1, 0x2A, 0x00, 0x00, 0xE5},
	{0x05, 0xF1, 0x4C, 0x00, 0x00, 0xF2},
	{0x05, 0xF1, 0x74, 0x00, 0x00, 0x9B},
	{0x05, 0xF1, 0x76, 0x00, 0x00, 0x99},
	{0x05, 0xF1, 0x78, 0x00, 0x00, 0xEB},
	{0x05, 0xF1, 0x80, 0x00, 0x00, 0x8F},
	{0x05, 0xF1, 0x0A, 0x00, 0x00, 0x05},//GET
	{0x05, 0xF1, 0x0B, 0x00, 0x00, 0x04},
	{0x05, 0xF1, 0xA4, 0x00, 0x00, 0x6B},
	{0x05, 0xF1, 0x91, 0x00, 0x00, 0x7E},
	{0x05, 0xF1, 0x92, 0x00, 0x00, 0x7D},
	{0x05, 0xF1, 0x82, 0x00, 0x00, 0x8D},
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
	for (int i = C_GET_ZOOM; i<=C_GET_DIGI_ZOOM; i++)
		syncList << i;
	nextSync = syncList.size();
#ifdef HAVE_PTZP_GRPC_API
	settings = {
		{"focus_in", {C_SET_FOCUS_INC_START,R_FOCUS}},
		{"focus_out", {C_SET_FOCUS_DEC_START,R_FOCUS}},
		{"focus_stop", {C_SET_FOCUS_STOP,R_FOCUS}},
		{"chip_version",{ NULL, R_CHIP_VERSION}},
		{"digi_zoom", { NULL, R_DIGI_ZOOM}},
		{"cam_open", {C_SET_OPEN, R_CAM_STATUS}},
		{"cam_close", {C_SET_CLOSE, R_CAM_STATUS}},
		{"auto_focus_on", {C_SET_AUTO_FOCUS_ON, R_AUTO_FOCUS_STATUS}},
		{"auto_focus_off", {C_SET_AUTO_FOCUS_OFF, R_AUTO_FOCUS_STATUS}},
		{"digi_zoom_on", {C_SET_DIGI_ZOOM_ON, R_DIGI_ZOOM_STATUS}},
		{"digi_zoom_off", {C_SET_DIGI_ZOOM_OFF, R_DIGI_ZOOM_STATUS}}
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
	} else if (r == C_SET_CLOSE) {
		sendCommand(r);
		setRegister(R_CAM_STATUS, 0);
	} else if (r == C_SET_ZOOM) {
//		unsigned char *p = protoBytes[C_SET_ZOOM];

//		sendCommand(C_SET_ZOOM);
//		setRegister(R_);
	} else if (r == C_SET_FOCUS_INC_START) {
		sendCommand(r);
	} else if (r == C_SET_FOCUS_STOP) {
		sendCommand(r);
	} else if (r == C_SET_FOCUS_DEC_START) {
		sendCommand(r);
	} else if (r == C_SET_FOCUS) {
//		sendCommand(r);
//		yapılacak
	} else if (r == C_SET_AUTO_FOCUS_ON) {
		if (x == 1)
			sendCommand(C_SET_AUTO_FOCUS_ON);
		else if (x == 0)
			sendCommand(C_SET_AUTO_FOCUS_OFF);
		setRegister(R_AUTO_FOCUS_STATUS, x);
	} else if (r == C_SET_DIGI_ZOOM_ON) {
		if (x == 0)
			sendCommand(C_SET_DIGI_ZOOM_OFF);
		else sendCommand(C_SET_DIGI_ZOOM_ON, x);
		setRegister(R_DIGI_ZOOM_STATUS, x);
	}
}

uint MgeoGunGorHead::getProperty(uint r)
{
	return getRegister(r);
}

int MgeoGunGorHead::getHeadStatus()
{
	if (nextSync != syncList.size())
		return ST_SYNCING;
	if (pingTimer.elapsed() < 3000)
		return ST_NORMAL;
	return ST_ERROR;
}

int MgeoGunGorHead::syncRegisters()
{
	if (!transport)
		return -ENOENT;
	nextSync = 0;
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

int MgeoGunGorHead::sendCommand(uint index, uchar data1, uchar data2)
{
	unsigned char *cmd = protoBytes[index];
	int cmdlen = cmd[0];
	cmd++;
	cmd[2] = data1;
	cmd[3] = data2;
	cmd[cmdlen - 1] = chksum(cmd, cmdlen - 1);
	return transport->send((const char *)cmd , cmdlen);
}

int MgeoGunGorHead::dataReady(const unsigned char *bytes, int len)
{
	if (bytes[0] != 0xF1)
		return -1;
	if (len < 5)
		return -1;

	if (nextSync != syncList.size()) {
		if (++nextSync == syncList.size()) {
			fDebug("DayCam register syncing completed, activating auto-sync");
		} else
			syncNext();
	}

	uchar opcode = bytes[1];
	const uchar *p = bytes + 2;

	if (opcode == 0x0a)
		setRegister(R_ZOOM, (p[1] << 8 | p[0]));
	else if (opcode == 0x0b)
		setRegister(R_FOCUS, (p[1] << 8 | p[0]));
	else if (opcode == 0xa4)
		setRegister(R_CHIP_VERSION, ((p[0] * 100) + p[1]));
	else if (opcode == 0x82)
		setRegister(R_DIGI_ZOOM, p[0]);
	return 5;
}

QByteArray MgeoGunGorHead::transportReady()
{
	sendCommand(C_GET_ZOOM);
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
		key = (QString)"reg" + key;
		setProperty(ind, root[key].toInt());
	}
}
