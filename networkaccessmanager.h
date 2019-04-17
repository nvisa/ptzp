#ifndef NETWORKACCESSMANAGER_H
#define NETWORKACCESSMANAGER_H

#include <QObject>

#include <QUrl>
#include <QNetworkReply>
#include <QNetworkRequest>

class NetworkAccessManager : public QObject
{
	Q_OBJECT
public:
	explicit NetworkAccessManager(QObject *parent = 0);

	int post(const QString &host, const QString &uname, const QString &pass, const QString &cgipath, const QString &data);
	int getLastError() { return lastError; }
	QString getLastErrorString() {return lastErrorString; }
signals:
	void finished();

protected slots:
	void replyFinished(QNetworkReply *reply);
private:
	int lastError;
	QString lastErrorString;
};

#endif // NETWORKACCESSMANAGER_H
