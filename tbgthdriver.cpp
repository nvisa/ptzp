#include "tbgthdriver.h"
#include "yamanolenshead.h"
#include "evpupthead.h"
#include "ptzpserialtransport.h"
#include "ptzptcptransport.h"
#include "mgeothermalhead.h"
#include "debug.h"
#include "drivers/gpiocontroller.h"

#include <QFile>
#include <QTimer>
#include <errno.h>
#include <QTcpSocket>

#define GPIO_WIPER			187
#define GPIO_WATER			186
#define GPIO_WIPER_STATUS	89
#define GPIO_WATER_STATUS	202

TbgthDriver::TbgthDriver(bool useThermal, QObject *parent)
	: PtzpDriver(parent)
{
	headLens = new YamanoLensHead;
	headEvpuPt = new EvpuPTHead;
	headThermal = new MgeoThermalHead;
	state = INIT;
	defaultPTHead = headEvpuPt;
	defaultModuleHead = headLens;
	configLoad(QJsonObject());
	yamanoActive = false;
	evpuActive = false;
	controlThermal = useThermal;

	gpiocont->requestGpio(GPIO_WATER, GpioController::OUTPUT, "WaterControl");
	gpiocont->requestGpio(GPIO_WIPER, GpioController::OUTPUT, "WiperControl");
	gpiocont->requestGpio(GPIO_WATER_STATUS, GpioController::INPUT, "WaterStatus");
	gpiocont->requestGpio(GPIO_WIPER_STATUS, GpioController::INPUT, "WiperStatus");
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
			//hack us in for EVPU offloading
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

	return 0;
}

QVariant TbgthDriver::get(const QString &key)
{
	if (key == "ptz.get_zoom_pos")
		return QString("%1")
				.arg(headLens->getZoom());
	else if (key == "ptz.get_focus_pos")
		return QString("%1")
				.arg(headLens->getProperty(1));
	else if (key == "ptz.get_iris_pos")
		return QString("%1")
				.arg(headLens->getProperty(1));
	else if (key == "ptz.get_version")
		return QString("%1")
				.arg(headLens->getProperty(1));
	else if (key == "ptz.get_filter_pos")
		return QString("%1")
				.arg(headLens->getProperty(1));
	else if (key == "ptz.get_focus_mode")
		return QString("%1")
				.arg(headLens->getProperty(1));
	else if (key == "ptz.get_iris_mod")
		return QString("%1")
				.arg(headLens->getProperty(1));
	else if (key == "camera.model")
		return QString("%1")
				.arg(config.model);
	else if (key == "camera.type")
		return QString("%1")
				.arg(config.type);
	else if (key == "camera.cam_module")
		return QString("%1")
				.arg(config.cam_module);
	else if (key == "camera.pan_tilt_support")
		return QString("%1")
				.arg(config.ptSupport);
	else
		return PtzpDriver::get(key);
	return "almost_there";
}

int TbgthDriver::set(const QString &key, const QVariant &value)
{
	if (key == "ptz.cmd.focus_in")
		headLens->setProperty(4, value.toUInt());
	else if (key == "ptz.cmd.focus_out")
		headLens->setProperty(5, value.toUInt());
	else if (key == "ptz.cmd.focus_stop")
		headLens->setProperty(6, value.toUInt());
	else if (key == "ptz.cmd.focus_set")
		headLens->setProperty(7, value.toUInt());
	else if (key == "ptz.cmd.iris_open")
		headLens->setProperty(8, value.toUInt());
	else if (key == "ptz.cmd.iris_close")
		headLens->setProperty(9, value.toUInt());
	else if (key == "ptz.cmd.iris_stop")
		headLens->setProperty(10, value.toUInt());
	else if (key == "ptz.cmd.iris_set")
		headLens->setProperty(11, value.toUInt());
	else if (key == "ptz.cmd.ir_filter_in")
		headLens->setProperty(12, value.toUInt());
	else if (key == "ptz.cmd.ir_filter_out")
		headLens->setProperty(13, value.toUInt());
	else if (key == "ptz.cmd.ir_filter_stop")
		headLens->setProperty(14, value.toUInt());
	else if (key == "ptz.cmd.filter_pos")
		headLens->setProperty(15, value.toUInt());
	else if (key == "ptz.cmd.auto_focus")
		headLens->setProperty(16, value.toUInt());
	else if (key == "ptz.cmd.auto_iris")
		headLens->setProperty(17, value.toUInt());
	else if(key == "ptz.cmd.zoom_set")
		headLens->setZoom(value.toUInt());

	return PtzpDriver::set(key,value);
}

void TbgthDriver::configLoad(const QJsonObject &obj)
{
	Q_UNUSED(obj);
	QJsonObject o;
	o.insert("model",QString("Tbgth"));
	o.insert("type" , QString("moving"));
	o.insert("pan_tilt_support", 1);
	o.insert("cam_module", QString("Yamano"));
	return PtzpDriver::configLoad(o);
}

QByteArray TbgthDriver::sendFilter(const char *bytes, int len)
{
	if (bytes[0] == 0x35) {
		//dump(QByteArray(bytes, len));
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
			//ffDebug() << fstate.payload << fstate.fields << fstate.payload.size();
			ba.append(fstate.payload);
			fstate = FilteringState();
			return 0;
		}
		return -EAGAIN;
	}

	mDebug("Unexpected filter state - %d %d", fstate.payload.size(), fstate.payloadLen);
	return -EIO;
}

void TbgthDriver::enableDriver(bool value)
{
	PtzpDriver::enableDriver(value);
	if (!driverEnabled) {
		((PtzpTcpTransport *)tp1)->disconnectFrom();
	} else {
		tp1->connectTo(tp1ConnectionString);
	}
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
		if(time->elapsed() >= 10000) {
			time->restart();
		}
		break;
	}

	PtzpDriver::timeout();
}
