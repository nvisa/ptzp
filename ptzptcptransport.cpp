#include "ptzptcptransport.h"
#include "debug.h"

#include <QTcpSocket>

PtzpTcpTransport::PtzpTcpTransport(LineProtocol proto, QObject *parent)
	: QObject(parent),
	PtzpTransport(proto)
{
	sock = NULL;
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

