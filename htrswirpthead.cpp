#include "htrswirpthead.h"
#include "debug.h"
#include "ptzptcptransport.h"

enum CommandList {
	C_STOP_ALL,

	C_PAN_RIGHT_RATE,
	C_PAN_LEFT_RATE,

	C_TILT_UP_RATE,
	C_TILT_DOWN_RATE,

	C_PAN_TILT_MOVE,

	C_COUNT,
};

static QStringList ptzCommandList = {
	"$pt_stop",

	"$pt_cmd=%1,0.0",
	"$pt_cmd=%1,0.0",

	"$pt_cmd=0.0,%1",
	"$pt_cmd=0.0,%1",

	"$pt_cmd=%1,%2",

};

HtrSwirPtHead::HtrSwirPtHead()
{
}

int HtrSwirPtHead::getCapabilities()
{
	return CAP_PAN | CAP_TILT;
}

int HtrSwirPtHead::getHeadStatus()
{
		return ST_NORMAL;
}

int HtrSwirPtHead::panLeft(float speed)
{
	return sendCommand(ptzCommandList.at(C_PAN_LEFT_RATE).arg(speed * -1.0));
}

int HtrSwirPtHead::panRight(float speed)
{
	return sendCommand(ptzCommandList.at(C_PAN_RIGHT_RATE).arg(speed));
}

int HtrSwirPtHead::tiltUp(float speed)
{
	return sendCommand(ptzCommandList.at(C_TILT_UP_RATE).arg(speed));
}

int HtrSwirPtHead::tiltDown(float speed)
{
	return sendCommand(ptzCommandList.at(C_TILT_DOWN_RATE).arg(speed * -1.0));
}

int HtrSwirPtHead::panTiltAbs(float pan, float tilt)
{
	if (pan == 0 && tilt == 0) {
		sendCommand(ptzCommandList[C_STOP_ALL]);
		return 0;
	}
	sendCommand(ptzCommandList.at(C_PAN_TILT_MOVE).arg(pan).arg(tilt));
	return 0;
}

int HtrSwirPtHead::panTiltStop()
{
	return sendCommand(ptzCommandList.at(C_STOP_ALL));
}

int HtrSwirPtHead::sendCommand(const QString &key)
{
	QByteArray cmd = key.toStdString().c_str();
	return transport->send(cmd.data());
}
