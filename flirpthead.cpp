#include "flirpthead.h"
#include "debug.h"

#include <assert.h>

#include <ecl/ptzp/ptzphttptransport.h>

enum CommandList {
	C_STOP,
	C_GET_PT,
	C_PAN_LEFT,
	C_PAN_RIGHT,
	C_TILT_UP,
	C_TILT_DOWN,
	C_PT_HOME,
	C_PAN_WITH_OFFSET,
	C_TILT_WITH_OFFSET,
	C_PANTILT_WITH_OFFSET,
	C_PAN_SET,
	C_TILT_SET,
	C_PANTILT_SET,
	C_PAN_LEFT_TILT_UP,
	C_PAN_LEFT_TILT_DOWN,
	C_PAN_RIGHT_TILT_UP,
	C_PAN_RIGHT_TILT_DOWN,
	C_PTZ_CONFIG,

	C_COUNT,
};

static QStringList createPTZCommandList()
{
	QStringList cmdListFlir;
	cmdListFlir << QString("H");
	cmdListFlir << QString("PP&TP&PD&TD&C");
	cmdListFlir << QString("C=V&PS=-%1");
	cmdListFlir << QString("C=V&PS=%1");
	cmdListFlir << QString("C=V&TS=%1");
	cmdListFlir << QString("C=V&TS=-%1");
	cmdListFlir << QString("C=I&PP=0&TP=0&PS=2000&TS=2000");
	cmdListFlir << QString("C=I&PS=%1&PO=%2");
	cmdListFlir << QString("C=I&TS=%1&TO=%2");
	cmdListFlir << QString("C=I&PS=%1&TS=%2&PO=%3&TO=%4");
	cmdListFlir << QString("C=I&PP=%1&PS=%2");
	cmdListFlir << QString("C=I&TP=%1&TS=%2");
	cmdListFlir << QString("C=I&PP=%1&TP=%2&PS=%3&TS=%4");
	cmdListFlir << QString("C=V&PS=%1&TS=%2");
	cmdListFlir << QString("C=V&PS=%1&TS=%2");
	cmdListFlir << QString("C=V&PS=%1&TS=%2");
	cmdListFlir << QString("C=V&PS=%1&TS=%2");
	cmdListFlir << QString("V&VM&VS&PR&TR&PS&TS&PA&TA&PB&TB&PU&PL&TU&TL&PM&TM&"
						   "PH&TH&WP&WT&L&PXU&TXU&PNU&TNU&PXF&PNF&TXF&TNF&PC&"
						   "QP&QA&ATHB&CT");

	return cmdListFlir;
}

FlirPTHead::FlirPTHead() : PtzpHead()
{
	setHeadName("pt_head");
	ptzCommandList = createPTZCommandList();
	assert(ptzCommandList.size() == C_COUNT);

	timer = new QTimer(this);
	timer->start(100);
	connect(timer, SIGNAL(timeout()), this, SLOT(sendCommand()));

	setupFlirConfigs();
}

void FlirPTHead::setupFlirConfigs()
{
	flir.maxPanPos = flirConfig.contains("PXU") ? flirConfig.value("PXU").toFloat() : 28000.0;
	flir.minPanPos = flirConfig.contains("PNU") ? flirConfig.value("PNU").toFloat() : -27999.0;
	flir.minTiltPos = flirConfig.contains("TNU") ? flirConfig.value("TNU").toFloat() : 27999.0;
	flir.maxTiltPos = flirConfig.contains("TXU") ? flirConfig.value("TXU").toFloat() : 9330.0;
	flir.panUpperSpeed = flirConfig.contains("PU") ? flirConfig.value("PU").toFloat() : 650;
	flir.tiltUpperSpeed = flirConfig.contains("TU") ? flirConfig.value("TU").toFloat() : 650;
}

void FlirPTHead::fillCapabilities(ptzp::PtzHead *head)
{
	head->add_capabilities(ptzp::PtzHead_Capability_PAN);
	head->add_capabilities(ptzp::PtzHead_Capability_TILT);
}

