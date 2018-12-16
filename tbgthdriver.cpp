#include "tbgthdriver.h"
#include "yamanolenshead.h"
#include "evpupthead.h"
#include "ptzpserialtransport.h"
#include "ptzptcptransport.h"
#include "debug.h"

#include <QFile>
#include <QTimer>
#include <errno.h>

TbgthDriver::TbgthDriver(QObject *parent)
	: PtzpDriver(parent)
{
	headLens = new YamanoLensHead;
	headEvpuPt = new EvpuPTHead;
	state = INIT;
	defaultPTHead = headEvpuPt;
	defaultModuleHead = headLens;
}

PtzpHead *TbgthDriver::getHead(int index)
{
	if (index == 0)
		return headLens;
	else if (index == 1)
		return headEvpuPt;
	return NULL;
}

int TbgthDriver::setTarget(const QString &targetUri)
{
	QStringList fields = targetUri.split(";");
	tp = new PtzpSerialTransport();
	headLens->setTransport(tp);
	headLens->enableSyncing(true);
	if (tp->connectTo(fields[0]))
		return -EPERM;
	tp1 = new PtzpTcpTransport(PtzpTransport::PROTO_BUFFERED);
	headEvpuPt->setTransport(tp1);
	int err = tp1->connectTo(QString("%1").arg(fields[1]));
	if (err)
		return err;
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
	else return PtzpDriver::get(key);
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
	else return PtzpDriver::set(key,value);
}

void TbgthDriver::timeout()
{
	switch (state) {
	case INIT:
		headLens->syncRegisters();
		state = SYNC_HEAD_LENS;
		break;
	case SYNC_HEAD_LENS:
		if (headLens->getHeadStatus() == PtzpHead::ST_NORMAL) {
			state = NORMAL;
			tp->enableQueueFreeCallbacks(true);
			tp1->enableQueueFreeCallbacks(true);
		}
		break;
	case NORMAL:
		if(time->elapsed() >= 10000) {
			time->restart();
		}
		break;
	}

	PtzpDriver::timeout();
}
