#ifndef FLIRMODULEHEAD_H
#define FLIRMODULEHEAD_H

#include <ecl/ptzp/ptzphead.h>
#include "ecl/net/cgi/cgidevicedata.h"

#include <QStringList>
#include <QElapsedTimer>

class CgiManager;

class FlirModuleHead : public PtzpHead
{
public:
	FlirModuleHead();

	int connect(QString &targetUri);
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

protected:
	CgiManager* cgiMan;
	CgiDeviceData cgiConnData;
};

#endif // FLIRMODULEHEAD_H
