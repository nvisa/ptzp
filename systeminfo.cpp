#include "systeminfo.h"

#include "debug.h"

#include <QFile>
#include <QTimer>
#include <QProcess>
#include <QStringList>

#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef __arm__
#include <linux/i2c-dev.h>
#endif
#include <linux/i2c.h>

/*
 * Note that load calculation operation can last some time
 * ~5 msec, so it is best to keep timer period maximum
 */
#define PERIOD 1000
#define COUNT_END (1000 / PERIOD)

SystemInfo SystemInfo::inst;

#ifdef __arm__

static inline __s32 i2c_smbus_access(int file, char read_write, __u8 command,
									 int size, union i2c_smbus_data *data)
{
	struct i2c_smbus_ioctl_data args;

	args.read_write = read_write;
	args.command = command;
	args.size = size;
	args.data = data;
	return ioctl(file,I2C_SMBUS,&args);
}

static inline __s32 i2c_smbus_write_byte_data(int file, __u8 command,
											  __u8 value)
{
	union i2c_smbus_data data;
	data.byte = value;
	return i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
							I2C_SMBUS_BYTE_DATA, &data);
}

static inline __u8 i2c_smbus_read_byte_data(int file, __u8 command)
{
	union i2c_smbus_data data;
	int status;
	status = i2c_smbus_access(file, I2C_SMBUS_READ, command, I2C_SMBUS_BYTE_DATA,&data);
	return (status < 0) ? status : data.byte;
}

int SystemInfo::getTVPVersion()
{
	int fd = open("/dev/i2c-1", O_RDWR);

	/* With force, let the user read from/write to the registers
		 even when a driver is also running */
	if (ioctl(fd, I2C_SLAVE_FORCE, 0x5d) < 0) {
		fprintf(stderr,
				"Error: Could not set address to 0x%02x: %s\n",
				0x5f, strerror(errno));
		return -errno;
	}

	short ver = i2c_smbus_read_byte_data(fd, 0x80) << 8;
	ver |= i2c_smbus_read_byte_data(fd, 0x81);
	::close(fd);
	return ver;
}

int SystemInfo::getADV7842Version()
{
	int fd = open("/dev/i2c-1", O_RDWR);

	/* With force, let the user read from/write to the registers
		 even when a driver is also running */
	if (ioctl(fd, I2C_SLAVE_FORCE, 0x20) < 0) {
		fprintf(stderr,
				"Error: Could not set address to 0x%02x: %s\n",
				0x20, strerror(errno));
		return -errno;
	}

	short ver = i2c_smbus_read_byte_data(fd, 0xea) << 8;
	ver |= i2c_smbus_read_byte_data(fd, 0xeb);
	::close(fd);
	return ver;
}

int SystemInfo::getTFP410DevId()
{
	int fd = open("/dev/i2c-1", O_RDWR);

	/* With force, let the user read from/write to the registers
		 even when a driver is also running */
	if (ioctl(fd, I2C_SLAVE_FORCE, 0x3f) < 0) {
		fprintf(stderr,
				"Error: Could not set address to 0x%02x: %s\n",
				0x3f, strerror(errno));
		return -errno;
	}

	short ver = i2c_smbus_read_byte_data(fd, 0x03) << 8;
	ver |= i2c_smbus_read_byte_data(fd, 0x02);
	::close(fd);
	return ver;
}

int SystemInfo::getTVP5158Version(int addr)
{
	int fd = open("/dev/i2c-1", O_RDWR);

	/* With force, let the user read from/write to the registers
		 even when a driver is also running */
	if (ioctl(fd, I2C_SLAVE_FORCE, addr) < 0) {
		fprintf(stderr,
				"Error: Could not set address to 0x%02x: %s\n",
				addr, strerror(errno));
		return -errno;
	}

	short ver = i2c_smbus_read_byte_data(fd, 0x8) << 8;
	ver |= i2c_smbus_read_byte_data(fd, 0x9);
	::close(fd);
	return ver;
}

#else

int SystemInfo::getTVPVersion()
{
	return -ENOENT;
}

int SystemInfo::getADV7842Version()
{
	return -ENOENT;
}

int SystemInfo::getTVP5158Version(int addr)
{
	return -ENOENT;
}

#endif

SystemInfo::SystemInfo(QObject *parent) :
	QObject(parent)
{
	meminfo = NULL;
	memtime.start();
	lastMemFree = 0;
}

int SystemInfo::getUptime()
{
	QFile f("/proc/uptime");
	if (!f.open(QIODevice::ReadOnly | QIODevice::Unbuffered))
		return -EPERM;
	QList<QByteArray> list = f.readLine().split(' ');
	return list[0].toDouble();
}

int SystemInfo::getCpuLoad()
{
	return inst.cpu.getInstLoadFromProc();
}

