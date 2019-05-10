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
	for (int i = 0 ; i < len; i++)
		sum += buf[i];
	return (0x100 - (sum & 0xff));
}

#define MAX_CMD_LEN 10

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
	cmd[3] = 0x10;
	cmd[9] = chksum(cmd, cmd[1]);
	sendCommand(cmd, cmd[1]);
	return 0;
}

int MgeoYamGozHead::startZoomOut(int speed)
{
	Q_UNUSED(speed);
	uchar *cmd = protoBytes[C_SET_E_ZOOM];
	cmd[3] = 0x20;
	cmd[9] = chksum(cmd, cmd[1]);
	sendCommand(cmd, cmd[1]);
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
	cmd[3] = pos;
	cmd[9] = chksum(cmd, cmd[1]);
	sendCommand(cmd, cmd[1]);
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
		 unsigned char *p = protoBytes[r];
		 p[3] = x ? 0x01 : 0x00;
		 p[9] = chksum(p, p[1]);
		 sendCommand(p, p[1]);
		 setRegister(R_FREEZE, p[3]);
	 }
	 else if (r == C_SET_POLARITY) {
		 unsigned char *p = protoBytes[r];
		 p[3] = x;
		 p[9] = chksum(p, p[1]);
		 sendCommand(p, p[1]);
		 setRegister(R_POLARITY, p[3]);
	 }
	 else if (r == C_SET_1_POINT_NUC) {
		 unsigned char *p = protoBytes[r];
		 p[3] = x;
		 p[9] = chksum(p, p[1]);
		 sendCommand(p, p[1]);
	 }
	 else if (r == C_SET_VIDEO_SOURCE) {
		 unsigned char *p = protoBytes[r];
		 p[3] = x ? 0x01 : 0x00;
		 p[9] = chksum(p, p[1]);
		 sendCommand(p, p[1]);
		 setRegister(R_VIDEO_SOURCE, p[3]);
	 }
	 else if (r == C_SET_IMAGE_FLIP) {
		 unsigned char *p = protoBytes[r];
		 p[3] = x;
		 p[9] = chksum(p, p[1]);
		 sendCommand(p, p[1]);
		 setRegister(R_IMAGE_FLIP, p[3]);
	 }
}

uint MgeoYamGozHead::getProperty(uint r)
{
	return getRegister(r);
}

int MgeoYamGozHead::dataReady(const unsigned char *bytes, int len)
{
	dump(bytes, len)
	if (bytes[0] != 0xCA)
		return -ENOENT;
	if (len < bytes[1])
		return -EAGAIN;
	if (bytes[2] == 0x10)
		return -ENOENT; //alive
	uchar chk = chksum(bytes, len - 1);
	if (chk != bytes[len -1]){
		fDebug("Checksum error");
		return -ENOENT;
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
}

int MgeoYamGozHead::sendCommand(const unsigned char *cmd, int len)
{
	return transport->send((const char *)cmd, len);
}

