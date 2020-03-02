#ifndef HTRSWIRDRIVER_H
#define HTRSWIRDRIVER_H

#include <ecl/ptzp/ptzpdriver.h>
#include <ecl/ptzp/ptzptcptransport.h>

class HtrSwirPtHead;
class HtrSwirModuleHead;

class HtrSwirDriver: public PtzpDriver
{
	Q_OBJECT
public:
	HtrSwirDriver();

	PtzpHead *getHead(int index);
	int setTarget(const QString &targetUri);

protected slots:
	void timeout();

protected:
	enum DriverState {
		INIT,
		HEAD_MODULE,
		LOAD_MODULE_REGISTERS,
		NORMAL,
	};

	HtrSwirModuleHead *headModule;
	HtrSwirPtHead *headDome;
	DriverState state;
	PtzpTcpTransport *tp;
};

#endif // HTRSWIRDRIVER_H