int SystemInfo::getProcFreeMemInfo()
{
	if (lastMemFree && memtime.elapsed() < 1000)
		return lastMemFree;
	if (!meminfo) {
		meminfo = new QFile("/proc/meminfo");
		meminfo->open(QIODevice::ReadOnly | QIODevice::Unbuffered);
	} else
		meminfo->seek(0);
	QStringList lines = QString::fromUtf8(meminfo->readAll()).split("\n");
	int free = 0;
	foreach (const QString line, lines) {
		/*
		 * 2 fields are important, MemFree and Cached
		 */
		if (line.startsWith("MemFree:"))
			free += line.split(" ", QString::SkipEmptyParts)[1].toInt();
		else if (line.startsWith("Cached:")) {
			free += line.split(" ", QString::SkipEmptyParts)[1].toInt();
			break;
		}
	}
	memtime.restart();
	/* prevent false measurements */
	if (free)
		lastMemFree = free;
	return lastMemFree;
}

int SystemInfo::getFreeMemory()
{
	return inst.getProcFreeMemInfo();
}

SystemInfo::CpuInfo::CpuInfo()
{
	total = 0;
	count = 0;
	//avg = 0;
	load = 0;
	proc = NULL;
	userTime = 0;
	prevTotal = 0;
	t0Idle = t0NonIdle;
	lastIdle = lastNonIdle = 0;

	proc = new QFile("/proc/stat");
	proc->open(QIODevice::ReadOnly | QIODevice::Unbuffered);
}

int SystemInfo::CpuInfo::getAvgLoadFromProc()
{
	proc->seek(0);
	QList<QByteArray> list = proc->readLine().split(' ');
	int nonIdle = list[2].toInt()
			+ list[3].toInt()
			+ list[4].toInt()
			+ list[6].toInt()
			+ list[7].toInt()
			+ list[8].toInt()
			+ list[9].toInt()
			;
	int idle = list[5].toInt();
	if (!t0NonIdle) {
		t0NonIdle = nonIdle;
		t0Idle = idle;
		return 0;
	}
	nonIdle -= t0NonIdle;
	idle -= t0Idle;
	return 100 * nonIdle / (nonIdle + idle);
}

int SystemInfo::CpuInfo::getInstLoadFromProc()
{
	proc->seek(0);
	QList<QByteArray> list = proc->readLine().split(' ');
	int nonIdle = list[2].toInt()
			+ list[3].toInt()
			+ list[4].toInt()
			+ list[6].toInt()
			+ list[7].toInt()
			+ list[8].toInt()
			+ list[9].toInt()
			;
	int idle = list[5].toInt();
	int load = 100 * (nonIdle - lastNonIdle) / (nonIdle + idle - lastNonIdle - lastIdle);
	lastIdle = idle;
	lastNonIdle = nonIdle;
	return load;
}

static const QStringList getProcMounts()
{
	QFile f("/proc/mounts");
	if (!f.open(QFile::ReadOnly))
		return QStringList();
	QStringList rt = QString(f.readAll()).split(QRegExp("[\r\n]"), QString::SkipEmptyParts);
	f.close();
	return rt;
}

SystemInfo::PathMountState SystemInfo::getMountState(const QString &dev)
{
	QStringList mountList = getProcMounts();
	foreach (QString l, mountList) {
		QStringList words = l.split(' ');
		if (words.size() < 4)
			continue;
		if (words.at(0) != dev)
			continue;
		QStringList flags = words.at(3).split(',');
		if (flags.contains("ro"))
			return READ_ONLY;
		if (flags.contains("rw"))
			return READ_WRITE;
		if (flags.contains("wo"))
			return WRITE_ONLY;
	}
	return UMOUNT;
}

int SystemInfo::remountState(const QString &dev, SystemInfo::PathMountState state)
{
	if (!QFile(dev).exists())
		return -ENOTDIR;
	QProcess p;
	if (state == UMOUNT) {
		return -ENOLINK;
	} else {
		QStringList params;
		params << "-o";
		QString remount = "remount,";
		if (state == READ_WRITE)
			params << remount.append("rw");
		else if (state == WRITE_ONLY)
			params << remount.append("wo");
		else
			params << remount.append("ro");
		params << dev;
		p.start("mount", params);
	}
	p.waitForFinished();
	if (p.readAllStandardOutput().size())
		return -EBUSY;
	::sync();
	return 0;
}

int SystemInfo::mount(const QString &dev, const QString &dest, SystemInfo::PathMountState state)
{
	if (!QFile(dev).exists() || !QFile(dest).exists())
		return -ENOTDIR;
	QProcess p;
	if (state == UMOUNT) {
		return -ENOLINK;
	} else {
		QStringList params;
		params << dev << dest << "-o";
		if (state == READ_WRITE)
			params << "rw";
		else if (state == WRITE_ONLY)
			params << "wo";
		else
			params << "ro";
		p.start("mount", params);
	}
	p.waitForFinished();
	if (p.readAllStandardOutput().size())
		return -EBUSY;
	::sync();
	return 0;
}

int SystemInfo::umount(const QString &dest)
{
	QProcess p;
	p.start("umount", QStringList() << dest);
	p.waitForFinished();
	if (p.readAllStandardOutput().size())
		return -EBUSY;
	::sync();
	return 0;
}
