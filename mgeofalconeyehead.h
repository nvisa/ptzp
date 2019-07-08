#ifndef MGEOFALCONEYEHEAD_H
#define MGEOFALCONEYEHEAD_H

#include <ecl/ptzp/ptzphead.h>
#include <drivers/i2cdevice.h>

#include <QStringList>
#include <QElapsedTimer>

class PCA9538Driver;
class RelayControlThread;

class MgeoFalconEyeHead : public PtzpHead
{
public:
	MgeoFalconEyeHead(QList<int> relayConfig);

	enum Registers {
		R_ZOOM,
		R_FOCUS,
		R_FOV,
		R_CAM,
		R_DIGI_ZOOM_POS,
		R_IR_POLARITY,
		R_RETICLE_MODE,
		R_RETICLE_TYPE,
		R_RETICLE_INTENSITY,
		R_SYMBOLOGY,
		R_RAYLEIGH_SIGMA,
		R_RAYLEIGH_E,
		R_IMAGE_PROC,
		R_HF_SIGMA,
		R_HF_FILTER,
		R_NUC,
		R_DMC_PARAM,
		R_HEKOS_PARAM,
		R_CURRENT_INTEGRATION_MODE,
		R_FLASH_PROTECTION,
		R_OPTIC_STEP,
		R_DMC_AZIMUTH,
		R_DMC_ELEVATION,
		R_DMC_BANK,
		R_DMC_OFFSET_SIGN,
		R_LASER_STATUS,
		R_LASER_MODE,
		R_SOFTWARE_VERSION,
		R_COOLER,
		R_IBIT_POWER,
		R_IBIT_SYSTEM,
		R_IBIT_OPTIC,
		R_IBIT_LRF,
		R_GMT,
		R_GPS_LAT_DEGREE,
		R_GPS_LAT_MINUTE,
		R_GPS_LAT_SECOND,
		R_GPS_LONG_DEGREE,
		R_GPS_LONG_MINUTE,
		R_GPS_LONG_SECOND,
		R_GPS_ALTITUDE,
		R_GPS_UTM_NORTH,
		R_GPS_UTM_EAST,
		R_GPS_UTM_ZONE,
		R_GPS_DATE_AND_TIME_DAY,
		R_GPS_DATE_AND_TIME_MOUNTH,
		R_GPS_DATE_AND_TIME_YEAR,
		R_GPS_DATE_AND_TIME_HOURS,
		R_GPS_DATE_AND_TIME_MIN,
		R_REF_1_GEO_RANGE,
		R_REF_1_GEO_LAT,
		R_REF_1_GEO_LONG,
		R_REF_1_GEO_HEIGHT,
		R_REF_2_GEO_RANGE,
		R_REF_2_GEO_LAT,
		R_REF_2_GEO_LONG,
		R_REF_2_GEO_HEIGHT,
		R_REF_3_GEO_RANGE,
		R_REF_3_GEO_LAT,
		R_REF_3_GEO_LONG,
		R_REF_3_GEO_HEIGHT,
		R_RELAY_STATUS,
		R_DMC_OFFSET_AZIMUTH,
		R_DMC_OFFSET_ELEVATION,
		R_DMC_OFFSET_BANK,

		R_COUNT
	};

	int getCapabilities();
	virtual int syncRegisters();
	virtual int startZoomIn(int speed);
	virtual int startZoomOut(int speed);
	virtual int stopZoom();
	virtual int setZoom(uint pos);
	virtual int getZoom();
	virtual int getHeadStatus();
	virtual void setProperty(uint r, uint x);
	virtual uint getProperty(uint r);
	virtual QVariant getProperty(const QString &key);
	virtual void setProperty(const QString &key, const QVariant &value);
	virtual int getFOV(float &hor, float &ver);
	virtual QString whoAmI();

protected:
	int syncNext();
	virtual int dataReady(const unsigned char *bytes, int len);
	QByteArray transportReady();
	uint nextSync;

private:
	void setPropertyInt(uint r, int x);
	int sendCommand(const unsigned char *cmd, int len);
	int readRelayConfig(QString filename);
	QElapsedTimer syncTimer;
	PCA9538Driver *i2c;
	RelayControlThread *relth;
	bool fastSwitch;
	int thermalRelay;
	int dayCamRelay;
	int standbyRelay;
	struct LaserReflection {
		int range;
		int height;
		float latdegree;
		float latminute;
		float latsecond;
		float londegree;
		float lonminute;
		float lonsecond;
	};
	QList<LaserReflection> reflections;
};

#endif // MGEOFALCONEYEHEAD_H
