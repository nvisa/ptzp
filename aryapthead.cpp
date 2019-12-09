#include "aryapthead.h"
#include "debug.h"
#include "ptzptcptransport.h"

#include <assert.h>

#define MaxPanPos  6399999.0
#define MaxTiltPos 799999.0

/**
 * Input parameter: example ascii parameter is "<IP2PRR:%1;IP0"
 * Because this function can calculate checksum values
 * and append to ascii parameters
 * Output value: return value is "<IP2PRR:%1;IP05C>"
 * */
static QString createPTZCommand(const QString &ascii)
{
	if (!ascii.size())
		return "";
	const QByteArray &ba = ascii.toUtf8();
	int len = ba.length();
	const char *el = ba.constData();
	char checksum = el[0];
	for (int i = 1; i < len; i++)
		checksum = checksum ^ el[i];
	return QString("%1%2>").arg(ascii).arg(
		QString::number((uint)checksum, 16).toUpper());
}

enum CommandList {
	C_STOP_ALL,

	C_PAN_RIGHT_RATE,
	C_PAN_LEFT_RATE,

	C_TILT_UP_RATE,
	C_TILT_DOWN_RATE,

	C_PAN_RIGHT_TILT_UP,
	C_PAN_RIGHT_TILT_DOWN,

	C_PAN_LEFT_TILT_UP,
	C_PAN_LEFT_TILT_DOWN,

	C_PAN_TILT_POS,

	C_GET_PAN_POS,

	C_COUNT,
};

static QStringList createPTZCommandList()
{
	QStringList cmdList;
	cmdList << QString("<IP2STP:;IP0");

	cmdList << QString("<IP2PRR:%1;IP0");
	cmdList << QString("<IP2PLR:%1;IP0");

	cmdList << QString("<IP2TUR:%1;IP0");
	cmdList << QString("<IP2TDR:%1;IP0");

	cmdList << QString("<IP2PTR:%1,%2;IP0");
	cmdList << QString("<IP2PTR:%1,-%2;IP0");

	cmdList << QString("<IP2PTR:-%1,%2;IP0");
	cmdList << QString("<IP2PTR:-%1,-%2;IP0");

	cmdList << QString("<IP2PTA:%1,%2;IP0");

	cmdList << QString("<ZZZPTT:;ZZZ\?\?>");
	return cmdList;
}

AryaPTHead::AryaPTHead() : PtzpHead()
{
	MaxSpeed = 700000;
	ptzCommandList = createPTZCommandList();
	assert(ptzCommandList.size() == C_COUNT);
}

int AryaPTHead::getCapabilities()
{
	return CAP_PAN | CAP_TILT;
}

int AryaPTHead::getHeadStatus()
{
	if (pingTimer.elapsed() < 1500)
		return ST_NORMAL;
	return ST_ERROR;
}

int AryaPTHead::panLeft(float speed)
{
	int pspeed = qAbs(speed) * MaxSpeed;
	return sendCommand(ptzCommandList.at(C_PAN_LEFT_RATE).arg(pspeed));
}

int AryaPTHead::panRight(float speed)
{
	int pspeed = qAbs(speed) * MaxSpeed;
	return sendCommand(ptzCommandList.at(C_PAN_RIGHT_RATE).arg(pspeed));
}

int AryaPTHead::tiltUp(float speed)
{
	int tspeed = qAbs(speed) * MaxSpeed;
	return sendCommand(ptzCommandList.at(C_TILT_UP_RATE).arg(tspeed));
}

int AryaPTHead::tiltDown(float speed)
{
	int tspeed = qAbs(speed) * MaxSpeed;
	return sendCommand(ptzCommandList.at(C_TILT_DOWN_RATE).arg(tspeed));
}

int AryaPTHead::panTiltAbs(float pan, float tilt)
{
	int pspeed = qAbs(pan) * MaxSpeed;
	int tspeed = qAbs(tilt) * MaxSpeed;
	if (pspeed == 0 && tspeed == 0) {
		sendCommand(ptzCommandList[C_STOP_ALL]);
		return 0;
	}
	if (pan < 0) {
		// left
		if (tilt < 0) {
			// up
			sendCommand(
				ptzCommandList.at(C_PAN_LEFT_TILT_UP).arg(pspeed).arg(tspeed));
		} else if (tilt > 0) {
			// down
			sendCommand(ptzCommandList.at(C_PAN_LEFT_TILT_DOWN)
							.arg(pspeed)
							.arg(tspeed));
		} else {
			sendCommand(ptzCommandList.at(C_PAN_LEFT_RATE).arg(pspeed));
		}
	} else {
		// right
		if (tilt < 0) {
			// up
			sendCommand(
				ptzCommandList.at(C_PAN_RIGHT_TILT_UP).arg(pspeed).arg(tspeed));
		} else if (tilt > 0) {
			// down
			sendCommand(ptzCommandList.at(C_PAN_RIGHT_TILT_DOWN)
							.arg(pspeed)
							.arg(tspeed));
		} else {
			sendCommand(ptzCommandList.at(C_PAN_RIGHT_RATE).arg(pspeed));
		}
	}
	return 0;
}

int AryaPTHead::panTiltDegree(float pan, float tilt)
{
	// 1 mil = 17.777
	float pans = (pan * 17.777) * 1000 / MaxSpeed;
	float tilts = (tilt * 17.777) * 1000 / MaxSpeed;
	return panTiltAbs(pans, tilts);
}

int AryaPTHead::panTiltStop()
{
	return sendCommand(ptzCommandList[C_STOP_ALL]);
}

float AryaPTHead::getPanAngle()
{
	return panPos / MaxPanPos * 360;
}

float AryaPTHead::getTiltAngle()
{
	return tiltPos / MaxTiltPos * 45;
}

int AryaPTHead::panTiltGoPos(float ppos, float tpos)
{
	ppos = qAbs(ppos) / 360 * MaxPanPos;
	tpos = tpos / 45 * MaxTiltPos;
	return sendCommand(
		ptzCommandList.at(C_PAN_TILT_POS).arg((int)ppos).arg((int)tpos));
}

void AryaPTHead::setMaxSpeed(int value)
{
	MaxSpeed = value;
}

int AryaPTHead::sendCommand(const QString &key)
{
	QString cmd = key;
	if (!cmd.contains(">"))
		cmd = createPTZCommand(key);
	mInfo("Sending command '%s'", qPrintable(cmd));
	return transport->send(cmd.toUtf8());
}

int AryaPTHead::dataReady(const unsigned char *bytes, int len)
{
	const QString data = QString::fromUtf8((const char *)bytes, len);
	if (data.contains(":")) {
		if (systemChecker == 0)
			systemChecker = 1;
		QString value = data.split(":").last().split(";").first();
		panPos = value.split(",").first().toInt();
		tiltPos = value.split(",").last().toInt();
		pingTimer.restart();
	}

	return len;
}

QByteArray AryaPTHead::transportReady()
{
	return ptzCommandList.at(C_GET_PAN_POS).toUtf8();
}
