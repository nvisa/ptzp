#include "oem4kdriver.h"

#include "oem4kmodulehead.h"
#include "debug.h"

#include <ptzptransport.h>
#include <QNetworkReply>

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
		break;
	}
	return;
}

