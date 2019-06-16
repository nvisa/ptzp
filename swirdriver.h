#ifndef SWIRDRIVER_H
#define SWIRDRIVER_H

#include <ecl/ptzp/ptzpdriver.h>

class MgeoSwirHead;
class AryaPTHead;
class PtzpSerialTransport;

class SwirDriver : public PtzpDriver
{
	Q_OBJECT
public:
	SwirDriver();
	PtzpHead * getHead(int index);
	int setTarget(const QString &targetUri);

protected slots:
	void timeout();

protected:
	enum DriverState {
		INIT,
		SYNC_HEAD_MODULE,
		NORMAL,
	};

	MgeoSwirHead *headModule;
	AryaPTHead *headDome;
	DriverState state;
	PtzpTransport *tp1;
	PtzpTransport *tp2;
};

#endif // SWIRDRIVER_H
