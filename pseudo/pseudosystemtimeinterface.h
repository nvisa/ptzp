#ifndef PSEUDOSYSTEMTIMEINTERFACE_H
#define PSEUDOSYSTEMTIMEINTERFACE_H

#include <interfaces/systemtimeinterface.h>

class PseudoSystemTimeInterface : public SystemTimeInterface
{
public:
	PseudoSystemTimeInterface();

	virtual QDateTime getSystemTime();
	virtual QString getTimeZone();
	virtual int setSystemTime(const QDateTime &dt);
	virtual int setTimeZone(const QString &str);
	virtual TimeKeepingMethod getTimeKeepingMethod();
	virtual int setTimeKeepingMethod(TimeKeepingMethod m);
	virtual int setTimeServer(int index, const QString &val);
	virtual QString getTimeServer(int index);
	virtual QStringList getTimeServers();
	virtual bool isDaylightSavingOn();
};

#endif // PSEUDOSYSTEMTIMEINTERFACE_H
