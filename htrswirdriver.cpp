#include "htrswirdriver.h"
#include "htrswirpthead.h"
#include "htrswirmodulehead.h"
#include "debug.h"
#include "errno.h"
#include "ptzptcptransport.h"

#include "QTimer"

HtrSwirDriver::HtrSwirDriver()
{
	registerSavingEnabled = true;
	headModule = new HtrSwirModuleHead;
	headDome = new HtrSwirPtHead;
	state = INIT;
	tp = new PtzpTcpTransport(PtzpTransport::PROTO_BUFFERED);
	defaultPTHead = headDome;
	defaultModuleHead = headModule;
}

PtzpHead *HtrSwirDriver::getHead(int index)
{
	if (index == 0)
		return headDome;
	else if (index == 1)
		return headModule;
	return NULL;
}

int HtrSwirDriver::setTarget(const QString &targetUri)
{
	headDome->setTransport(tp);
	headModule->setTransport(tp);
	int err = tp->connectTo(targetUri);
	return err;
}

void HtrSwirDriver::timeout()
{
	switch (state) {
	case INIT:
		headModule->syncRegisters();
		state = HEAD_MODULE;
		break;
	case HEAD_MODULE:
		if (registerSavingEnabled)
			state = LOAD_MODULE_REGISTERS;
		else
			state = NORMAL;
		break;
	case LOAD_MODULE_REGISTERS:
		headModule->loadRegisters("head1.json");
		state = NORMAL;
		break;
	case NORMAL:
		manageRegisterSaving();
		break;
	}
	PtzpDriver::timeout();
}
