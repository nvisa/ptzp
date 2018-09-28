#include "mgeothermalhead.h"
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
	C_IBIT,
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
	{0x35, 0x0a, 0xca, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
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

#ifdef HAVE_PTZP_GRPC_API
	settings = {
		{"brightness", {C_BRIGHTNESS, R_BRIGHTNESS}},
		{"contrast", {C_CONTRAST, R_CONTRAST}},
		{"fov", {C_FOV, R_FOV}},
		{"focus_in",{C_FOCUS, R_FOCUS}},
		{"focus_out",{C_FOCUS, R_FOCUS}},
		{"focus_stop",{C_FOCUS, R_FOCUS}},
		{"angle", {NULL, R_ANGLE}},
		{"nuc_table", {C_NUC_SELECT, R_NUC_TABLE}},
		{"polarity", {C_POL_CHANGE, R_POLARITY}},
		{"reticle", {C_RETICLE_ONOFF, R_RETICLE}},
		{"digi_zoom_on", {C_DIGITAL_ZOOM, R_DIGITAL_ZOOM}},
		{"digi_zoom_off", {C_DIGITAL_ZOOM, R_DIGITAL_ZOOM}},
		{"image_freeze", {C_FREEZE_IMAGE, R_IMAGE_FREEZE}},
		{"agc", {C_AGC_SELECT, R_AGC}},
		{"reticle_intensity", {C_RETICLE_CHANGE, R_RETICLE_INTENSITY }},
		{"nuc",{C_NUC, R_NUC}},
		{"ibit", {C_IBIT, R_IBIT}},
		{"ipm_change", {C_IPM_CHANGE, R_IPM}},
		{"hpf_gain_change", {C_HPF_GAIN_CHANGE, R_HPF_GAIN}},
		{"hpf_spatial_change", {C_HPF_SPATIAL_CHANGE, R_HPF_SPATIAL}},
		{"flip", {C_FLIP, R_FLIP}},
		{"image_update_speed", {C_IMAGE_UPDATE_SPEED, R_IMAGE_UPDATE_SPEED}},
	};
#endif
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
	sendCommand(C_CONT_ZOOM,cmd[3]);
	return 0;
}

int MgeoThermalHead::startZoomOut(int speed)
{
	Q_UNUSED(speed);
	uchar *cmd = protoBytes[C_CONT_ZOOM];
	cmd[3] = 0xff;
	cmd[9] = chksum(cmd, 9);
	dump(cmd, 10);
	sendCommand(C_CONT_ZOOM,cmd[3]);
	return 0;
}

int MgeoThermalHead::stopZoom()
{
	uchar *cmd = protoBytes[C_CONT_ZOOM];
	cmd[3] = 0x00;
	cmd[9] = chksum(cmd, 9);
	dump(cmd, 10);
	sendCommand(C_CONT_ZOOM,cmd[3]);
	return 0;
}

int MgeoThermalHead::getZoom()
{
	return getRegister(R_ZOOM);
}

int MgeoThermalHead::getHeadStatus()
{
	if (nextSync != C_COUNT)
		return ST_SYNCING;
	if (pingTimer.elapsed() < 1500)
		return ST_NORMAL;
	return ST_ERROR;
}

