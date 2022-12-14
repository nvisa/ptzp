#include "tbgthdriver.h"
#include "debug.h"
#include "gpiocontroller.h"
#include "evpupthead.h"
#include "mgeothermalhead.h"
#include "ptzpserialtransport.h"
#include "ptzptcptransport.h"
#include "yamanolenshead.h"

#include <QFile>
#include <QTcpSocket>
#include <QTimer>
#include <errno.h>

#define GPIO_WIPER		  187
#define GPIO_WATER		  186
#define GPIO_WIPER_STATUS 89
#define GPIO_WATER_STATUS 202

#define zoomEntry(zoomv, h, v)                                                 \
	zooms.push_back(zoomv);                                                    \
	hmap.push_back(h);                                                         \
	vmap.push_back(v)

static float speedRegulateDay(float speed, float fovs[])
{
	int fov = fovs[0];
	if (fov < 0.8)
		return 0.01;
	if (fov > 25.6)
		return 1.0;
	float zoomr = (fov - 0.8) / (float)(25.6 - 0.8);
	speed *= zoomr;
	if (speed < 0.01)
		speed = 0.01;
	if (speed > 1.0)
		speed = 1.0;
	return speed;
}

static float speedRegulateThermal(float speed, float fovs[])
{
	int fov = fovs[0];
	if (fov < 0.82)
		return 0.01;
	if (fov > 9.38)
		return 1.0;
	float zoomr = (fov - 0.82) / (float)(9.38 - 0.82);
	speed *= zoomr;
	if (speed < 0.01)
		speed = 0.01;
	if (speed > 1.0)
		speed = 1.0;
	return speed;
}

TbgthDriver::TbgthDriver(bool useThermal, QObject *parent) : PtzpDriver(parent)
{
	headLens = new YamanoLensHead;
	headEvpuPt = new EvpuPTHead;
	headThermal = new MgeoThermalHead;
	state = INIT;
	defaultPTHead = headEvpuPt;
	if (useThermal)
		defaultModuleHead = headThermal;
	else
		defaultModuleHead = headLens;
	yamanoActive = false;
	evpuActive = false;
	controlThermal = useThermal;

	gpiocont->requestGpio(GPIO_WATER, GpioController::OUTPUT, "WaterControl");
	gpiocont->requestGpio(GPIO_WIPER, GpioController::OUTPUT, "WiperControl");
	gpiocont->requestGpio(GPIO_WATER_STATUS, GpioController::INPUT,
						  "WaterStatus");
	gpiocont->requestGpio(GPIO_WIPER_STATUS, GpioController::INPUT,
						  "WiperStatus");

	/* yamano fov mapping */
	std::vector<float> hmap, vmap;
	std::vector<int> zooms;
	if (!useThermal) {
		zoomEntry(88, 25.6, 15);
		zoomEntry(122, 22.8, 13.3);
		zoomEntry(157, 20.4, 12);
		zoomEntry(191, 18.2, 10.7);
		zoomEntry(225, 16.2, 9.6);
		zoomEntry(259, 14.4, 8.5);
		zoomEntry(294, 12.8, 7.6);
		zoomEntry(328, 11.3, 6.7);
		zoomEntry(362, 9.9, 6);
		zoomEntry(397, 8.8, 5.3);
		zoomEntry(431, 7.7, 4.7);
		zoomEntry(465, 6.7, 4.1);
		zoomEntry(499, 5.9, 3.6);
		zoomEntry(534, 5.1, 3.2);
		zoomEntry(568, 4.4, 2.8);
		zoomEntry(602, 3.8, 2.5);
		zoomEntry(637, 3.3, 2.2);
		zoomEntry(671, 2.8, 1.9);
		zoomEntry(705, 2.4, 1.6);
		zoomEntry(740, 2.1, 1.4);
		zoomEntry(774, 1.7, 1.2);
		zoomEntry(808, 1.5, 1);
		zoomEntry(843, 1.3, 0.9);
		zoomEntry(877, 1.1, 0.7);
		zoomEntry(911, 0.9, 0.6);
		zoomEntry(945, 0.8, 0.46);
		headLens->getRangeMapper()->setLookUpValues(zooms);
		headLens->getRangeMapper()->addMap(hmap);
		headLens->getRangeMapper()->addMap(vmap);
	}
	// hmap.clear();
	// vmap.clear();
	// zooms.clear();
	else if (useThermal) {
		zoomEntry(6875, 0.82, 0.66);
		// zoomEntry(6875, 9.38, 7.44);
		zoomEntry(10625, 0.92, 0.75);
		// zoomEntry(10625, 5.28, 3.98);
		zoomEntry(13750, 1.25, 0.93);
		// zoomEntry(13750, 3.19, 2.25);
		zoomEntry(17500, 1.42, 1.08);
		// zoomEntry(17500, 1.46, 1.28);
		zoomEntry(20625, 1.46, 1.28);
		// zoomEntry(20625, 1.42, 1.08);
		zoomEntry(24375, 3.19, 2.25);
		// zoomEntry(24375, 1.25, 0.93);
		zoomEntry(27656, 5.28, 3.98);
		// zoomEntry(27656, 0.92, 0.75);
		zoomEntry(31249, 9.38, 7.44);
		// zoomEntry(31249, 0.82, 0.66);
		headThermal->getRangeMapper()->setLookUpValues(zooms);
		headThermal->getRangeMapper()->addMap(hmap);
		headThermal->getRangeMapper()->addMap(vmap);
	}
}

