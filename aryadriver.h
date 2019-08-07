#ifndef ARYADRIVER_H
#define ARYADRIVER_H

#include <ecl/ptzp/ptzpdriver.h>
#include <QElapsedTimer>

class AryaPTHead;
class MgeoThermalHead;
class MgeoGunGorHead;
class PtzpTcpTransport;
class PtzpHttpTransport;
class MoxaControlHead;
class AryaDriver : public PtzpDriver
{
	Q_OBJECT
public:
	explicit AryaDriver(QObject *parent = 0);

	virtual int setTarget(const QString &targetUri);
	virtual PtzpHead * getHead(int index);
	QVariant get(const QString &key);
	int set(const QString &key, const QVariant &value);

	int setMoxaControl(const QString &targetThermal, const QString &targetDay);
	void updateZoomOverlay(int thermal, int day);
	void setOverlayInterval(int ms);
protected slots:
	void timeout();

protected:
	enum DriverState {
		INIT,
		NORMAL,
		SYNC_THERMAL_MODULES,
		SYNC_GUNGOR_MODULES,
		SYSTEM_CHECK
	};

	DriverState state;
	MgeoThermalHead *thermal;
	AryaPTHead *aryapt;
	MgeoGunGorHead *gungor;
	PtzpTcpTransport *tcp1;
	PtzpTcpTransport *tcp2;
	PtzpTcpTransport *tcp3;
	QElapsedTimer *checker;
	QElapsedTimer overlayLaps;
	PtzpHttpTransport *httpThermal;
	PtzpHttpTransport *httpDay;
	MoxaControlHead *moxaThermal;
	MoxaControlHead *moxaDay;
	int overlayInterval;
};

#endif // ARYADRIVER_H
