#ifndef PTZPTRANSPORT_H
#define PTZPTRANSPORT_H

#include <QList>
#include <QString>
#include <QByteArray>
#include <QElapsedTimer>

class PtzpTransport
{
public:
	PtzpTransport();
	typedef int (*dataReady)(const unsigned char *bytes, int len, void *priv);
	typedef QByteArray (*queueFree)(void *priv);

	virtual int connectTo(const QString &targetUri) = 0;
	virtual int send(const QByteArray &ba);
	virtual int send(const char *bytes, int len) = 0;
	virtual int addReadyCallback(dataReady callback, void *priv);
	virtual int addQueueFreeCallback(queueFree callback, void *priv);

	void enableQueueFreeCallbacks(bool en, int timeout = 0);
	void setQueueFreeCallbackMask(uint mask);
	uint getQeueuFreeCallbackMask();

	static int dataReadyCallback(const unsigned char *bytes, int len, void *priv);
	static QByteArray queueFreeCallback(void *priv);

protected:
	int dataReadyCallback(const unsigned char *bytes, int len);
	QByteArray queueFreeCallback();

	QElapsedTimer queueFreeEnabledTimer;
	int queueFreeEnabledTimeout;
	bool queueFreeEnabled;
	uint queueFreeCallbackMask;
	QList<dataReady> dataReadyCallbacks;
	QList<queueFree> queueFreeCallbacks;
	QList<void *> dataReadyCallbackPrivs;
	QList<void *> queueFreeCallbackPrivs;
};

#endif // PTZPTRANSPORT_H
