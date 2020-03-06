#ifndef NETWORKCOMMON_H
#define NETWORKCOMMON_H

#include <QHostAddress>

class NetworkCommon
{
public:
	NetworkCommon();

	static QHostAddress findIp(const QString &ifname);
	static QString findMacAddr(const QString &ifname);
};

#endif // NETWORKCOMMON_H
