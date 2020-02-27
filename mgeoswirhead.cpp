#include "mgeoswirhead.h"

#include "debug.h"
#include "errno.h"
#include "ptzptransport.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <unistd.h>

static QString createSwirCommand(const QString &data)
{
	QByteArray ba = data.toUtf8();
	uint ch = 0;
	for (int i = 1; i < ba.size(); i++) {
		ch += data.at(i).unicode();
	}
	ch = (0x100 - (ch & 0xFF)) & 0xFF;
	return QString("%1~%2>").arg(data).arg(
		QString::number((uint)ch, 16).toUpper());
	;
}

enum Commands {
	C_SET_ZOOM,
	C_SET_FOCUS,
	C_SET_GAIN,
	C_START_IBIT,
	C_SET_SYMBOLOGY,

	C_GET_ZOOM_POS,
	C_GET_FOCUS_POS,
	C_GET_GAIN,
	C_GET_IBIT_RESULT,
	C_GET_SYMBOLOGY,

	C_COUNT,
};
static QStringList commandList = {
	"<0001#SET,ZOOM:IR~%1",	   "<00015#SET,FOCUS:IR~%1", "<0003#SET,GAIN:IR~%1",
	"<0004#GET,BIT:ALL",	   "<0005#SET,SYMBOLOGY:%1",

	"<0005#GET,ZMENC:IR",	   "<0006#GET,FCSENC:IR",	 "<0007#GET,GAIN:IR",
	"<0008#GET,BITRESULT:ALL", "<0011#GET,SYMBOLOGY:",
};

MgeoSwirHead::MgeoSwirHead()
{
	syncTimer.start();
	settings = {
		{"focus", {NULL, NULL}},
		{"gain", {C_SET_GAIN, R_GAIN}},
		{"ibit", {C_START_IBIT, C_GET_IBIT_RESULT}},
		{"symbology", {C_SET_SYMBOLOGY, R_SYMBOLOGY}},
	};
}

void MgeoSwirHead::fillCapabilities(ptzp::PtzHead *head)
{
	head->add_capabilities(ptzp::PtzHead_Capability_ZOOM);
}

int MgeoSwirHead::syncRegisters()
{
	if (!transport)
		return -ENOENT;
	nextSync = C_GET_ZOOM_POS;
	return syncNext();
}

int MgeoSwirHead::startZoomIn(int speed)
{
	Q_UNUSED(speed);
	sendCommand(commandList.at(C_SET_ZOOM).arg("IN"));
	return 0;
}

int MgeoSwirHead::startZoomOut(int speed)
{
	Q_UNUSED(speed);
	sendCommand(commandList.at(C_SET_ZOOM).arg("OUT"));
	return 0;
}

int MgeoSwirHead::stopZoom()
{
	sendCommand(commandList.at(C_SET_ZOOM).arg("STOP"));
	return 0;
}

int MgeoSwirHead::getZoom()
{
	return getRegister(R_ZOOM_POS);
}

int MgeoSwirHead::focusIn(int speed)
{
	Q_UNUSED(speed);
	return sendCommand(commandList.at(C_SET_FOCUS).arg("FAR"));
}

int MgeoSwirHead::focusOut(int speed)
{
	Q_UNUSED(speed);
	return sendCommand(commandList.at(C_SET_FOCUS).arg("NEAR"));
}

int MgeoSwirHead::focusStop()
{
	sendCommand(commandList.at(C_SET_FOCUS).arg("STOP"));
}

int MgeoSwirHead::getHeadStatus()
{
	if (nextSync == C_COUNT)
		return ST_NORMAL;
	return ST_SYNCING;
}

void MgeoSwirHead::setProperty(uint r, uint x)
{
	if (r == C_SET_GAIN) {
		if (x == 0) // light
			sendCommand(commandList.at(r).arg("LIGHT"));
		else if (x == 1) // normal
			sendCommand(commandList.at(r).arg("NORMAL"));
		else if (x == 2) // dark
			sendCommand(commandList.at(r).arg("DARK"));
		setRegister(R_GAIN, x);
	} else if (r == C_START_IBIT)
		sendCommand(commandList.at(r));
	else if (r == C_GET_IBIT_RESULT)
		sendCommand(commandList.at(r));
	else if (r == C_SET_SYMBOLOGY) {
		if (x == 0) // off
			sendCommand(commandList.at(r).arg("OFF"));
		else if (x == 1) // on
			sendCommand(commandList.at(r).arg("ON"));
	}
}

uint MgeoSwirHead::getProperty(uint r)
{
	return getRegister(r);
}

int MgeoSwirHead::syncNext()
{
	return sendCommand(commandList.at(nextSync));
}

QByteArray MgeoSwirHead::transportReady()
{
	if(syncTimer.elapsed() > 700){
		syncTimer.restart();
		return createSwirCommand(commandList.at(C_GET_ZOOM_POS)).toLatin1();
	}
	return QByteArray();
}

QJsonValue MgeoSwirHead::marshallAllRegisters()
{
	QJsonObject json;
	for (int i = 0; i < R_COUNT; i++)
		json.insert(QString("reg%1").arg(i), (int)getRegister(i));
	return json;
}

void MgeoSwirHead::unmarshallloadAllRegisters(const QJsonValue &node)
{
	QJsonObject root = node.toObject();
	QString key = "reg%1";
	/*
	 * according to our command table visca set commands doesn't get any
	 * response from module, in this case ordinary sleep should work here
	 */
	int sleepDur = 1000 * 100;
	setProperty(C_SET_SYMBOLOGY, root.value(key.arg(R_SYMBOLOGY)).toInt());
	usleep(sleepDur);
	setProperty(C_SET_GAIN, root.value(key.arg(R_GAIN)).toInt());
	usleep(sleepDur);
}

int MgeoSwirHead::dataReady(const unsigned char *bytes, int len)
{

	if (nextSync != C_COUNT) {
		/* we are in sync mode, let's sync next */
		if (++nextSync == C_COUNT) {
			ffDebug()
				<< "Swir register syncing completed, activating auto-sync";
		} else
			syncNext();
	}

	const QString data = QString::fromUtf8((const char *)bytes, len);
	if (data.contains("RET,ZMENC")){
		setRegister(R_ZOOM_POS, data.split("~").at(1).toInt());
		pingTimer.restart();
	}
	if (data.contains("RET,FCSENC"))
		setRegister(R_FOCUS_POS, data.split("~").at(1).toInt());
	if (data.contains("RET,GAIN")) {
		if (data.split("~").at(1) == "LIGHT")
			setRegister(R_GAIN, 0);
		if (data.split("~").at(1) == "NORMAL")
			setRegister(R_GAIN, 1);
		if (data.split("~").at(1) == "DARK")
			setRegister(R_GAIN, 2);
	}
	if (data.contains("RET,BITRESULT")) {
		QString res = data.split(":").at(1);
		setRegister(R_IBIT_RESULT, res.split("~").first().toInt());
	}
	if (data.contains("RET,SYMBOLOGY")) {
		QString res = data.split(":").at(1);
		if (res.split("~").first() == "OFF")
			setRegister(R_SYMBOLOGY, 0);
		if (res.split("~").first() == "ON")
			setRegister(R_SYMBOLOGY, 1);
	}
	return data.length();
}

int MgeoSwirHead::sendCommand(const QString &key)
{
	QString data = key;
	if (!data.contains(">"))
		data = createSwirCommand(key);
	mInfo("Sending command %s", qPrintable(data));
	QByteArray cmd = data.toLatin1();
	return transport->send(cmd.data());
}
