#include "pseudonetworkmanagementinterface.h"

#include <errno.h>

PseudoNetworkManagementInterface::PseudoNetworkManagementInterface()
{
}

/**
 * @brief PseudoNetworkManagementInterface::getStaticDnsAddresses Returns system's static DNS server addresses.
 * @return IPv4 addresses of all static DNS servers used by the system.
 *
 * This function should return only the static DNS addresses configured. DHCP retrieved addresses
 * should not be in the list of return addresses. All addresses should be IPv4.
 */
QStringList PseudoNetworkManagementInterface::getStaticDnsAddresses()
{
	return QStringList();
}

/**
 * @brief PseudoNetworkManagementInterface::getDhcpDnsAddresses Returns DHCP configured DNS server addresses.
 * @return IPv4 addresses DHCP retrieved DNS servers.
 *
 * This function should not return static DNS configurations, only the ones retrieved from a DHCP
 * server should be in the list. Addresses should be IPv4.
 */
QStringList PseudoNetworkManagementInterface::getDhcpDnsAddresses()
{
	return QStringList();
}

/**
 * @brief PseudoNetworkManagementInterface::getStaticNtpServers Returns static NTP server addresses.
 * @return IPv4 addresses of static NTP servers.
 *
 * DHCP retrieved addresses should not be in the returned values.
 */
QStringList PseudoNetworkManagementInterface::getStaticNtpServers()
{
	return QStringList();
}

/**
 * @brief PseudoNetworkManagementInterface::getDhcpNtpServers Returns DHCP configured NTP server addresses.
 * @return IPv4 addresses of DHCP retrieved NTP servers.
 *
 * Statically configured NTP servers should not be present in the returned list.
 */
QStringList PseudoNetworkManagementInterface::getDhcpNtpServers()
{
	return QStringList();
}

/**
 * @brief PseudoNetworkManagementInterface::getDefaultGateway Returns system default gateway.
 * @return IPv4 address of current network gateway.
 */
QString PseudoNetworkManagementInterface::getDefaultGateway()
{
	return QString();
}

/**
 * @brief PseudoNetworkManagementInterface::setDefaultGateway Changes systems default gateway.
 * @param addr IPv4 address of new gateway.
 * @return '0' on success, negative error code otherwise.
 */
int PseudoNetworkManagementInterface::setDefaultGateway(const QString &addr)
{
	Q_UNUSED(addr);
	return -EPERM;
}

/**
 * @brief PseudoNetworkManagementInterface::getAddressMethod Returns current network addressing method.
 * @param iface Network interface to query for.
 * @return Network management method for the given interface.
 */
NetworkManagementInterface::AddressMethod PseudoNetworkManagementInterface::getAddressMethod(const QNetworkInterface &iface)
{
	Q_UNUSED(iface);
	return STATIC;
}

/**
 * @brief PseudoNetworkManagementInterface::getHostname Returns system's current hostname.
 * @return Hostname of the system.
 */
QString PseudoNetworkManagementInterface::getHostname()
{
	return QString();
}

/**
 * @brief PseudoNetworkManagementInterface::setHostname Changes system's current hostname.
 * @param name New hostname.
 * @return '0' on success, negative error code otherwise.
 */
int PseudoNetworkManagementInterface::setHostname(const QString &name)
{
	Q_UNUSED(name);
	return -EPERM;
}

/**
 * @brief PseudoNetworkManagementInterface::setHostnameDhcp Enables hostname retrieval from the DHPC server.
 * @return '0' on success, negative error code otherwise.
 *
 * This function should flag DHCP implementation to get hostname from the DHCP server but it should not
 * restart DHCP and IP assignment. Change should be active next time DHCP address is updated.
 */
int PseudoNetworkManagementInterface::setHostnameDhcp()
{
	return -EPERM;
}