PtzpHead *TbgthDriver::getHead(int index)
{
	if (index == 0)
		return headLens;
	else if (index == 1)
		return headEvpuPt;
	else if (index == 2)
		return headThermal;
	return NULL;
}

int TbgthDriver::setTarget(const QString &targetUri)
{
	QStringList fields = targetUri.split(";");
	foreach (const QString &field, fields) {
		if (field.contains("baud")) {
			/* setting-up serial head */
			tp = new PtzpSerialTransport();
			headLens->setTransport(tp);
			headLens->enableSyncing(true);
			if (tp->connectTo(field))
				return -EPERM;
			yamanoActive = true;
		} else if (field.contains(":10000")) {
			/* setting-up tcp head */
			tp1 = new PtzpTcpTransport(PtzpTransport::PROTO_BUFFERED);
			headEvpuPt->setTransport(tp1);
			if (controlThermal)
				headThermal->setTransport(tp1);
			// hack us in for EVPU offloading
			((PtzpTcpTransport *)tp1)->setFilter(this);

			tp1ConnectionString = QString("%1").arg(field);
			int err = tp1->connectTo(tp1ConnectionString);
			if (err)
				return err;
			evpuActive = true;
			tp1->send(QString("s4 mode 57600 8 1 n\r\n").toUtf8());
			/* initialize device and outputs */
			headEvpuPt->syncDevice();
			for (int i = 0; i < 16; i++)
				headEvpuPt->setOutput(i, 1);
		}
	}

	if (yamanoActive) {
		SpeedRegulation sreg = getSpeedRegulation();
		sreg.enable = true;
		sreg.ipol = SpeedRegulation::CUSTOM;
		sreg.interFunc = speedRegulateDay;
		sreg.zoomHead = headLens;
		setSpeedRegulation(sreg);
	} else if (controlThermal) {
		SpeedRegulation sreg = getSpeedRegulation();
		sreg.enable = true;
		sreg.ipol = SpeedRegulation::CUSTOM;
		sreg.interFunc = speedRegulateThermal;
		sreg.zoomHead = headThermal;
		setSpeedRegulation(sreg);
	}

	return 0;
}

QByteArray TbgthDriver::sendFilter(const char *bytes, int len)
{
	if (bytes[0] == 0x35) {
		// dump(QByteArray(bytes, len));
		QByteArray mes = QString("s4 data %1 ").arg(len).toUtf8();
		mes.append(bytes, len);
		return mes;
	}

	return QByteArray();
}

int TbgthDriver::readFilter(QAbstractSocket *sock, QByteArray &ba)
{
	int spcnt = fstate.prefix.count(QChar(' '));
	if (spcnt < 3) {
		char ch;
		sock->read(&ch, 1);
		fstate.prefix.append(QChar(ch));
		/* sync logic */
		if (fstate.prefix.size() && fstate.prefix[0] != 's')
			fstate.prefix.clear();
		if (fstate.prefix.size() > 1) {
			bool ok;
			int num = fstate.prefix.mid(1, 1).toInt(&ok);
			if (!ok || num > 4)
				fstate.prefix.clear();
		}
		if (fstate.prefix.size() > 6 && fstate.prefix.mid(3, 4) != "data")
			fstate.prefix.clear();

		return -EAGAIN;
	}

	if (!fstate.fields.size())
		fstate.fields = fstate.prefix.split(' ');
	if (!fstate.payloadLen)
		fstate.payloadLen = fstate.fields[2].toInt();

	if (fstate.payload.size() < fstate.payloadLen) {
		int maxlen = fstate.payloadLen - fstate.payload.size();
		if (maxlen <= 0) {
			mDebug("Unexpected maxlen");
			return -EIO;
		}
		QByteArray pba = sock->read(maxlen);
		fstate.payload.append(pba);
		if (fstate.payload.size() == fstate.payloadLen) {
			// ffDebug() << fstate.payload << fstate.fields <<
			// fstate.payload.size();
			ba.append(fstate.payload);
			fstate = FilteringState();
			return 0;
		}
		return -EAGAIN;
	}

	mDebug("Unexpected filter state - %d %d", fstate.payload.size(),
		   fstate.payloadLen);
	return -EIO;
}

