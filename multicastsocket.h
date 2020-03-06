#ifndef MULTICASTSOCKET_H
#define MULTICASTSOCKET_H

#include <QUdpSocket>

class MulticastSocket : public QUdpSocket
{
	Q_OBJECT
public:
	explicit MulticastSocket(QObject *parent = 0);

	void setTTLValue(int val);
	bool joinMulticastGroupLinux(const QHostAddress & groupAddress, const QHostAddress &ifaceIp);

signals:

public slots:
protected:
	int ttl;
};

#endif // MULTICASTSOCKET_H