void MgeoThermalHead::setProperty(uint r, uint x)
{
/*
 * p[0]		p[1]		p[2]		p[3]		p[4]		p[5]		p[6]		p[7]		p[8]		p[9]
 * 35		0A		(OPCODE)		(DATA1)		(DATA2)		(DATA3)		(DATA4)		(DATA5)		(DATA6)		CHECKSUM
 */
	if (r == C_BRIGHTNESS) {
		unsigned char *p = protoBytes[C_BRIGHTNESS];
		p[4] = x;
		sendCommand(C_BRIGHTNESS, 0x01, p[4]);
		setRegister(R_BRIGHTNESS, x);
	} else if(r == C_CONTRAST) {
		unsigned char *p = protoBytes[C_CONTRAST];
		p[4] = x;
		sendCommand(C_CONTRAST,0x01, p[4]);
		setRegister(R_CONTRAST, x);
	} else if(r == C_FOV) {
		unsigned char *p = protoBytes[C_FOV];
		p[3] = x;
		sendCommand(C_FOV, p[3]);
		setRegister(R_FOV, x);
	} else if(r == C_CONT_ZOOM) {
		unsigned char *p = protoBytes[C_CONT_ZOOM];
		if(x == 0)
			p[3] = 0x00; //zoom stop
		else if(x == 1)
			p[3] = 0x01; //zoom in
		else if (x == 2)
			p[3] = 0xFF; //zoom out
		sendCommand(C_CONT_ZOOM, p[3]);
	} else if(r == C_FOCUS) {
		unsigned char *p = protoBytes[C_FOCUS];
		if(x == 0)
			p[3] = 0x00; //focus stop
		else if(x == 1)
			p[3] = 0x01; //focus far
		else if (x == 2)
			p[3] = 0xFF; //focus near
		sendCommand(C_FOCUS, p[3]);
	} else if(r == C_NUC_SELECT) {
		unsigned char *p = protoBytes[C_NUC_SELECT];
		p[3] = x;
		sendCommand(C_NUC_SELECT, p[3]);
		setRegister(R_NUC_TABLE, x);
	} else if(r == C_POL_CHANGE) {
		unsigned char *p = protoBytes[C_POL_CHANGE];
		p[3] = x;
		sendCommand(C_POL_CHANGE, p[3]);
		setRegister(R_POLARITY, x);
	} else if(r == C_RETICLE_ONOFF) {
		unsigned char *p = protoBytes[C_RETICLE_ONOFF];
		p[3] = x;
		sendCommand(C_RETICLE_ONOFF, p[3]);
		setRegister(R_RETICLE, x);
	} else if(r == C_DIGITAL_ZOOM) {
		unsigned char *p = protoBytes[C_DIGITAL_ZOOM];
		p[3] = x ? 0x01 : 0x00;
		sendCommand(C_DIGITAL_ZOOM, p[3]);
		setRegister(R_DIGITAL_ZOOM, x);
	} else if(r == C_FREEZE_IMAGE) {
		unsigned char *p = protoBytes[C_FREEZE_IMAGE];
		p[3] = x ? 0x01 : 0x00;
		sendCommand(C_FREEZE_IMAGE, p[3]);
		setRegister(R_IMAGE_FREEZE, x);
	} else if (r == C_AGC_SELECT) {
		unsigned char *p = protoBytes[C_AGC_SELECT];
		p[3] = x ? 0x01 : 0x00;
		sendCommand(C_AGC_SELECT,p[3]);
		setRegister(R_AGC, x);
	}else if(r == C_BRIGHTNESS_CHANGE) {
		unsigned char *p = protoBytes[C_BRIGHTNESS_CHANGE];
		p[3] = x;
		sendCommand(C_BRIGHTNESS_CHANGE, p[3]);
	} else if(r == C_CONTRAST_CHANGE) {
		unsigned char *p = protoBytes[C_CONTRAST_CHANGE];
		p[3] = x;
		sendCommand(C_CONTRAST_CHANGE, p[3]);
	} else if(r == C_RETICLE_CHANGE) {
		unsigned char *p = protoBytes[C_RETICLE_CHANGE];
		p[3] = x;
		sendCommand(C_RETICLE_CHANGE, p[3]);
		setRegister(R_RETICLE_INTENSITY, x);
	} else if(r == C_NUC) {
		sendCommand(C_NUC);
	} else if(r == C_IBIT) {
		sendCommand(C_IBIT);
	} else if(r == C_IPM_CHANGE) {
		unsigned char *p = protoBytes[C_IPM_CHANGE];
		p[3] = x;
		sendCommand(C_IPM_CHANGE, p[3]);
		setRegister(R_IPM, x);
	} else if(r == C_HPF_GAIN_CHANGE) {
		unsigned char *p = protoBytes[C_HPF_GAIN_CHANGE];
		p[3] = x;
		sendCommand(C_HPF_GAIN_CHANGE, p[3]);
		setRegister(R_HPF_GAIN, x);
	} else if(r == C_HPF_SPATIAL_CHANGE) {
		unsigned char *p = protoBytes[C_HPF_SPATIAL_CHANGE];
		p[3] = x;
		sendCommand(C_HPF_SPATIAL_CHANGE, p[3]);
		setRegister(R_HPF_SPATIAL, x);
	} else if(r == C_FLIP) {
		unsigned char *p = protoBytes[C_FLIP];
		p[3] = x;
		sendCommand(C_FLIP, p[3]);
		setRegister(R_FLIP, x);
	} else if(r == C_IMAGE_UPDATE_SPEED) {
		unsigned char *p = protoBytes[C_IMAGE_UPDATE_SPEED];
		p[3] = x;
		sendCommand(C_IMAGE_UPDATE_SPEED, p[3]);
		setRegister(R_IMAGE_UPDATE_SPEED, x);
	}
}

