#ifndef FLIRDRIVER_H
#define FLIRDRIVER_H

#include <ptzpdriver.h>
#include <ptzphttptransport.h>
#include <ptzptcptransport.h>

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

	void finished(QNetworkReply *reply);
protected:
	void bumpStart();
protected:
	FlirPTTcpHead *headDome;
	FlirModuleHead *headModule;
	PtzpTcpTransport *transportDome;
	PtzpHttpTransport *httpTransportModule;
	QNetworkRequest req;
private:
	QString pturl;
	bool reinitPT;
};

#endif // FLIRDRIVER_H
