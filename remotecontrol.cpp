#include "remotecontrol.h"
#include "debug.h"
#include "settings/applicationsettings.h"

#include <QUdpSocket>
#include <QTcpSocket>
#include <QTcpServer>
#include <QCryptographicHash>

#include <errno.h>

static QString createRandomString(int len)
{
	static QString chars = "QWERTYUIOPASDFGHJKLZXCVBNM01234567890";
	QString s;
	for (int i = 0; i < len; i++) {
		int ch = rand() % chars.size();
		s.append(chars[ch]);
	}
	return s;
}

static QByteArray digestMd5ResponseHelper(
		const QByteArray &alg,
		const QByteArray &userName,
		const QByteArray &realm,
		const QByteArray &password,
		const QByteArray &nonce,       /* nonce from server */
		const QByteArray &nonceCount,  /* 8 hex digits */
		const QByteArray &cNonce,      /* client nonce */
		const QByteArray &qop,         /* qop-value: "", "auth", "auth-int" */
		const QByteArray &method,      /* method from the request */
		const QByteArray &digestUri,   /* requested URL */
		const QByteArray &hEntity       /* H(entity body) if qop="auth-int" */
		)
{
	QCryptographicHash hash(QCryptographicHash::Md5);
	hash.addData(userName);
	hash.addData(":", 1);
	hash.addData(realm);
	hash.addData(":", 1);
	hash.addData(password);
	QByteArray ha1 = hash.result();
	if (alg.toLower() == "md5-sess") {
		hash.reset();
		// RFC 2617 contains an error, it was:
		// hash.addData(ha1);
		// but according to the errata page at http://www.rfc-editor.org/errata_list.php, ID 1649, it
		// must be the following line:
		hash.addData(ha1.toHex());
		hash.addData(":", 1);
		hash.addData(nonce);
		hash.addData(":", 1);
		hash.addData(cNonce);
		ha1 = hash.result();
	};
	ha1 = ha1.toHex();

	// calculate H(A2)
	hash.reset();
	hash.addData(method);
	hash.addData(":", 1);
	hash.addData(digestUri);
	if (qop.toLower() == "auth-int") {
		hash.addData(":", 1);
		hash.addData(hEntity);
	}
	QByteArray ha2hex = hash.result().toHex();

	// calculate response
	hash.reset();
	hash.addData(ha1);
	hash.addData(":", 1);
	hash.addData(nonce);
	hash.addData(":", 1);
	if (!qop.isNull()) {
		hash.addData(nonceCount);
		hash.addData(":", 1);
		hash.addData(cNonce);
		hash.addData(":", 1);
		hash.addData(qop);
		hash.addData(":", 1);
	}
	hash.addData(ha2hex);
	return hash.result().toHex();
}

RemoteControl::RemoteControl(QObject *parent, KeyValueInterface *iface) :
	QObject(parent)
{
	kviface = iface;
	nonSecureCompat = true;
}

void RemoteControl::setAuthCredentials(const QString uname, const QString pass)
{
	username = uname;
	password = pass;
}

void RemoteControl::setNonSecureCompat(bool v)
{
	nonSecureCompat = v;
}

bool RemoteControl::listen(const QHostAddress &address, quint16 port)
{
	serv = new QTcpServer(this);
	connect(serv, SIGNAL(newConnection()), SLOT(newConnection()));

	sock = new QUdpSocket(this);
	sock->bind(QHostAddress::Any, port);
	connect(sock, SIGNAL(readyRead()), SLOT(readPendingDatagrams()));

	return serv->listen(address, port);
}

