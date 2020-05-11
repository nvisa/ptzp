#ifndef DORTGOZDRIVER_H
#define DORTGOZDRIVER_H

#include <ptzpdriver.h>

class MgeoDortgozHead;
class AryaPTHead;
class PtzpSerialTransport;

class DortgozDriver : public PtzpDriver
{
		Q_OBJECT
public:
	explicit DortgozDriver(QList<int> relayConfig, QObject *parent = 0);
	PtzpHead *getHead(int index);
	int setTarget(const QString &targetUri);
	QString getCapString(ptzp::PtzHead_Capability cap);

protected slots:
	void timeout();

protected:
	enum DriverState {
		INIT,
		WAIT_ALIVE,
		SYNC_HEAD_MODULE,
		NORMAL,
	};

	MgeoDortgozHead *headModule;
	AryaPTHead *headDome;
	DriverState state;
	PtzpTransport *tp1;
	PtzpTransport *tp2;
};

#endif // DORTGOZDRIVER_H
