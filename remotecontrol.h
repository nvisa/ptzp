#ifndef REMOTECONTROL_H
#define REMOTECONTROL_H

#include <QHash>
#include <QObject>
#include <QHostAddress>

class QTcpServer;
class QUdpSocket;
class ApplicationSettings;

class RemoteControl : public QObject
{
	Q_OBJECT
public:
	explicit RemoteControl(QObject *parent = 0);

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

	QTcpServer *serv;
	QUdpSocket *sock;
};

#endif // REMOTECONTROL_H
