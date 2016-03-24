#ifndef PSEUDONETWORKMANAGEMENTINTERFACE_H
#define PSEUDONETWORKMANAGEMENTINTERFACE_H

#include <interfaces/networkmanagementinterface.h>

class PseudoNetworkManagementInterface : public NetworkManagementInterface
{
public:
	PseudoNetworkManagementInterface();

	QStringList getStaticDnsAddresses();
	QStringList getDhcpDnsAddresses();
	QStringList getStaticNtpServers();
	QStringList getDhcpNtpServers();
	QString getDefaultGateway();
	int setDefaultGateway(const QString &addr);
	AddressMethod getAddressMethod(const QNetworkInterface &iface);
	QString getHostname();
	int setHostname(const QString &name);
	int setHostnameDhcp();
};

#endif // PSEUDONETWORKMANAGEMENTINTERFACE_H
