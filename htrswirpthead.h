#ifndef HTRSWIRPTHEAD_H
#define HTRSWIRPTHEAD_H

#include <ecl/ptzp/ptzphead.h>

#include "QStringList"

class HtrSwirPtHead : public PtzpHead
{
public:
	HtrSwirPtHead();

	void fillCapabilities(ptzp::PtzHead *head);
	int getHeadStatus();

	virtual int panLeft(float speed);
	virtual int panRight(float speed);
	virtual int tiltUp(float speed);
	virtual int tiltDown(float speed);
	virtual int panTiltAbs(float pan, float tilt);
	virtual int panTiltStop();
	/*
	 * NOTE: useless because device not support.
	 */
//	virtual int panTiltDegree(float pan, float tilt);
	virtual float getPanAngle();
	virtual float getTiltAngle();
	int panTiltGoPos(float ppos, float tpos);

protected:

	int sendCommand(const QString &key);
	/*
	 * NOTE: useless because device not support.
	 */
	QElapsedTimer syncTimer;
	int panPos;
	int tiltPos;
//	int syncFlag;
	int dataReady(const unsigned char *bytes, int len);
	QByteArray transportReady();
};

#endif // HTRSWIRPTHEAD_H
