#include "virtualptzpdriver.h"
#include "ptzp/ptzphead.h"

#include <QDebug>
#include <QElapsedTimer>

class LifeTimeHead : public PtzpHead
{
public:
	LifeTimeHead() : PtzpHead()
	{
		lifetc = 0;
		lifet.start();
	}

	virtual QJsonValue marshallAllRegisters()
	{
		lifetc += lifet.elapsed() / 1000;
		lifet.restart();
		QJsonObject obj;
		obj.insert("lifet", lifetc);
		return obj;
	}

	virtual void unmarshallloadAllRegisters(const QJsonValue &node)
	{
		QJsonObject obj = node.toObject();
		lifetc = obj["lifet"].toInt();
		lifet.start();
	}

	int getLifetime() { return lifetc + lifet.elapsed() / 1000; }

	QElapsedTimer lifet;
	int lifetc;
};

class VirtualPtzpPTHead : public LifeTimeHead
{
public:
	VirtualPtzpPTHead() : LifeTimeHead() {}

	int getCapabilities() { return PtzpHead::CAP_TILT | PtzpHead::CAP_PAN; }
};

class VirtualPtzpZoomHead : public LifeTimeHead
{
public:
	VirtualPtzpZoomHead() : LifeTimeHead() {}

	int getCapabilities() { return PtzpHead::CAP_ZOOM; }
};

VirtualPtzpDriver::VirtualPtzpDriver(QObject *parent) : PtzpDriver(parent)
{
	head0 = new VirtualPtzpPTHead;
	head1 = new VirtualPtzpPTHead;
}

int VirtualPtzpDriver::setTarget(const QString &targetUri)
{
	Q_UNUSED(targetUri);
	if (registerSavingEnabled) {
		head0->loadRegisters("head0.json");
		head1->loadRegisters("head1.json");
	}
	return 0;
}

PtzpHead *VirtualPtzpDriver::getHead(int index)
{
	if (index == 0)
		return head0;
	if (index == 1)
		return head1;
	return nullptr;
}

void VirtualPtzpDriver::timeout()
{
	manageRegisterSaving();
	PtzpDriver::timeout();
}
