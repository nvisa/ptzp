#include "kayidriver.h"
#include "aryapthead.h"
#include "mgeofalconeyehead.h"
#include "ptzpserialtransport.h"
#include "debug.h"

#include <QFile>
#include <QTimer>
#include <errno.h>
#include <QTcpSocket>

KayiDriver::KayiDriver(QObject *parent) : PtzpDriver(parent)
{
	headModule = new MgeoFalconEyeHead;
	headDome = new AryaPTHead;
	state = INIT;
	defaultPTHead = headDome;
	defaultModuleHead = headModule;
}

PtzpHead *KayiDriver::getHead(int index)
{
	if(index == 0)
		return headModule;
	else if (index == 1)
		return headDome;
	return NULL;
}

int KayiDriver::setTarget(const QString &targetUri)
{
	QStringList fields = targetUri.split(";");
	tp1 = new PtzpSerialTransport();
	headModule->setTransport(tp1);
	headModule->enableSyncing(true);
	if (tp1->connectTo(fields[0]))
		return -EPERM;

	tp2 = new PtzpSerialTransport();
	headDome->setTransport(tp2);
	headDome->enableSyncing(true);
	if (tp2->connectTo(fields[1]))
		return -EPERM;
	return 0;
}

int KayiDriver::set(const QString &key, const QVariant &value)
{
	if (key == "abc"){
		headModule->setProperty(5,value.toUInt());
	}
	else if (key == "laser.up")
		headModule->setProperty(40,value.toUInt());
	return 0;
}

void KayiDriver::timeout()
{
	state = NORMAL;
		switch (state) {
		case INIT:
			headModule->syncRegisters();
			state = SYNC_HEAD_MODULE;
			break;
		case SYNC_HEAD_MODULE:
			if (headModule->getHeadStatus() == PtzpHead::ST_NORMAL) {
				state = SYNC_HEAD_DOME;
				tp1->enableQueueFreeCallbacks(true);
				headDome->syncRegisters();
			}
			break;
		case SYNC_HEAD_DOME:
			if (headDome->getHeadStatus() == PtzpHead::ST_NORMAL) {
				state = NORMAL;
				headModule->loadRegisters("oemmodule.json");
				tp2->enableQueueFreeCallbacks(true);
				timer->setInterval(1000);
			}
			break;
		case NORMAL:
			if(time->elapsed() >= 10000) {
				headModule->saveRegisters("oemmodule.json");
				time->restart();
			}
			break;
		}

		PtzpDriver::timeout();
}