int FlirPTHead::getHeadStatus()
{
	if (pingTimer.elapsed() < 1500)
		return ST_NORMAL;
	return ST_ERROR;
}

int FlirPTHead::panLeft(float speed)
{
	speed = qAbs(speed) * flir.panUpperSpeed;
	return saveCommand(ptzCommandList.at(C_PAN_LEFT).arg(speed));
}

int FlirPTHead::panRight(float speed)
{
	speed = qAbs(speed) * flir.panUpperSpeed;
	return saveCommand(ptzCommandList.at(C_PAN_RIGHT).arg(speed));
}

int FlirPTHead::tiltUp(float speed)
{
	speed = qAbs(speed) * flir.tiltUpperSpeed;
	return saveCommand(ptzCommandList.at(C_TILT_UP).arg(speed));
}

int FlirPTHead::tiltDown(float speed)
{
	speed = qAbs(speed) * flir.tiltUpperSpeed;
	return saveCommand(ptzCommandList.at(C_TILT_DOWN).arg(speed));
}

int FlirPTHead::panTiltAbs(float pan, float tilt)
{
	int pspeed = qAbs(pan) * flir.panUpperSpeed;
	int tspeed = qAbs(tilt) * flir.tiltUpperSpeed;
	if (pspeed == 0 && tspeed == 0) {
		saveCommand(ptzCommandList[C_STOP]);
		return 0;
	}
	if (pan < 0) {
		// left
		if (tilt < 0) {
			// up
			saveCommand(
				ptzCommandList.at(C_PAN_LEFT_TILT_UP)
					.arg(-pspeed)
					.arg(
						tspeed)); // C_PAN_LEFT_TILT_UP).arg(pspeed).arg(tspeed));
		} else if (tilt > 0) {
			// down
			saveCommand(ptzCommandList.at(C_PAN_LEFT_TILT_DOWN)
							.arg(-pspeed)
							.arg(-tspeed));
		} else {
			saveCommand(ptzCommandList.at(C_PAN_LEFT).arg(-pspeed));
		}
	} else {
		// right
		if (tilt < 0) {
			// up
			saveCommand(
				ptzCommandList.at(C_PAN_RIGHT_TILT_UP).arg(pspeed).arg(tspeed));
		} else if (tilt > 0) {
			// down
			saveCommand(ptzCommandList.at(C_PAN_RIGHT_TILT_DOWN)
							.arg(pspeed)
							.arg(-tspeed));
		} else {
			saveCommand(ptzCommandList.at(C_PAN_RIGHT).arg(pspeed));
		}
	}
	return 0;
}

/*
 * [CR] [fo] Pt modülene özgü komutları setSettings() içerisine taşıyabiliriz.
 */
int FlirPTHead::home()
{
	return saveCommand(ptzCommandList.at(C_PT_HOME));
}

int FlirPTHead::panWOffset(int speed, int offset)
{
	speed = qAbs(speed) * flir.panUpperSpeed;
	return saveCommand(
		ptzCommandList.at(C_PAN_WITH_OFFSET).arg(speed).arg(offset));
}

int FlirPTHead::tiltWOffset(int speed, int offset)
{
	speed = qAbs(speed) * flir.tiltUpperSpeed;
	return saveCommand(
		ptzCommandList.at(C_TILT_WITH_OFFSET).arg(speed).arg(offset));
}

int FlirPTHead::pantiltWOffset(int pSpeed, int pOffset, int tSpeed, int tOffset)
{
	tSpeed = tSpeed * flir.tiltUpperSpeed;
	pSpeed = pSpeed * flir.panUpperSpeed;
	return saveCommand(ptzCommandList.at(C_PANTILT_WITH_OFFSET)
						   .arg(pSpeed)
						   .arg(tSpeed)
						   .arg(pOffset)
						   .arg(tOffset));
}

int FlirPTHead::panSet(int pDeg, int pSpeed)
{
	int pPoint;
	if (pDeg <= 180)
		pPoint = (int)((pDeg / 180) * flir.maxPanPos);
	else
		pPoint = (int)((1 - ((pDeg - 180) / 180)) * flir.minPanPos);

	pSpeed = pSpeed * flir.panUpperSpeed;
	return saveCommand(ptzCommandList.at(C_PAN_SET).arg(pPoint).arg(pSpeed));
}

