#include "mgeoyamgozhead.h"
#include "debug.h"
#include "ptzptransport.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include <errno.h>

#define dump(p, len) \
	for (int i = 0; i < len; i++) \
		qDebug("%s: %d: 0x%x", __func__, i, p[i]);

static uchar chksum(const uchar *buf, int len)
{
	uchar sum = 0;
	for (int i = len - 10 ; i < len - 1; i++)
		sum += buf[i];
	return (0x100 - (sum & 0xff));
}

#define MAX_CMD_LEN 19

enum Commands {
	C_SET_E_ZOOM,
	C_SET_FREEZE,
	C_SET_POLARITY,
	C_SET_1_POINT_NUC,
	C_SET_VIDEO_SOURCE,
	C_SET_IMAGE_FLIP,

	C_COUNT
};

static unsigned char protoBytes[C_COUNT][MAX_CMD_LEN] = {
	{0x12, 0x40, 0x28, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x35, 0x0a, 0xb7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},// e-zoom
	{0x12, 0x40, 0x28, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x35, 0x0a, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},// freeze
	{0x12, 0x40, 0x28, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x35, 0x0a, 0xb5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},// polarity
	{0x12, 0x40, 0x28, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x35, 0x0a, 0xc3, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},// 1 point nuc
	{0x12, 0x40, 0x28, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x35, 0x0a, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},// video source
	{0x12, 0x40, 0x28, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x35, 0x0a, 0x8e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},// image flip
};

MgeoYamGozHead::MgeoYamGozHead()
{

}

int MgeoYamGozHead::getCapabilities()
{
	return CAP_ZOOM;
}

int MgeoYamGozHead::startZoomIn(int speed)
{
	Q_UNUSED(speed);
	uchar *cmd = protoBytes[C_SET_E_ZOOM];
	int len = cmd[0];
	cmd++;
	cmd[11] = 0x10;
	cmd[17] = chksum(cmd, len);
	sendCommand(cmd, len);
	return 0;
}

int MgeoYamGozHead::startZoomOut(int speed)
{
	Q_UNUSED(speed);
	uchar *cmd = protoBytes[C_SET_E_ZOOM];
	int len = cmd[0];
	cmd++;
	cmd[11] = 0x20;
	cmd[17] = chksum(cmd, len);
	sendCommand(cmd, len);
	return 0;
}

int MgeoYamGozHead::stopZoom()
{
	return 0;
}

int MgeoYamGozHead::setZoom(uint pos)
{
	Q_UNUSED(pos);
	uchar *cmd = protoBytes[C_SET_E_ZOOM];
	int len = cmd[0];
	cmd++;
	cmd[11] = pos;
	cmd[17] = chksum(cmd, len);
	sendCommand(cmd, len);
	return 0;
}

int MgeoYamGozHead::getZoom()
{
	return getRegister(R_E_ZOOM);
}

int MgeoYamGozHead::getHeadStatus()
{
	return ST_NORMAL;
}

void MgeoYamGozHead::setProperty(uint r, uint x)
{
	if(r == C_SET_FREEZE) {
		mDebug("Freeze");
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[11] = x ? 0x01 : 0x00;
		p[17] = chksum(p, len);
		sendCommand(p, len);
		setRegister(R_FREEZE, p[11]);
	}
	else if (r == C_SET_POLARITY) {
		mDebug("Polarity");
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[11] = x;
		p[17] = chksum(p, len);
		sendCommand(p, len);
		setRegister(R_POLARITY, p[11]);
	}
	else if (r == C_SET_1_POINT_NUC) {
		mDebug("1 point nuc");
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[11] = x;
		p[17] = chksum(p, len);
		sendCommand(p, len);
	}
	else if (r == C_SET_VIDEO_SOURCE) {
		mDebug("Video source");
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[11] = x ? 0x01 : 0x00;
		p[17] = chksum(p, len);
		sendCommand(p, len);
		setRegister(R_VIDEO_SOURCE, p[11]);
	}
	else if (r == C_SET_IMAGE_FLIP) {
		mDebug("Image rotation");
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[11] = x;
		p[17] = chksum(p, len);
		sendCommand(p, len);
		setRegister(R_IMAGE_FLIP, p[11]);
	}
}

uint MgeoYamGozHead::getProperty(uint r)
{
	return getRegister(r);
}

int MgeoYamGozHead::dataReady(const unsigned char *bytes, int len)
{
	if (bytes[0] != 0xCA)
		return -ENOENT;
	if (len < bytes[1])
		return -EAGAIN;
	uchar chk = chksum(bytes, len);
	if (chk != bytes[len -1]){
		fDebug("Checksum error");
		return -ENOENT;
	}
	if (bytes[2] == 0x10){
		setRegister(R_HEART_BEAT, (getRegister(R_HEART_BEAT)) + 1);
		return -ENOENT; //alive
	}

	if (bytes[2] == 0xb7)
		setRegister(R_E_ZOOM,bytes[3]);
	else if (bytes[2] == 0xb8)
		setRegister(R_FREEZE,bytes[3]);
	else if (bytes[2] == 0xb5)
		setRegister(R_POLARITY,bytes[3]);
	else if (bytes[2] == 0x80)
		setRegister(R_VIDEO_SOURCE,bytes[3]);
	else if (bytes[2] == 0x8e)
		setRegister(R_IMAGE_FLIP,bytes[3]);

	return len;
}

int MgeoYamGozHead::sendCommand(const unsigned char *cmd, int len)
{
	return transport->send((const char *)cmd, len);
}

