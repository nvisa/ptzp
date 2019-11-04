#ifndef FLIRDRIVER_H
#define FLIRDRIVER_H

#include <ecl/ptzp/ptzpdriver.h>
#include <ecl/ptzp/ptzphttptransport.h>
#include <ecl/ptzp/ptzptcptransport.h>

class FlirModuleHead;
class FlirPTTcpHead;
class FlirDriver : public PtzpDriver
{
	Q_OBJECT
public:
	FlirDriver();
	~FlirDriver();

	PtzpHead *getHead(int index);
	int setTarget(const QString &targetUri);
	QJsonObject doExtraDeviceTests();
protected slots:
	void timeout();

protected:
	FlirPTTcpHead *headDome;
	FlirModuleHead *headModule;
	PtzpTcpTransport *transportDome;
	PtzpHttpTransport *httpTransportModule;
};

#endif // FLIRDRIVER_H
