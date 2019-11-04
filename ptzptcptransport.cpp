#include "ptzptcptransport.h"
#include "debug.h"

#include <QHostAddress>
#include <QTcpSocket>
#include <QUdpSocket>

PtzpTcpTransport::PtzpTcpTransport(LineProtocol proto, QObject *parent)
	: QObject(parent), PtzpTransport(proto)
{
	sock = NULL;
	devStatus = DEAD;
	timer = new QTimer();
	filterInterface = NULL;
	timer->start(periodTimer);
	connect(timer, SIGNAL(timeout()), SLOT(callback()));
	connect(this, SIGNAL(sendSocketMessage2Main(QByteArray)),
			SLOT(sendSocketMessage(QByteArray)));
}

int PtzpTcpTransport::connectTo(const QString &targetUri)
{
	QStringList flds = targetUri.split(":");
	if (flds.size() == 1)
		flds << "4002";
	if (flds.size() == 2)
		flds << "tcp";
	if (flds.size() == 3)
		flds << "0";
	if (flds.size() == 4)
		flds << "0";

	isUdp = false;
	if (flds[2] == "tcp")
		sock = new QTcpSocket();
	else {
		isUdp = true;
		sock = new QUdpSocket();
		sendDstPort = flds[4].toInt();
	}
	if (flds[3].toInt()) {
		if (!sock->bind(flds[3].toUInt()))
			mDebug("error binding to port %d", flds[3].toInt());
	}
	serverUrl = flds[0];
	serverPort = flds[1].toInt();
	sock->connectToHost(serverUrl, serverPort);
	connect(sock, SIGNAL(connected()), SLOT(connected()));
	connect(sock, SIGNAL(disconnected()), SLOT(clientDisconnected()));
	connect(sock, SIGNAL(readyRead()), SLOT(dataReady()));
	if (!sock->waitForConnected(3000)) {
		mDebug("Socket connection error: Error code: %d, Error string: %s",
			   sock->error(), qPrintable(sock->errorString()));
		return -1;
	}
	return 0;
}

void PtzpTcpTransport::reConnect()
{
	mDebug("Socket dead!!!! Why so serious ?");
	if (sock->state() != QAbstractSocket::ConnectingState) {
		mDebug("Trying to connect '%s'", qPrintable(serverUrl));
		sock->connectToHost(serverUrl, serverPort);
	}
}

int PtzpTcpTransport::disconnectFrom()
{
	if (!sock)
		return -ENOENT;
	sock->disconnectFromHost();
	sock->deleteLater();
	sock = NULL;
	devStatus = DEAD;
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

void PtzpTcpTransport::setFilter(
	PtzpTcpTransport::TransportFilterInteface *iface)
{
	filterInterface = iface;
}

void PtzpTcpTransport::connected()
{
	mInfo("connected");
	devStatus = ALIVE;
}

PtzpTcpTransport::DevStatus PtzpTcpTransport::getStatus()
{
	return devStatus;
}

void PtzpTcpTransport::dataReady()
{
	if (devStatus != ALIVE)
		return;
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
	devStatus = DEAD;
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
	if (devStatus != ALIVE || !sock)
		return;
	if (isUdp && sendDstPort)
		((QUdpSocket *)sock)
			->writeDatagram(ba, sock->peerAddress(), sendDstPort);
	else if (sock)
		sock->write(ba);
}
