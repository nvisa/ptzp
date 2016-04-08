#include "i2cdevice.h"
#include "debug.h"

#include <errno.h>
#include <error.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

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

static inline __u8 i2c_smbus_write_byte_data(int file, __u8 command,
											  __u8 value)
{
	union i2c_smbus_data data;
	data.byte = value;
	return i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
							I2C_SMBUS_BYTE_DATA, &data);
}

static inline __u8 i2c_smbus_read_byte_data(int file, __u8 command, int *status)
{
	union i2c_smbus_data data;
	*status = i2c_smbus_access(file, I2C_SMBUS_READ, command, I2C_SMBUS_BYTE_DATA,&data);
	return data.byte;
}

static inline __u8 i2c_smbus_write_word_data(int file, __u8 command,
											  __u16 value)
{
	union i2c_smbus_data data;
	data.word = value;
	return i2c_smbus_access(file,I2C_SMBUS_WRITE,command,
							I2C_SMBUS_WORD_DATA, &data);
}

static inline __u16 i2c_smbus_read_word_data(int file, __u8 command, int *status)
{
	union i2c_smbus_data data;
	*status = i2c_smbus_access(file, I2C_SMBUS_READ, command, I2C_SMBUS_WORD_DATA,&data);
	return data.word;
}

I2CDevice::I2CDevice(QObject *parent) :
	QObject(parent),
	readError(true)
{
}

int I2CDevice::openDevice(uint addr)
{
	int fd = open("/dev/i2c-1", O_RDWR);
	/* With force, let the user read from/write to the registers
		 even when a driver is also running */
	if (ioctl(fd, I2C_SLAVE_FORCE, addr) < 0) {
		qDebug("Error: Could not set address to 0x%02x: %s\n",
				addr, strerror(errno));
		return -ENOENT;
	}
	this->addr = addr;
	return fd;
}

uchar I2CDevice::i2cWrite(uchar reg, uchar val)
{
	return i2c_smbus_write_byte_data(fd, reg, val);
}

uchar I2CDevice::i2cWriteW(uchar reg, ushort val)
{
	return i2c_smbus_write_word_data(fd, reg, val);
}

uchar I2CDevice::i2cWrite(int fd, uchar reg, uchar val)
{
	return i2c_smbus_write_byte_data(fd, reg, val);
}

uchar I2CDevice::i2cWriteW2(ushort reg, ushort val)
{
	uchar buf[32];
	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;
	buf[2] = val >> 8;
	buf[3] = val & 0xff;

	struct i2c_msg msg[1];
	msg[0].addr = addr >> 1;
	msg[0].flags = 0;
	msg[0].len = 4;
	msg[0].buf = buf;

	struct i2c_rdwr_ioctl_data args;
	args.msgs = msg;
	args.nmsgs = 1;

	int res = ioctl(fd, I2C_RDWR, &args);
	if (res == 1)
		return 0;
	return res;
}

uchar I2CDevice::i2cWriteW3(ushort reg, uchar val)
{
	uchar buf[32];
	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;
	buf[2] = val;

	struct i2c_msg msg[1];
	msg[0].addr = addr >> 1;
	msg[0].flags = 0;
	msg[0].len = 3;
	msg[0].buf = buf;

	struct i2c_rdwr_ioctl_data args;
	args.msgs = msg;
	args.nmsgs = 1;

	int res = ioctl(fd, I2C_RDWR, &args);
	if (res == 1)
		return 0;
	return res;
}

uchar I2CDevice::i2cRead(int fd, uchar reg)
{
	int s = 0;
	uchar ret = i2c_smbus_read_byte_data(fd, reg, &s);
	if (s < 0)
		readError = true;
	return ret;
}

uchar I2CDevice::i2cRead(uchar reg)
{
	int s = 0;
	uchar ret = i2c_smbus_read_byte_data(fd, reg, &s);
	if (s < 0)
		readError = true;
	return ret;
}

ushort I2CDevice::i2cReadW(uchar reg)
{
	int s = 0;
	ushort ret = i2c_smbus_read_word_data(fd, reg, &s);
	if (s < 0)
		readError = true;
	return ret;
}

ushort I2CDevice::i2cReadW2(ushort reg)
{
	uchar buf[32];
	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	struct i2c_msg msg[2];
	msg[0].addr = addr >> 1;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = buf;

	msg[1].addr = addr >> 1;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 2;
	msg[1].buf = buf;

	struct i2c_rdwr_ioctl_data args;
	args.msgs = msg;
	args.nmsgs = 2;

	int res = ioctl(fd, I2C_RDWR, &args);
	if (res == 2)
		return (buf[0] << 8) | buf[1];
	return res;
}

uchar I2CDevice::i2cReadW3(ushort reg)
{
	uchar buf[32];
	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	struct i2c_msg msg[2];
	msg[0].addr = addr >> 1;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = buf;

	msg[1].addr = addr >> 1;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;

	struct i2c_rdwr_ioctl_data args;
	args.msgs = msg;
	args.nmsgs = 2;

	int res = ioctl(fd, I2C_RDWR, &args);
	if (res == 2)
		return buf[0];
	return res;
}
