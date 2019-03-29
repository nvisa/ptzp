#ifndef EXARCONFIG_H
#define EXARCONFIG_H
#include <QString>

#define XR_USB_SERIAL_IOC_MAGIC       	'v'
#define XRIOC_SET_REG           	_IOWR(XR_USB_SERIAL_IOC_MAGIC, 2, int)

class ExarConfig
{
public:
	ExarConfig(int fd);

	static void m_sethwf(int fd, int on);
	void write_reg(int block, int addr, int value);
	void exarOpen (QString portname);
	void setOutput();
	void setHalfDuplex();
	void setFullDuplex();
	void setPort(QString port_type);
	void exarClose();

	int xrfd;
	int reg_value;
	int result;
	int reg_addr;
};

#endif // EXARCONFIG_H
