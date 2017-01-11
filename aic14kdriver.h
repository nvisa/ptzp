#ifndef AIC14KDRIVER_H
#define AIC14KDRIVER_H

#include <ecl/drivers/i2cdevice.h>

#include <QHash>

class AIC14KDriver : public I2CDevice
{
	Q_OBJECT
public:
	explicit AIC14KDriver(QObject *parent = 0);

	int init(bool setRegisters = true);
	void dumpRegisters();
	int getRate(int mclk);

	void setCommonMode(bool internal);
signals:

public slots:
protected:
	void updateRegister(int reg);
	void updateRegister(int reg, uchar val);
	void syncReg4();
	void syncReg5();

	QHash<int, uchar> regCache;
};

#endif // AIC14KDRIVER_H
