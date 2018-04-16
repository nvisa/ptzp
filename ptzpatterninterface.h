#ifndef PTZPATTERNINTERFACE
#define PTZPATTERNINTERFACE

class PtzPatternInterface
{
public:
	virtual void positionUpdate(int pan, int tilt, int zoom) = 0;
	virtual void commandUpdate(int pan, int tilt, int zoom, int c, int par1, int par2) = 0;
	virtual int start(int pan, int tilt, int zoom) = 0;
	virtual void stop(int pan, int tilt, int zoom) = 0;
};

#endif // PTZPATTERNINTERFACE

