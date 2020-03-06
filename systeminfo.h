#ifndef SYSTEMINFO_H
#define SYSTEMINFO_H

#include <QObject>
#include <QElapsedTimer>

class QFile;
class QTimer;

class SystemInfo : public QObject
{
	Q_OBJECT
public:
	enum PathMountState {
		READ_ONLY,
		WRITE_ONLY,
		READ_WRITE,
		UMOUNT
	};
	static int getFreeMemory();
	static int getTVPVersion();
	static int getTVP5158Version(int addr);
	static int getADV7842Version();
	static int getTFP410DevId();
	static int getUptime();
	static int getCpuLoad();
	static SystemInfo::PathMountState getMountState(const QString &dev);
	static int remountState(const QString &dev, SystemInfo::PathMountState state);
	static int mount(const QString &dev, const QString &dest, SystemInfo::PathMountState state);
	static int umount(const QString &dest);
signals:
	
public slots:
private:
	explicit SystemInfo(QObject *parent = 0);
	int getProcFreeMemInfo();

	class CpuInfo
	{
	public:
		CpuInfo();

		int total;
		int count;
		//int avg;
		int load;
		int userTime;
		int prevTotal;
		int t0Idle;
		int t0NonIdle;
		int lastNonIdle;
		int lastIdle;
		QFile *proc;

		int getAvgLoadFromProc();
		int getInstLoadFromProc();
	};

	static SystemInfo inst;
	QFile *meminfo;
	QElapsedTimer memtime;
	int lastMemFree;
	CpuInfo cpu;
};

#endif // SYSTEMINFO_H
