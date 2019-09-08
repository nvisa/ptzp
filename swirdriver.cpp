#include "swirdriver.h"

#include "aryapthead.h"
#include "debug.h"
#include "errno.h"
#include "mgeoswirhead.h"
#include "ptzpserialtransport.h"

#include "QTimer"

SwirDriver::SwirDriver()
{
	headModule = new MgeoSwirHead;
	headDome = new AryaPTHead;
	headDome->setMaxSpeed(888889);
	state = INIT;
	defaultModuleHead = headModule;
	defaultPTHead = headDome;
}

PtzpHead *SwirDriver::getHead(int index)
{
	if (index == 0)
		return headModule;
	else if (index == 1)
		return headDome;
	return NULL;
}

int SwirDriver::setTarget(const QString &targetUri)
{
	QStringList fields = targetUri.split(";");
	tp1 = new PtzpSerialTransport(PtzpTransport::PROTO_STRING_DELIM);
	headModule->setTransport(tp1);
	headModule->enableSyncing(true);
	if (tp1->connectTo(fields[0]))
		return -EPERM;

	tp2 = new PtzpSerialTransport(PtzpTransport::PROTO_STRING_DELIM);
	headDome->setTransport(tp2);
	headDome->enableSyncing(true);
	if (tp2->connectTo(fields[1]))
		return -EPERM;
	return 0;
}

void SwirDriver::timeout()
{
	mLog("Driver state: %d", state);
	switch (state) {
	case INIT:
		headModule->syncRegisters();
		state = SYNC_HEAD_MODULE;
		break;
	case SYNC_HEAD_MODULE:
		if (headModule->getHeadStatus() == PtzpHead::ST_NORMAL) {
			if (registerSavingEnabled)
				state = LOAD_MODULE_REGISTERS;
			else
				state = NORMAL;
			//			tp1->enableQueueFreeCallbacks(true);
			tp2->enableQueueFreeCallbacks(true);
			timer->setInterval(1000);
		}
		break;
	case LOAD_MODULE_REGISTERS:
		headModule->loadRegisters("head0.json");
		state = NORMAL;
		break;
	case NORMAL:
		manageRegisterSaving();
		break;
	}

	PtzpDriver::timeout();
}
