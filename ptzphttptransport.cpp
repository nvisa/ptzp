#include "ptzphttptransport.h"
#include "debug.h"

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
	: QObject(parent), PtzpTransport(proto)
{
	priv = new PtzpHttpTransportPriv();
	priv->port = 80;
	contentType = Unknown;
	authType = Basic;
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
	mInfo("Url '%s', port '%d', Username '%s', Password '%s'",
		  qPrintable(priv->uri), priv->port, qPrintable(priv->username),
		  qPrintable(priv->password));

	if (contentType == AppXFormUrlencoded)
		request.setHeader(QNetworkRequest::ContentTypeHeader,
						  "application/x-www-form-urlencoded");
	else if (contentType == AppJson)
		request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	else if (contentType == TextPlain)
		request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
	else {
		mDebug("unknown content type");
		return -EPROTO;
	}

	QUrl url;
	url.setUrl(priv->uri);
	url.setPort(priv->port);

	if (!priv->username.isEmpty()) {
		if (authType == Basic)
			request.setRawHeader("Authorization", "Basic "
								+ QString("%1:%2").arg(priv->username).arg(priv->password).toLatin1().toBase64());
		else if (authType = Digest) {
			url.setUserName(priv->username);
			url.setPassword(priv->password);
		}
	}
	request.setUrl(url);

	netman = new QNetworkAccessManager(this);
	connect(this, SIGNAL(sendPostMessage2Main(QByteArray)),
			SLOT(sendPostMessage(QByteArray)));
	connect(this, SIGNAL(sendGetMessage2Main(QByteArray)),
			SLOT(sendGetMessage(QByteArray)));
	connect(netman, SIGNAL(finished(QNetworkReply *)),
			SLOT(dataReady(QNetworkReply *)));
	return 0;
}

int PtzpHttpTransport::send(const char *bytes, int len)
{
	switch (contentType) {
	case AppJson: {
		QByteArray params = prepareMessage(QByteArray(bytes, len));
		emit sendPostMessage2Main(params);
		break;
	}
	case AppXFormUrlencoded:
		emit sendPostMessage2Main(QByteArray(bytes, len));
		break;
	case TextPlain: {
		QByteArray params = prepareMessage(QByteArray(bytes, len));
		emit sendPostMessage2Main(params);
		break;
	}
	default:
		break;
	}
	return len;
}

void PtzpHttpTransport::setContentType(
	PtzpHttpTransport::KnownContentTypes type)
{
	contentType = type;
}

void PtzpHttpTransport::setAuthorizationType(PtzpHttpTransport::AuthorizationTypes type)
{
	authType = type;
}

void PtzpHttpTransport::dataReady(QNetworkReply *reply)
{
	if (reply->error() == QNetworkReply::NoError) {
		protocol->dataReady(reply->readAll());
	} else {
		mDebug("Error in URL '%s', error no: '%d' error str: '%s'",
				qPrintable(reply->url().url()), reply->error(),
				qPrintable(reply->errorString()));
	}
	reply->deleteLater();
}

void PtzpHttpTransport::sendPostMessage(const QByteArray &ba)
{
	if (ba.isEmpty())
		return;
	l.lock();
	netman->post(request, ba);
	l.unlock();
}

void PtzpHttpTransport::sendGetMessage(const QByteArray &ba)
{
	QString data = QString::fromUtf8(ba);
	if (data.contains("?")) {
		QStringList flds = data.split("?");
		QString path = flds.first();
		QString query = flds.last();
		l.lock();
		QUrl url = request.url();
		url.setPath(path, QUrl::ParsingMode::StrictMode);
		url.setQuery(query);
		request.setUrl(url);

		netman->get(request);
		url.setQuery("");
		request.setUrl(url);
		l.unlock();
	}
}

void PtzpHttpTransport::callback()
{
	QByteArray m = PtzpTransport::queueFreeCallback();
	if (m.isEmpty())
		return;
	if (contentType == AppJson)
		emit sendGetMessage2Main(m);
	else if (contentType == AppXFormUrlencoded)
		emit sendPostMessage2Main(m);
	timer->setInterval(periodTimer);
	return;
}

QByteArray PtzpHttpTransport::prepareMessage(const QByteArray &ba)
{
	QString data = QString::fromUtf8(ba);
	if (data.contains("?")) {
		QString path = data.split("?").first();
		QString params = data.split("?").last();
		l.lock();
		QUrl url = request.url();
		url.setPath(path, QUrl::ParsingMode::StrictMode);
		request.setUrl(url);
		l.unlock();
		return params.toLatin1();
	}
	return QByteArray();
}

