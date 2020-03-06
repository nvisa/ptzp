#ifndef PSEUDOONVIFDEVICEMGTSYSTEMINTERFACE_H
#define PSEUDOONVIFDEVICEMGTSYSTEMINTERFACE_H

#include <interfaces/onvifdevicemgtsysteminterface.h>

class PseudoOnvifDeviceMgtSystemInterface : public OnvifDeviceMgtSystemInterface
{
public:
	PseudoOnvifDeviceMgtSystemInterface();

	int createUser(const QString &userName, const QString &passwd, int level);
	int deleteUser(const QString &userName);
	QStringList getUserNames();
	int getUserLevel(const QString &userName);
	QStringList getSystemBackupNames();
	QByteArray getSystemBackup(const QString &name);
	int restoreSystem(const QString &name);
	int restoreSystem(const QByteArray &ba);
	int restoreToFactoryDefaults(bool hard);
	QString getSystemLog();
	QString getAccessLog();
	QStringList getApplicationLogList();
	QString getApplicationLog(const QString &name);
};

#endif // PSEUDOONVIFDEVICEMGTSYSTEMINTERFACE_H
