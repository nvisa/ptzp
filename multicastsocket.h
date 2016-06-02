#ifndef MULTICASTSOCKET_H
#define MULTICASTSOCKET_H

#include <QUdpSocket>

class MulticastSocket : public QUdpSocket
{
	Q_OBJECT
public:
	explicit MulticastSocket(QObject *parent = 0);

	bool joinMulticastGroupLinux(const QHostAddress & groupAddress, const QHostAddress &ifaceIp);

signals:

public slots:

};

#endif // MULTICASTSOCKET_H
