#ifndef IRDOMEDRIVER_H
#define IRDOMEDRIVER_H

#include <ecl/ptzp/ptzpdriver.h>

class IRDomePTHead;
class OemModuleHead;
class PtzpSerialTransport;

class IRDomeDriver : public PtzpDriver
{
	Q_OBJECT
public:
	explicit IRDomeDriver(QObject *parent = 0);

	PtzpHead * getHead(int index);
	int setTarget(const QString &targetUri);
	QVariant get(const QString &key);
	int set(const QString &key, const QVariant &value);
	void configLoad(const QString filename);

protected slots:
	void timeout();

protected:
	enum DriverState {
		INIT,
		SYNC_HEAD_MODULE,
		SYNC_HEAD_DOME,
		NORMAL,
	};

	conf config;
	OemModuleHead * headModule;
	IRDomePTHead * headDome;
	PtzpTransport *transport;
	PtzpTransport *transport1;
	DriverState state;
	QStringList targetCam;
	QStringList targetDome;
};

#endif // IRDOMEDRIVER_H
