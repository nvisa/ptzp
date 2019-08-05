#include "flirdriver.h"

#include "flirmodulehead.h"
#include "flirpthead.h"

#include <ecl/ptzp/ptzptransport.h>
FlirDriver::FlirDriver()
{
	headDome = new FlirPTHead;
	headModule = new FlirModuleHead;
	defaultPTHead = headDome;
	defaultModuleHead = headModule;
	httpTransportDome = new PtzpHttpTransport(PtzpTransport::PROTO_BUFFERED);
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
	int ret = 0;
	if (!targetUri.contains(";"))
		return -ENODATA;
	QStringList fields = targetUri.split(";");
	headDome->setTransport(httpTransportDome);
	headModule->setTransport(httpTransportModule);

	httpTransportDome->setContentType(PtzpHttpTransport::AppXFormUrlencoded);
	ret = httpTransportDome->connectTo(fields[1]);
	if (ret)
		return ret;
	httpTransportDome->enableQueueFreeCallbacks(true);
	httpTransportDome->setTimerInterval(200);

	httpTransportModule->setContentType(PtzpHttpTransport::AppJson);
	ret = httpTransportModule->connectTo(fields[0]);
	if (ret)
		return ret;
	httpTransportModule->enableQueueFreeCallbacks(true);
	httpTransportModule->setTimerInterval(200);
	return 0;
}

void FlirDriver::timeout()
{
	PtzpDriver::timeout();
}
