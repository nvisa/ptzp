#ifndef AIC14KDRIVER_H
#define AIC14KDRIVER_H

#include <ecl/drivers/i2cdevice.h>

class AIC14KDriver : public I2CDevice
{
	Q_OBJECT
public:
	explicit AIC14KDriver(QObject *parent = 0);

	int init();
	void dumpRegisters();
	int getRate(int mclk);

	void setCommonMode(bool internal);
signals:

public slots:
protected:
	void syncReg4();
};

#endif // AIC14KDRIVER_H
