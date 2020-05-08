#include "oem4kdriver.h"
#include "oem4kmodulehead.h"
#include <ptzptransport.h>
#include <QNetworkReply>
#include "debug.h"

Oem4kDriver::Oem4kDriver()
{
	headModule = new Oem4kModuleHead;
	defaultPTHead = NULL;
	defaultModuleHead = headModule;

	httpTransportModule = new PtzpHttpTransport(PtzpTransport::PROTO_BUFFERED, this, true);
	state = INIT;
}

Oem4kDriver::~Oem4kDriver()
{

}

PtzpHead *Oem4kDriver::getHead(int index)
{
	if (index == 0)
		return headModule;
	return NULL;
}

int Oem4kDriver::setTarget(const QString &targetUri)
{
	int ret = 0;
	QStringList fields = targetUri.split(";");
	headModule->setTransport(httpTransportModule);
	httpTransportModule->setContentType(PtzpHttpTransport::AppJson);
	httpTransportModule->setAuthorizationType(PtzpHttpTransport::Digest);
	ret = httpTransportModule->connectTo(fields[0]);
	if (ret)
		return ret;
	httpTransportModule->enableQueueFreeCallbacks(true);
	httpTransportModule->setTimerInterval(2000);

	gpioPin = gpiocont->getGpioNo("LdrControl");
	gpioValue = -1;

	createHeadMaps();

	return 0;
}

void Oem4kDriver::timeout()
{
	switch (state) {
	case INIT:
		headModule->syncRegisters();
		state = SYNC;
	case SYNC:
		if (headModule->getHeadStatus() == PtzpHead::ST_SYNCING)
			break;
		state = NORMAL;
		break;
	case NORMAL:
		if(gpioValue != gpiocont->getGpioValue(gpioPin)) {
			gpioValue = gpiocont->getGpioValue(gpioPin);
			if(gpioValue)
				headModule->setProperty(12, 2);
			else
				headModule->setProperty(12, 1);
			break;
		}
	}
	return;
}

