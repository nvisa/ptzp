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

protected slots:
	void timeout();

protected:
	enum DriverState {
		UNINIT,
		INIT,
		SYNC_HEAD_MODULE,
		SYNC_HEAD_DOME,
		NORMAL,
	};

	OemModuleHead * headModule;
	IRDomePTHead * headDome;
	DriverState state;
	PtzpTransport *tp;
	PtzpTransport *tp1;
	bool ptSupport;
};

#endif // IRDOMEDRIVER_H
