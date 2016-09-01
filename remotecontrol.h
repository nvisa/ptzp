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
	void addMapping(const QString &group, ApplicationSettings *);
signals:

protected slots:
	void readPendingDatagrams();
	void newConnection();
	void dataReady();

protected:
	const QString processUdpMessage(const QString &mes);
	const QVariant getSetting(const QString &setting);
	int setSetting(const QString &setting, const QVariant &value);
	ApplicationSettings * getMapping(const QString &setting);

	QTcpServer *serv;
	QUdpSocket *sock;
	QHash<QString, ApplicationSettings *> mappings;
};

#endif // REMOTECONTROL_H
