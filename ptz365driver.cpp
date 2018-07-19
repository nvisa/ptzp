#include "ptz365driver.h"
#include "drivers/qextserialport/qextserialport.h"
#include "debug.h"

#include <QTimer>

#include <errno.h>
#include <unistd.h>

#define I2CR_PELCOD_ADDR		0x02
#define I2CR_PELCOD_SPEED		0x03
#define I2CR_PELCOD_DATA		0x04
#define I2CR_US_BAUDRATE_HIGH	0x05
#define I2CR_US_BAUDRATE_LOW	0x06
#define I2CR_US_STOPBITS		0x07
#define I2CR_US_PARITY			0x08
#define I2CR_US_COMMAND			0x09
#define I2CR_VER_MAJOR			0x0A
#define I2CR_VER_MINOR			0x0B
#define I2CR_VER_PATCH			0x0C
#define I2CR_US_RXCNT			0x0D
#define I2CR_PD_CUSTOM1			0x0E
#define I2CR_PD_CUSTOM2			0x0F
#define I2CR_PD_CUSTOM3			0x10
#define I2CR_PD_CUSTOM4			0x11
#define I2CR_PD_CUSTOM5			0x12
#define I2CR_PD_CUSTOM6			0x13
#define I2CR_PD_CUSTOM7			0x14
#define I2CR_PD_PAN_POS_HIGH	0x15
#define I2CR_PD_PAN_POS_LOW		0x16
#define I2CR_PD_TILT_POS_HIGH	0x17
#define I2CR_PD_TILT_POS_LOW	0x18

enum pelco_d_commands {
	PELCOD_CMD_STOP,
	PELCOD_CMD_PANR,
	PELCOD_CMD_PANL,
	PELCOD_CMD_TILTU,
	PELCOD_CMD_TILTD,
	PELCOD_CMD_SET_PRESET,
	PELCOD_CMD_CLEAR_PRESET,
	PELCOD_CMD_GOTO_PRESET,
	PELCOD_CMD_FLIP180,
	PELCOD_CMD_ZERO_PAN,
	PELCOD_CMD_RESET,
	PELCOD_QUE_PAN_POS,
	PELCOD_QUE_TILT_POS,
	PELCOD_SET_ZERO_POS,
};

static char pelcod_buf[7];
static char * get_pelcod(int addr, int cmd, int speed, int arg)
{
	char *buf = pelcod_buf;
	buf[0] = 0xff;
	buf[1] = addr;
	buf[2] = 0x00;
	buf[3] = 0x00;
	buf[4] = speed;
	buf[5] = speed;
	buf[6] = 0;

	if (cmd == PELCOD_CMD_PANR)
		buf[3] = 0x02;
	else if (cmd == PELCOD_CMD_PANL)
		buf[3] = 0x04;
	else if (cmd == PELCOD_CMD_TILTU)
		buf[3] = 0x08;
	else if (cmd == PELCOD_CMD_TILTD)
		buf[3] = 0x10;
	else if (cmd == PELCOD_CMD_SET_PRESET) {
		buf[3] = 0x03;
		buf[4] = 0x00;
		buf[5] = arg;
	} else if (cmd == PELCOD_CMD_CLEAR_PRESET) {
		buf[3] = 0x05;
		buf[4] = 0x00;
		buf[5] = arg;
	} else if (cmd == PELCOD_CMD_GOTO_PRESET) {
		buf[3] = 0x07;
		buf[4] = 0x00;
		buf[5] = arg;
	} else if (cmd == PELCOD_CMD_FLIP180) {
		buf[3] = 0x07;
		buf[4] = 0x00;
		buf[5] = 21;
	} else if (cmd == PELCOD_CMD_ZERO_PAN) {
		buf[3] = 0x07;
		buf[4] = 0x00;
		buf[5] = 22;
	} else if (cmd == PELCOD_CMD_RESET) {
		buf[3] = 0x0F;
		buf[4] = 0x00;
		buf[5] = 0x00;
	} else if (cmd == PELCOD_QUE_PAN_POS) {
		buf[3] = 0x51;
		buf[4] = 0x00;
		buf[5] = 0x00;
	} else if (cmd == PELCOD_QUE_TILT_POS) {
		buf[3] = 0x53;
		buf[4] = 0x00;
		buf[5] = 0x00;
	} else if (cmd == PELCOD_SET_ZERO_POS) {
		buf[3] = 0x49;
		buf[4] = 0x00;
		buf[5] = 0x00;
	} else
		buf[3] = 0;


	/* calculate checksum */
	int sum = 0;
	for (int i = 1; i < 6; i++)
		sum += buf[i];
	buf[6] = sum;

	return buf;
}

