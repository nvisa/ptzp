#ifndef VIRTUALPTZPDRIVER_H
#define VIRTUALPTZPDRIVER_H

#include <ecl/ptzp/ptzpdriver.h>

class VirtualPtzpDriver : public PtzpDriver
{
	Q_OBJECT
public:
	VirtualPtzpDriver(QObject *parent = 0);

	// PtzpDriver interface
public:
	int setTarget(const QString &targetUri);
	PtzpHead *getHead(int index);

	PtzpHead *head0;
	PtzpHead *head1;
};

#endif // VIRTUALPTZPDRIVER_H
