#include "flirpttcphead.h"
#include "debug.h"
#include <ptzp/ptzptransport.h>

enum CommandList {
	C_STOP,
	C_PAN_LEFT,
	C_PAN_RIGHT,
	C_TILT_UP,
	C_TILT_DOWN,
	C_PAN_LEFT_TILT_UP,
	C_PAN_LEFT_TILT_DOWN,
	C_PAN_RIGHT_TILT_UP,
	C_PAN_RIGHT_TILT_DOWN,
	C_PAN_TILT_GO_POS,
	C_GET_PT,
	C_GET_PAN_SPEED,
	C_GET_TILT_SPEED,
	C_GET_MAX_MIN_POS,
	C_COUNT
};

static QStringList createPtzCommands()
{
	QStringList cmdList;
	cmdList << QString("PS0 TS0");
	cmdList << QString("PS-%1");
	cmdList << QString("PS%1");
	cmdList << QString("TS-%1");
	cmdList << QString("TS%1");
	cmdList << QString("PS-%1 TS-%2");
	cmdList << QString("PS-%1 TS%2");
	cmdList << QString("PS%1 TS-%2");
	cmdList << QString("PS%1 TS%2");
	cmdList << QString("PS1200 TS1200 PP%1 TP%2");
	cmdList << QString("PP TP");
	cmdList << QString("PS");
	cmdList << QString("TS");
	cmdList << QString("PN PX TN TX");
	return cmdList;
}

FlirPTTcpHead::FlirPTTcpHead() :
	PtzpHead()
{
	ptzCommandList = createPtzCommands();

}

void FlirPTTcpHead::initialize()
{
	sendCommand(ptzCommandList.at(C_GET_PAN_SPEED));
	sendCommand(ptzCommandList.at(C_GET_TILT_SPEED));
	sendCommand(ptzCommandList.at(C_GET_PT));
	sendCommand(ptzCommandList.at(C_GET_MAX_MIN_POS));
}

int FlirPTTcpHead::getCapabilities()
{
	return CAP_PAN | CAP_TILT;
}

int FlirPTTcpHead::getHeadStatus()
{
	if (pingTimer.elapsed() < 1500)
		return ST_NORMAL;
	return ST_ERROR;
}

int FlirPTTcpHead::panLeft(float speed)
{
	speed = qAbs(speed) * flirConfig.panSpeed;
	return sendCommand(ptzCommandList.at(C_PAN_LEFT).arg(speed));
}

int FlirPTTcpHead::panRight(float speed)
{
	speed = qAbs(speed) * flirConfig.panSpeed;
	return sendCommand(ptzCommandList.at(C_PAN_RIGHT).arg(speed));
}

int FlirPTTcpHead::tiltUp(float speed)
{
	speed = qAbs(speed) * flirConfig.tiltSpeed;
	return sendCommand(ptzCommandList.at(C_TILT_UP).arg(speed));
}

int FlirPTTcpHead::tiltDown(float speed)
{
	speed = qAbs(speed) * flirConfig.tiltSpeed;
	return sendCommand(ptzCommandList.at(C_TILT_DOWN).arg(speed));
}

int FlirPTTcpHead::panTiltAbs(float pan, float tilt)
{
	int pspeed = qAbs(pan) * flirConfig.panSpeed;
	int tspeed = qAbs(tilt) * flirConfig.tiltSpeed;
	if (pspeed == 0 && tspeed == 0) {
		sendCommand(ptzCommandList[C_STOP]);
		return 0;
	}
	if (pan < 0) {
		// left
		if (tilt < 0) {
			// up
			sendCommand(
				ptzCommandList.at(C_PAN_LEFT_TILT_UP)
					.arg(pspeed)
					.arg(tspeed));
		} else if (tilt > 0) {
			// down
			sendCommand(ptzCommandList.at(C_PAN_LEFT_TILT_DOWN)
							.arg(pspeed)
							.arg(tspeed));
		} else {
			sendCommand(ptzCommandList.at(C_PAN_LEFT).arg(pspeed));
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
			sendCommand(ptzCommandList.at(C_PAN_RIGHT).arg(pspeed));
		}
	}
	return 0;
}

int FlirPTTcpHead::panTiltGoPos(float ppos, float tpos)
{
	return sendCommand(ptzCommandList.at(C_PAN_TILT_GO_POS).arg(ppos).arg(tpos));
}

int FlirPTTcpHead::panTiltStop()
{
	return sendCommand(ptzCommandList.at(C_STOP));
}

float FlirPTTcpHead::getPanAngle()
{
	return flirConfig.panPos * 180.0 / flirConfig.maxPanPos;
}

float FlirPTTcpHead::getTiltAngle()
{
	return flirConfig.tiltPos * 90.0 / flirConfig.maxTiltPos / 3;
}

int FlirPTTcpHead::dataReady(const unsigned char *bytes, int len)
{
	const QString mes = QString::fromUtf8((const char *)bytes, len).split("\r\n").first();
	if (mes.contains("position")) {
		int value = parsePositions(mes);
		if (mes.contains("Pan speed"))
			flirConfig.panSpeed = value;
		if (mes.contains("Tilt speed"))
			flirConfig.tiltSpeed = value;
		if (mes.contains("Pan position"))
			flirConfig.panPos = value;
		if (mes.contains("Tilt position"))
			flirConfig.tiltPos = value;
		if (mes.contains("Minimum Pan"))
			flirConfig.minPanPos = value;
		if (mes.contains("Maximum Pan"))
			flirConfig.maxPanPos = value;
		if (mes.contains("Minimum Tilt"))
			flirConfig.minTiltPos = value;
		if (mes.contains("Maximum Tilt"))
			flirConfig.maxTiltPos = value;
		printFlirConfigs();
	}
	return len;
}

int FlirPTTcpHead::parsePositions(QString mes)
{
	QString is = mes.split("is").last();
	QStringList flds = is.split(" ");
	flds.removeAll(" ");
	flds.removeAll("");
	return flds.first().toInt();
}

void FlirPTTcpHead::printFlirConfigs()
{
	mInfo("%d, %d, %d, %d, %d, %d, %d, %d",
		flirConfig.maxPanPos,
		flirConfig.maxTiltPos,
		flirConfig.minPanPos,
		flirConfig.minTiltPos,
		flirConfig.panPos,
		flirConfig.panSpeed,
		flirConfig.tiltPos,
		flirConfig.tiltSpeed);
}

int FlirPTTcpHead::sendCommand(const QString &key)
{
	QString cmd = key;
	cmd.append("\r\n");
	return transport->send(cmd.toUtf8());
}

QByteArray FlirPTTcpHead::transportReady()
{
	QString cmd = ptzCommandList.at(C_GET_PT);
	cmd.append("\r\n");
	return cmd.toUtf8();
}
