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
	timer->start(100);
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
	return 0;
}

int PtzpTcpTransport::send(const char *bytes, int len)
{
	return sock->write(bytes, len);
}

void PtzpTcpTransport::connected()
{
	ffDebug() << "connected";
}

void PtzpTcpTransport::dataReady()
{
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
}
