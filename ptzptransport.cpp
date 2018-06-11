#include "ptzptransport.h"
#include "debug.h"

PtzpTransport::PtzpTransport()
{
	queueFreeEnabled = false;
	queueFreeEnabledTimeout = 0;
	queueFreeCallbackMask = 0xffffffff;
}

int PtzpTransport::send(const QByteArray &ba)
{
	return send(ba.constData(), ba.size());
}

int PtzpTransport::addReadyCallback(PtzpTransport::dataReady callback, void *priv)
{
	dataReadyCallbacks << callback;
	dataReadyCallbackPrivs << priv;
	return 0;
}

int PtzpTransport::addQueueFreeCallback(PtzpTransport::queueFree callback, void *priv)
{
	queueFreeCallbacks << callback;
	queueFreeCallbackPrivs << priv;
	return 0;
}

void PtzpTransport::enableQueueFreeCallbacks(bool en, int timeout)
{
	queueFreeEnabled = en;
	if (timeout) {
		queueFreeEnabledTimeout = timeout;
		queueFreeEnabledTimer.start();
	}
}

void PtzpTransport::setQueueFreeCallbackMask(uint mask)
{
	queueFreeCallbackMask = mask;
}

uint PtzpTransport::getQeueuFreeCallbackMask()
{
	return queueFreeCallbackMask;
}

int PtzpTransport::dataReadyCallback(const unsigned char *bytes, int len, void *priv)
{
	return ((PtzpTransport *)priv)->dataReadyCallback(bytes, len);
}

QByteArray PtzpTransport::queueFreeCallback(void *priv)
{
	return ((PtzpTransport *)priv)->queueFreeCallback();
}

int PtzpTransport::dataReadyCallback(const unsigned char *bytes, int len)
{
	for (int i = 0; i < dataReadyCallbacks.size(); i++) {
		int read = dataReadyCallbacks[i](bytes, len, dataReadyCallbackPrivs[i]);
		if (read > 0)
			return read;
	}
	return -1;
}

QByteArray PtzpTransport::queueFreeCallback()
{
	if (!queueFreeEnabled) {
		if (queueFreeEnabledTimeout && queueFreeEnabledTimer.elapsed() > queueFreeEnabledTimeout) {
			queueFreeEnabled = false;
		} else
			return QByteArray();
	}
	for (int i = 0; i < queueFreeCallbacks.size(); i++) {
		if (((1 << i) & queueFreeCallbackMask) == 0)
			continue;
		QByteArray ba = queueFreeCallbacks[i](queueFreeCallbackPrivs[i]);
		if (ba.size())
			return ba;
	}
	return QByteArray();
}

