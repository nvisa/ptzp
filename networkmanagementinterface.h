#ifndef NETWORKMANAGEMENTINTERFACE_H
#define NETWORKMANAGEMENTINTERFACE_H

#include <QString>
#include <QStringList>
#include <QNetworkInterface>

class NetworkManagementInterface
{
public:
	enum AddressMethod {
		DHCP = 1,
		STATIC
	};

	NetworkManagementInterface();

	virtual QStringList getStaticDnsAddresses() = 0;
	virtual QStringList getDhcpDnsAddresses() = 0;
	virtual QStringList getStaticNtpServers() = 0;
	virtual QStringList getDhcpNtpServers() = 0;
	virtual QString getDefaultGateway() = 0;
	virtual int setDefaultGateway(const QString &addr) = 0;
	virtual AddressMethod getAddressMethod(const QNetworkInterface &iface) = 0;
	virtual QString getHostname() = 0;
	virtual int setHostname(const QString &name) = 0;
	virtual int setHostnameDhcp() = 0;
};

#endif // NETWORKMANAGEMENTINTERFACE_H
