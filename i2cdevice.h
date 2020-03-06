#ifndef I2CDEVICE_H
#define I2CDEVICE_H

#include <QObject>

struct I2CAddresses;

class I2CDevice : public QObject
{
	Q_OBJECT
public:
	explicit I2CDevice(QObject *parent = 0);
	bool hasError() { return readError; }
	void clearError() { readError = false; }
protected:
	int fd;
	bool readError;
	uchar addr;

	int openDevice(uint addr);
	uchar i2cWrite(int fd, uchar reg, uchar val);
	uchar i2cWriteW(uchar reg, ushort val);
	uchar i2cWrite(uchar reg, uchar val);
	uchar i2cWriteW2(ushort reg, ushort val);
	uchar i2cWriteW3(ushort reg, uchar val);
	uchar i2cRead(uchar reg);
	ushort i2cReadW(uchar reg);
	ushort i2cReadW2(ushort reg);
	uchar i2cReadW3(ushort reg);
	uchar i2cRead(int fd, uchar reg);
};

#endif // I2CDEVICE_H
