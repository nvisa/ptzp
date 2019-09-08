#ifndef PTZPTRANSPORT_H
#define PTZPTRANSPORT_H

#include <QByteArray>
#include <QElapsedTimer>
#include <QList>
#include <QString>

class LineProto;

class PtzpTransport
{
public:
	enum LineProtocol {
		PROTO_STRING_DELIM,
		PROTO_BUFFERED,
	};
	PtzpTransport();
	PtzpTransport(LineProtocol proto);
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
	void setMaxBufferLength(int length);
	void setTimerInterval(int value);

	static QByteArray queueFreeCallback(void *priv);

	class LineProto
	{
	public:
		PtzpTransport *transport;
		virtual void dataReady(const QByteArray &ba) = 0;
		int processNewFrame(const unsigned char *bytes, int len);
	};

protected:
	QByteArray queueFreeCallback();

	QElapsedTimer queueFreeEnabledTimer;
	int queueFreeEnabledTimeout;
	bool queueFreeEnabled;
	uint queueFreeCallbackMask;
	QList<dataReady> dataReadyCallbacks;
	QList<queueFree> queueFreeCallbacks;
	QList<void *> dataReadyCallbackPrivs;
	QList<void *> queueFreeCallbackPrivs;
	LineProtocol lineProto;
	LineProto *protocol;
	friend class LineProto;
	/*
	 * In case of programming bugs, our input buffer may accumulate
	 * too much data. To prevent this, we introduce a mechanism for
	 * cleaning-up input data at some point. By default, this feature
	 * is off. You can set 'maxBufferLength' in your transport implementation
	 * to some sane default value to enable this feature
	 */
	int maxBufferLength;
	int periodTimer;
};

#endif // PTZPTRANSPORT_H
