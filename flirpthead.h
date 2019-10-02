#ifndef FLIRPTHEAD_H
#define FLIRPTHEAD_H

#include <QStringList>

#include "ecl/ptzp/ptzphead.h"

class FlirPTHead : public PtzpHead
{
public:
	FlirPTHead();

	int getCapabilities();
	int getHeadStatus();
	int panLeft(float speed);
	int panRight(float speed);
	int tiltUp(float speed);
	int tiltDown(float speed);
	int panTiltAbs(float pan, float tilt);
	int home();
	int panWOffset(int speed, int offset);
	int tiltWOffset(int speed, int offset);
	int pantiltWOffset(int pSpeed, int pOffset, int tSpeed, int tOffset);
	int panSet(int pDeg, int pSpeed);
	int tiltSet(int tDeg, int tSpeed);
	int pantiltSet(float pDeg, float tDeg, int pSpeed, int tSpeed);
	int panTiltGoPos(float ppos, float tpos) override;
	int panTiltStop();
	float getPanAngle();
	float getTiltAngle();
	int getFlirConfig();

protected:
	struct FlirConfigs
	{
		float panUpperSpeed;
		float tiltUpperSpeed;
		float maxPanPos;
		float minPanPos;
		float maxTiltPos;
		float minTiltPos;
	};
	FlirConfigs flir;
	QHash<QString, QString> flirConfig;
	void setupFlirConfigs();
protected:
	QStringList ptzCommandList;
	int panPos;
	int tiltPos;
	int nextSync;
	QList<uint> syncList;
	int dataReady(const unsigned char *bytes, int len);
	int saveCommand(const QString &key);
	QByteArray transportReady();
};

#endif // FLIRPTHEAD_H
