#ifndef REMOTETCPCONNECTION_H
#define REMOTETCPCONNECTION_H

#include <QTimer>
#include <QTcpSocket>
#include <QEventLoop>
#include <QStringList>
#include <QHostAddress>

class RemoteTcpConnection : public QTcpSocket
{
	Q_OBJECT
public:
	explicit RemoteTcpConnection(QObject *parent = 0);
	QString get(const QString &key);
	int set(const QString &key, const QString &value);
	void setTarget(const QString &targetIp, int port);

signals:

protected slots:
	void dataReady();
	void connectedTo();
	void disconnectedFrom();

protected:
	QHostAddress target;
	QTimer timer;
	quint16 dstPort;
	QEventLoop el;
	QStringList resp;
};

#endif // REMOTETCPCONNECTION_H
