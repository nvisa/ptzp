#include "kayidriver.h"
#include "aryapthead.h"
#include "debug.h"
#include "mgeofalconeyehead.h"
#include "ptzpserialtransport.h"

#include <QFile>
#include <QTcpSocket>
#include <QTimer>
#include <errno.h>
#include <unistd.h>

KayiDriver::KayiDriver(QList<int> relayConfig, bool gps, QString type, QObject *parent)
	: PtzpDriver(parent)
{
	headModule = new MgeoFalconEyeHead(relayConfig, gps, type);
	headDome = new AryaPTHead;
	headDome->setMaxSpeed(888889);
	state = INIT;
	defaultPTHead = headDome;
	defaultModuleHead = headModule;
	firmwareType = type;
	brightnessMode = MANUAL;
	fovValues = {0 ,1 ,2 ,3 ,4};
}

PtzpHead *KayiDriver::getHead(int index)
{
	if (index == 0)
		return headModule;
	else if (index == 1)
		return headDome;
	return NULL;
}

#define zoomEntry(zoomv, h)                                                    \
	zooms.push_back(zoomv);                                                    \
	hmap.push_back(h);                                                         \
	vmap.push_back(h *(float)4 / 3)

int KayiDriver::setTarget(const QString &targetUri)
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

	if (1) {
		std::vector<float> hmap, vmap;
		std::vector<int> zooms;
		zoomEntry(9000, 11);
		zoomEntry(9500, 10);
		zoomEntry(10000, 9);
		zoomEntry(10500, 8);
		zoomEntry(11000, 7);
		zoomEntry(11500, 6);
		zoomEntry(12300, 5);
		zoomEntry(13200, 4);
		zoomEntry(14100, 3);
		zoomEntry(15000, 2);
		headModule->getRangeMapper()->setLookUpValues(zooms);
		headModule->getRangeMapper()->addMap(hmap);
		headModule->getRangeMapper()->addMap(vmap);
	}

	return 0;
}

int KayiDriver::set(const QString &key, const QVariant &value)
{
	if (key == "abc") {
		headModule->setProperty(5, value.toUInt());
	} else if (key == "laser.up")
		headModule->setProperty(40, value.toUInt());
	return 0;
}

bool KayiDriver::isReady()
{
	if (state == NORMAL)
		return true;
	return false;
}

QString KayiDriver::getCapString(ptzp::PtzHead_Capability cap)
{
	static QHash<int, QString> _map;
	if (_map.isEmpty()) {
		_map[ptzp::PtzHead_Capability_DAY_NIGHT] = "choose_cam";
		_map[ptzp::PtzHead_Capability_KARDELEN_NIGHT_VIEW] = "choose_cam";
		_map[ptzp::PtzHead_Capability_KARDELEN_DAY_VIEW] = "choose_cam";
		_map[ptzp::PtzHead_Capability_KARDELEN_MENU_OVER_VIDEO] = "button_press";
		_map[ptzp::PtzHead_Capability_KARDELEN_LAZER_RANGE_FINDER] = "laser_fire";
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

grpc::Status KayiDriver::GetZoom(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	Q_UNUSED(request);
	Q_UNUSED(context);

	if (firmwareType == "absgs"){
		response->add_supported_values(0);
		response->add_supported_values(1);
		response->add_supported_values(2);
		response->add_supported_values(3);
		response->add_supported_values(4);
	}
	else {
		response->add_supported_values(0);
		response->add_supported_values(1);
		response->add_supported_values(2);
	}
	response->set_value(headModule->getProperty(MgeoFalconEyeHead::R_FOV));
	return grpc::Status::OK;
}

grpc::Status KayiDriver::SetZoom(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	Q_UNUSED(response);
	Q_UNUSED(context);
	float value = request->new_value();

	if (fovValues.contains(value))
		headModule->setProperty(4, value);
	else
		return grpc::Status::CANCELLED;

	return grpc::Status::OK;
}

grpc::Status KayiDriver::GetAdvancedControl(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response, ptzp::PtzHead_Capability cap)
{
	if (cap == ptzp::PtzHead_Capability_KARDELEN_BRIGHTNESS) {
		response->set_enum_field(true); // means we got auto-manual brightness for the kardelen dudes.
	}

	if (cap == ptzp::PtzHead_Capability_KARDELEN_CONTRAST) {
		response->set_enum_field(true); // means we got auto-manual contrast for the kardelen dudes.
	}

	if ( cap == ptzp::PtzHead_Capability_KARDELEN_LAZER_RANGE_FINDER){
		QVariant range = defaultModuleHead->getProperty("laser_reflections");
		defaultModuleHead->getProperty("laser_reflections_clear");
		QStringList rangeStr = range.toString().split(",");
		if (rangeStr.size() > 1)
			response->set_value(rangeStr.first().toFloat());
		else
			response->set_value(-1);
		return grpc::Status::OK;
	}

	return PtzpDriver::GetAdvancedControl(context, request, response, cap);
}

grpc::Status KayiDriver::SetAdvancedControl(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response, ptzp::PtzHead_Capability cap)
{
	return PtzpDriver::SetAdvancedControl(context, request, response, cap);
}

grpc::Status KayiDriver::SetImagingControl(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, google::protobuf::Empty *response)
{
	uint val = request->new_value();
	headModule->setProperty(38, val);
	headModule->setProperty(39, val);
	return grpc::Status::OK;

}

void KayiDriver::timeout()
{
	mLog("Driver state: %d", state);
	switch (state) {
	case INIT:
		if(1){
			if(firmwareType == "absgs") {
				headModule->setProperty(37, 2);
				headModule->setProperty(5, 0);
				headModule->setProperty(37, 2);
				while (headModule->getProperty(61) != 2) {
					usleep(1000);
					headModule->setProperty(37, 0);
				}
			}
			else {
				headModule->setProperty(37, 2);
				headModule->setProperty(5, 1);
				headModule->setProperty(37, 1);
				while (headModule->getProperty(61) != 1) {
					usleep(1000);
					headModule->setProperty(37, 1);
				}
			}
		}
		if (QFile::exists("/run/dont_wait_eye_for_pt"))
			tp2->enableQueueFreeCallbacks(true);
		state = WAIT_ALIVE;
		break;
	case WAIT_ALIVE:
		if (headModule->isAlive() == true) {
			headModule->syncRegisters();
			tp2->enableQueueFreeCallbacks(true);
			state = SYNC_HEAD_MODULE;
		}
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

	PtzpDriver::timeout();
}
