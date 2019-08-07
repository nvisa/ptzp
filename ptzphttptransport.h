#ifndef PTZPHTTPTRANSPORT_H
#define PTZPHTTPTRANSPORT_H

#include <QTimer>
#include <QMutex>
#include <QObject>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

#include "ptzptransport.h"

class PtzpHttpTransportPriv;
class PtzpHttpTransport : public QObject, public PtzpTransport
{
	Q_OBJECT
public:
	enum KnownContentTypes {
		AppJson,
		AppXFormUrlencoded,
		TextPlain,
		Unknown
	};
	enum AuthorizationTypes {
		Basic,
		Digest
	};

	explicit PtzpHttpTransport(LineProtocol proto, QObject *parent = nullptr);
	int connectTo(const QString &targetUri);
	int send(const char *bytes, int len);
	void setContentType(KnownContentTypes type);
	void setAuthorizationType(PtzpHttpTransport::AuthorizationTypes type);
signals:
	void sendPostMessage2Main(const QByteArray &ba);
	void sendGetMessage2Main(const QByteArray &ba);
protected slots:
	void dataReady(QNetworkReply *reply);
	void callback();
	void sendPostMessage(const QByteArray &ba);
	void sendGetMessage(const QByteArray &ba);
protected:
	QByteArray prepareMessage(const QByteArray &ba);
	AuthorizationTypes authType;
	KnownContentTypes contentType;
	PtzpHttpTransportPriv *priv;
private:
	QMutex l;
	QTimer *timer;
	QNetworkRequest request;
	QNetworkAccessManager *netman;
};

#endif // PTZPHTTPTRANSPORT_H
