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
	C_GO_POS,

	C_GET_POS,

	C_COUNT,
};

static QStringList ptzCommandList = {
	"$pt_stop",

	"$pt_cmd=%1,0.0",
	"$pt_cmd=%1,0.0",

	"$pt_cmd=0.0,%1",
	"$pt_cmd=0.0,%1",

	"$pt_cmd=%1,%2",
	"$go_pos=%1,%2",

	"$ptz_pos",

};

HtrSwirPtHead::HtrSwirPtHead()
{
	syncTimer.start();
}

void HtrSwirPtHead::fillCapabilities(ptzp::PtzHead *head)
{
	head->add_capabilities(ptzp::PtzHead_Capability_PAN);
	head->add_capabilities(ptzp::PtzHead_Capability_TILT);
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

float HtrSwirPtHead::getPanAngle()
{
	return panPos / 100;
}

float HtrSwirPtHead::getTiltAngle()
{
	float tpos = tiltPos / 100;
	if (tpos > 180)
		tpos = tpos - 360;
	return tpos;
}

int HtrSwirPtHead::panTiltGoPos(float ppos, float tpos)
{
	ppos = ppos / 180;
	tpos = tpos/ 90;
	return sendCommand(ptzCommandList.at(C_GO_POS).arg(ppos).arg(tpos));
}

QVariant HtrSwirPtHead::getCapabilityValues(ptzp::PtzHead_Capability c)
{
	return QVariant();
}

void HtrSwirPtHead::setCapabilityValues(ptzp::PtzHead_Capability c, uint val)
{
	//TODO
}

int HtrSwirPtHead::sendCommand(const QString &key)
{
	QByteArray cmd = key.toStdString().c_str();
	return transport->send(cmd.data());
}

int HtrSwirPtHead::dataReady(const unsigned char *bytes, int len)
{
	const QString data = QString::fromUtf8((const char *)bytes, len);
	if(!data.startsWith("!<pt"))
		return -ENOENT;
	if(data.contains("<ptz_pos>")){
		QString str = data.split(">").at(1);
		panPos = str.split(",").first().toInt();
		tiltPos =str.split(",").at(1).toInt();
		pingTimer.restart();
	}
	return len;
}

QByteArray HtrSwirPtHead::transportReady()
{
	if(syncTimer.elapsed() > 1000){
		syncTimer.restart();
		return ptzCommandList.at(C_GET_POS).toUtf8();
	}
	return QByteArray();
}
