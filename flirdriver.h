#ifndef FLIRDRIVER_H
#define FLIRDRIVER_H

#include <ecl/ptzp/ptzpdriver.h>
#include <ecl/ptzp/ptzphttptransport.h>

class FlirModuleHead;
class FlirPTHead;

class FlirDriver : public PtzpDriver
{
	Q_OBJECT
public:
	FlirDriver();
	~FlirDriver();

	PtzpHead *getHead(int index);
	int setTarget(const QString &targetUri);
protected slots:
	void timeout();

protected:
	FlirModuleHead *headModule;
	FlirPTHead *headDome;
	PtzpHttpTransport *httpTransportDome;
	PtzpHttpTransport *httpTransportModule;
};

#endif // FLIRDRIVER_H