uint MgeoThermalHead::getProperty(uint r)
{
	return getRegister(r);
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
		int x = 0;
		if (p[0] == 1)
			x = 0;
		else if (p[0] == 2)
			x = 1;
		setRegister(R_DIGITAL_ZOOM, x);
	} else if (opcode == 0xaa) {
		//image freeze
		setRegister(R_IMAGE_FREEZE, p[0]);
	} else if (opcode == 0xae) {
		//contrast brightness auto/manual
		if (p[0] == 0 )
			setRegister(R_AGC, 1);
		else setRegister(R_AGC, 0);
	} else if (opcode == 0xaf) {
		//brightness change/get
		setRegister(R_BRIGHTNESS, p[0]);
	} else if (opcode == 0xb1) {
		//contrast
		setRegister(R_CONTRAST, p[0]);
	} else if (opcode == 0xb5) {
		//reticle
		setRegister(R_RETICLE_INTENSITY, p[0]);
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

QByteArray MgeoThermalHead::transportReady()
{
	sendCommand(C_GET_ZOOM_FOCUS);
	return QByteArray();
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

QJsonValue MgeoThermalHead::marshallAllRegisters()
{
	QJsonObject json;
	json.insert(QString("reg0"), (int)getRegister(1));
	json.insert(QString("reg1"), (int)getRegister(2));
	json.insert(QString("reg2"), (int)getRegister(3));
	json.insert(QString("reg6"), (int)getRegister(7));
	json.insert(QString("reg7"), (int)getRegister(8));
	json.insert(QString("reg8"), (int)getRegister(9));
	json.insert(QString("reg9"), (int)getRegister(10));
	json.insert(QString("reg10"), (int)getRegister(11));
	json.insert(QString("reg11"), (int)getRegister(12));
	json.insert(QString("reg14"), (int)getRegister(13));
	json.insert(QString("reg17"), (int)getRegister(16));
	json.insert(QString("reg18"), (int)getRegister(17));
	json.insert(QString("reg19"), (int)getRegister(18));
	json.insert(QString("reg21"), (int)getRegister(20));
	return json;
}

void MgeoThermalHead::unmarshallloadAllRegisters(const QJsonValue &node)
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

int MgeoThermalHead::sendCommand(uint index, uchar data1, uchar data2)
{
	unsigned char *cmd = protoBytes[index];
	int cmdlen = cmd[1];
	cmd[3] = data1;
	cmd[4] = data2;
	cmd[cmdlen - 1] = chksum(cmd, cmdlen - 1);
	return transport->send((const char *)cmd , cmdlen);
}
