#ifndef YAMGOZDRIVER_H
#define YAMGOZDRIVER_H

#include <ecl/ptzp/ptzpdriver.h>

class MgeoYamGozHead;
class PtzpTcpTransport;

class YamGozDriver : public PtzpDriver
{
	Q_OBJECT

public:
	explicit YamGozDriver(QObject *parent = 0);

	PtzpHead * getHead(int index);
	int setTarget(const QString &targetUri);

protected slots:
	void timeout();

protected:
	enum DriverState {
		INIT,
		NORMAL,
	};

	DriverState state;

	MgeoYamGozHead *head0;
	MgeoYamGozHead *head1;
	MgeoYamGozHead *head2;

	PtzpTransport *tp0;
	PtzpTransport *tp1;
	PtzpTransport *tp2;
};

#endif // YAMGOZDRIVER_H
