#ifndef REMOTECONTROL_H
#define REMOTECONTROL_H

#include <QHash>
#include <QObject>
#include <QHostAddress>
#include <QElapsedTimer>

#include <ecl/interfaces/keyvalueinterface.h>

class QTcpSocket;
class QTcpServer;
class QUdpSocket;
class ApplicationSettings;

class RemoteControl : public QObject
{
	Q_OBJECT
public:
	explicit RemoteControl(QObject *parent = 0, KeyValueInterface *iface = 0);
	void setAuthCredentials(const QString uname, const QString pass);
	void setNonSecureCompat(bool v);

	bool listen(const QHostAddress &address, quint16 port);
signals:

protected slots:
	void readPendingDatagrams();
	void newConnection();
	void dataReady();

protected:
	const QString processUdpMessage(const QString &mes);
	const QVariant getSetting(const QString &setting);
	int setSetting(const QString &setting, const QVariant &value);
	bool checkAuth(QTcpSocket *sock, const QStringList &flds);

	QTcpServer *serv;
	QUdpSocket *sock;
	KeyValueInterface *kviface;
	bool nonSecureCompat;
	struct AuthStruct {
		QString nonce;
		QString realm;
		QElapsedTimer t;
	};
	QString username;
	QString password;

	QHash<QTcpSocket *, AuthStruct *> auths;
};

#endif // REMOTECONTROL_H
