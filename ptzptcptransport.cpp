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
	filterInterface = NULL;
	timer->start(periodTimer);
	connect(timer, SIGNAL(timeout()), SLOT(callback()));
	connect(this, SIGNAL(sendSocketMessage2Main(QByteArray)), SLOT(sendSocketMessage(QByteArray)));
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

int PtzpTcpTransport::disconnectFrom()
{
	if (!sock)
		return -ENOENT;
	sock->disconnectFromHost();
	sock->deleteLater();
	sock = NULL;
	return 0;
}

int PtzpTcpTransport::send(const char *bytes, int len)
{
	if (filterInterface) {
		const QByteArray ba = filterInterface->sendFilter(bytes, len);
		if (ba.size())
			emit sendSocketMessage2Main(ba);
		else
			emit sendSocketMessage2Main(QByteArray(bytes, len));
	} else
		emit sendSocketMessage2Main(QByteArray(bytes, len));
	return len;
}

void PtzpTcpTransport::setFilter(PtzpTcpTransport::TransportFilterInteface *iface)
{
	filterInterface = iface;
}

void PtzpTcpTransport::connected()
{
	ffDebug() << "connected";
}

void PtzpTcpTransport::dataReady()
{
	if (filterInterface) {
		while (sock->bytesAvailable()) {
			QByteArray ba;
			int err = filterInterface->readFilter(sock, ba);
			if (err == -EAGAIN)
				continue;
			if (err == -ENODATA)
				break;
			if (err == -EIO)
				continue;
			protocol->dataReady(ba);
		}
	} else
		protocol->dataReady(sock->readAll());
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

void PtzpTcpTransport::sendSocketMessage(const QByteArray &ba)
{
	if (sock)
		sock->write(ba);
}
