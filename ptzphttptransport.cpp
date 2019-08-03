#include "ptzphttptransport.h"
#include <ecl/debug.h>

#include <QNetworkReply>

PtzpHttpTransport::PtzpHttpTransport(LineProtocol proto, QObject *parent)
	: QObject(parent),
	  PtzpTransport(proto)
{
	netman = NULL;
	cgiFlag = false;
	timer = new QTimer();
	timer->start(periodTimer);
	connect(timer, SIGNAL(timeout()), SLOT(callback()));
}

int PtzpHttpTransport::connectTo(const QString &targetUri)
{
	QUrl url;
	if (targetUri.contains("?")) {
		qDebug() << "headmodule" << targetUri;
		QStringList tgts = targetUri.split("?");
		QString ip = tgts.first();
		if (!ip.contains("http://"))
			ip = "http://" + ip;
		url.setUrl(ip);
		int port = 80;
		if (tgts.size() == 4)
			port = tgts.last().toInt();
		url.setPort(port);
		request.setUrl(url);
		request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
		request.setRawHeader("Authorization", "Basic "
							 + QString("%1:%2").arg(tgts.at(1)).arg(tgts.at(2)).toLatin1().toBase64());
	} else {
		request.setUrl(QUrl(targetUri));
		request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
	}
	netman = new QNetworkAccessManager(this);
	connect(this, SIGNAL(sendPostMessage2Main(QByteArray)), SLOT(sendPostMessage(QByteArray)));
	connect(this, SIGNAL(sendGetMessage2Main(QByteArray)), SLOT(sendGetMessage(QByteArray)));
	connect(netman, SIGNAL(finished(QNetworkReply*)), SLOT(dataReady(QNetworkReply *)));
	mDebug("Connection definated %s", qPrintable(targetUri));
	return 0;
}

int PtzpHttpTransport::send(const char *bytes, int len)
{
	if (cgiFlag) {
		QString data = QString::fromUtf8(bytes, len);
		if (data.contains("?")) {
			QString path = data.split("?").first();
			QUrl url = request.url();
			url.setPath(path, QUrl::ParsingMode::StrictMode);
			request.setUrl(url);
			emit sendPostMessage2Main(data.split("?").last().toLatin1());
		}
	} else
		emit sendPostMessage2Main(QByteArray(bytes, len));
	return len;
}

void PtzpHttpTransport::setCGIFlag(bool v)
{
	cgiFlag = v;
}

void PtzpHttpTransport::dataReady(QNetworkReply *reply)
{
	if (reply->error() != QNetworkReply::NoError) {
		mDebug("Error in send/get command %d", reply->error());
		return;
	}
	QByteArray ba = reply->readAll();
	if (ba.contains("root.ERR.no=4"))
		return;
	protocol->dataReady(ba);
}

void PtzpHttpTransport::sendPostMessage(const QByteArray &ba)
{
	netman->post(request, ba);
}

void PtzpHttpTransport::sendGetMessage(const QByteArray &ba)
{
	QString data = QString::fromUtf8(ba);
	if (data.contains("?")) {
		QString path = data.split("?").first();
		QUrl url = request.url();
		url.setPath(path, QUrl::ParsingMode::StrictMode);
		url.setQuery(data.split("?").last());
		request.setUrl(url);
		netman->get(request);
		url.setQuery("");
		request.setUrl(url);
	}

}

void PtzpHttpTransport::callback()
{
	QByteArray m = PtzpTransport::queueFreeCallback();
	if (cgiFlag) {
		emit sendGetMessage2Main(m);
		return;
	}
	send(m.data(), m.size());
}
