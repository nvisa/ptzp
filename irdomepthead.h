#ifndef IRDOMEPTHEAD_H
#define IRDOMEPTHEAD_H

#include <ecl/ptzp/ptzphead.h>

#include <QElapsedTimer>

class IRDomePTHead : public PtzpHead
{
public:
	IRDomePTHead();

	enum Registers {
		R_PAN_ANGLE,
		R_TILT_ANGLE,

		R_COUNT
	};

	int getCapabilities();
	int panLeft(float speed);
	int panRight(float speed);
	int tiltUp(float speed);
	int tiltDown(float speed);
	int panTiltAbs(float pan, float tilt);
	int panTiltStop();
	float getPanAngle();
	float getTiltAngle();

protected:
	int dataReady(const unsigned char *bytes, int len);
	QByteArray transportReady();
	int panTilt(uint cmd, int pspeed, int tspeed);

	QElapsedTimer syncTime;
};

#endif // IRDOMEPTHEAD_H
