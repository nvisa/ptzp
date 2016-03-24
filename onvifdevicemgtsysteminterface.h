#ifndef ONVIFDEVICEMGTSYSTEMINTERFACE_H
#define ONVIFDEVICEMGTSYSTEMINTERFACE_H

#include <QDateTime>

class OnvifDeviceMgtSystemInterface
{
public:
	virtual int createUser(const QString &userName, const QString &passwd, int level) = 0;
	virtual int deleteUser(const QString &userName) = 0;
	virtual QStringList getUserNames() = 0;
	virtual int getUserLevel(const QString &userName) = 0;
	virtual QStringList getSystemBackupNames() = 0;
	virtual QByteArray getSystemBackup(const QString &name) = 0;
	virtual int restoreSystem(const QString &name) = 0;
	virtual int restoreSystem(const QByteArray &ba) = 0;
	virtual int restoreToFactoryDefaults(bool hard) = 0;
	virtual QString getSystemLog() = 0;
	virtual QString getAccessLog() = 0;
	virtual QStringList getApplicationLogList() = 0;
	virtual QString getApplicationLog(const QString &name) = 0;
};

#endif // ONVIFDEVICEMGTSYSTEMINTERFACE_H
