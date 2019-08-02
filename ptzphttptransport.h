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
signals:
	void sendSocketMessage2Main(const QByteArray &ba);

public slots:
protected slots:
	void dataReady(QNetworkReply *reply);
	void sendSocketMessage(const QByteArray &ba);
	void callback();
protected:
	QNetworkRequest request;
	QNetworkAccessManager *netman;
	QTimer *timer;
};

#endif // PTZPHTTPTRANSPORT_H
