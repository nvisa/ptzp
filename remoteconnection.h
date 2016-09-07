#ifndef REMOTECONNECTION_H
#define REMOTECONNECTION_H

#include <QUdpSocket>
#include <QEventLoop>
#include <QStringList>

class RemoteConnection : public QUdpSocket
{
	Q_OBJECT
public:
	explicit RemoteConnection(int port = 8945, QObject *parent = 0);
	QString get(const QString &key);
	int set(const QString &key, const QString &value);
	void setTarget(const QString &targetIp);
signals:
	
private slots:
	void readPendingDatagrams();
private:
	QEventLoop el;
	QHostAddress sender;
	QHostAddress target;
	quint16 senderPort;
	QStringList resp;
	int dstPort;
};

#endif // REMOTECONNECTION_H
