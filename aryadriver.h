#ifndef ARYADRIVER_H
#define ARYADRIVER_H

#include <ecl/ptzp/ptzpdriver.h>

class AryaPTHead;
class MgeoThermalHead;
class PtzpTcpTransport;

class AryaDriver : public PtzpDriver
{
	Q_OBJECT
public:
	explicit AryaDriver(QObject *parent = 0);

	virtual int setTarget(const QString &targetUri);
	virtual PtzpHead * getHead(int index);
	QVariant get(const QString &key);
	int set(const QString &key, const QVariant &value);
protected slots:
	void timeout();

protected:
	enum DriverState {
		INIT,
		NORMAL,
		SYNC_THERMAL_MODULE,
	};

	DriverState state;
	MgeoThermalHead *thermal;
	AryaPTHead *aryapt;
	PtzpTcpTransport *tcp1;
	PtzpTcpTransport *tcp2;
};

#endif // ARYADRIVER_H
