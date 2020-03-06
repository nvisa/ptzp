#ifndef VIRTUALPTZPDRIVER_H
#define VIRTUALPTZPDRIVER_H

#include <ptzpdriver.h>

class LifeTimeHead;

class VirtualPtzpDriver : public PtzpDriver
{
	Q_OBJECT
public:
	VirtualPtzpDriver(QObject *parent = 0);

	// PtzpDriver interface
public:
	int setTarget(const QString &targetUri);
	PtzpHead *getHead(int index);

protected slots:
	virtual void timeout();

protected:
	LifeTimeHead *head0;
	LifeTimeHead *head1;
};

#endif // VIRTUALPTZPDRIVER_H
