#ifndef OEM4KDRIVER_H
#define OEM4KDRIVER_H

#include <ecl/ptzp/ptzpdriver.h>
#include <ecl/ptzp/ptzphttptransport.h>
#include <ecl/ptzp/ptzptcptransport.h>

class Oem4kDriver : public PtzpDriver
{
	Q_OBJECT
public:
	Oem4kDriver();
	~Oem4kDriver();

	enum DriverState {
		INIT,
		SYNC,
		NORMAL
	};

	DriverState state;

	PtzpHead *getHead(int index);
	int setTarget(const QString &targetUri);
protected slots:
	void timeout();
protected:
	PtzpHead *headModule;
	PtzpHttpTransport *httpTransportModule;
	QNetworkRequest req;
};

#endif // OEM4KDRIVER_H
