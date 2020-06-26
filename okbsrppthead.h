#ifndef OKBSRPPTHEAD_H
#define OKBSRPPTHEAD_H

#include <ptzphead.h>

#include "QStringList"

class OkbSrpPTHead : public PtzpHead
{
public:
	OkbSrpPTHead();

	void fillCapabilities(ptzp::PtzHead *head);
	int getHeadStatus();

//	virtual int panLeft(float speed);
//	virtual int panRight(float speed);
//	virtual int tiltUp(float speed);
//	virtual int tiltDown(float speed);
//	virtual int panTiltAbs(float pan, float tilt);
//	virtual int panTiltDegree(float pan, float tilt);
//	virtual int panTiltStop();
	virtual float getPanAngle();
	virtual float getTiltAngle();
//	int panTiltGoPos(float ppos, float tpos);
	QVariant getCapabilityValues(ptzp::PtzHead_Capability c);
	void setCapabilityValues(ptzp::PtzHead_Capability c, uint val);

//	void setMaxSpeed(int value);

protected:
//	QStringList ptzCommandList;
	float panPos;
	float tiltPos;
//	int MaxSpeed;
//	int sendCommand(const QString &key);
	int dataReady(const unsigned char *bytes, int len);
//	QByteArray transportReady();
};

#endif // OKBSRPPTHEAD_H
