#include "ptzptcptransport.h"
#include "debug.h"

#include <QTcpSocket>
#include <QHostAddress>

PtzpTcpTransport::PtzpTcpTransport(LineProtocol proto, QObject *parent)
	: QObject(parent),
	PtzpTransport(proto)
{
	sock = NULL;
	timer = new QTimer();
	timer->start(periodTimer);
	connect(timer, SIGNAL(timeout()), SLOT(callback()));
}

int PtzpTcpTransport::connectTo(const QString &targetUri)
{
	QStringList flds = targetUri.split(":");
	if (flds.size() == 1)
		flds << "4002";
	sock = new QTcpSocket();
	sock->connectToHost(flds[0], flds[1].toInt());
	connect(sock, SIGNAL(connected()), SLOT(connected()));
	connect(sock, SIGNAL(disconnected()), SLOT(clientDisconnected()));
	connect(sock, SIGNAL(readyRead()), SLOT(dataReady()));
	if (!sock->waitForConnected(3000)){
		mDebug("Socket connection error: Error code: %d, Error string: %s", sock->error(), qPrintable(sock->errorString()));
		return -1;
	}
	return 0;
}

int PtzpTcpTransport::send(const char *bytes, int len)
{
	lock.lock();
	int size = sock->write(bytes, len);
	lock.unlock();
	return size;
}

void PtzpTcpTransport::connected()
{
	ffDebug() << "connected";
}

void PtzpTcpTransport::dataReady()
{
	lock.lock();
	protocol->dataReady(sock->readAll());
	lock.unlock();
}

void PtzpTcpTransport::clientDisconnected()
{
	ffDebug() << "disconnected";
}
/**
 * @brief PtzpTcpTransport::callback
 * This function waiting data to send.
 * Some `head` classes have sendcommand API, for himself.
 * If returning data from `queueFreeCallBack` isn't empty,
 * The data will send over standart TCP API's.
 */
void PtzpTcpTransport::callback()
{
	QByteArray m = PtzpTransport::queueFreeCallback();
	if (!m.isEmpty())
		send(m.data(), m.size());
	timer->setInterval(periodTimer);
}
