#ifndef FLIRPTHEAD_H
#define FLIRPTHEAD_H

#include <ecl/ptzp/ptzphead.h>

#include <QTimer>
#include "QStringList"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QQueue>

class FlirPTHead :public PtzpHead
{
	Q_OBJECT
public:
	FlirPTHead();

	int connectHTTP(const QString &targetUri);
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

private:
	QQueue<QString> queue;
	QTimer *timerWrtData;
	QTimer *timerGetData;
	QNetworkAccessManager *netman;
	QNetworkRequest request;

private slots:
	void sendCommand();
	void getPositions();
	void dataReady(QNetworkReply* reply);

protected:
	QStringList ptzCommandList;
	int panPos;
	int tiltPos;
	int nextSync;
	QList<uint> syncList;
	int saveCommand(const QString &key);

};

#endif // FLIRPTHEAD_H
