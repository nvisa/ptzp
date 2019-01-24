#include "evpupthead.h"
#include "ptzptcptransport.h"
#include "debug.h"

#define MaxSpeedA	25736
#define MaxSpeedE	21446
#define MaxPanPos	62831
#define AvgTiltPos	31416
#define MinTiltPos	23562
#define MaxTiltPos	39270

enum CommandList {
	C_STOP_ALL,

	C_PAN_RIGHT_RATE,
	C_PAN_LEFT_RATE,

	C_TILT_UP_RATE,
	C_TILT_DOWN_RATE,

	C_PAN_TILT_GO_POS,

	C_GET_PAN_POS,
	C_GET_TILT_POS,

	C_COUNT,
};

static QStringList ptzCommandList = {
	"a vel 0\r\ne vel 0\r\n",

	"a vel %1\r\n",
	"a vel %1\r\n",

	"e vel %1\r\n",
	"e vel %1\r\n",

	"a pos %1\r\ne pos %2\r\n",

	"a get\r\n",
	"e get\r\n",
};

EvpuPTHead::EvpuPTHead()
{

}

int EvpuPTHead::getCapabilities()
{
	return CAP_PAN | CAP_TILT;
}

int EvpuPTHead::getHeadStatus()
{
	if (pingTimer.elapsed() < 1500)
		return ST_NORMAL;
	return ST_ERROR;
}

int EvpuPTHead::panTiltStop()
{
	return sendCommand(ptzCommandList.at(C_STOP_ALL));
}

float EvpuPTHead::getPanAngle()
{
	return (float)panPos / MaxPanPos * 360.0;
}

float EvpuPTHead::getTiltAngle()
{
	return (float)(90.0 * (tiltPos - AvgTiltPos) / (float)(MaxTiltPos - MinTiltPos));
}

int EvpuPTHead::panTiltGoPos(float ppos, float tpos)
{
	ppos = ppos / 360.0 * MaxPanPos;
	tpos = (tpos / 90.0) * (float)(MaxTiltPos - MinTiltPos) + AvgTiltPos;
	return sendCommand(ptzCommandList.at(C_PAN_TILT_GO_POS).arg((int)ppos).arg((int)tpos));
}

int EvpuPTHead::setOutput(int no, bool on)
{
	return sendCommand(QString("o %1 %2\r\n").arg(no).arg(on ? "1" : "0"));
}

int EvpuPTHead::panLeft(float speed)
{
	speed = speed * -MaxSpeedA;
	return sendCommand(ptzCommandList.at(C_PAN_LEFT_RATE).arg((int)speed));
}

int EvpuPTHead::panRight(float speed)
{
	speed = speed * MaxSpeedA;
	return sendCommand(ptzCommandList.at(C_PAN_RIGHT_RATE).arg((int)speed));
}

int EvpuPTHead::tiltUp(float speed)
{
	speed = speed * -MaxSpeedE;
	return sendCommand(ptzCommandList.at(C_TILT_UP_RATE).arg((int)speed));
}

int EvpuPTHead::tiltDown(float speed)
{
	speed = speed * MaxSpeedE;
	return sendCommand(ptzCommandList.at(C_TILT_DOWN_RATE).arg((int)speed));
}

int EvpuPTHead::panTiltAbs(float pan, float tilt)
{
	pan = pan * MaxSpeedA;
	tilt = tilt * MaxSpeedE;
	if (pan == 0 && tilt == 0) {
		sendCommand(ptzCommandList[C_STOP_ALL]);
		return 0;
	}
	if (pan < 0) {
		// left
		sendCommand(ptzCommandList.at(C_PAN_LEFT_RATE).arg((int)pan));
		if (tilt < 0) {
			// down
			sendCommand(ptzCommandList.at(C_TILT_DOWN_RATE).arg((int)tilt));
		} else if (tilt > 0) {
			// up
			sendCommand(ptzCommandList.at(C_TILT_UP_RATE).arg((int)tilt));
		}
	} else {
		// right
		sendCommand(ptzCommandList.at(C_PAN_RIGHT_RATE).arg((int)pan));
		if (tilt < 0) {
			// down
			sendCommand(ptzCommandList.at(C_TILT_DOWN_RATE).arg((int)tilt));
		} else if (tilt > 0) {
			// up
			sendCommand(ptzCommandList.at(C_TILT_UP_RATE).arg((int)tilt));
		}
	}
	return 0;
}

int EvpuPTHead::sendCommand(const QString &key)
{
	QByteArray cmd = key.toStdString().c_str();
	return transport->send(cmd.data());
}

int EvpuPTHead::dataReady(const unsigned char *bytes, int len)
{
	const QString data = QString::fromUtf8((const char *)bytes, len);
	if (data.startsWith("a syn ok") || data.startsWith("e syn ok"))
		return 10;
	/*
	 * When we first connect to EVPU, 1 byte of data may be eaten causing
	 * us to receive 1-less character for the syn messages
	 */
	if (data.startsWith(" syn ok"))
		return 9;
	if (data.startsWith("e end") || data.startsWith("a end"))
		return 7;
	if (data.startsWith("a get")) {
		QStringList flds = data.split(" ");
		if (flds.size() < 3) //we have some bug
			return len;
		panPos = flds[2].toInt();
		pingTimer.restart();
		return flds[0].size() + flds[1].size() + flds[2].size() + 4;
	} else if (data.startsWith("e get")) {
		QStringList flds = data.split(" ");
		if (flds.size() < 3) //we have some bug
			return len;
		tiltPos = flds[2].toInt();
		pingTimer.restart();
		return flds[0].size() + flds[1].size() + flds[2].size() + 4;
	} else /* this is not for us */ {
		mLog("Incoming message from EVPU: %s", bytes);
		return 0;
	}

	return len;
}

QByteArray EvpuPTHead::transportReady()
{
	syncFlag = (syncFlag + 1) & 0x01;
	if (syncFlag)
		return ptzCommandList.at(C_GET_PAN_POS).toUtf8();
	return ptzCommandList.at(C_GET_TILT_POS).toUtf8();
}
