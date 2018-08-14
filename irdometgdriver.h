#ifndef IRDOMETGDRIVER_H
#define IRDOMETGDRIVER_H

#include <ecl/ptzp/ptzpdriver.h>

class IRDomePTHead;
class OemModuleHead;
class PtzpSerialTransport;

class IRDomeTGDriver : public PtzpDriver
{
	Q_OBJECT
public:
	explicit IRDomeTGDriver(QObject *parent = 0);

	PtzpHead * getHead(int index);
	int setTarget(const QString &targetUri);
	QVariant get(const QString &key);
	int set(const QString &key, const QVariant &value);
protected slots:
	void timeout();

protected:
	enum DriverState {
		INIT,
		SYNC_HEAD_MODULE,
		SYNC_HEAD_DOME,
		NORMAL,
	};

	OemModuleHead * headModule;
	IRDomePTHead * headDome;
	PtzpTransport *transport;
	PtzpTransport *transport1;
	DriverState state;
};

#endif // IRDOMETGDRIVER_H
