#ifndef AT88DRIVER_H
#define AT88DRIVER_H

#include <ecl/drivers/i2cdevice.h>

class AT88Driver : public I2CDevice
{
	Q_OBJECT
public:
	enum Model {
		SC0104C,
		SC0808C,
		SC1616C,
	};

	explicit AT88Driver(QObject *parent = 0);
	int init();
	int getEepromModel();
	int getEepromSize();
	int getZoneCount();
	int getPageSize();
	int getZoneSize();
	QByteArray readZone(int zone);
	int writeZone(int zone, const QByteArray &ba);
	int writePage(int zone, int page, const QByteArray &ba);

	quint64 getAtr() { return atr; }
	quint16 getFab() { return fabCode; }
	quint16 getMtz() { return mtz; }
	quint32 getCmc() { return cmc; }
	quint64 getLot() { return lot; }
	uchar getDcr() { return dcr; }

protected:
	QByteArray readConfigMem();
	int selectUserZone(int zone);
	int writeUserZone(int zone, quint16 off, const QByteArray &data);
	QByteArray readUserZone(int zone);

private:
	quint64 atr;
	quint16 fabCode;
	quint16 mtz;
	quint32 cmc;
	quint64 lot;
	uchar dcr;
	QByteArray ident;

};

#endif // AT88DRIVER_H
