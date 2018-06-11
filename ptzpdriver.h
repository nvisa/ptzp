#ifndef PTZPDRIVER_H
#define PTZPDRIVER_H

#include <QObject>

class QTimer;
class PtzpHead;
class PtzpTransport;

class PtzpDriver : public QObject
{
	Q_OBJECT
public:
	explicit PtzpDriver(QObject *parent = 0);

	virtual int setTarget(const QString &targetUri) = 0;
	virtual PtzpHead * getHead(int index) = 0;

protected slots:
	virtual void timeout();

protected:
	QTimer *timer;
};

#endif // PTZPDRIVER_H
