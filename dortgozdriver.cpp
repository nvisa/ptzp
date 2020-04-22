#include "dortgozdriver.h"
#include "aryapthead.h"
#include "debug.h"
#include "mgeodortgozhead.h"
#include "ptzpserialtransport.h"

#include <QFile>
#include <QTcpSocket>
#include <QTimer>
#include <errno.h>
#include <unistd.h>

DortgozDriver::DortgozDriver(QList<int> relayConfig)
{
	ffDebug() << "dortgoz driver başlatıldı";
	headModule = new MgeoDortgozHead(relayConfig);
	headDome = new AryaPTHead;
	headDome->setMaxSpeed(888889);
	state = INIT;
	defaultPTHead = headDome;
	defaultModuleHead = headModule;
}

PtzpHead *DortgozDriver::getHead(int index)
{
	if (index == 0)
		return headModule;
	else if (index == 1)
		return headDome;
	return NULL;
}

int DortgozDriver::setTarget(const QString &targetUri)
{
	QStringList fields = targetUri.split(";");
	tp1 = new PtzpSerialTransport();
	headModule->setTransport(tp1);
	headModule->enableSyncing(true);
	if (tp1->connectTo(fields[0]))
		return -EPERM;

	tp2 = new PtzpSerialTransport(PtzpTransport::PROTO_STRING_DELIM);
	headDome->setTransport(tp2);
	headDome->enableSyncing(true);
	if (tp2->connectTo(fields[1]))
		return -EPERM;
	return 0;
}

QString DortgozDriver::getCapString(ptzp::PtzHead_Capability cap)
{
	static QHash<int, QString> _map;
	if (_map.isEmpty()) {
		_map[ptzp::PtzHead_Capability_DAY_NIGHT] = "choose_cam";
		_map[ptzp::PtzHead_Capability_KARDELEN_NIGHT_VIEW] = "choose_cam";
		_map[ptzp::PtzHead_Capability_KARDELEN_DAY_VIEW] = "choose_cam";
		_map[ptzp::PtzHead_Capability_KARDELEN_SHOW_HIDE_SYMBOLOGY] = "symbology";
		_map[ptzp::PtzHead_Capability_KARDELEN_NUC] = "one_point_nuc";
		_map[ptzp::PtzHead_Capability_KARDELEN_DIGITAL_ZOOM] = "digital_zoom";
		_map[ptzp::PtzHead_Capability_KARDELEN_POLARITY] = "polarity";
		_map[ptzp::PtzHead_Capability_KARDELEN_THERMAL_STANDBY_MODES] = "relay_control";
		_map[ptzp::PtzHead_Capability_KARDELEN_SHOW_RETICLE] = "reticle_mode";
		_map[ptzp::PtzHead_Capability_KARDELEN_BRIGHTNESS] = "brightness_change";
		_map[ptzp::PtzHead_Capability_KARDELEN_CONTRAST] = "contrast_change";
	}

	return _map[cap];
}

void DortgozDriver::timeout()
{
	mLog("Driver state: %d", state);
	switch (state) {
	case INIT:

		headModule->setProperty(16, 0);
		headModule->setProperty(5, 1);
		headModule->setProperty(16, 0);
		while (headModule->getProperty(20) != 1) {
			usleep(1000);
			headModule->setProperty(16, 0);
		}

		headModule->syncRegisters();
		tp2->enableQueueFreeCallbacks(true);
		state = SYNC_HEAD_MODULE;

		break;
	case SYNC_HEAD_MODULE:
		if (headModule->getHeadStatus() == PtzpHead::ST_NORMAL) {
			state = NORMAL;
			tp1->enableQueueFreeCallbacks(true);
			timer->setInterval(1000);
		}
		break;
	case NORMAL:
		break;
	}
}

