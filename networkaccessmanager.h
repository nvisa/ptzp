#ifndef NETWORKACCESSMANAGER_H
#define NETWORKACCESSMANAGER_H

#include <QObject>

#include <QUrl>
#include <QNetworkRequest>

class QNetworkReply;
class QNetworkAccessManager;
class NetworkAccessManager : public QObject
{
	Q_OBJECT
public:
	explicit NetworkAccessManager(QObject *parent = 0);
	void setUrl(const QString &host, int port = 80);
	void setAuthenticationInfo(const QString &user, const QString &pass);
	void setPath(const QString &path);
	void GET(QString data = "");
	void POST(const QString &data);
signals:

public slots:
protected slots:
	void replyFinished(QNetworkReply *reply);
private:
	QNetworkAccessManager *netman;
	QUrl url;
	QNetworkRequest request;
	QString username;
	QString password;
	QString hostname;
	QString filePath;
};

#endif // NETWORKACCESSMANAGER_H
