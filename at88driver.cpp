#include "at88driver.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <linux/i2c-dev.h>
#include <linux/i2c.h>

#include <sys/ioctl.h>

#include <QIODevice>
#include <QDataStream>

/**
	\class AT88Driver

	\brief Implements AT88 family of EEPROM driver.

	At the moment following models are supported:

		* AT88SC0104C
		* AT88SC1616C

	After creating an instance of this class you can call init() function
	for initing driver which will return the detected EEPROM model. You can use
	readZone() function for reading EEPROM data and writeZone()/writePage() functions
	for programming.
*/

/**
 * @brief AT88Driver::AT88Driver
 * @param parent
 */
AT88Driver::AT88Driver(QObject *parent) :
	I2CDevice(parent)
{
}

/**
 * @brief AT88Driver::getEepromModel
 * @return EEPROM's sub-model on success, -EINVAL for en-recognized EEPROMs.
 */
int AT88Driver::getEepromModel()
{
	if (atr == 0x3bb2110010800001ull)
		return SC0104C;
	if (atr == 0x3bb2110010800016ull)
		return SC1616C;
	return -EINVAL;
}

/**
 * @brief AT88Driver::getEepromSize
 * @return EEPROM's memory in bytes on success, -EINVAL for en-recognized EEPROMs, -ENOENT otherwise.
 */
int AT88Driver::getEepromSize()
{
	int m = getEepromModel();
	if (m == SC0104C)
		return 128;
	if (m == SC1616C)
		return 2048;
	if (m < 0)
		return -ENOENT;
	return -EINVAL;
}

/**
 * @brief AT88Driver::getZoneCount
 * @return Number of user zones in the EEPROM on success, -EINVAL for en-recognized EEPROMs, -ENOENT otherwise.
 */
int AT88Driver::getZoneCount()
{
	int m = getEepromModel();
	if (m == SC0104C)
		return 4;
	if (m == SC1616C)
		return 16;
	if (m < 0)
		return -ENOENT;
	return -EINVAL;
}

/**
 * @brief AT88Driver::getPageSize
 * @return Page size of the EEPROM, -EINVAL for en-recognized EEPROMs, -ENOENT otherwise.
 */
int AT88Driver::getPageSize()
{
	int m = getEepromModel();
	if (m == SC0104C
			|| m == SC1616C)
		return 16;
	if (m < 0)
		return -ENOENT;
	return -EINVAL;
}

/**
 * @brief AT88Driver::getZoneSize
 * @return Zone size of the EEPROM, -EINVAL for en-recognized EEPROMs, -ENOENT otherwise.
 */
int AT88Driver::getZoneSize()
{
	int m = getEepromModel();
	if (m == SC0104C)
		return 32;
	if (m == SC1616C)
		return 128;
	if (m < 0)
		return -ENOENT;
	return -EINVAL;
}

/**
 * @brief AT88Driver::readZone
 * @param zone zone number
 * @return read data on success, NULL QByteArray otherwise.
 */
QByteArray AT88Driver::readZone(int zone)
{
	return readUserZone(zone);
}

/**
 * @brief AT88Driver::writeZone
 * @param zone zone number
 * @param ba write argument
 * @return '0' on success, negative error code otherwise.
 */
int AT88Driver::writeZone(int zone, const QByteArray &ba)
{
	if (ba.length() > getZoneSize())
		return -E2BIG;
	else {
		int index = ba.length() / getPageSize();
		int curr_array = 0;
		for (int count = 0; count < index; count++) {
			usleep(7000);
			curr_array = count * getPageSize();
			if (writePage(zone, curr_array, ba.mid(curr_array, getPageSize())))
				return -35;
		}
		index = index * getPageSize();
		if (index < ba.length()) {
			usleep(7000);
			if (writePage(zone, index, ba.mid(curr_array, ba.length() - index)))
				return -35;
		}
	}
	return 0;
}

/**
 * @brief AT88Driver::writePage
 * @param zone zone number
 * @param page page number
 * @param ba write argument
 * @return '0' on success, negative error code otherwise.
 */
int AT88Driver::writePage(int zone, int page, const QByteArray &ba)
{
	if (page + ba.length() > getZoneSize())
		return -ENOMEM;
	else
		return writeUserZone(zone, page, ba);
}

