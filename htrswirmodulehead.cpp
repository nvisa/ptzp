#include "htrswirmodulehead.h"

#include "debug.h"
#include "errno.h"
#include "ptzptransport.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <unistd.h>

enum Commands {
	C_SET_ZOOM_LEVEL,
	C_SET_FOCUS,
	C_SET_DGAIN,
	C_SET_AUTO_GAIN,

	C_GET_CIT,

	C_COUNT,
};

static QStringList commandList = {
	"$aux_cmd=tt:SetZoomLevel|%1",
	"$aux_cmd=tt:Focus|%1",
	"$aux_cmd=tt:IB|DGain|%1",
	"$aux_cmd=tt:IB|AG|%1",

	"$cit",
};

HtrSwirModuleHead::HtrSwirModuleHead()
{
	syncTimer.start();
	settings = {
		{"focus", {NULL, R_COUNT}},
		{"zoom_level", {C_SET_ZOOM_LEVEL, R_ZOOM_LEVEL}},
		{"digital_gain", {C_SET_DGAIN, R_DGAIN}},
		{"auto_gain", {C_SET_AUTO_GAIN, R_AUTO_GAIN}},
	};
}

void HtrSwirModuleHead::fillCapabilities(ptzp::PtzHead *head)
{
	head->add_capabilities(ptzp::PtzHead_Capability_ZOOM);
}

int HtrSwirModuleHead::getHeadStatus()
{
	return ST_NORMAL;
}

int HtrSwirModuleHead::focusIn(int speed)
{
	Q_UNUSED(speed);
	return sendCommand(commandList.at(C_SET_FOCUS).arg("+5"));
}

int HtrSwirModuleHead::focusOut(int speed)
{
	Q_UNUSED(speed);
	return sendCommand(commandList.at(C_SET_FOCUS).arg("-5"));
}

void HtrSwirModuleHead::setProperty(uint r, uint x)
{
	if(r == C_SET_ZOOM_LEVEL){
		sendCommand(commandList.at(r).arg(x));
		setRegister(R_ZOOM_LEVEL, x);
	} else if (r == C_SET_DGAIN){
		sendCommand(commandList.at(r).arg(x));
		setRegister(R_DGAIN, x);
	} else if (r == C_SET_AUTO_GAIN){
		sendCommand(commandList.at(r).arg(x ? "ON" : "OFF"));
		setRegister(R_AUTO_GAIN, x);
	}

}

uint HtrSwirModuleHead::getProperty(uint r)
{
	if(r == R_COUNT)
		return -1;
	return getRegister(r);
}

int HtrSwirModuleHead::sendCommand(const QString &key)
{
	QByteArray cmd = key.toStdString().c_str();
	return transport->send(cmd.data());
}

QJsonValue HtrSwirModuleHead::marshallAllRegisters()
{
	QJsonObject json;
	for (int i = 0; i < R_COUNT; i++)
		json.insert(QString("reg%1").arg(i), (int)getRegister(i));
	return json;
}

void HtrSwirModuleHead::unmarshallloadAllRegisters(const QJsonValue &node)
{
	QJsonObject root = node.toObject();
	QString key = "reg%1";

	int sleepDur = 1000 * 100;
	setProperty(C_SET_ZOOM_LEVEL, root.value(key.arg(R_ZOOM_LEVEL)).toInt());
	usleep(sleepDur);
	setProperty(C_SET_DGAIN, root.value(key.arg(R_DGAIN)).toInt());
	usleep(sleepDur);
	setProperty(C_SET_AUTO_GAIN, root.value(key.arg(R_AUTO_GAIN)).toInt());
	usleep(sleepDur);
}

QByteArray HtrSwirModuleHead::transportReady()
{
	if(syncTimer.elapsed() > 1000){
		syncTimer.restart();
		return commandList.at(C_GET_CIT).toUtf8();
	}
	return QByteArray();
}

int HtrSwirModuleHead::dataReady(const unsigned char *bytes, int len)
{
	const QString data = QString::fromUtf8((const char *)bytes, len);
	if(!data.startsWith("!<cit>"))
		return -ENOENT;
	if(data.contains("<cit>")){
		if(data.contains("True"))
			pingTimer.restart();
	}
	return len;
}
