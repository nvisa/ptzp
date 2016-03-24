#include "pseudosystemtimeinterface.h"

#include <errno.h>

PseudoSystemTimeInterface::PseudoSystemTimeInterface()
{
}

/**
 * @brief PseudoSystemTimeInterface::setTimeZone Changes system's current timezone.
 * @param str New timezone information string as a posix timezone.
 * @return '0' on success, negative error code otherwise.
 *
 * This function is used to change system's current timezone. If provided timezone
 * information is not a valid one this function should return -ENOENT. If timezone
 * change is succesful it should return '0', otherwise a negative error code.
 */
int PseudoSystemTimeInterface::setTimeZone(const QString &str)
{
	Q_UNUSED(str);
	return -EPERM;
}

/**
 * @brief PseudoSystemTimeInterface::getTimeKeepingMethod Returns system's current time keeping method.
 * @return
 */
SystemTimeInterface::TimeKeepingMethod PseudoSystemTimeInterface::getTimeKeepingMethod()
{
	return TIME_MANUAL;
}

/**
 * @brief PseudoSystemTimeInterface::setTimeKeepingMethod Changes system's current time keeping method.
 * @param m
 * @return
 */
int PseudoSystemTimeInterface::setTimeKeepingMethod(SystemTimeInterface::TimeKeepingMethod m)
{
	Q_UNUSED(m);
	return -EPERM;
}

/**
 * @brief PseudoSystemTimeInterface::setTimeServer Changes system's time server.
 * @param index Time server index to change.
 * @param val IPv4 address of the new time server.
 * @return '0' on success, negative error code otherwise.
 *
 * This function changes system's time server information. 'index' is desired time server
 * to change. Time servers may be used for automatic time keeping methods like NTP. This
 * function should return -EINVAL if index is out of range of supported number of time servers.
 */
int PseudoSystemTimeInterface::setTimeServer(int index, const QString &val)
{
	Q_UNUSED(index);
	Q_UNUSED(val);
	return -EPERM;
}

/**
 * @brief PseudoSystemTimeInterface::getTimeServer Returns address of a selected time server.
 * @param index Time server index to get.
 * @return IPv4 address of the given time server.
 *
 * This function should return empty QString in case index is out of range of supported number
 * of time servers.
 */
QString PseudoSystemTimeInterface::getTimeServer(int index)
{
	Q_UNUSED(index);
	return QString();
}

/**
 * @brief PseudoSystemTimeInterface::getTimeServers Return time server addresses used by the device.
 * @return IPv4 address list of all time servers.
 */
QStringList PseudoSystemTimeInterface::getTimeServers()
{
	return QStringList();
}

/**
 * @brief PseudoSystemTimeInterface::isDaylightSavingOn Returns if daylight saving is on or off.
 * @return true if daylight saving is on, false otherwise.
 */
bool PseudoSystemTimeInterface::isDaylightSavingOn()
{
	return false;
}

/**
 * @brief PseudoSystemTimeInterface::setSystemTime Changes system current time.
 * @param dt New time/data information.
 * @return '0' on success, negative error code otherwise.
 */
int PseudoSystemTimeInterface::setSystemTime(const QDateTime &dt)
{
	Q_UNUSED(dt);
	return 0;
}

/**
 * @brief PseudoSystemTimeInterface::getTimeZone Returns system timezone information.
 * @return current timezone as a 'posix' timezone string.
 */
QString PseudoSystemTimeInterface::getTimeZone()
{
	return QString();
}

/**
 * @brief PseudoSystemTimeInterface::getSystemTime Return system current time.
 * @return current system data/time as QDateTime object.
 */
QDateTime PseudoSystemTimeInterface::getSystemTime()
{
	return QDateTime::currentDateTime();
}
