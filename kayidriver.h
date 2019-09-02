#ifndef KAYIDRIVER_H
#define KAYIDRIVER_H

#include <ecl/ptzp/ptzpdriver.h>

class MgeoFalconEyeHead;
class AryaPTHead;
class PtzpSerialTransport;

class KayiDriver : public PtzpDriver
{
	Q_OBJECT
public:
	explicit KayiDriver(QList<int> relayConfig, QObject *parent = 0);

	PtzpHead * getHead(int index);
	int setTarget(const QString &targetUri);
	int set(const QString &key, const QVariant &value);
	bool isReady();

protected slots:
	void timeout();

protected:
	enum DriverState {
		INIT,
		WAIT_ALIVE,
		SYNC_HEAD_MODULE,
		NORMAL,
	};

	MgeoFalconEyeHead *headModule;
	AryaPTHead *headDome;
	DriverState state;
	PtzpTransport *tp1;
	PtzpTransport *tp2;
};

#endif // KAYIDRIVER_H
