#include "flirdriver.h"

#include "flirmodulehead.h"
#include "flirpthead.h"

#include <ecl/ptzp/ptzptransport.h>
FlirDriver::FlirDriver()
{
	headModule = new FlirModuleHead;
	headDome = new FlirPTHead;
	defaultPTHead = headDome;
	defaultModuleHead = headModule;
	httpTransport = new PtzpHttpTransport(PtzpTransport::PROTO_BUFFERED);
	httpTransportModule = new PtzpHttpTransport(PtzpTransport::PROTO_BUFFERED);
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

	headDome->setTransport(httpTransport);
	headModule->setTransport(httpTransportModule);

	httpTransport->connectTo(fields[1]);
	httpTransport->enableQueueFreeCallbacks(true);

	httpTransportModule->connectTo(fields[0]);
	httpTransportModule->setCGIFlag(true);
	httpTransportModule->enableQueueFreeCallbacks(true);
	return 0;
}

void FlirDriver::timeout()
{
	PtzpDriver::timeout();
}
