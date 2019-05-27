/*
 * This class using simple network ability from HTTP protocol. (Get file, Post file)
 * We can use two options;
 * Option 1: Get file from server..
 * Option 2: Post file from server..
 * The file like be cgi, text or etc. You can change any data on server, You must send data to POST function.
 * The data type must be compatible for the opposite network server.
 * This class support of basic network file control.
 * TODO:
 *		- Reply data parse.
 *		- function return value.(Specially post,get)
 *		- Improve class for different states
 */
#include "networkaccessmanager.h"
#include "debug.h"

#include <QNetworkAccessManager>

NetworkAccessManager::NetworkAccessManager(QObject *parent) :
	QObject(parent)
{
}

int NetworkAccessManager::post(const QString &host, const QString &uname, const QString &pass, const QString &cgipath, const QString &data)
{
	QNetworkAccessManager *netman = new QNetworkAccessManager(this);
	connect(netman, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
	QUrl url(QString("http://%1").arg(host));
	if (!uname.isEmpty())
		url.setUserName(uname);
	if (!pass.isEmpty())
		url.setPassword(pass);
	url.setPath(cgipath);
	QNetworkRequest req;
	req.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
	req.setUrl(url);
	netman->post(req, data.toLatin1());
	return 0;
}

void NetworkAccessManager::replyFinished(QNetworkReply *reply)
{
	if (reply->error() != QNetworkReply::NoError)
		mInfo("Network connection error, %s", qPrintable(reply->errorString()));
	lastError = reply->error();
	lastErrorString = reply->errorString();

	QByteArray data = reply->readAll();
	mInfo("Reply Data : %s", qPrintable(data));
	emit finished();
}
