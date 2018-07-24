#ifndef MGEOTHERMALHEAD_H
#define MGEOTHERMALHEAD_H

#include <ecl/ptzp/ptzphead.h>

#include <QStringList>
#include <QElapsedTimer>

class MgeoThermalHead : public PtzpHead
{
public:
	MgeoThermalHead();

	enum Registers {
		R_COOLED_DOWN,
		R_BRIGHTNESS,
		R_CONTRAST,
		R_FOV,
		R_ZOOM,
		R_FOCUS,
		R_ANGLE,
		R_NUC_TABLE,
		R_POLARITY,
		R_RETICLE,
		R_DIGITAL_ZOOM,
		R_IMAGE_FREEZE,
		R_AGC,
		R_RETICLE_INTENSITY,
		R_NUC,
		R_IBIT,
		R_IPM,
		R_HPF_GAIN,
		R_HPF_SPATIAL,
		R_FLIP,
		R_IMAGE_UPDATE_SPEED,

		R_COUNT,
	};

	int getCapabilities();

	virtual int syncRegisters();
	virtual int startZoomIn(int speed);
	virtual int startZoomOut(int speed);
	virtual int stopZoom();
	virtual int getZoom();
	virtual int getHeadStatus();
	virtual void setProperty(int r, uint x);
	virtual uint getProperty(uint r);

protected:
	int sendCommand(const QString &key);
	int dataReady(const unsigned char *bytes, int len);
	int syncNext();

	QStringList ptzCommandList;
	int panPos;
	int tiltPos;
	QElapsedTimer pingTimer;
	int nextSync;
	QList<uint> syncList;

private:
	/* mgeo/thermal API */
	int sendCommand(uint index, uchar data1 = 0x00, uchar data2 = 0x00);
};

#endif // MGEOTHERMALHEAD_H
