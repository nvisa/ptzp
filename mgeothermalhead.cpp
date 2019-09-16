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
		mInfo("%s: %d: 0x%x", __func__, i, p[i]);

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
	C_GET_DIGI_ZOOM,
	C_GET_POLARITY,
	C_GET_FOV,
	C_GET_IMAGE_FREEZE,

	C_COUNT,
// missing registers
	R_ANGLE,
	R_COOLED_DOWN
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
	{0x35, 0x0a, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	{0x35, 0x0a, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
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
	syncList << C_GET_DIGI_ZOOM;
	syncList << C_GET_POLARITY;
	syncList << C_GET_FOV;
	syncList << C_GET_IMAGE_FREEZE;
	nextSync = syncList.size();
	alive = false;

#ifdef HAVE_PTZP_GRPC_API
	settings = {
		{"brightness", {C_BRIGHTNESS, C_BRIGHTNESS}},
		{"contrast", {C_CONTRAST, C_CONTRAST}},
		{"fov", {C_FOV, C_FOV}},
		{"focus",{C_FOCUS, C_FOCUS}},
		{"angle", {NULL, R_ANGLE}},
		{"nuc_table", {C_NUC_SELECT, C_NUC_SELECT}},
		{"polarity", {C_POL_CHANGE, C_POL_CHANGE}},
		{"reticle", {C_RETICLE_ONOFF, C_RETICLE_ONOFF}},
		{"digi_zoom", {C_DIGITAL_ZOOM, C_DIGITAL_ZOOM}},
		{"image_freeze", {C_FREEZE_IMAGE, C_FREEZE_IMAGE}},
		{"agc", {C_AGC_SELECT, C_AGC_SELECT}},
		{"reticle_intensity", {C_RETICLE_CHANGE, C_RETICLE_CHANGE }},
		{"nuc",{C_NUC, C_NUC}},
		{"ibit", {C_IBIT, C_IBIT}},
		{"ipm_change", {C_IPM_CHANGE, C_IPM_CHANGE}},
		{"hpf_gain_change", {C_HPF_GAIN_CHANGE, C_HPF_GAIN_CHANGE}},
		{"hpf_spatial_change", {C_HPF_SPATIAL_CHANGE, C_HPF_SPATIAL_CHANGE}},
		{"flip", {C_FLIP, C_FLIP}},
		{"image_update_speed", {C_IMAGE_UPDATE_SPEED, C_IMAGE_UPDATE_SPEED}},
	};
#endif
}

int MgeoThermalHead::getCapabilities()
{
	return CAP_ZOOM;
}

bool MgeoThermalHead::isAlive()
{
	return alive;
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

int MgeoThermalHead::getAngle(){
	return getRegister(R_ANGLE);
}

int MgeoThermalHead::getZoom()
{
	return getRegister(C_CONT_ZOOM);
}

int MgeoThermalHead::focusIn(int speed)
{
	Q_UNUSED(speed);
	unsigned char *p = protoBytes[C_FOCUS];
	p[3] = 0x01; //focus far
	return sendCommand(C_FOCUS, p[3]);
}

int MgeoThermalHead::focusOut(int speed)
{
	Q_UNUSED(speed);
	unsigned char *p = protoBytes[C_FOCUS];
	p[3] = 0xFF; //focus near
	return sendCommand(C_FOCUS, p[3]);
}

int MgeoThermalHead::focusStop()
{
	return sendCommand(C_FOCUS);
}

int MgeoThermalHead::getHeadStatus()
{
	if (nextSync != syncList.size())
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
	if (r >= C_COUNT)
		return;
	unsigned char *p = protoBytes[r];
	p[3] = 0x00;
	p[4] = 0x00;
	if (r == C_BRIGHTNESS) {
		p[3] = 0x01;
		p[4] = x;
		setRegister(r, x);
	} else if(r == C_CONTRAST) {
		p[4] = x;
	} else if(r == C_FOV) {
		p[3] = x;
	} else if(r == C_CONT_ZOOM) {
		if(x == 0)
			p[3] = 0x00; //zoom stop
		else if(x == 1)
			p[3] = 0x01; //zoom in
		else if (x == 2)
			p[3] = 0xFF; //zoom out
	} else if(r == C_NUC_SELECT) {
		p[3] = x;
	} else if(r == C_POL_CHANGE) {
		p[3] = x;
	} else if(r == C_RETICLE_ONOFF) {
		p[3] = x;
	} else if(r == C_DIGITAL_ZOOM) {
		p[3] = x ? 0x01 : 0x00;
	} else if(r == C_FREEZE_IMAGE) {
		p[3] = x ? 0x01 : 0x00;
	} else if (r == C_AGC_SELECT) {
		p[3] = x ? 0x01 : 0x00;
	}else if(r == C_BRIGHTNESS_CHANGE) {
		p[3] = x;
	} else if(r == C_CONTRAST_CHANGE) {
		p[3] = x;
	} else if(r == C_RETICLE_CHANGE) {
		p[3] = x;
	} else if(r == C_NUC) {

	} else if(r == C_IBIT) {

	} else if(r == C_IPM_CHANGE) {
		p[3] = x;
	} else if(r == C_HPF_GAIN_CHANGE) {
		p[3] = x;
	} else if(r == C_HPF_SPATIAL_CHANGE) {
		p[3] = x;
	} else if(r == C_FLIP) {
		p[3] = x;
	} else if(r == C_IMAGE_UPDATE_SPEED) {
		p[3] = x;
	} else
		return;
	sendCommand(r, p[3], p[4]);
	setRegister(r, x);
}

uint MgeoThermalHead::getProperty(uint r)
{
	return getRegister(r);
}

int MgeoThermalHead::setZoom(uint pos)
{
	Q_UNUSED(pos);
	mDebug("Set zoom process not available from ICD documents.");
	return 0;
}

int MgeoThermalHead::headSystemChecker()
{
	if (systemChecker == -1) {
		int ret = sendCommand(C_GET_ZOOM_FOCUS);
		mInfo("ThermalHead system checker started. %d", ret);
		systemChecker = 0;
	} else if (systemChecker == 0) {
		mInfo("Waiting Response from thermal CAM");
	} else if (systemChecker == 1) {
		mInfo("Completed System Check. Zoom: %f Focus: %f Angle: %f", getRegister(C_CONT_ZOOM), getRegister(C_FOCUS), getRegister(R_ANGLE));
		systemChecker = 2;
	}
	return systemChecker;
}

int MgeoThermalHead::dataReady(const unsigned char *bytes, int len)
{
	if (bytes[0] != 0xca)
		return len;
	if (len < 2)
		return len;
	int meslen = bytes[1];
	if (len < meslen)
		return len;

	if (meslen == 5 && bytes[2] == 0xc3) {
		alive = true;
		/* ping message */
		pingTimer.restart();
		return meslen;
	}

	/* register sync support */
	if (nextSync != syncList.size()) {
		/* we are in sync mode, let's sync next */
		if (++nextSync == syncList.size()) {
			mDebug("Thermal register syncing completed, activating auto-sync");
		} else
			syncNext();
	}

	uchar chk = chksum(bytes, meslen - 1);
	if (chk != bytes[meslen - 1]) {
		mInfo("Checksum error");
		return meslen;
	}

	uchar opcode = bytes[2];
	const uchar *p = bytes + 3;

	if (opcode == 0xa0) {
		//cooled down
		setRegister(R_COOLED_DOWN, p[0]);
	} else if (opcode == 0xa1) {
		//fov change
		setRegister(C_FOV, p[0]);
	} else if (opcode == 0xbc) {
		//zoom/focus
		setRegister(C_CONT_ZOOM, (p[1] << 8));
		setRegister(C_FOCUS, (p[3] << 8 | p[2]));
		setRegister(R_ANGLE, (p[5] << 8 | p[4]));
		if (systemChecker == 0)
			systemChecker = 1;
	} else if (opcode == 0xa4) {
		//nuc table selection
		setRegister(C_NUC_SELECT, p[0]);
	} else if (opcode == 0xa5) {
		//polarity change
		setRegister(C_POL_CHANGE, p[0]);
	} else if (opcode == 0xa7) {
		//reticle on-off
		setRegister(C_RETICLE_ONOFF, p[0]);
	} else if (opcode == 0xa9) {
		//digital zoom
		int x = 0;
		if (p[0] == 1)
			x = 0;
		else if (p[0] == 2)
			x = 1;
		setRegister(C_DIGITAL_ZOOM, x);
	} else if (opcode == 0xaa) {
		//image freeze
		setRegister(C_FREEZE_IMAGE, p[0]);
	} else if (opcode == 0xae) {
		//contrast brightness auto/manual
		if (p[0] == 0 )
			setRegister(C_AGC_SELECT, 1);
		else setRegister(C_AGC_SELECT, 0);
	} else if (opcode == 0xaf) {
		//brightness change/get
		setRegister(C_BRIGHTNESS, p[0]);
	} else if (opcode == 0xb1) {
		//contrast
		setRegister(C_CONTRAST, p[0]);
	} else if (opcode == 0xb5) {
		//reticle
		setRegister(C_RETICLE_CHANGE, p[0]);
	} else if (opcode == 0xa2) {
		//one-point
		setRegister(C_NUC, p[0]);
	} else if (opcode == 0xba) {
		//menu
	} else if (opcode == 0xbb) {
		//ibit
		setRegister(C_IBIT, p[0]);
	} else if (opcode == 0xc3) {
		//c3 ops
		if (p[0] == 0xd7)
			setRegister(C_IPM_CHANGE, p[1]);
		else if (p[0] == 0xd8)
			setRegister(C_HPF_GAIN_CHANGE, p[1]);
		else if (p[0] == 0xd9)
			setRegister(C_HPF_SPATIAL_CHANGE, p[1]);
		else if (p[0] == 0xde)
			setRegister(C_IMAGE_UPDATE_SPEED, p[1]);
	}
	return meslen;
}

QByteArray MgeoThermalHead::transportReady()
{
	if (nextSync != syncList.size())
		return QByteArray();
	unsigned char *cmd = protoBytes[C_GET_ZOOM_FOCUS];
	int cmdlen = cmd[1];
	cmd[cmdlen - 1] = chksum(cmd, cmdlen - 1);
	return QByteArray((const char*)cmd, cmdlen);
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
	if (cmd == C_GET_DIGI_ZOOM)
		return sendCommand(cmd);
	if (cmd == C_GET_POLARITY)
		return sendCommand(cmd);
	if (cmd == C_GET_FOV)
		return sendCommand(cmd);
	if (cmd == C_GET_IMAGE_FREEZE)
		return sendCommand(cmd);

	return -ENOENT;
}

QJsonValue MgeoThermalHead::marshallAllRegisters()
{
	QJsonObject json;
	QString reg("reg%1");
	foreach (int r, loadDefaultRegisterValues())
		json.insert(reg.arg(r), (int)getRegister(r));
	return json;
}

void MgeoThermalHead::unmarshallloadAllRegisters(const QJsonValue &node)
{
	QJsonObject root = node.toObject();
	foreach (QString key, root.keys()) {
		if (!key.startsWith("reg"))
			continue;
		int v = root[key].toInt();
		int ind = key.remove("reg").toInt();
		mInfo("Loading register: %d: \t %d",ind, v);
		setProperty(ind, v);
	}
}

QList<int> MgeoThermalHead::loadDefaultRegisterValues()
{
	QList<int> regs;
	regs << C_FOV;
	regs << C_CONTRAST;
	regs << C_BRIGHTNESS;
	regs << C_POL_CHANGE;
	regs << C_AGC_SELECT;
	regs << C_IPM_CHANGE;
	regs << C_DIGITAL_ZOOM;
	regs << C_FREEZE_IMAGE;
	regs << C_RETICLE_ONOFF;
	regs << C_RETICLE_CHANGE;
	regs << C_HPF_GAIN_CHANGE;
	regs << C_HPF_SPATIAL_CHANGE;
	regs << C_IMAGE_UPDATE_SPEED;
	return regs;
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