class ComInterface
{
public:
	virtual int write(uchar reg, uchar val) = 0;
	virtual uchar read(uchar reg) = 0;
	virtual QByteArray write(const QByteArray &ba, int timeout) = 0;
};

class I2CInterface : public ComInterface
{
public:
	I2CInterface(Ptz365Driver *p)
	{
		parent = p;
	}

	int write(uchar reg, uchar val)
	{
		return parent->i2cWrite(reg, val);
	}

	uchar read(uchar reg)
	{
		return parent->i2cRead(reg);
	}

	QByteArray write(const QByteArray &ba, int timeout)
	{
		return QByteArray();
	}


	Ptz365Driver *parent;
};

class RS232Interface : public ComInterface
{
public:
	RS232Interface(QextSerialPort *p)
	{
		port = p;
		i2c_regs[0] = 0x4;
		i2c_regs[1] = 0x11;
		i2c_regs[2] = 0x1;
		i2c_regs[3] = 0xff;
		i2c_regs[4] = 0x0;
		i2c_regs[5] = 0x0;
		i2c_regs[6] = 0x0;
		i2c_regs[7] = 0x0;
		i2c_regs[8] = 0x4;
		i2c_regs[9] = 0x4;
		i2c_regs[10] = 1;
		i2c_regs[11] = 2;
		i2c_regs[12] = 6;
		i2c_regs[13] = 0;
	}

	int write(uchar reg, uchar val)
	{
		i2c_regs[reg] = val;
		if (reg == 0x00) {
			int addr = i2c_regs[I2CR_PELCOD_ADDR];
			int speed = i2c_regs[I2CR_PELCOD_SPEED];
			int par = i2c_regs[I2CR_PELCOD_DATA];
			QByteArray ba(get_pelcod(addr, val, speed, par), 7);
			port->write(ba);
		} else if (reg == I2CR_US_COMMAND) {
			/*struct usart *usart = get_usart(1);
			usart->baudrate = i2c_regs[I2CR_US_BAUDRATE_HIGH] * 256 + i2c_regs[I2CR_US_BAUDRATE_LOW];
			usart->baudrate *= 10;
			usart->parity = i2c_regs[I2CR_US_PARITY];
			usart->stop_bits = i2c_regs[I2CR_US_STOPBITS];
			usart_reinit(usart);*/
		} else if (reg == I2CR_PD_CUSTOM7) {
			char tmp[7];
			tmp[0] = i2c_regs[I2CR_PD_CUSTOM1];
			tmp[1] = i2c_regs[I2CR_PD_CUSTOM2];
			tmp[2] = i2c_regs[I2CR_PD_CUSTOM3];
			tmp[3] = i2c_regs[I2CR_PD_CUSTOM4];
			tmp[4] = i2c_regs[I2CR_PD_CUSTOM5];
			tmp[5] = i2c_regs[I2CR_PD_CUSTOM6];
			tmp[6] = i2c_regs[I2CR_PD_CUSTOM7];
			port->write(tmp, 7);
		}
		return 0;
	}

	QByteArray write(const QByteArray &ba, int timeout)
	{
		port->write(ba);
		usleep(timeout * 1000);
		return port->readAll();
	}

	uchar read(uchar reg)
	{
		return i2c_regs[reg];
	}

	void dump(const QByteArray &ba)
	{
		for (int i = 0; i < ba.size(); i++)
			qDebug() << (int)ba.at(i);
	}

protected:
	uchar i2c_regs[128];
	QextSerialPort *port;
};

Ptz365Driver::Ptz365Driver(QObject *parent) :
	I2CDevice(parent)
{
	fd = openDevice(0x19);
	timer = new QTimer(this);
	timer->setSingleShot(true);
	connect(timer, SIGNAL(timeout()), SLOT(timeout()));
	iface = new I2CInterface(this);
}

