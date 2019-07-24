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

void FlirDriver::timeout()
{
	PtzpDriver::timeout();
}
