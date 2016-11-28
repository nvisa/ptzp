#include "tvp5158.h"

#include <ecl/debug.h>

#include <errno.h>

Tvp5158::Tvp5158(QObject *parent) :
	I2CDevice(parent)
{
	fd = openDevice(0x58);
}

void Tvp5158::enable(bool value)
{
	i2cWrite(0xb2, value ? 0x25 : 0x20);
}

int Tvp5158::getCurrentChannel()
{
	return (i2cRead(0xb3) & 0x3);
}

int Tvp5158::setCurrentChannel(int ch)
{
	QList<int> chs;
	for (int i = 0; i < 4; i++)
		chs << i;
	int portA = ch;
	chs.removeAll(ch);
	int portB = chs[0];
	int portC = chs[1];
	int portD = chs[2];
	i2cWrite(0xb3, portA | (portB << 2) | (portC << 4) | (portD << 6));
	return 0;
}

int Tvp5158::setOutputMode(Tvp5158::OutputMode m)
{
	if (m == MODE_4_CHANNEL_D1_PI)
		return i2cWrite(0xb0, 0x60);
	else if (m == MODE_SINGLE_CHANNEL_D1)
		return i2cWrite(0xb0, 0x00);
	else if (m == MODE_4_CHANNEL_D1_LI)
		return i2cWrite(0xb0, 0xa0);
	return -EINVAL;
}
