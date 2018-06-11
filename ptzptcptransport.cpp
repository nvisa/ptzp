#include "ptzptcptransport.h"
#include "debug.h"

#include <QTcpSocket>

PtzpTcpTransport::PtzpTcpTransport(QObject *parent)
	: QObject(parent)
{
	sock = NULL;
}

int PtzpTcpTransport::connectTo(const QString &targetUri)
{
	QStringList flds = targetUri.split(":");
	if (flds.size() == 1)
		flds << "4002";
	sock = new QTcpSocket();
	connect(sock, SIGNAL(connected()), SIGNAL(connected()));
	connect(sock, SIGNAL(disconnected()), SLOT(clientDisconnected()));
	connect(sock, SIGNAL(readyRead()), SLOT(dataReady()));
	return 0;
}

void PtzpTcpTransport::connected()
{
	ffDebug() << "connected";
}

void PtzpTcpTransport::dataReady()
{

}

void PtzpTcpTransport::clientDisconnected()
{
	ffDebug() << "disconnected";
}

