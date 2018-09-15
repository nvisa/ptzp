#ifndef PTZPATTERNINTERFACE
#define PTZPATTERNINTERFACE

#include "QString"

class PtzPatternInterface
{
public:
	virtual void positionUpdate(int pan, int tilt, int zoom) = 0;
	virtual void commandUpdate(int pan, int tilt, int zoom, int c, float par1, float par2) = 0;
	virtual int start(int pan, int tilt, int zoom) = 0;
	virtual void stop(int pan, int tilt, int zoom) = 0;

	virtual void addPreset(QString name, float panPos, float tiltPos, int zoomPos) = 0;
	virtual void goToPreset(QString name) = 0;
	virtual void deletePreset(QString name) = 0;
	virtual void addPatrol(QString name, QString pSet, QString time) = 0;
	virtual void runPatrol(QString name) = 0;
	virtual void stopPatrol(QString name) = 0;
	virtual void deletePatrol(QString name) = 0;
};

#endif // PTZPATTERNINTERFACE

