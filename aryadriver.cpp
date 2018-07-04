#include "aryadriver.h"
#include "mgeothermalhead.h"
#include "ptzptcptransport.h"
#include "aryapthead.h"
#include "debug.h"

AryaDriver::AryaDriver(QObject *parent)
	: PtzpDriver(parent)
{
	aryapt = new AryaPTHead;
	thermal = new MgeoThermalHead;
	tcp1 = new PtzpTcpTransport(PtzpTransport::PROTO_STRING_DELIM);
	tcp2 = new PtzpTcpTransport(PtzpTransport::PROTO_BUFFERED);
	state = INIT;
}

int AryaDriver::setTarget(const QString &targetUri)
{
	aryapt->setTransport(tcp1);
	thermal->setTransport(tcp2);
	int err = tcp1->connectTo(QString("%1:4001").arg(targetUri));
	if (err)
		return err;
	err = tcp2->connectTo(QString("%1:4002").arg(targetUri));
	if (err)
		return err;
	return 0;
}

PtzpHead *AryaDriver::getHead(int index)
{
	if (index == 0)
		return aryapt;
	return thermal;
}

void AryaDriver::timeout()
{
	switch (state) {
	case INIT:
		state = SYNC_THERMAL_MODULE;
		thermal->syncRegisters();
		break;
	case SYNC_THERMAL_MODULE:
		if (thermal->getHeadStatus() == PtzpHead::ST_NORMAL) {
			state = NORMAL;
		}
		break;
	case NORMAL:
		//ffDebug() << "running";
		static int once = 1;
		if (!once) {
			once = 1;
			//aryapt->panTiltStop();
			thermal->startZoomIn(1);
		}
		break;
	}

	PtzpDriver::timeout();
}

