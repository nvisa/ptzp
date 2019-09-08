#ifndef FLIRMODULEHEAD_H
#define FLIRMODULEHEAD_H

#include <ecl/ptzp/ptzphead.h>

class FlirModuleHead : public PtzpHead
{
public:
	FlirModuleHead();

	int getCapabilities();
	virtual int startZoomIn(int speed);
	virtual int startZoomOut(int speed);
	virtual int stopZoom();
	virtual int getZoom();
	virtual int focusIn(int speed);
	virtual int focusOut(int speed);
	virtual int focusStop();
	virtual void setProperty(uint r, uint x);
	virtual uint getProperty(uint r);

	int setFocusMode(uint x);
	int setFocusValue(uint x);
	int getFocusMode();
	int getFocus();

protected:
	QStringList commandList;
	int sendCommand(const QString &key);
	QByteArray transportReady();
	int dataReady(const unsigned char *bytes, int len);
	int focusPos;
	int zoomPos;
	int lastGetCommand;
	QHash<QString, QString> camModes;
};

#endif // FLIRMODULEHEAD_H
