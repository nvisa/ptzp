#ifndef PTZPHTTPTRANSPORT_H
#define PTZPHTTPTRANSPORT_H

#include <QObject>
#include <ecl/ptzp/ptzptransport.h>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QMutex>

class PtzpHttpTransportPriv;
class PtzpHttpTransport : public QObject, public PtzpTransport
{
	Q_OBJECT
public:
	enum KnownContentTypes {
		AppJson,
		AppXFormUrlencoded,
		Unknown
	};
	explicit PtzpHttpTransport(LineProtocol proto, QObject *parent = nullptr);
	int connectTo(const QString &targetUri);
	int send(const char *bytes, int len);
	void setContentType(KnownContentTypes type);
signals:
	void sendPostMessage2Main(const QByteArray &ba);
	void sendGetMessage2Main(const QByteArray &ba);

protected slots:
	void dataReady(QNetworkReply *reply);
	void callback();
	void sendPostMessage(const QByteArray &ba);
	void sendGetMessage(const QByteArray &ba);
protected:
	PtzpHttpTransportPriv *priv;
	QNetworkRequest request;
	QNetworkAccessManager *netman;
	QTimer *timer;
	KnownContentTypes contentType;
	QByteArray prepareAppJsonTypeMessage(const QByteArray &ba);
	QMutex l;
};

#endif // PTZPHTTPTRANSPORT_H
