#include "mgeothermalhead.h"
#include "debug.h"
#include "ptzptransport.h"

#include <errno.h>

#define dump(p, len) \
	for (int i = 0; i < len; i++) \
		qDebug("%s: %d: 0x%x", __func__, i, p[i]);

#define MAX_CMD_LEN	10

enum Commands {
	C_BRIGHTNESS,
	C_CONTRAST,
	C_FOV,
	C_CONT_ZOOM,
	C_GET_ZOOM_FOCUS,
	C_FOCUS,
	C_NUC_SELECT,
	C_POL_CHANGE,
	C_RETICLE_ONOFF,
	C_DIGITAL_ZOOM,
	C_FREEZE_IMAGE,
	C_AGC_SELECT,
	C_BRIGHTNESS_CHANGE,
	C_CONTRAST_CHANGE,
	C_RETICLE_CHANGE,
	C_NUC,
	C_IPM_CHANGE,
	C_HPF_GAIN_CHANGE,
	C_HPF_SPATIAL_CHANGE,
	C_FLIP,
	C_IMAGE_UPDATE_SPEED,

	C_COUNT
};

static unsigned char protoBytes[C_COUNT][MAX_CMD_LEN] = {
	{0x35, 0x0a, 0xab, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0xac, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0xb0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0xb2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0xcb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0xb3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0xb4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0xb5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0xb6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0xb7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0xbc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0xbd, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0xbe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0xc3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0xd7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0xd8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0xd9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0xdb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0xde, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};

static uchar chksum(const uchar *buf, int len)
{
	uchar sum = 0;
	for (int i = 0; i < len; i++)
		sum += buf[i];
	return ~sum + 1;
}

MgeoThermalHead::MgeoThermalHead()
{
	syncList << C_BRIGHTNESS;
	syncList << C_CONTRAST;
	syncList << C_GET_ZOOM_FOCUS;
	nextSync = syncList.size();
}

int MgeoThermalHead::getCapabilities()
{
	return CAP_ZOOM;
}

int MgeoThermalHead::syncRegisters()
{
	if (!transport)
		return -ENOENT;
	nextSync = 0;
	return syncNext();
}

int MgeoThermalHead::startZoomIn(int speed)
{
	Q_UNUSED(speed);
	uchar *cmd = protoBytes[C_CONT_ZOOM];
	cmd[3] = 0x01;
	cmd[9] = chksum(cmd, 9);
	dump(cmd, 10);
	return 0;
}

int MgeoThermalHead::startZoomOut(int speed)
{
	Q_UNUSED(speed);
	uchar *cmd = protoBytes[C_CONT_ZOOM];
	cmd[3] = 0xff;
	cmd[9] = chksum(cmd, 9);
	dump(cmd, 10);
	return 0;
}

int MgeoThermalHead::stopZoom()
{
	uchar *cmd = protoBytes[C_CONT_ZOOM];
	cmd[3] = 0x00;
	cmd[9] = chksum(cmd, 9);
	dump(cmd, 10);
	return 0;
}

int MgeoThermalHead::getZoom()
{
	return 0;
}

int MgeoThermalHead::getHeadStatus()
{
	if (nextSync == C_COUNT)
		return ST_NORMAL;
	return ST_SYNCING;
}

int MgeoThermalHead::dataReady(const unsigned char *bytes, int len)
{
	if (bytes[0] != 0xca)
		return -1;
	if (len < 2)
		return -1;
	int meslen = bytes[1];
	if (len < meslen)
		return -1;

	if (meslen == 5 && bytes[2] == 0xc3) {
		/* ping message */
		pingTimer.restart();
		return meslen;
	}

	/* register sync support */
	if (nextSync != syncList.size()) {
		/* we are in sync mode, let's sync next */
		if (++nextSync == syncList.size()) {
			fDebug("Thermal register syncing completed, activating auto-sync");
		} else
			syncNext();
	}

	uchar chk = chksum(bytes, meslen - 1);
	if (chk != bytes[meslen - 1]) {
		fDebug("Checksum error");
		return meslen;
	}

	uchar opcode = bytes[2];
	const uchar *p = bytes + 3;

	if (opcode == 0xa0) {
		//cooled down
		setRegister(R_COOLED_DOWN, p[0]);
	} else if (opcode == 0xa1) {
		//fov change
		setRegister(R_FOV, p[0]);
	} else if (opcode == 0xbc) {
		//zoom/focus
		setRegister(R_ZOOM, (p[1] << 8 | p[0]));
		setRegister(R_FOCUS, (p[3] << 8 | p[2]));
		setRegister(R_ANGLE, (p[5] << 8 | p[4]));
	} else if (opcode == 0xa4) {
		//nuc table selection
		setRegister(R_NUC_TABLE, p[0]);
	} else if (opcode == 0xa5) {
		//polarity change
		setRegister(R_POLARITY, p[0]);
	} else if (opcode == 0xa7) {
		//reticle on-off
		setRegister(R_RETICLE, p[0]);
	} else if (opcode == 0xa9) {
		//digital zoom
		setRegister(R_DIGITAL_ZOOM, p[0]);
	} else if (opcode == 0xaa) {
		//image freeze
		setRegister(R_IMAGE_FREEZE, p[0]);
	} else if (opcode == 0xae) {
		//contrast brightness auto/manual
		setRegister(R_AGC, p[0]);
	} else if (opcode == 0xaf) {
		//brightness change/get
		setRegister(R_BRIGHTNESS, p[0]);
	} else if (opcode == 0xb1) {
		//contrast
		setRegister(R_CONTRAST, p[0]);
	} else if (opcode == 0xb5) {
		//reticle
		setRegister(R_INTENSITY, p[0]);
	} else if (opcode == 0xa2) {
		//one-point
		setRegister(R_NUC, p[0]);
	} else if (opcode == 0xba) {
		//menu
	} else if (opcode == 0xbb) {
		//ibit
		setRegister(R_IBIT, p[0]);
	} else if (opcode == 0xc3) {
		//c3 ops
		if (p[0] == 0xd7)
			setRegister(R_IPM, p[1]);
		else if (p[0] == 0xd8)
			setRegister(R_HPF_GAIN, p[1]);
		else if (p[0] == 0xd9)
			setRegister(R_HPF_SPATIAL, p[1]);
		else if (p[0] == 0xde)
			setRegister(R_IMAGE_UPDATE_SPEED, p[1]);
	}
	return meslen;
}

int MgeoThermalHead::syncNext()
{
	uint cmd = syncList[nextSync];
	if (cmd == C_BRIGHTNESS)
		return sendCommand(C_BRIGHTNESS, 0x02);
	if (cmd == C_CONTRAST)
		return sendCommand(C_CONTRAST, 0x02);
	if (cmd == C_GET_ZOOM_FOCUS)
		return sendCommand(C_GET_ZOOM_FOCUS);
	return -ENOENT;
}

int MgeoThermalHead::sendCommand(uint index, uchar data1, uchar data2)
{
	unsigned char *cmd = protoBytes[index];
	int cmdlen = cmd[1];
	cmd[3] = data1;
	cmd[4] = data2;
	cmd[cmdlen - 1] = chksum(cmd, cmdlen - 1);
	return transport->send((const char *)cmd , cmdlen);
}
