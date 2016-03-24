#ifndef SYSTEMTIMEINTERFACE_H
#define SYSTEMTIMEINTERFACE_H

#include <QDateTime>
#include <QStringList>

class SystemTimeInterface
{
public:
	enum TimeKeepingMethod {
		TIME_MANUAL,
		TIME_NTP,
	};

	virtual QDateTime getSystemTime() = 0;
	virtual QString getTimeZone() = 0;
	virtual int setSystemTime(const QDateTime &dt) = 0;
	virtual int setTimeZone(const QString &str) = 0;
	virtual TimeKeepingMethod getTimeKeepingMethod() = 0;
	virtual int setTimeKeepingMethod(TimeKeepingMethod m) = 0;
	virtual int setTimeServer(int index, const QString &val) = 0;
	virtual QString getTimeServer(int index) = 0;
	virtual QStringList getTimeServers() = 0;
	virtual bool isDaylightSavingOn() = 0;
};

#endif // SYSTEMTIMEINTERFACE_H
