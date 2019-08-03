#include "ptzphttptransport.h"
#include <ecl/debug.h>

#include <QNetworkReply>
class PtzpHttpTransportPriv
{
public:
	int port;
	QString uri;
	QString username;
	QString password;
};

PtzpHttpTransport::PtzpHttpTransport(LineProtocol proto, QObject *parent)
	: QObject(parent),
	  PtzpTransport(proto)
{
	priv = new PtzpHttpTransportPriv();
	priv->port = 80;
	contentType = Unknown;
	netman = NULL;
	timer = new QTimer();
	timer->start(periodTimer);
	connect(timer, SIGNAL(timeout()), SLOT(callback()));
}

int PtzpHttpTransport::connectTo(const QString &targetUri)
{
	if (targetUri.isEmpty())
		return -1;
	if (targetUri.contains("?")) {
		QStringList flds = targetUri.split("?");
		priv->uri = flds.at(0);
		if (!priv->uri.contains("http://"))
			priv->uri = "http://" + priv->uri;
		priv->username = flds.at(1);
		priv->password = flds.at(2);
		if (flds.size() > 3)
			priv->port = flds.at(3).toInt();
	} else
		priv->uri = targetUri;
	mInfo("Url '%s', port '%d', Username '%s', Password '%s'", qPrintable(priv->uri), priv->port, qPrintable(priv->username), qPrintable(priv->password));

	if (contentType == AppXFormUrlencoded)
		request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
	else if (contentType == AppJson)
		request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	else {
		mDebug("unknown content type");
		return -EPROTO;
	}
	if (!priv->username.isEmpty())
		request.setRawHeader("Authorization", "Basic "
							 + QString("%1:%2").arg(priv->username).arg(priv->password).toLatin1().toBase64());
	QUrl url;
	url.setUrl(priv->uri);
	url.setPort(priv->port);
	request.setUrl(url);

	netman = new QNetworkAccessManager(this);
	connect(this, SIGNAL(sendPostMessage2Main(QByteArray)), SLOT(sendPostMessage(QByteArray)));
	connect(this, SIGNAL(sendGetMessage2Main(QByteArray)), SLOT(sendGetMessage(QByteArray)));
	connect(netman, SIGNAL(finished(QNetworkReply*)), SLOT(dataReady(QNetworkReply *)));
	return 0;
}

int PtzpHttpTransport::send(const char *bytes, int len)
{
	switch (contentType) {
	case AppJson: {
		QByteArray params = prepareAppJsonTypeMessage(QByteArray(bytes, len));
		emit sendPostMessage2Main(params);
		break;
	}
	case AppXFormUrlencoded:
		emit sendPostMessage2Main(QByteArray(bytes, len));
		break;
	default:
		break;
	}
	return len;
}

void PtzpHttpTransport::setContentType(PtzpHttpTransport::KnownContentTypes type)
{
	contentType = type;
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

void PtzpHttpTransport::sendPostMessage(const QByteArray &ba)
{
	netman->post(request, ba);
}

void PtzpHttpTransport::sendGetMessage(const QByteArray &ba)
{
	QString data = QString::fromUtf8(ba);
	if (data.contains("?")) {
		QStringList flds = data.split("?");
		QString path = flds.first();
		QString query = flds.last();
		QUrl url = request.url();
		url.setPath(path, QUrl::ParsingMode::StrictMode);
		url.setQuery(query);
		request.setUrl(url);

		netman->get(request);
		url.setQuery("");
		request.setUrl(url);
	}
}

void PtzpHttpTransport::callback()
{
	QByteArray m = PtzpTransport::queueFreeCallback();
	if (contentType == AppJson) {
		emit sendGetMessage2Main(m);
		return;
	}
	send(m.data(), m.size());
}

QByteArray PtzpHttpTransport::prepareAppJsonTypeMessage(const QByteArray &ba)
{
	QString data = QString::fromUtf8(ba);
	if (data.contains("?")) {
		QString path = data.split("?").first();
		QString params = data.split("?").last();
		QUrl url = request.url();
		url.setPath(path, QUrl::ParsingMode::StrictMode);
		request.setUrl(url);
		return params.toLatin1();
	}
}