void RemoteControl::readPendingDatagrams()
{
	while (sock->hasPendingDatagrams()) {
		QByteArray datagram;
		datagram.resize(sock->pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;

		sock->readDatagram(datagram.data(), datagram.size(),
								&sender, &senderPort);
		QByteArray ba = processUdpMessage(datagram).toUtf8();
		if (ba.size())
			sock->writeDatagram(ba, sender, senderPort);
	}
}

void RemoteControl::newConnection()
{
	QTcpSocket *sock = serv->nextPendingConnection();
	connect(sock, SIGNAL(disconnected()), sock, SLOT(deleteLater()));
	connect(sock, SIGNAL(readyRead()), SLOT(dataReady()));
	mDebug("client %s connected", qPrintable(sock->peerAddress().toString()));
}

void RemoteControl::dataReady()
{
	QTcpSocket *sock = (QTcpSocket *)sender();

	while (sock->canReadLine()) {
		QString mes = QString::fromUtf8(sock->readLine()).trimmed();
		if (mes.isEmpty())
			continue;
		QStringList flds = mes.split(" ");
		mDebug("%s:%s < %s", qPrintable(sock->peerAddress().toString()), qPrintable(QString::number(sock->peerPort())), qPrintable(mes));
		if (flds.size() < 2) {
			sock->write(QString("error:no enough arguments\n").toUtf8());
			continue;
		}
		if (flds[0] == "gets" || flds[1] != "sets") {
			if (!checkAuth(sock, flds)) {
				/* wrong or none auth field notify */
				if (!auths.contains(sock)) {
					AuthStruct *s = new AuthStruct;
					s->t.start();
					auths.insert(sock, s);
				}
				AuthStruct *s = auths[sock];
				if (s->nonce.isEmpty() || s->t.elapsed() > 3600 * 1000) {
					s->nonce = createRandomString(16);
					s->realm = createRandomString(8);
					s->t.restart();
				}
				sock->write(QString("noauth:once=%1:realm=%2").arg(s->nonce).arg(s->realm).toUtf8());
				return;
			}
			/* access granted */
			if (flds[0] == "gets") {
				const QString set = getSetting(flds[2].trimmed()).toString();
				sock->write(QString("gets:%1:%2\n").arg(flds[2].trimmed()).arg(set).toUtf8());
			} else if (flds[0] == "sets") {
				int err = setSetting(flds[2].trimmed(), flds[3].trimmed());
				sock->write(QString("sets:%1:%2\n").arg(flds[2].trimmed()).arg(err).toUtf8());
			} else
				sock->write(QString("error:un-supported command\n").toUtf8());
		} else if (nonSecureCompat) {
			if (flds[0] == "get") {
				const QString set = getSetting(flds[1].trimmed()).toString();
				sock->write(QString("get:%1:%2\n").arg(flds[1].trimmed()).arg(set).toUtf8());
			} else if (flds[0] == "set") {
				if (flds.size() < 3) {
					sock->write(QString("error:no enough arguments\n").toUtf8());
					continue;
				}
				int err = setSetting(flds[1].trimmed(), flds[2].trimmed());
				sock->write(QString("set:%1:%2\n").arg(flds[1].trimmed()).arg(err).toUtf8());
			} else
				sock->write(QString("error:un-supported command\n").toUtf8());
		}
	}
}

const QString RemoteControl::processUdpMessage(const QString &mes)
{
	QUdpSocket *udpSock = (QUdpSocket *)sender();
	mInfo("%s:%s < %s", qPrintable(udpSock->peerAddress().toString()), qPrintable(QString::number(udpSock->peerPort())), qPrintable(mes));
	if (mes.trimmed().isEmpty())
		return "";
	QString resp;
	QStringList fields = mes.trimmed().split(" ", QString::SkipEmptyParts);
	if (fields.size() > 1 && fields[0] == "get") {
		QString set = fields[1].trimmed();
		resp = QString("get:%1:%2").arg(set).arg(getSetting(set).toString());
	} else if (fields.size() > 2 && fields[0] == "set") {
		QString set = fields[1].trimmed();
		int err = setSetting(set, fields[2].trimmed());
		resp = QString("set:%1:%2").arg(set).arg(err);
	}
	return resp;
}

const QVariant RemoteControl::getSetting(const QString &setting)
{
	if (kviface)
		return kviface->get(setting);
	return ApplicationSettings::instance()->getm(setting);
}

int RemoteControl::setSetting(const QString &setting, const QVariant &value)
{
	if (kviface)
		return kviface->set(setting, value);
	QVariant var = getSetting(setting);
	QVariant next = value;
	if (next.canConvert(var.type()))
		next.convert(var.type());
	return ApplicationSettings::instance()->setm(setting, next);
}

bool RemoteControl::checkAuth(QTcpSocket *sock, const QStringList &flds)
{
	if (flds.size() < 3)
		return false;

	QString authfld = flds[1];

	if (authfld.isEmpty() || !auths.contains(sock))
		return false;

	QString realm = auths[sock]->realm;
	QString nonce = auths[sock]->nonce;
	QString method = flds[0];
	QString uri = "rtcp://";

	QByteArray response = digestMd5ResponseHelper(QByteArray(),
												  username.toLatin1(),
												  realm.toLatin1(),
												  password.toLatin1(),
												  nonce.toLatin1(),
												  QByteArray(),
												  QByteArray(),
												  QByteArray(),
												  method.toLatin1(),
												  uri.toLatin1(),
												  QByteArray()
												  );
	QString authstr = QString::fromUtf8(response);

	if (authstr != authfld)
		return false;

	return true;
}
