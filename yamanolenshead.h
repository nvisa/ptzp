#ifndef YAMANOLENSHEAD_H
#define YAMANOLENSHEAD_H

#include <ecl/ptzp/ptzphead.h>

class YamanoLensHead : public PtzpHead
{
public:
	YamanoLensHead();

	enum Register {
		R_ZOOM_POS,
		R_FOCUS_POS,
		R_IRIS_POS,
		R_VERSION,
		R_FILTER_POS,
		R_FOCUS_MODE,
		R_IRIS_MODE,

		R_COUNT
	};

	void fillCapabilities(ptzp::PtzHead *head);
	int syncRegisters();
	int getHeadStatus();

	int startZoomIn(int speed);
	int startZoomOut(int speed);
	int stopZoom();
	int getZoom();
	int setZoom(uint pos);
	int focusIn(int speed);
	int focusOut(int speed);
	int focusStop();
	uint getProperty(uint r);
	void setProperty(uint r, uint x);

	void enableSyncing(bool en);
	void setSyncInterval(int interval);

protected:
	int syncNext();
	int dataReady(const unsigned char *bytes, int len);
	QByteArray transportReady();

	bool syncEnabled;
	int syncInterval;
	uint nextSync;
	QElapsedTimer syncTime;
};

#endif // YAMANOLENSHEAD_H