int FlirPTHead::tiltSet(int tDeg, int tSpeed)
{
	int tPoint;
	if (tDeg <= 180)
		tPoint = (tDeg / 180) * flir.maxTiltPos;
	else
		tPoint = (1 - ((tDeg - 180) / 180)) * flir.minTiltPos;
	tSpeed = tSpeed * flir.tiltUpperSpeed;
	return saveCommand(ptzCommandList.at(C_TILT_SET).arg(tPoint).arg(tSpeed));
}

int FlirPTHead::pantiltSet(float pDeg, float tDeg, int pSpeed, int tSpeed)
{
	int pPoint, tPoint;

	if (pDeg <= 180)
		pPoint = (int)((pDeg / 180) * flir.maxPanPos);
	else
		pPoint = (int)((1 - ((pDeg - 180) / 180)) * flir.minPanPos);

	if (tDeg <= 180)
		tPoint = (int)((tDeg / 180) * flir.maxTiltPos);
	else
		tPoint = (int)((1 - ((tDeg - 180) / 180)) * flir.minTiltPos);

	pSpeed = qAbs(pSpeed) * flir.panUpperSpeed;
	tSpeed = qAbs(tSpeed) * flir.tiltUpperSpeed;
	return saveCommand(ptzCommandList.at(C_PANTILT_SET)
						   .arg(pPoint)
						   .arg(tPoint)
						   .arg(pSpeed)
						   .arg(tSpeed));
}

int FlirPTHead::panTiltGoPos(float ppos, float tpos)
{
	return pantiltSet(ppos, tpos, 1, 1);
}

int FlirPTHead::panTiltStop()
{
	return saveCommand(ptzCommandList.at(C_STOP));
}

float FlirPTHead::getPanAngle()
{
	return panPos * 180.0 / flir.maxPanPos;
}

float FlirPTHead::getTiltAngle()
{
	return tiltPos * 90.0 / flir.maxTiltPos / 3;
}

int FlirPTHead::getFlirConfig()
{
	return saveCommand(ptzCommandList.at(C_PTZ_CONFIG));
}

int FlirPTHead::dataReady(const unsigned char *bytes, int len)
{
	pingTimer.restart();
	const QString mes = QString::fromUtf8((const char *)bytes, len);
	mLog("coming message %s, %d", qPrintable(mes), mes.size());
	if (mes.contains("VS")) {
		QStringList flds = mes.split(",");
		foreach (QString fld, flds) {
			fld = fld.remove(" ").remove("\"").remove("{");
			if (fld.isEmpty() || fld.contains("status"))
				continue;
			QString key = fld.split(":").first();
			QString value = fld.split(":").last();
			flirConfig.insert(key, value);
			mInfo("'%s': '%s'", qPrintable(key), qPrintable(value));
		}
		setupFlirConfigs();
		return len;
	}
	if (!mes.contains("PD") || !mes.startsWith("{") || !mes.endsWith("}"))
		return -ENOBUFS;

	QStringList flds = mes.split(",");
	foreach (QString fld, flds) {
		int value = fld.split(":").last().remove(" ").remove("\"").toInt();
		if (fld.contains("PP"))
			panPos = value;
		if (fld.contains("TP"))
			tiltPos = value;
	}
	mInfo("Real position values pan: %d tilt: %d", panPos, tiltPos);
	return len;
}

int FlirPTHead::saveCommand(const QString &key)
{
	if (key == ptzCommandList.at(C_STOP)) {
		cmdList.clear();
	}
	cmdList << key;
	mLog("Command in queue %d '%s'", cmdList.size(), qPrintable(key));
	return 0;
}

void FlirPTHead::sendCommand()
{
	if (cmdList.isEmpty())
		return;
	QString key = cmdList.first();
	transport->send(key.toUtf8());
	cmdList.pop_front();
	return;
}

QByteArray FlirPTHead::transportReady()
{
	return ptzCommandList.at(C_GET_PT).toUtf8();
}
