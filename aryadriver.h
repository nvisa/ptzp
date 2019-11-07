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
	virtual PtzpHead *getHead(int index);

	int setMoxaControl(const QString &targetThermal, const QString &targetDay);
	void updateZoomOverlay(int thermal, int day);
	void setOverlayInterval(int ms);
	void setThermalInterval(int ms);
	void setGungorInterval(int ms);
	void setHeadType(QString type);
	grpc::Status GetSettings(grpc::ServerContext *context, const ptzp::Settings *request, ptzp::Settings *response);
	grpc::Status SetSettings(grpc::ServerContext *context, const ptzp::Settings *request, ptzp::Settings *response);
protected slots:
	void timeout();
protected:
	void reboot();
	int connectPT(const QString &target);
	int connectThermal(const QString &target);
	int connectDay(const QString &target);
	void setRegulateSettings();
protected:
	MgeoThermalHead *thermal;
	AryaPTHead *aryapt;
	MgeoGunGorHead *gungor;
	PtzpTcpTransport *tcp1;
	PtzpTcpTransport *tcp2;
	PtzpTcpTransport *tcp3;
	QElapsedTimer overlayLaps;
	PtzpHttpTransport *httpThermal;
	PtzpHttpTransport *httpDay;
	MoxaControlHead *moxaThermal;
	MoxaControlHead *moxaDay;
	int overlayInterval;
	int thermalInterval;
	int gungorInterval;
	QString headType;
	bool apiEnable;
	bool reinitPT;
	bool reinitThermal;
	bool reinitDay;
};

#endif // ARYADRIVER_H
