#include "ptzpdriver.h"
#include "debug.h"
#include "net/remotecontrol.h"

#include <ecl/ptzp/ptzphead.h>
#include <ecl/ptzp/ptzptransport.h>

#include <QTimer>

#include <errno.h>

PtzpDriver::PtzpDriver(QObject *parent)
	: QObject(parent)
{
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), SLOT(timeout()));
	timer->start(10);

	defaultPTHead = NULL;
	defaultModuleHead = NULL;
}

int PtzpDriver::getHeadCount()
{
	int count = 0;
	while (getHead(count++)) {}
	return count;
}

void PtzpDriver::startSocketApi(quint16 port)
{
	RemoteControl *rcon = new RemoteControl(this, this);
	rcon->listen(QHostAddress::Any, port);
}

QVariant PtzpDriver::get(const QString &key)
{
	if (defaultPTHead == NULL || defaultModuleHead == NULL)
		return QVariant();

	if (key == "ptz.all_pos")
		return QString("%1,%2,%3")
				.arg(defaultPTHead->getPanAngle())
				.arg(defaultPTHead->getTiltAngle())
				.arg(defaultModuleHead->getZoom());
//	else if (key == "ptz.head.count")
//		return getHeadCount();
//	else if (key.startsWith("ptz.head.")) {
//		QStringList fields = key.split(".");
//		if (fields.size() <= 2)
//			return -EINVAL;
//		int index = fields[2].toInt();
//		fields.removeFirst();
//		fields.removeFirst();
//		fields.removeFirst();
//		if (index < 0 || index >= getHeadCount())
//			return -ENONET;
//		return headInfo(fields.join("."), getHead(index));
//	}
//	return "almost_there";
//	return QVariant();
}

int PtzpDriver::set(const QString &key, const QVariant &value)
{
	if (defaultPTHead == NULL || defaultModuleHead == NULL)
		return -ENODEV;

	if (key == "ptz.cmd.pan_left")
		defaultPTHead->panLeft(value.toFloat());
	else if (key == "ptz.cmd.pan_right")
		defaultPTHead->panRight(value.toFloat());
	else if (key == "ptz.cmd.tilt_down")
		defaultPTHead->tiltDown(value.toFloat());
	else if (key == "ptz.cmd.tilt_up")
		defaultPTHead->tiltUp(value.toFloat());
	else if (key == "ptz.cmd.pan_stop")
		defaultPTHead->panTiltAbs(0, 0);
	else if (key == "ptz.cmd.pan_tilt_abs") {
		const QStringList &vals = value.toString().split(";");
		if (vals.size() < 2)
			return -EINVAL;
		float pan = vals[0].toFloat();
		float tilt = vals[1].toFloat();
		defaultPTHead->panTiltAbs(pan, tilt);
	} else if (key == "ptz.cmd.zoom_in")
		defaultModuleHead->startZoomIn(value.toInt());
	else if (key == "ptz.cmd.zoom_out")
		defaultModuleHead->startZoomOut(value.toInt());
	else if (key == "ptz.cmd.zoom_stop")
		defaultModuleHead->stopZoom();
	else
		return -ENOENT;

	return 0;
}

void PtzpDriver::timeout()
{
}

QVariant PtzpDriver::headInfo(const QString &key, PtzpHead *head)
{
	if (key == "status")
		return head->getHeadStatus();
	return QVariant();
}
