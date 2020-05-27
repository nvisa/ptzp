#ifndef FLIRPTTCPHEAD_H
#define FLIRPTTCPHEAD_H

#include "ptzphead.h"

class FlirPTTcpHead : public PtzpHead
{
	Q_OBJECT
public:
	struct FlirConfig
	{
		int panSpeed;
		int tiltSpeed;
		int panPos;
		int tiltPos;
		int minPanPos;
		int maxPanPos;
		int minTiltPos;
		int maxTiltPos;
	};

	FlirPTTcpHead();
	int getHeadStatus() override;
	void fillCapabilities(ptzp::PtzHead *head) override;
	int panLeft(float speed) override;
	int panRight(float speed) override;
	int tiltUp(float speed) override;
	int tiltDown(float speed) override;
	int panTiltAbs(float pan, float tilt) override;
	int panTiltGoPos(float ppos, float tpos) override;
	int panTiltStop() override;
	float getPanAngle() override;
	float getTiltAngle() override;
	int home();
	int panWOffset(int speed, int offset);
	int tiltWOffset(int speed, int offset);
	int pantiltWOffset(int pSpeed, int pOffset, int tSpeed, int tOffset);
	int panSet(int pDeg, int pSpeed);
	int tiltSet(int tDeg, int tSpeed);
	int pantiltSet(float pDeg, float tDeg, int pSpeed, int tSpeed);
	QVariant getCapabilityValues(ptzp::PtzHead_Capability c);
	void setCapabilityValues(ptzp::PtzHead_Capability c, uint val);

	void initialize();
signals:

public slots:
protected:
	QByteArray transportReady() override;
	int dataReady(const unsigned char *bytes, int len) override;
protected:
	int sendCommand(const QString &key);
	int parsePositions(QString mes);
	void printFlirConfigs();
private:
	FlirConfig flirConfig;
	QStringList ptzCommandList;

};

#endif // FLIRPTTCPHEAD_H