void TbgthDriver::enableDriver(bool value)
{
	PtzpDriver::enableDriver(value);
	if (!driverEnabled) {
		state = EVPU_RELEASE;
	} else {
		state = EVPU_GRAB;
	}
}

grpc::Status TbgthDriver::SetIO(grpc::ServerContext *context,
								const ptzp::IOCmdPar *request,
								ptzp::IOCmdPar *response)
{
	if (request->name_size() != request->state_size())
		return grpc::Status::CANCELLED;
	for (int i = 0; i < request->name_size(); i++) {
		std::string name = request->name(i);
		const ptzp::IOState &sin = request->state(i);
		ptzp::IOState *sout = response->add_state();
		sout->set_direction(ptzp::IOState_Direction_OUTPUT);
		sout->set_value(sin.value());

		qDebug() << "relay name is " << QString::fromStdString(name);
		if (name.compare(std::string("thermal")) == 0) {
			qDebug() << "sin value is " << sin.value();
			if (sin.value() == ptzp::IOState_OutputValue_HIGH)
				headEvpuPt->setOutput(7, 1);
			else
				headEvpuPt->setOutput(7, 0);
		} else if (name.compare(std::string("daytv")) == 0) {
			if (sin.value() == ptzp::IOState_OutputValue_HIGH)
				headEvpuPt->setOutput(6, 1);
			else
				headEvpuPt->setOutput(6, 0);
		} else if (name.compare(std::string("heater")) == 0) {
			if (sin.value() == ptzp::IOState_OutputValue_HIGH)
				headEvpuPt->setOutput(5, 1);
			else
				headEvpuPt->setOutput(5, 0);
		} else if (name.compare(std::string("wiper")) == 0) {
			if (sin.value() == ptzp::IOState_OutputValue_HIGH)
				headEvpuPt->setOutput(4, 1);
			else
				headEvpuPt->setOutput(4, 0);
		}
	}
	return PtzpDriver::SetIO(context, request, response);
}

QJsonObject TbgthDriver::doExtraDeviceTests()
{
	QJsonObject jsonObject;

	if (!controlThermal) {
		jsonObject.insert("device_type", QString("day_camera"));
		int wiperStatus = gpiocont->getGpioValue(89);
		jsonObject.insert("wiper_status", wiperStatus);
		int waterStatus = gpiocont->getGpioValue(202);
		jsonObject.insert("water_status", waterStatus);
		return jsonObject;
	}
	jsonObject.insert("device_type", QString("thermal_camera"));
	return jsonObject;
}

bool TbgthDriver::isReady()
{
	if (state == NORMAL)
		return true;
	return false;
}

void TbgthDriver::timeout()
{
	switch (state) {
	case INIT:
		if (yamanoActive) {
			headLens->syncRegisters();
			state = SYNC_HEAD_LENS;
		} else if (controlThermal) {
			headThermal->syncRegisters();
			state = SYNC_HEAD_THERMAL;
		} else {
			state = NORMAL;
			if (evpuActive)
				tp1->enableQueueFreeCallbacks(true);
		}
		break;
	case SYNC_HEAD_LENS:
		if (headLens->getHeadStatus() == PtzpHead::ST_NORMAL) {
			if (controlThermal) {
				headThermal->syncRegisters();
				state = SYNC_HEAD_THERMAL;
			} else {
				state = NORMAL;
				if (evpuActive)
					tp1->enableQueueFreeCallbacks(true);
			}
			tp->enableQueueFreeCallbacks(true);
		}
		break;
	case SYNC_HEAD_THERMAL:
		if (headThermal->getHeadStatus() == PtzpHead::ST_NORMAL) {
			state = NORMAL;
			if (evpuActive)
				tp1->enableQueueFreeCallbacks(true);
		}
	case NORMAL:
		break;
	case EVPU_GRAB:
		tp1->connectTo(tp1ConnectionString);
		state = NORMAL;
		break;
	case EVPU_RELEASE:
		((PtzpTcpTransport *)tp1)->disconnectFrom();
		state = EVPU_GRAB_WAIT;
		break;
	case EVPU_GRAB_WAIT:
		break;
	}

	PtzpDriver::timeout();
}
