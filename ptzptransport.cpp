#include "ptzptransport.h"
#include "debug.h"

#include <errno.h>

/*
 * [CR] [fo] transport yapısı kesinlikle dökümante edilmeli.
 * Buranın çalışma yapısı bilinmediği için yapılan geliştirmeler
 * sistemi etkileyebilecek hatalara sebebiyet verebiliyor.
 * Örn; tbgth termak head sync sorunu, ki bu hatanın sebebi pt head de yapılan
 * değişiklikten kaynaklanıyordu. Bu nedenle bu yapı için dökümantasyon gerekli.
 */
class BufferedProto : public PtzpTransport::LineProto
{
public:
	void dataReady(const QByteArray &ba)
	{
		buf.append(ba);

		int readlen = 0;
		do {
			readlen =
				processNewFrame((const uchar *)buf.constData(), buf.size());
			if (readlen > 0)
				buf = buf.mid(readlen);
		} while (readlen > 0 && buf.size());
	}

	QByteArray buf;
};

class StringProto : public PtzpTransport::LineProto
{
public:
	StringProto() { delim = ">"; }
	void dataReady(const QByteArray &ba)
	{
		buf.append(QString::fromUtf8(ba));

		while (buf.contains(delim)) {
			int off = buf.indexOf(delim);
			QString mes = buf.left(off + 1);
			processNewFrame((const uchar *)mes.toUtf8().constData(),
							mes.length());
			buf = buf.mid(off + 1);
		}
	}
	void setDelimiter(const QString &key) {delim = key;}
protected:
	QString buf;
	QString delim;
};

PtzpTransport::PtzpTransport()
{
	periodTimer = 100;
	maxBufferLength = INT_MAX;
	queueFreeEnabled = false;
	queueFreeEnabledTimeout = 0;
	queueFreeCallbackMask = 0xffffffff;
	lineProto = PROTO_BUFFERED;
	protocol = new BufferedProto;
	protocol->transport = this;
}

PtzpTransport::PtzpTransport(PtzpTransport::LineProtocol proto)
{
	periodTimer = 100;
	maxBufferLength = INT_MAX;
	queueFreeEnabled = false;
	queueFreeEnabledTimeout = 0;
	queueFreeCallbackMask = 0xffffffff;
	lineProto = proto;
	if (proto == PROTO_STRING_DELIM)
		protocol = new StringProto;
	else if (proto == PROTO_BUFFERED)
		protocol = new BufferedProto;
	protocol->transport = this;
}

void PtzpTransport::setDelimiter(const QString &delim)
{
	protocol->setDelimiter(delim);
}

int PtzpTransport::send(const QByteArray &ba)
{
	return send(ba.constData(), ba.size());
}

int PtzpTransport::addReadyCallback(PtzpTransport::dataReady callback,
									void *priv)
{
	dataReadyCallbacks << callback;
	dataReadyCallbackPrivs << priv;
	return 0;
}

int PtzpTransport::addQueueFreeCallback(PtzpTransport::queueFree callback,
										void *priv)
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

void PtzpTransport::setMaxBufferLength(int length)
{
	maxBufferLength = length;
}

void PtzpTransport::setTimerInterval(int value)
{
	periodTimer = value;
}

QByteArray PtzpTransport::queueFreeCallback(void *priv)
{
	return ((PtzpTransport *)priv)->queueFreeCallback();
}

QByteArray PtzpTransport::queueFreeCallback()
{
	if (!queueFreeEnabled) {
		if (queueFreeEnabledTimeout &&
			queueFreeEnabledTimer.elapsed() > queueFreeEnabledTimeout) {
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

int PtzpTransport::LineProto::processNewFrame(const unsigned char *bytes,
											  int len)
{
	int erroredCount = 0;
	for (int i = 0; i < transport->dataReadyCallbacks.size(); i++) {
		int read = transport->dataReadyCallbacks[i](
			bytes, len, transport->dataReadyCallbackPrivs[i]);
		if (read > 0)
			return read;

		/* error case */
		if (read == -EAGAIN)
			continue;

		if (read == -ENOENT)
			erroredCount++;
	}

	if (erroredCount == transport->dataReadyCallbacks.size())
		/*
		 * This bytestream contains some sort of garbage, everybody reports
		 * error so it is wise to discard one byte here
		 */
		return 1;

	return -1;
}
