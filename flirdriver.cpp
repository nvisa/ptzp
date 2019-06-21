#include "flirdriver.h"

#include "flirmodulehead.h"
#include "flirpthead.h"

#include <QFile>

FlirDriver::FlirDriver()
{
	headModule = new FlirModuleHead;
	headDome = new FlirPTHead;
	defaultPTHead = headDome;
	defaultModuleHead = headModule;
	configLoad(QJsonObject());
}

FlirDriver::~FlirDriver()
{

}

PtzpHead *FlirDriver::getHead(int index)
{
	if(index == 0)
		return headModule;
	else if (index == 1)
		return headDome;
	return NULL;
}

int FlirDriver::setTarget(const QString &targetUri)
{
	QStringList fields = targetUri.split(";");
	headModule->connect(fields[0]);
	headDome->connectHTTP(fields[1]);
	return 0;
}

void FlirDriver::configLoad(const QJsonObject &obj)
{
	Q_UNUSED(obj);
	QJsonObject o;
	o.insert("model",QString("Flir"));
	o.insert("type" , QString("moving"));
	o.insert("pan_tilt_support", 1);
	o.insert("cam_module", QString("2MP-HIK"));
	return PtzpDriver::configLoad(o);
}

void FlirDriver::timeout()
{
	PtzpDriver::timeout();
}
