#include "ptzphttptransport.h"
#include <ecl/debug.h>

#include <QNetworkReply>

PtzpHttpTransport::PtzpHttpTransport(LineProtocol proto, QObject *parent)
	: QObject(parent),
	  PtzpTransport(proto)
{
	netman = NULL;
	timer = new QTimer();
	timer->start(periodTimer);
	connect(timer, SIGNAL(timeout()), SLOT(callback()));
}

int PtzpHttpTransport::connectTo(const QString &targetUri)
{
	if (targetUri.split("/").size() < 3)
		return -1;
	request.setUrl(QUrl(targetUri));
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
	netman = new QNetworkAccessManager(this);
	connect(this, SIGNAL(sendSocketMessage2Main(QByteArray)), SLOT(sendSocketMessage(QByteArray)));
	connect(netman, SIGNAL(finished(QNetworkReply*)), SLOT(dataReady(QNetworkReply *)));
	mDebug("Connection definated %s", qPrintable(targetUri));
	return 0;
}

int PtzpHttpTransport::send(const char *bytes, int len)
{
	emit sendSocketMessage2Main(QByteArray(bytes, len));
	return len;
}

void PtzpHttpTransport::dataReady(QNetworkReply *reply)
{
	if (reply->error() != QNetworkReply::NoError) {
		mDebug("Error in send/get command %d", reply->error());
		return;
	}
	QByteArray ba = reply->readAll();
	protocol->dataReady(ba);
}

void PtzpHttpTransport::sendSocketMessage(const QByteArray &ba)
{
	netman->post(request, ba);
}

void PtzpHttpTransport::callback()
{
	QByteArray m = PtzpTransport::queueFreeCallback();
	send(m.data(), m.size());
}
