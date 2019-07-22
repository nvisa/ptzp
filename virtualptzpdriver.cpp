#include "virtualptzpdriver.h"
#include "ptzp/ptzphead.h"

class VirtualPtzpPTHead : public PtzpHead
{
public:
	VirtualPtzpPTHead()
	{

	}

	int getCapabilities()
	{
		return PtzpHead::CAP_TILT | PtzpHead::CAP_PAN;
	}
};

class VirtualPtzpZoomHead : public PtzpHead
{
public:
	VirtualPtzpZoomHead()
	{

	}

	int getCapabilities()
	{
		return PtzpHead::CAP_ZOOM;
	}
};

VirtualPtzpDriver::VirtualPtzpDriver(QObject *parent)
	: PtzpDriver(parent)
{
	head0 = new VirtualPtzpPTHead;
	head1 = new VirtualPtzpPTHead;
}

int VirtualPtzpDriver::setTarget(const QString &targetUri)
{
	Q_UNUSED(targetUri);
	return 0;
}

PtzpHead *VirtualPtzpDriver::getHead(int index)
{
	if (index)
		return head1;
	return head0;
}
