#ifndef MGEOSWIRHEAD_H
#define MGEOSWIRHEAD_H

#include <ecl/ptzp/ptzphead.h>

#include <QElapsedTimer>
#include <QStringList>

class MgeoSwirHead : public PtzpHead
{
public:
	MgeoSwirHead();

	enum Registers {
		R_ZOOM_POS,
		R_FOCUS_POS,
		R_GAIN,
		R_IBIT_RESULT,
	};

	int getCapabilities();
	virtual int syncRegisters();
	virtual int startZoomIn(int speed);
	virtual int startZoomOut(int speed);
	virtual int stopZoom();
	virtual int getZoom();
	virtual int getHeadStatus();
	virtual void setProperty(uint r, uint x);
	virtual uint getProperty(uint r);

protected:
	uint nextSync;

	int syncNext();
	int sendCommand(const QString &key);
	QByteArray transportReady();
	virtual int dataReady(const unsigned char *bytes, int len);

};

#endif // MGEOSWIRHEAD_H
