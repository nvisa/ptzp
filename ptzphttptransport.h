#ifndef PTZPHTTPTRANSPORT_H
#define PTZPHTTPTRANSPORT_H

#include <QObject>
#include <ecl/ptzp/ptzptransport.h>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QTimer>

class PtzpHttpTransport : public QObject, public PtzpTransport
{
	Q_OBJECT
public:
	explicit PtzpHttpTransport(LineProtocol proto, QObject *parent = nullptr);
	int connectTo(const QString &targetUri);
	int send(const char *bytes, int len);
	void setCGIFlag(bool v);
signals:
	void sendGetMessage2Main(const QByteArray &ba);
	void sendPostMessage2Main(const QByteArray &ba);

public slots:
protected slots:
	void dataReady(QNetworkReply *reply);
	void callback();
	void sendPostMessage(const QByteArray &ba);
	void sendGetMessage(const QByteArray &ba);
protected:
	bool cgiFlag;
	QNetworkRequest request;
	QNetworkAccessManager *netman;
	QTimer *timer;
};

#endif // PTZPHTTPTRANSPORT_H
