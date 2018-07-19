#ifndef ARYAPTHEAD_H
#define ARYAPTHEAD_H

#include <ecl/ptzp/ptzphead.h>

class AryaPTHead : public PtzpHead
{
public:
	AryaPTHead();

	int getCapabilities();
	int getHeadStatus();

	virtual int panLeft(float speed);
	virtual int panRight(float speed);
	virtual int tiltUp(float speed);
	virtual int tiltDown(float speed);
	virtual int panTiltAbs(float pan, float tilt);
	virtual int panTiltStop();
	virtual float getPanAngle();
	virtual float getTiltAngle();

protected:
	QStringList ptzCommandList;
	int panPos;
	int tiltPos;

	int sendCommand(const QString &key);
	int dataReady(const unsigned char *bytes, int len);
};

#endif // ARYAPTHEAD_H
