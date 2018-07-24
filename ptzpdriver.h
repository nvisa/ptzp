#ifndef PTZPDRIVER_H
#define PTZPDRIVER_H

#include <QObject>

#include <ecl/interfaces/keyvalueinterface.h>

class QTimer;
class PtzpHead;
class PtzpTransport;

class PtzpDriver : public QObject, public KeyValueInterface
{
	Q_OBJECT
public:
	explicit PtzpDriver(QObject *parent = 0);

	virtual int setTarget(const QString &targetUri) = 0;
	virtual PtzpHead * getHead(int index) = 0;
	virtual int getHeadCount();

	void startSocketApi(quint16 port);
	virtual QVariant get(const QString &key);
	virtual int set(const QString &key, const QVariant &value);

protected slots:
	virtual void timeout();
	QVariant headInfo(const QString &key, PtzpHead *head);

protected:
	QTimer *timer;
	PtzpHead *defaultPTHead;
	PtzpHead *defaultModuleHead;
};

#endif // PTZPDRIVER_H
