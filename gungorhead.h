#ifndef GUNGORHEAD_H
#define GUNGORHEAD_H

#include <ecl/ptzp/ptzphead.h>

class MgeoGunGorHead : public PtzpHead
{
	Q_OBJECT
public:
	MgeoGunGorHead();

	enum Registers {
		R_ZOOM,
		R_FOCUS,
		R_CHIP_VERSION,
		R_SOFTWARE_VERSION,
		R_HARDWARE_VERSION,
		R_DIGI_ZOOM,
		R_CAM_STATUS,		 // manuel set
		R_AUTO_FOCUS_STATUS, // manuel set
		R_DIGI_ZOOM_STATUS,	 // manuel set
		R_FOG_STATUS,
		R_EXT_STATUS,
		R_AUTO_FOCUS_MODE,
		R_FOV_STATUS,

		R_COUNT,
	};

	void fillCapabilities(ptzp::PtzHead *head);

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
	void setFocusStepper();
	float  getAngle();
	QJsonObject factorySettings(const QString &file);
	void initHead();

protected:
	int syncNext();
	virtual int dataReady(const unsigned char *bytes, int len);
	virtual QByteArray transportReady();
	QJsonValue marshallAllRegisters();
	void unmarshallloadAllRegisters(const QJsonValue &node);
	int sendCommand(uint index, uchar data1 = 0x00, uchar data2 = 0x00);
protected slots:
	void timeout();
private:
	int nextSync;
	QList<uint> syncList;
	QElapsedTimer pingTimer;
	QElapsedTimer syncTimer;
};

#endif // GUNGORHEAD_H
