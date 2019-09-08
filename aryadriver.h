#ifndef ARYADRIVER_H
#define ARYADRIVER_H

#include <ecl/net/networkaccessmanager.h>
#include <ecl/ptzp/ptzpdriver.h>

class AryaPTHead;
class MgeoThermalHead;
class MgeoGunGorHead;
class PtzpTcpTransport;

class AryaDriver : public PtzpDriver
{
	Q_OBJECT
public:
	enum OverlayPos { LEFT_UP, RIGHT_UP, LEFT_DOWN, RIGHT_DOWN, CUSTOM };

	struct Overlay {
		OverlayPos pos;
		int posx;
		int posy;
		int textSize;
		bool disabled;
	};

	struct VideoDeviceParams {
		QString ip;
		QString pass;
		QString uname;
	};

	/*
	 * [CR] [yca] Bu enum'a gerek var mi? setZoomOverlayString()
	 * fonksiyonuna ilgili kafanin kendisi gecilemez mi?
	 */
	enum overlayForHead { THERMAL, DAY };

	explicit AryaDriver(QObject *parent = 0);

	virtual int setTarget(const QString &targetUri);
	virtual PtzpHead *getHead(int index);
	QVariant get(const QString &key);
	int set(const QString &key, const QVariant &value);
	int setZoomOverlay();

	int setOverlay(const QString data);
	QString setZoomOverlayString(overlayForHead head);
	void setVideoDeviceParams(const QString &ip, const QString &uname,
							  const QString &pass);
protected slots:
	void timeout();
	void overlayFinished();

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
	NetworkAccessManager *netman;
	Overlay olay;
	QElapsedTimer *checker;
	VideoDeviceParams vdParams;
};

#endif // ARYADRIVER_H