/**
 * @brief AT88Driver::selectUserZone
 * @param zone
 * @return '0' on success, negative error code otherwise.
 */
int AT88Driver::selectUserZone(int zone)
{
	uchar buf[32];
	buf[0] = 0x03;
	buf[1] = zone;
	buf[2] = 0;

	struct i2c_msg msg[2];
	msg[0].addr = 0xB4 >> 1;
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

/**
 * @brief AT88Driver::writeUserZone
 * @param zone Target user zone for writing.
 * @param off Offset of address in the given zone.
 * @param data Data will be written.
 * @return '0' on success, negative error code otherwise.
 *
 * AT88 series supports writing at most one page-size of data at once to a given zone,
 * so this function accepts an offset argument for positioning data in the given zone.
 * This function selects target zone internally.
 */
int AT88Driver::writeUserZone(int zone, quint16 off, const QByteArray &data)
{
	if (data.size() > getPageSize())
		return -EINVAL;

	int err = selectUserZone(zone);
	if (err)
		return err;

	uchar buf[32];
	buf[0] = off >> 8;
	buf[1] = off;
	buf[2] = data.size();
	memcpy(&buf[3], data.constData(), data.size());

	struct i2c_msg msg[2];
	msg[0].addr = 0xB0 >> 1;
	msg[0].flags = 0;
	msg[0].len = 3 + data.size();
	msg[0].buf = buf;

	struct i2c_rdwr_ioctl_data args;
	args.msgs = msg;
	args.nmsgs = 1;

	int res = ioctl(fd, I2C_RDWR, &args);
	if (res < 0)
		return res;

	return 0;
}

/**
 * @brief AT88Driver::readUserZone
 * @param zone Target zone for reading.
 * @return Data-read from EEPROM.
 *
 * This function reads entire zone. This function selects target zone internally.
 */
QByteArray AT88Driver::readUserZone(int zone)
{
	int err = selectUserZone(zone);
	if (err)
		return QByteArray();

	uchar buf[32];
	buf[0] = 0;
	buf[1] = 0;
	buf[2] = getZoneSize();

	struct i2c_msg msg[2];
	msg[0].addr = 0xB2 >> 1;
	msg[0].flags = 0;
	msg[0].len = 3;
	msg[0].buf = buf;

	msg[1].addr = 0x5B;
	msg[1].flags = I2C_M_RD | I2C_M_NOSTART;
	msg[1].len = buf[2];
	msg[1].buf = buf;

	struct i2c_rdwr_ioctl_data args;
	args.msgs = msg;
	args.nmsgs = 2;

	int res = ioctl(fd, I2C_RDWR, &args);
	if (res == 2)
		return QByteArray((const char *)buf, getZoneSize());
	return QByteArray();
}

/**
 * @brief AT88Driver::init
 * @return A valid EEPROM model on success, negative error code otherwise.
 */
int AT88Driver::init()
{
	fd = open("/dev/i2c-1", O_RDWR);
	if (fd < 0)
		return -ENOENT;

	QByteArray ba = readConfigMem();
	if (!ba.size())
		return -EINVAL;

	QDataStream in(&ba, QIODevice::ReadOnly);
	in.setByteOrder(QDataStream::BigEndian);
	in >> atr;
	in >> fabCode;
	in >> mtz;
	in >> cmc;
	in >> lot;
	in >> dcr;

	return getEepromModel();
}

/**
 * @brief AT88Driver::readConfigMem
 * @return Internal configuration memory of the underlying EEPROM.
 */
QByteArray AT88Driver::readConfigMem()
{
	uchar buf[32];
	buf[0] = 0x00;
	buf[1] = 0x00;
	buf[2] = 0x20;

	struct i2c_msg msg[2];
	msg[0].addr = 0x5B;
	msg[0].flags = 0;
	msg[0].len = 3;
	msg[0].buf = buf;

	msg[1].addr = 0x5B;
	msg[1].flags = I2C_M_RD | I2C_M_NOSTART;
	msg[1].len = buf[2];
	msg[1].buf = buf;

	struct i2c_rdwr_ioctl_data args;
	args.msgs = msg;
	args.nmsgs = 2;

	int res = ioctl(fd, I2C_RDWR, &args);

	if (res == 2)
		return QByteArray((const char *)buf, 32);
	return QByteArray();
}