Ptz365Driver::Ptz365Driver(QextSerialPort *port, QObject *parent) :
	I2CDevice(parent)
{
	timer = new QTimer(this);
	timer->setSingleShot(true);
	connect(timer, SIGNAL(timeout()), SLOT(timeout()));
	iface = new RS232Interface(port);
}

uchar Ptz365Driver::getPelcoDAddress()
{
	return iface->read(0x02);
}

void Ptz365Driver::setPelcoDAddress(uchar addr)
{
	iface->write(0x02, addr);
}

uchar Ptz365Driver::getPelcoDSpeed()
{
	return iface->read(0x03);
}

void Ptz365Driver::setPelcoDSpeed(uchar speed)
{
	iface->write(0x03, speed);
}

uchar Ptz365Driver::getPelcoDData()
{
	return iface->read(0x04);
}

void Ptz365Driver::setPelcoDData(uchar data)
{
	iface->write(0x04, data);
}

void Ptz365Driver::setCommand(uchar cmd, int timeout)
{
	iface->write(0x00, cmd);
	if (timeout)
		timer->start(timeout);
}

uchar Ptz365Driver::getCommand(uchar cmd)
{
	return iface->read(cmd);
}

int Ptz365Driver::setCustomCommand(const QByteArray &ba)
{
	if (ba.size() != 7)
		return -EINVAL;
	int breg = 0x0e;
	for (int i = 0; i < 7; i++)
		iface->write(breg + i, (uchar)ba.at(i));
	return 0;
}

QByteArray Ptz365Driver::writeRead(const QByteArray &ba, int timeout)
{
	return iface->write(ba, timeout);
}

static QByteArray setupDegree(int degree, uchar cmd)
{
	while (degree < 0)
		degree += 360;
	//degree *= 100;
	QByteArray ba;
	ba.append(char(0xff));
	ba.append(char(0x01));
	ba.append(char(0x00));
	ba.append(char(cmd));
	ba.append(char((degree >> 8) & 0xff));
	ba.append(char(degree & 0xff));

	/* calculate checksum */
	int sum = 0;
	for (int i = 1; i < 6; i++)
		sum += (uchar)ba.at(i);
	ba.append(char(sum & 0xff));
	return ba;
}

static QByteArray setupMovement(Ptz365Driver::movDirection mDr, uchar panSpd, uchar tiltSpd) {
	QByteArray ba;
	ba.append(char(0xff));
	ba.append(char(0x01));
	ba.append(char(0x00));
	ba.append(char(mDr));
	ba.append(char(panSpd));
	ba.append(char(tiltSpd));

	/* calculate checksum */
	int sum = 0;
	for (int i = 1; i < 6; i++)
		sum += (uchar)ba.at(i);
	ba.append(char(sum & 0xff));
	qDebug() << "movement ba: " << ba.toHex();
	return ba;
}

int Ptz365Driver::panTo(int degree)
{
	return setCustomCommand(setupDegree(degree, 0x4b));
}

int Ptz365Driver::tiltTo(int degree)
{
	return setCustomCommand(setupDegree(degree, 0x4d));
}

int Ptz365Driver::moveTo(movDirection mDr, uchar panSpd, uchar tiltSpd)
{
	return setCustomCommand(setupMovement(mDr, panSpd, tiltSpd));
}

int Ptz365Driver::getPanPosition()
{
	setCommand(11);
	usleep(10000);
	uchar b1 = iface->read(0x15);
	uchar b2 = iface->read(0x16);
	return b1 * 256 + b2;
}

int Ptz365Driver::getTiltPosition()
{
	setCommand(12);
	usleep(10000);
	uchar b1 = iface->read(0x17);
	uchar b2 = iface->read(0x18);
	return b1 * 256 + b2;
}

int Ptz365Driver::readPosition(int &pan, int &tilt)
{
	char mes[23];// = {0x3a, 0xff, 0x82, 0x00, 0, 0, 0, 0x81, 0x5c};
	QByteArray ba = writeRead(QByteArray(mes, 9), 100);
	const uchar *buf = (const uchar *)ba.constData();
	if (ba.size() < 9) {
		pan = 0;
		tilt = 0;
		return -EINVAL;
	}
	pan = buf[3] * 256 + buf[4];
	tilt = buf[5] * 256 + buf[6];
	return 0;
}

void Ptz365Driver::timeout()
{
	mDebug("stopping operation");
	setCommand(0);
}
