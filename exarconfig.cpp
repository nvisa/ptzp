#include "exarconfig.h"
#include <sys/ioctl.h>
#include <termio.h>

struct xrioctl_rw_reg {
	int block;
	int reg;
	int regvalue;
};

ExarConfig::ExarConfig(int fd)
{
	xrfd = fd;
}

void ExarConfig::m_sethwf(int fd, int on)
{
	struct termios tty;
	tcgetattr(fd, &tty);
	if (on)
		tty.c_cflag |= CRTSCTS;
	else
		tty.c_cflag &= ~CRTSCTS;
	tcsetattr(fd, TCSANOW, &tty);
}

void ExarConfig::write_reg(int block, int addr, int value)
{
	struct xrioctl_rw_reg regwrite;
	regwrite.block = block;
	regwrite.reg = addr;
	regwrite.regvalue = value;
	ioctl(xrfd, XRIOC_SET_REG, &regwrite);
}

void ExarConfig::setOutput()
{
	write_reg(-1, 0x1b, 0x3f);
}

void ExarConfig::setHalfDuplex()
{
	write_reg(-1, 0x0c, 0x08);
}

void ExarConfig::setFullDuplex()
{
	write_reg(-1, 0x0c, 0x00);
}

void ExarConfig::setPort(QString port_type)
{
	if (port_type == "232")
		write_reg(-1, 0x1d, 0x28);
	if (port_type == "422")
		write_reg(-1, 0x1d, 0x38);
	m_sethwf(xrfd,0);
	setOutput();
	setFullDuplex();
}
