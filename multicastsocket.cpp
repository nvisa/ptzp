#include "multicastsocket.h"
#include "debug.h"

#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <QNetworkInterface>

MulticastSocket::MulticastSocket(QObject *parent) :
	QUdpSocket(parent)
{
	ttl = 5;
}

void MulticastSocket::setTTLValue(int val)
{
	ttl = val;
}

bool MulticastSocket::joinMulticastGroupLinux(const QHostAddress &groupAddress, const QHostAddress &ifaceIp)
{
	int sd = socketDescriptor();
	int reuse = 1;
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
	char loopch = 0;
	setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch));
	/* adjust ttl */
	setsockopt(sd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
	/* set target interface */
	struct in_addr localInterface;
	localInterface.s_addr = inet_addr(qPrintable(ifaceIp.toString()));
	setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface));
	/* adjust multicast */
	ip_mreq mreq;
	memset(&mreq, 0, sizeof(ip_mreq));
	mreq.imr_multiaddr.s_addr = inet_addr(qPrintable(groupAddress.toString()));
	mreq.imr_interface.s_addr = inet_addr(qPrintable(ifaceIp.toString()));
	if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
		ffDebug() << "error joining multicast group";
		return false;
	}
	memset(&mreq, 0, sizeof(ip_mreq));
	mreq.imr_multiaddr.s_addr = inet_addr(qPrintable(groupAddress.toString()));
	mreq.imr_interface.s_addr = htons(INADDR_ANY);
	return true;
}
