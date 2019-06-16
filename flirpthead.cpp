#include "flirpthead.h"
#include "debug.h"

#include <QTimer>
#include <QThread>
#include <assert.h>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkConfigurationManager>

#define MaxSpeed	0x7D0		//		2.000
#define MaxPanPos	28000.0		//L-R	+180deg
#define MinPanPos	-27999.0	//R-L	-180deg
#define MaxTiltPos	9330.0		//U		+30deg
#define MinTiltPos	27999.0		//D		-90deg


enum CommandList {
	C_STOP,
	C_GET_PT,
	C_PAN_LEFT,
	C_PAN_RIGHT,
	C_TILT_UP,
	C_TILT_DOWN,
	C_PT_HOME,
	C_PAN_WITH_OFFSET,
	C_TILT_WITH_OFFSET,
	C_PANTILT_WITH_OFFSET,
	C_PAN_SET,
	C_TILT_SET,
	C_PANTILT_SET,
	C_PAN_LEFT_TILT_UP,
	C_PAN_LEFT_TILT_DOWN,
	C_PAN_RIGHT_TILT_UP,
	C_PAN_RIGHT_TILT_DOWN,

	C_COUNT,
};

static QStringList createPTZCommandList()
{
	QStringList cmdListFlir;
	cmdListFlir << QString("H");
	cmdListFlir << QString("PP&TP&PD&TD&C");
	cmdListFlir << QString("C=V&PS=-%1"); //C=V&PS=%1&TS=%2&PS=-%3
	cmdListFlir << QString("C=V&PS=%1"); //C=V&PS=%1&TS=%2&PS=%3
	cmdListFlir << QString("C=V&TS=%1"); //C=V&TS=%1&PS=%2&TS=%3
	cmdListFlir << QString("C=V&TS=-%1"); // C=V&TS=-%1&PS=%2&TS=-%3
	cmdListFlir << QString("C=I&PP=0&TP=0&PS=2000&TS=2000");
	cmdListFlir << QString("C=I&PS=%1&PO=%2");
	cmdListFlir << QString("C=I&TS=%1&TO=%2");
	cmdListFlir << QString("C=I&PS=%1&TS=%2&PO=%3&TO=%4");
	cmdListFlir << QString("C=I&PP=%1&PS=%2");
	cmdListFlir << QString("C=I&TP=%1&TS=%2");
	cmdListFlir << QString("C=I&PP=%1&TP=%2&PS=%3&TS=%4");
	cmdListFlir << QString("C=V&PS=%1&TS=%2");
	cmdListFlir << QString("C=V&PS=%1&TS=%2");
	cmdListFlir << QString("C=V&PS=%1&TS=%2");
	cmdListFlir << QString("C=V&PS=%1&TS=%2");

	return cmdListFlir;
}

FlirPTHead::FlirPTHead()
	: PtzpHead()
{
	netman = NULL;
	ptzCommandList = createPTZCommandList();
	assert(ptzCommandList.size() == C_COUNT);

	//Cihaz bufferı çok hızlı doluyor, (yaklaşık 3-4 mesajda), cevap dönme süresi 7ms, bu sebeple;
	//+Gönderilecek komutlar bir queue objesinde tutularak timer yardımıyla 15ms'de 1 komut olacak şekilde gönderiliyor.
	timerWrtData = new QTimer(this);
	connect(timerWrtData, &QTimer::timeout, this, &FlirPTHead::sendCommand);
	timerWrtData->start(15);

	//200ms'de bir pan-tilt pozisyon güncellemesi
	timerGetData = new QTimer(this);
	connect(timerGetData, &QTimer::timeout, this, &FlirPTHead::getPositions);
	timerGetData->start(200);

	netman = new QNetworkAccessManager(this);
	connect(netman, SIGNAL(finished(QNetworkReply*)), this, SLOT(dataReady(QNetworkReply*)));
}

int FlirPTHead::connectHTTP(const QString &targetUri)
{
	QString url;
	if (!targetUri.startsWith("http://"))
		url = QString("http://%1").arg(targetUri);
	request.setUrl(QUrl(targetUri));
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
	return 0;
}

int FlirPTHead::getCapabilities()
{
	return CAP_PAN | CAP_TILT;
}

int FlirPTHead::getHeadStatus()
{
	if (pingTimer.elapsed() < 1500)
		return ST_NORMAL;
	return ST_ERROR;
}

int FlirPTHead::panLeft(float speed)
{
	speed = qAbs(speed) * MaxSpeed;
	return saveCommand(ptzCommandList.at(C_PAN_LEFT).arg(speed));
}

int FlirPTHead::panRight(float speed)
{
	speed = qAbs(speed) * MaxSpeed;
	return saveCommand(ptzCommandList.at(C_PAN_RIGHT).arg(speed));
}

int FlirPTHead::tiltUp(float speed)
{
	speed = qAbs(speed) * MaxSpeed;
	return saveCommand(ptzCommandList.at(C_TILT_UP).arg(speed));
}

int FlirPTHead::tiltDown(float speed)
{
	speed = qAbs(speed) * MaxSpeed;
	return saveCommand(ptzCommandList.at(C_TILT_DOWN).arg(speed));
}

