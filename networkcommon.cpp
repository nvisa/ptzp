#include "networkcommon.h"

#include <QNetworkInterface>

NetworkCommon::NetworkCommon()
{
}

QHostAddress NetworkCommon::findIp(const QString &ifname)
{
	QHostAddress myIpAddr;
	/* Let's find our IP address */
	foreach (const QNetworkInterface iface, QNetworkInterface::allInterfaces()) {
		if (iface.name() != ifname)
			continue;
		if (iface.addressEntries().size()) {
			myIpAddr = iface.addressEntries().at(0).ip();
			break;
		}
	}
	return myIpAddr;
}

QString NetworkCommon::findMacAddr(const QString &ifname)
{
	/* Let's find our IP address */
	foreach (const QNetworkInterface iface, QNetworkInterface::allInterfaces()) {
		if (iface.name() != ifname)
			continue;
		return iface.hardwareAddress();
	}
	return "";
}
