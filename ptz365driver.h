#ifndef PTZ365DRIVER_H
#define PTZ365DRIVER_H

#include "i2cdevice.h"

class QTimer;
class ComInterface;
class I2CInterface;
class QextSerialPort;

class Ptz365Driver : public I2CDevice
{
	Q_OBJECT
public:
	enum movDirection {
		NO_MOVE = 0,
		MOVE_RIGHT = 0x02,
		MOVE_LEFT = 0x04,
		MOVE_UP = 0x08,
		MOVE_DOWN = 0x10,
		MOVE_UPRIGHT = 0x0A,
		MOVE_UPLEFT = 0x0C,
		MOVE_DOWNRIGHT = 0x12,
		MOVE_DOWNLEFT = 0x14
	};

	explicit Ptz365Driver(QObject *parent = 0);
	explicit Ptz365Driver(QextSerialPort *port, QObject *parent = 0);

	uchar getPelcoDAddress();
	void setPelcoDAddress(uchar addr);
	uchar getPelcoDSpeed();
	void setPelcoDSpeed(uchar speed);
	uchar getPelcoDData();
	void setPelcoDData(uchar data);
	void setCommand(uchar cmd, int timeout = 0);
	uchar getCommand(uchar cmd);
	int setCustomCommand(const QByteArray &ba);

	int panTo(int degree);
	int tiltTo(int degree);
	int moveTo(movDirection mDr, uchar panSpd, uchar tiltSpd);
	int getPanPosition();
	int getTiltPosition();
signals:

public slots:
	void timeout();
protected:
	friend class I2CInterface;
	QTimer *timer;
	ComInterface *iface;
};

#endif // PTZ365DRIVER_H
