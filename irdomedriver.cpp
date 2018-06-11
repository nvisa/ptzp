#include "irdomedriver.h"
#include "mgeothermalhead.h"
#include "ptzptcptransport.h"
#include "oemmodulehead.h"
#include "ptzpserialtransport.h"
#include "irdomepthead.h"

#include <QTimer>

IRDomeDriver::IRDomeDriver(QObject *parent)
	: PtzpDriver(parent)
{
	headModule = new OemModuleHead;
	headDome = new IRDomePTHead;
	transport = new PtzpSerialTransport;
	state = INIT;
}

PtzpHead *IRDomeDriver::getHead(int index)
{
	if (index == 0)
		return headModule;
	return headDome;
}

int IRDomeDriver::setTarget(const QString &targetUri)
{
	headModule->setTransport(transport);
	headDome->setTransport(transport);
	int err = transport->connectTo(targetUri);
	if (err)
		return err;
	return 0;
}

void IRDomeDriver::timeout()
{
	switch (state) {
	case INIT:
		headModule->syncRegisters();
		state = SYNC_HEAD_MODULE;
		break;
	case SYNC_HEAD_MODULE:
		if (headModule->getHeadStatus() == PtzpHead::ST_NORMAL) {
			state = SYNC_HEAD_DOME;
			headDome->syncRegisters();
		}
		break;
	case SYNC_HEAD_DOME:
		if (headDome->getHeadStatus() == PtzpHead::ST_NORMAL) {
			state = NORMAL;
			transport->enableQueueFreeCallbacks(true);
			timer->setInterval(1000);
		}
		break;
	case NORMAL:

		break;
	}

	PtzpDriver::timeout();
}

