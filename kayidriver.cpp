#include "kayidriver.h"
#include "aryapthead.h"
#include "mgeofalconeyehead.h"
#include "ptzpserialtransport.h"
#include "debug.h"

#include <QFile>
#include <QTimer>
#include <errno.h>
#include <QTcpSocket>

KayiDriver::KayiDriver(QList<int> relayConfig, bool gps, QObject *parent) : PtzpDriver(parent)
{
	headModule = new MgeoFalconEyeHead(relayConfig, gps);
	headDome = new AryaPTHead;
	headDome->setMaxSpeed(888889);
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

#define zoomEntry(zoomv, h) \
	zooms.push_back(zoomv); \
	hmap.push_back(h); \
	vmap.push_back(h * (float)4 / 3)

int KayiDriver::setTarget(const QString &targetUri)
{
	QStringList fields = targetUri.split(";");
	tp1 = new PtzpSerialTransport();
	headModule->setTransport(tp1);
	headModule->enableSyncing(true);
	if (tp1->connectTo(fields[0]))
		return -EPERM;

	tp2 = new PtzpSerialTransport(PtzpTransport::PROTO_STRING_DELIM);
	headDome->setTransport(tp2);
	headDome->enableSyncing(true);
	if (tp2->connectTo(fields[1]))
		return -EPERM;

	if (1) {
		std::vector<float> hmap, vmap;
		std::vector<int> zooms;
		zoomEntry(9000, 11);
		zoomEntry(9500, 10);
		zoomEntry(10000, 9);
		zoomEntry(10500, 8);
		zoomEntry(11000, 7);
		zoomEntry(11500, 6);
		zoomEntry(12300, 5);
		zoomEntry(13200, 4);
		zoomEntry(14100, 3);
		zoomEntry(15000, 2);
		headModule->getRangeMapper()->setLookUpValues(zooms);
		headModule->getRangeMapper()->addMap(hmap);
		headModule->getRangeMapper()->addMap(vmap);
	}

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

bool KayiDriver::isReady()
{
	if (state == NORMAL)
		return true;
	return false;
}

void KayiDriver::timeout()
{
	mLog("Driver state: %d", state);
	switch (state) {
	case INIT:
		headModule->setProperty(37, 2);
		headModule->setProperty(5, 1);
		headModule->setProperty(37, 1);
		state = WAIT_ALIVE;
		break;
	case WAIT_ALIVE:
		if (headModule->isAlive() == true){
			headModule->syncRegisters();
			tp2->enableQueueFreeCallbacks(true);
			state = SYNC_HEAD_MODULE;
		}
		break;
	case SYNC_HEAD_MODULE:
		if (headModule->getHeadStatus() == PtzpHead::ST_NORMAL) {
			state = NORMAL;
//			tp1->enableQueueFreeCallbacks(true);
			timer->setInterval(1000);
		}
		break;
	case NORMAL:
		break;
	}

	PtzpDriver::timeout();
}
