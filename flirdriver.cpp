#include "flirdriver.h"

#include "flirpttcphead.h"
#include "flirmodulehead.h"
#include <ecl/ptzp/ptzptransport.h>

FlirDriver::FlirDriver()
{
	headDome = new FlirPTTcpHead;
	headModule = new FlirModuleHead;
	defaultPTHead = headDome;
	defaultModuleHead = headModule;

	transportDome = new PtzpTcpTransport(PtzpTransport::PROTO_STRING_DELIM);
	httpTransportModule = new PtzpHttpTransport(PtzpTransport::PROTO_BUFFERED);
}

FlirDriver::~FlirDriver()
{
}

PtzpHead *FlirDriver::getHead(int index)
{
	if (index == 0)
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
	headDome->setTransport(transportDome);
	headModule->setTransport(httpTransportModule);


	transportDome->connectTo(fields[1]);
	transportDome->enableQueueFreeCallbacks(true);
	transportDome->setTimerInterval(2000);
	transportDome->setDelimiter("*");

	httpTransportModule->setContentType(PtzpHttpTransport::AppJson);
	ret = httpTransportModule->connectTo(fields[0]);
	if (ret)
		return ret;
	httpTransportModule->enableQueueFreeCallbacks(true);
	httpTransportModule->setTimerInterval(2000);

	headDome->initialize();
	return 0;
}

QJsonObject FlirDriver::doExtraDeviceTests()
{
	QJsonObject obj;
	if (headModule->getLaserTimer() < 10000)
		return obj;
	obj.insert("type", QString("control_module"));
	obj.insert("index", 0);
	obj.insert("name", QString("laser"));
	obj.insert("elapsed_since_last_valid", headModule->getLaserTimer());
	return obj;
}

/*
 * [CR] [fo] flirmodulehead içerisinde modülün durumuna göre
 * işlem yapılan birkaç özellik var.Bu yüzden buraya sync state'i ve
 * modulehead içerisine sync işlemi eklenmeli.
 */
void FlirDriver::timeout()
{
	PtzpDriver::timeout();
}
