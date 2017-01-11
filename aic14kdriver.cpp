#include "aic14kdriver.h"

#include <ecl/debug.h>

AIC14KDriver::AIC14KDriver(QObject *parent) :
	I2CDevice(parent)
{
}

int AIC14KDriver::init()
{
	fd = openDevice(0x40);
	if (fd < 0)
		return fd;

	syncReg4();
	syncReg5();
	/* set 1.35V, 8 khz */
	i2cWrite(0x01, 0x49);
	i2cWrite(0x02, 0x20);
	i2cWrite(0x03, 0x09);
	i2cWrite(0x04, 3 << 3);
	i2cWrite(0x04, 0x84);
	i2cWrite(0x05, 0x2A);
	i2cWrite(0x05, 0x6A);
	i2cWrite(0x05, 0xBC);
	i2cWrite(0x05, 0xC0);

	return 0;
}

void AIC14KDriver::setCommonMode(bool internal)
{
	if (internal)
		i2cWrite(0x06, 0x2);
	else
		i2cWrite(0x06, 0x4);
}

void AIC14KDriver::syncReg4()
{
	uchar reg = i2cRead(4);
	if (reg == 0x84)
		i2cRead(4);
}

void AIC14KDriver::syncReg5()
{
	uchar reg = i2cRead(0x05);
	while (reg != 0x2a)
		reg = i2cRead(0x05);
	i2cRead(5);
	/* normally we would do 2 more reads from this register but tests show that we shouldn't */
}

void AIC14KDriver::dumpRegisters()
{
	mDebug("AIC registers:");
	mDebug("%d: 0x%x", 1, i2cRead(1));
	mDebug("%d: 0x%x", 2, i2cRead(2));
	mDebug("%d: 0x%x", 3, i2cRead(3));
	mDebug("%d: 0x%x", 4, i2cRead(4));
	mDebug("%d: 0x%x", 4, i2cRead(4));
	mDebug("%d: 0x%x", 5, i2cRead(5));
	mDebug("%d: 0x%x", 5, i2cRead(5));
	mDebug("%d: 0x%x", 5, i2cRead(5));
	mDebug("%d: 0x%x", 5, i2cRead(5));
	mDebug("%d: 0x%x", 6, i2cRead(6));
}

int AIC14KDriver::getRate(int mclk)
{
	uchar p, m, n;
	for (int i = 0; i < 2; i++) {
		uchar val = i2cRead(0x04);
		if (val & 0x80) {
			/* this is an M value */
			m = val & 0x7f;
			if (m == 0)
				m = 128;
		} else {
			/* this is a PN value */
			p = val & 0x7;
			if (p == 0)
				p = 8;
			n = (val >> 3);
			if (n == 0)
				n = 16;
		}
	}
	mDebug("m=%d p=%d n=%d", m, p, n);
	return mclk / (16 * m * n * p);
}
