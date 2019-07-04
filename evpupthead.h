#ifndef EVPUPTHEAD_H
#define EVPUPTHEAD_H

#include <ecl/ptzp/ptzphead.h>

#include <QElapsedTimer>

class EvpuPTHead : public PtzpHead
{
public:
	EvpuPTHead();

	int getCapabilities();
	int getHeadStatus();

	virtual int panLeft(float speed);
	virtual int panRight(float speed);
	virtual int tiltUp(float speed);
	virtual int tiltDown(float speed);
	virtual int panTiltAbs(float pan, float tilt);
	virtual int panTiltDegree(float pan, float tilt);
	virtual int panTiltStop();
	virtual float getPanAngle();
	virtual float getTiltAngle();
	int panTiltGoPos(float ppos, float tpos);

	int setOutput(int no, bool on);
	void syncDevice();

protected:
	int panPos;
	int tiltPos;
	int syncFlag;

	int sendCommand(const QString &key);
	int dataReady(const unsigned char *bytes, int len);
	QByteArray transportReady();
};

#endif // EVPUPTHEAD_H
