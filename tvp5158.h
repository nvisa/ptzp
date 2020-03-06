#ifndef TVP5158_H
#define TVP5158_H

#include <ecl/drivers/i2cdevice.h>

class Tvp5158 : public I2CDevice
{
	Q_OBJECT
public:
	enum OutputMode {
		MODE_SINGLE_CHANNEL_D1,
		MODE_4_CHANNEL_D1_PI,
		MODE_4_CHANNEL_D1_LI,
	};
	explicit Tvp5158(QObject *parent = 0);
	void enable(bool value);
	int getCurrentChannel();
	int setCurrentChannel(int ch);
	int setOutputMode(OutputMode m);
signals:

public slots:

};

#endif // TVP5158_H