int FlirPTHead::panTiltAbs(float pan, float tilt)
{
	int pspeed = qAbs(pan) * MaxSpeed;
	int tspeed = qAbs(tilt) * MaxSpeed;
	if (pspeed == 0 && tspeed == 0) {
		saveCommand(ptzCommandList[C_STOP]);
		return 0;
	}
	if (pan < 0) {
		// left
		if (tilt < 0) {
			// up
			saveCommand(ptzCommandList.at(C_PAN_LEFT_TILT_UP).arg(-pspeed).arg(tspeed));			//	C_PAN_LEFT_TILT_UP).arg(pspeed).arg(tspeed));
		} else if (tilt > 0) {
			// down
			saveCommand(ptzCommandList.at(C_PAN_LEFT_TILT_DOWN).arg(-pspeed).arg(-tspeed));
		} else {
			saveCommand(ptzCommandList.at(C_PAN_LEFT).arg(-pspeed));
		}
	} else {
		// right
		if (tilt < 0) {
			// up
			saveCommand(ptzCommandList.at(C_PAN_RIGHT_TILT_UP).arg(pspeed).arg(tspeed));
		} else if (tilt > 0) {
			// down
			saveCommand(ptzCommandList.at(C_PAN_RIGHT_TILT_DOWN).arg(pspeed).arg(-tspeed));
		} else {
			saveCommand(ptzCommandList.at(C_PAN_RIGHT).arg(pspeed));
		}
	}
	return 0;
}

int FlirPTHead::home()
{
	return saveCommand(ptzCommandList.at(C_PT_HOME));
}

int FlirPTHead::panWOffset(int speed, int offset)
{
	speed = qAbs(speed) * MaxSpeed;
	return saveCommand(ptzCommandList.at(C_PAN_WITH_OFFSET).arg(speed).arg(offset));
}

int FlirPTHead::tiltWOffset(int speed, int offset)
{
	speed = qAbs(speed) * MaxSpeed;
	return saveCommand(ptzCommandList.at(C_TILT_WITH_OFFSET).arg(speed).arg(offset));
}

int FlirPTHead::pantiltWOffset(int pSpeed, int pOffset, int tSpeed, int tOffset)
{
	tSpeed = tSpeed * MaxSpeed;
	pSpeed = pSpeed * MaxSpeed;
	return saveCommand(ptzCommandList.at(C_PANTILT_WITH_OFFSET).arg(pSpeed).arg(tSpeed).arg(pOffset).arg(tOffset));
}

int FlirPTHead::panSet(int pDeg, int pSpeed)
{
	int pPoint;
	if(pDeg <= 180)
		pPoint = (int)((pDeg/180) * MaxPanPos);
	else
		pPoint = (int)((1 - ((pDeg-180) / 180)) * MinPanPos);

	pSpeed = pSpeed * MaxSpeed;
	return saveCommand(ptzCommandList.at(C_PAN_SET).arg(pPoint).arg(pSpeed));
}

int FlirPTHead::tiltSet(int tDeg, int tSpeed)
{
	int tPoint;
	if(tDeg <= 180)
		tPoint = (tDeg/180) * MaxTiltPos;
	else
		tPoint = (1 - ((tDeg-180) / 180)) * MinTiltPos;
	tSpeed = tSpeed * MaxSpeed;
	return saveCommand(ptzCommandList.at(C_TILT_SET).arg(tPoint).arg(tSpeed));
}

int FlirPTHead::pantiltSet(float pDeg, float tDeg, int pSpeed, int tSpeed)
{
	int pPoint, tPoint;

	if(pDeg <= 180)
		pPoint = (int)((pDeg/180) * MaxPanPos);
	else
		pPoint = (int)((1 - ((pDeg-180) / 180)) * MinPanPos);

	if(tDeg <= 180)
		tPoint = (int)((tDeg/180) * MaxTiltPos);
	else
		tPoint = (int)((1 - ((tDeg-180) / 180)) * MinTiltPos);

	pSpeed = qAbs(pSpeed) * MaxSpeed;
	tSpeed = qAbs(tSpeed) * MaxSpeed;
	return saveCommand(ptzCommandList.at(C_PANTILT_SET).arg(pPoint).arg(tPoint).arg(pSpeed).arg(tSpeed));
}

int FlirPTHead::panTiltStop()
{
	return saveCommand(ptzCommandList.at(C_STOP));
}

float FlirPTHead::getPanAngle()
{
	return panPos;
}

float FlirPTHead::getTiltAngle()
{
	return tiltPos;
}

void FlirPTHead::sendCommand()
{
	if(queue.isEmpty())
		return;
	else
		netman->post( request, queue.dequeue().toUtf8() );
}

int FlirPTHead::saveCommand(const QString &key)
{
	queue.enqueue(key);
	return 0;
}

void FlirPTHead::dataReady(QNetworkReply *reply)
{
	QByteArray mes = reply->readAll();
	if(!mes.contains("PD")) return;
	if(!mes.startsWith("{")) return;
	if(!mes.endsWith("}")) return;

	int indexPP = mes.indexOf("PP");
	int indexEPP = mes.indexOf(",",indexPP);
	panPos = mes.mid(indexPP+7,indexEPP-indexPP-8).toInt();

	int indexTP = mes.indexOf("TP");
	int indexETP = mes.indexOf(",",indexTP);
	tiltPos = mes.mid(indexTP+7,indexETP-indexTP-8).toInt();
}

void FlirPTHead::getPositions()
{
	saveCommand(ptzCommandList.at(C_GET_PT));
}
