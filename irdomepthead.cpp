#include "irdomepthead.h"
#include "debug.h"
#include "ioerrors.h"
#include "ptzptransport.h"

enum Commands {
	C_CUSTOM_PAN_TILT_POS,		//0
	C_CUSTOM_GO_TO_POS,

	/* pelco-d commands */
	C_PAN_LEFT,					//1
	C_PAN_RIGHT,				//2
	C_TILT_UP,					//3
	C_TILT_DOWN,				//4
	C_PAN_LEFT_TILT_UP,			//5
	C_PAN_LEFT_TILT_DOWN,		//6
	C_PAN_RIGHT_TILT_UP, 		//7
	C_PAN_RIGHT_TILT_DOWN,		//8
	C_PELCOD_STOP,				//9

	C_COUNT,
};

#define MAX_CMD_LEN	16

static uchar protoBytes[C_COUNT][MAX_CMD_LEN] = {
	/* custom commands */
	{0x09, 0x09, 0x3a, 0xff, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c},
	{0x09, 0x09, 0x3a, 0xff, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c},

	/* pelco-d commands */
	{0x07, 0x00, 0xff, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00},
	{0x07, 0x00, 0xff, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00},
	{0x07, 0x00, 0xff, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00},
	{0x07, 0x00, 0xff, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00},
	{0x07, 0x00, 0xff, 0x01, 0x00, 0x0c, 0x00, 0x00, 0x00},
	{0x07, 0x00, 0xff, 0x01, 0x00, 0x14, 0x00, 0x00, 0x00},
	{0x07, 0x00, 0xff, 0x01, 0x00, 0x0a, 0x00, 0x00, 0x00},
	{0x07, 0x00, 0xff, 0x01, 0x00, 0x12, 0x00, 0x00, 0x00},
	{0x07, 0x07, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}, //pelcod_stop
};

static uint checksum(const uchar *cmd, uint lenght)
{
	unsigned int sum = 0;
	for (uint i = 1; i < lenght; i++)
		sum += cmd[i];
	return sum & 0xff;
}

IRDomePTHead::IRDomePTHead()
{
	syncEnabled = true;
	syncInterval = 20;
	syncTime.start();
}

int IRDomePTHead::getCapabilities()
{
	return PtzpHead::CAP_PAN | PtzpHead::CAP_TILT;
}

int IRDomePTHead::panLeft(float speed)
{
	int pspeed = qAbs(speed) * 0x3f;
	return panTilt(C_PAN_LEFT, pspeed, 0);
}

int IRDomePTHead::panRight(float speed)
{
	int pspeed = qAbs(speed) * 0x3f;
	return panTilt(C_PAN_RIGHT, pspeed, 0);
}

int IRDomePTHead::tiltUp(float speed)
{
	int pspeed = qAbs(speed) * 0x3f;
	return panTilt(C_TILT_UP, 0, pspeed);
}

int IRDomePTHead::tiltDown(float speed)
{
	int pspeed = qAbs(speed) * 0x3f;
	return panTilt(C_TILT_DOWN, 0, pspeed);
}

int IRDomePTHead::panTiltAbs(float pan, float tilt)
{
	int pspeed = qAbs(pan) * 0x3f;
	int tspeed = qAbs(tilt) * 0x3f;
	if (pspeed == 0 && tspeed == 0) {
		panTilt(C_PELCOD_STOP, 0, 0);
		return 0;
	}
	if (pan < 0) {
		//left
		if (tilt < 0) {
			//up
			panTilt(C_PAN_LEFT_TILT_UP, pspeed, tspeed);
		} else if (tilt > 0) {
			//down
			panTilt(C_PAN_LEFT_TILT_DOWN, pspeed, tspeed);
		} else
			panTilt(C_PAN_LEFT, pspeed, tspeed);
	} else {
		//right
		if (tilt < 0) {
			//up
			panTilt(C_PAN_RIGHT_TILT_UP, pspeed, tspeed);
		} else if (tilt > 0) {
			//down
			panTilt(C_PAN_RIGHT_TILT_DOWN, pspeed, tspeed);
		} else
			panTilt(C_PAN_RIGHT, pspeed, tspeed);
	}
	return 0;
}

int IRDomePTHead::panTiltStop()
{
	return panTilt(C_PELCOD_STOP, 0, 0);
}

void setCustomCommandParameters(uchar *p, uint cmd, int arg1, int arg2)
{
	if (cmd == C_CUSTOM_GO_TO_POS) {
		p[5] = (arg1 >> 8) & 0xff;
		p[6] = arg1 & 0xff;
		p[7] = (arg2 >> 8) & 0xff;
		p[8] = arg2 & 0xff;
		p[9] = checksum(p + 2, 7);
	}
}

int IRDomePTHead::panTilt(uint cmd, int pspeed, int tspeed)
{
	/*
	 * our dome has one serial channel and it is a slow one,
	 * we disable other readers, zoom reading esspecially,
	 * so that communication is not adversely affected
	 */
	if (cmd == C_PELCOD_STOP)
		transport->setQueueFreeCallbackMask(0xffffffff);
	else
		transport->setQueueFreeCallbackMask(0xfffffffe);
	unsigned char *p = protoBytes[cmd];
	if (p[2] == 0x3a) {
		setCustomCommandParameters(p, cmd, pspeed, tspeed);
	} else {
		p[4 + 2] = pspeed;
		p[5 + 2] = tspeed;
		p[6 + 2] = checksum(p + 2, 6);
	}
	return transport->send(QByteArray((const char *)p + 2, p[0]));
}

float IRDomePTHead::getPanAngle()
{
	return getRegister(R_PAN_ANGLE) / 100.0;
}

float IRDomePTHead::getTiltAngle()
{
	return getRegister(R_TILT_ANGLE) / 100.0;
}

int IRDomePTHead::dataReady(const unsigned char *bytes, int len)
{
	for (int i = 0 ; i < len; i++)
		mLogv("%d", bytes[i]);
	const unsigned char *p = bytes;
	if (p[0] != 0x3a)
		return -1;
	if (len < 9)
		return -1;
	/* custom messages */
	if (p[2] != 0x82) {
		setIOError(IOE_UNSUPP_SPECIAL);
		return 9;
	}
	if (p[7] != checksum(p, 7)) {
		setIOError(IOE_SPECIAL_CHKSUM);
		return 9;
	}
	if (p[8] != 0x5c)
		setIOError(IOE_SPECIAL_LAST);
	setRegister(R_PAN_ANGLE, (p[3] << 8) + p[4]);
	setRegister(R_TILT_ANGLE, (p[5] << 8) + p[6]);
	return 9;
}

QByteArray IRDomePTHead::transportReady()
{
	if (syncEnabled && syncTime.elapsed() > syncInterval) {
		mLogv("Syncing pan and tilt pos");
		unsigned char *p = protoBytes[C_CUSTOM_PAN_TILT_POS];
		p[2 + 7]  = checksum(p + 2, 7);
		syncTime.restart();
		return QByteArray((const char *)p + 2, p[0]);
	}
	return QByteArray();
}

int IRDomePTHead::panTiltGoPos(float ppos, float tpos)
{
	panTilt(C_CUSTOM_GO_TO_POS, ppos*100, tpos*100);
	return 0;
}

void IRDomePTHead::enableSyncing(bool en)
{
	syncEnabled = en;
	mInfo("Sync status: %d", (int)syncEnabled);
}

void IRDomePTHead::setSyncInterval(int interval)
{
	syncInterval = interval;
	mInfo("Sync interval: %d", syncInterval);
}
