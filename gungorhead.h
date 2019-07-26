#ifndef GUNGORHEAD_H
#define GUNGORHEAD_H

#include <ecl/ptzp/ptzphead.h>

class MgeoGunGorHead : public PtzpHead
{
public:
	MgeoGunGorHead();

	enum Registers {
		R_ZOOM,
		R_FOCUS,
		R_CHIP_VERSION,
		R_SOFTWARE_VERSION,
		R_HARDWARE_VERSION,
		R_DIGI_ZOOM,
		R_CAM_STATUS,//manuel set
		R_AUTO_FOCUS_STATUS,//manuel set
		R_DIGI_ZOOM_STATUS,//manuel set

		R_COUNT,
	};

	int getCapabilities();

	virtual int syncRegisters();
	virtual int startZoomIn(int speed);
	virtual int startZoomOut(int speed);
	virtual int stopZoom();
	virtual int getZoom();
	virtual int setZoom(uint pos);
	virtual int focusIn(int speed);
	virtual int focusOut(int speed);
	virtual int focusStop();
	virtual int getHeadStatus();
	virtual void setProperty(uint r, uint x);
	virtual uint getProperty(uint r);
	int headSystemChecker();

protected:
	virtual int dataReady(const unsigned char *bytes, int len);
	virtual QByteArray transportReady();
	int syncNext();
	QJsonValue marshallAllRegisters();
	void unmarshallloadAllRegisters(const QJsonValue &node);

	int nextSync;
	QElapsedTimer pingTimer;
	QElapsedTimer syncTimer;
	QList<uint> syncList;

private:
	int sendCommand(uint index, uchar data1 = 0x00, uchar data2 = 0x00);
};

#endif // GUNGORHEAD_H
