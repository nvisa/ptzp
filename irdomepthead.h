#ifndef IRDOMEPTHEAD_H
#define IRDOMEPTHEAD_H

#include <ptzphead.h>

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

	enum DeviceVariant {
		BOTAS_DOME,
		ABSGS_DOME,
	};

	void fillCapabilities(ptzp::PtzHead *head);
	int panLeft(float speed);
	int panRight(float speed);
	int tiltUp(float speed);
	int tiltDown(float speed);
	int panTiltAbs(float pan, float tilt);
	int panTiltDegree(float pan, float tilt);
	int panTiltStop();
	float getPanAngle();
	float getTiltAngle();
	int panTiltGoPos(float ppos, float tpos);
	void enableSyncing(bool en);
	void setSyncInterval(int interval);

	float getMaxPatternSpeed() const;

	int setIRLed(int led);
	int getIRLed();

	void setDeviceVariant(DeviceVariant v);
	DeviceVariant getDeviceVariant() { return devvar; }
	QVariant getCapabilityValues(ptzp::PtzHead_Capability c);
	void setCapabilityValues(ptzp::PtzHead_Capability c, uint val);

protected:
	int dataReady(const unsigned char *bytes, int len);
	QByteArray transportReady();
	int panTilt(uint cmd, int pspeed, int tspeed);
	QJsonValue marshallAllRegisters();
	void unmarshallloadAllRegisters(const QJsonValue &node);

	/* [CR] [yca] bunu bir register yapamiyor muyuz? */
	int irLedLevel;
	int maxPatternSpeed;
	bool syncEnabled;
	int syncInterval;
	QElapsedTimer syncTime;
	std::vector<float> speedTableMapping;
	DeviceVariant devvar;
};

#endif // IRDOMEPTHEAD_H
