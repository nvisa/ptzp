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

#include <QNetworkReply>
#include <QNetworkAccessManager>

NetworkAccessManager::NetworkAccessManager(QObject *parent) :
	QObject(parent)
{
	netman = new QNetworkAccessManager(this);
	connect(netman, SIGNAL(finished(QNetworkReply*)), SLOT(replyFinished(QNetworkReply*)));
}

void NetworkAccessManager::setUrl(const QString &host, int port)
{
	hostname = host;
	url.setUrl(QString("http://%1").arg(host));
	url.setPort(port);
	url.setUserName(username);
	url.setPassword(password);
}

void NetworkAccessManager::setAuthenticationInfo(const QString &user, const QString &pass)
{
	username = user;
	password = pass;
}

void NetworkAccessManager::setPath(const QString &path)
{
	filePath = path;
	url.setPath(path);
}

void NetworkAccessManager::replyFinished(QNetworkReply *reply)
{
	if (reply->error() != QNetworkReply::NetworkError::NoError) {
		mDebug("Network connection error, %s", qPrintable(reply->errorString()));
		return;
	}
	QString data = reply->readAll();
	mDebug("Reply Data : %s", qPrintable(data));
}

void NetworkAccessManager::GET(QString data)
{
	mDebug(": host %1, user %2, pass %3, path %4, data %5",
			qPrintable(hostname),
			qPrintable(username),
			qPrintable(password),
			qPrintable(filePath),
			qPrintable(data));
	request.setUrl(url);
	netman->get(request);
}

void NetworkAccessManager::POST(const QString &data)
{
	mDebug(": host %1, user %2, pass %3, path %4, data %5",
			qPrintable(hostname),
			qPrintable(username),
			qPrintable(password),
			qPrintable(filePath),
			qPrintable(data));
	request.setUrl(url);
	netman->post(request, data.toLatin1());
}
