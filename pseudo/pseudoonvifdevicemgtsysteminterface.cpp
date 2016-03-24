#include "pseudoonvifdevicemgtsysteminterface.h"

#include <errno.h>

PseudoOnvifDeviceMgtSystemInterface::PseudoOnvifDeviceMgtSystemInterface()
{
}

/**
 * @brief PseudoOnvifDeviceMgtSystemInterface::createUser Creates a new user.
 * @param userName Name of the newly created user.
 * @param passwd Plain (unencrypted) password for the user.
 * @param level Security level for the user.
 * @return '0' on success, negative error code otherwise.
 *
 * This function should create a new user in the underlying backend. Depending on the backend
 * functionality passwords can be stored plain or encrypted. Security levels should be equal to
 * or greater than '0'. Lower the level, higher the security priveleges.
 */
int PseudoOnvifDeviceMgtSystemInterface::createUser(const QString &userName, const QString &passwd, int level)
{
	Q_UNUSED(userName);
	Q_UNUSED(passwd);
	Q_UNUSED(level);
	return -EPERM;
}

/**
 * @brief PseudoOnvifDeviceMgtSystemInterface::deleteUser Deletes a given user.
 * @param userName Name of the user to delete.
 * @return '0' on success, negative error code otherwise.
 */
int PseudoOnvifDeviceMgtSystemInterface::deleteUser(const QString &userName)
{
	Q_UNUSED(userName);
	return -EPERM;
}

/**
 * @brief PseudoOnvifDeviceMgtSystemInterface::getUserNames Returns all usernames present.
 * @return List of users present in the underlying backend.
 */
QStringList PseudoOnvifDeviceMgtSystemInterface::getUserNames()
{
	return QStringList();
}

/**
 * @brief PseudoOnvifDeviceMgtSystemInterface::getUserLevel
 * @param userName Name of the user.
 * @return Level of the given user or negative error code.
 */
int PseudoOnvifDeviceMgtSystemInterface::getUserLevel(const QString &userName)
{
	Q_UNUSED(userName);
	return -EPERM;
}

/**
 * @brief PseudoOnvifDeviceMgtSystemInterface::getSystemBackupNames Returns backups present.
 * @return List of backup names.
 *
 * This function should return currently present backups in the system. Return names
 * will be used by other backup management functions like getSystemBackup() and restoreSystem().
 */
QStringList PseudoOnvifDeviceMgtSystemInterface::getSystemBackupNames()
{
	return QStringList();
}

/**
 * @brief PseudoOnvifDeviceMgtSystemInterface::getSystemBackup Returns data for a selected backup.
 * @param name Name of the backup, one of the values returned by getSystemBackupNames().
 * @return Binary data for the desired backup. Empty QByteArray means an error.
 *
 * This function should return an empty QByteArray in case given backup name is not
 * present in the system.
 */
QByteArray PseudoOnvifDeviceMgtSystemInterface::getSystemBackup(const QString &name)
{

}

/**
 * @brief PseudoOnvifDeviceMgtSystemInterface::restoreSystem Restores system from a given backup.
 * @param name Backup name to restore.
 * @return '0' on success, negative error code otherwise.
 *
 * This function should restore system from a given backup name.
 */
int PseudoOnvifDeviceMgtSystemInterface::restoreSystem(const QString &name)
{

}

/**
 * @brief PseudoOnvifDeviceMgtSystemInterface::restoreSystem
 * @param ba
 * @return
 */
int PseudoOnvifDeviceMgtSystemInterface::restoreSystem(const QByteArray &ba)
{

}

int PseudoOnvifDeviceMgtSystemInterface::restoreToFactoryDefaults(bool hard)
{

}

QString PseudoOnvifDeviceMgtSystemInterface::getSystemLog()
{

}

QString PseudoOnvifDeviceMgtSystemInterface::getAccessLog()
{

}

QStringList PseudoOnvifDeviceMgtSystemInterface::getApplicationLogList()
{

}

QString PseudoOnvifDeviceMgtSystemInterface::getApplicationLog(const QString &name)
{

}
