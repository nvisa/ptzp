#include "okbsrppthead.h"
#include "debug.h"
#include "ptzptransport.h"

#include <assert.h>

#define dump(p, len)                                                           \
	for (int i = 0; i < len; i++)                                              \
		qDebug("%s: %d: 0x%x", __func__, i, p[i]);

static uint checksum(const uchar *p, uint lenght)
{
	unsigned int sum = 0;
	for (uint i = 23; i < lenght; i++)
		sum += p[i];
	return ((~sum )+ 0x01) & 0xff;
}

OkbSrpPTHead::OkbSrpPTHead()
{
	setControlOwner(THIRD_PARTY);
	pingTimer.start();
}

void OkbSrpPTHead::fillCapabilities(ptzp::PtzHead *head)
{
	head->add_capabilities(ptzp::PtzHead_Capability_PAN);
	head->add_capabilities(ptzp::PtzHead_Capability_TILT);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_JOYSTICK_CONTROL);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_PT);
}

int OkbSrpPTHead::getHeadStatus()
{
	if (pingTimer.elapsed() < 3000)
		return ST_NORMAL;
	return ST_ERROR;
}

float OkbSrpPTHead::getPanAngle()
{
	return panPos;
}

float OkbSrpPTHead::getTiltAngle()
{
	return -1 * tiltPos;
}

QVariant OkbSrpPTHead::getCapabilityValues(ptzp::PtzHead_Capability c)
{
	return QVariant();
}

void OkbSrpPTHead::setCapabilityValues(ptzp::PtzHead_Capability c, uint val)
{
	//TODO
}

int OkbSrpPTHead::dataReady(const unsigned char *bytes, int len)
{
	if(bytes[0] != 0x1f || bytes[1] != 0x1f)
		return -ENOENT;
	if (bytes[20] != 0x51)
		return -ENOENT;
//	int chk = checksum(bytes, len);
//	if(bytes[22] != chk || bytes[23] != chk){
//		fDebug("Checksum error");
//		return len;
//	}
	pingTimer.restart();
	panPos = *(float *)(bytes + 40);
	tiltPos = *(float *)(bytes + 48);

	if (abs(panPos - prevPanPos) > 1 || abs(tiltPos - prevTiltPos) > 1)
		setIsPTchanged(true);
	else
		setIsPTchanged(false);

	prevPanPos = panPos;
	prevTiltPos = tiltPos;

	return len;
}
