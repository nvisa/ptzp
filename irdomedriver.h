#ifndef IRDOMEDRIVER_H
#define IRDOMEDRIVER_H

#include <ecl/ptzp/ptzpdriver.h>

class IRDomeDriver : public PtzpDriver
{
	Q_OBJECT
public:
	explicit IRDomeDriver(QObject *parent = 0);

	PtzpHead * getHead(int index);
	int setTarget(const QString &targetUri);

protected slots:
	void timeout();

protected:
	enum DriverState {
		INIT,
		SYNC_HEAD_MODULE,
		SYNC_HEAD_DOME,
		NORMAL,
	};

	PtzpHead * headModule;
	PtzpHead * headDome;
	PtzpTransport *transport;
	DriverState state;
};

#endif // IRDOMEDRIVER_H
