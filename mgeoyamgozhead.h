#ifndef MGEOYAMGOZHEAD_H
#define MGEOYAMGOZHEAD_H

#include <ptzphead.h>

#include <QElapsedTimer>
#include <QStringList>

class MgeoYamGozHead : public PtzpHead
{
public:
	MgeoYamGozHead();

	enum Registers {
		R_E_ZOOM,
		R_FREEZE,
		R_POLARITY,
		R_VIDEO_SOURCE,
		R_IMAGE_FLIP,
		R_HEART_BEAT,

		R_COUNT
	};

	void fillCapabilities(ptzp::PtzHead *head);
	virtual int startZoomIn(int speed);
	virtual int startZoomOut(int speed);
	virtual int stopZoom();
	virtual int setZoom(uint pos);
	virtual int getZoom();
	virtual int getHeadStatus();
	virtual void setProperty(uint r, uint x);
	virtual uint getProperty(uint r);
	QVariant getCapabilityValues(ptzp::PtzHead_Capability c);
	void setCapabilityValues(ptzp::PtzHead_Capability c, uint val);

protected:
	virtual int dataReady(const unsigned char *bytes, int len);

private:
	int sendCommand(const unsigned char *cmd, int len);
};

#endif // MGEOYAMGOZHEAD_H
