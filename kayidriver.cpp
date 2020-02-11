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
	fovResp = new ::ptzp::AdvancedCmdResponse;
	brightnessMode = MANUAL;
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

grpc::Status KayiDriver::GetZoom(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	if (firmwareType == "absgs")
		response->set_max(5);
	else
		response->set_max(3);

	response->set_value(headModule->getProperty(MgeoFalconEyeHead::R_FOV));
	return grpc::Status::OK;
}

grpc::Status KayiDriver::SetZoom(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	if (fovResp->enum_field() == false) {
		fovResp->set_enum_field(true);
		fovResp->add_supported_values(0);
		fovResp->add_supported_values(1);
		fovResp->add_supported_values(2);
		fovResp->add_supported_values(3);
		fovResp->add_supported_values(4);
	}

	if (request->new_value() == 0)
		headModule->setProperty(4, request->new_value());
	else if (request->new_value() == 1)
		headModule->setProperty(4, request->new_value());
	else if (request->new_value() == 2)
		headModule->setProperty(4, request->new_value());
	else if (request->new_value() == 3)
		headModule->setProperty(4, request->new_value());
	else if (request->new_value() == 4)
		headModule->setProperty(4, request->new_value());
	else
		return grpc::Status::CANCELLED;

	return grpc::Status::OK;
}

grpc::Status KayiDriver::GetAdvancedControl(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response, ptzp::PtzHead_Capability cap)
{
	if (cap == ptzp::PtzHead_Capability_KARDELEN_BRIGHTNESS) {
		response->set_enum_field(true); // means we got auto/manual brightness for the kardelen dudes.
		response->set_raw_value(brightnessMode);
	}

	if (cap == ptzp::PtzHead_Capability_KARDELEN_CONTRAST) {
		response->set_enum_field(true); // means we got auto/manual contrast for the kardelen dudes.
		response->set_raw_value(brightnessMode);
	}

	return PtzpDriver::GetAdvancedControl(context, request, response, cap);
}

grpc::Status KayiDriver::SetAdvancedControl(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response, ptzp::PtzHead_Capability cap)
{
	if (cap == ptzp::PtzHead_Capability_KARDELEN_BRIGHTNESS) {
		if (request->raw_value()) {
			headModule->setProperty(38, 1);
			brightnessMode = AUTO;
			return grpc::Status::OK;
		}
		else {
			headModule->setProperty(38,0);
			brightnessMode = MANUAL;
		}
	}

	if (cap == ptzp::PtzHead_Capability_KARDELEN_CONTRAST) {
		if (request->raw_value()) {
			headModule->setProperty(39, 1);
			brightnessMode = AUTO;
			return grpc::Status::OK;
		}
		else {
			headModule->setProperty(39,0);
			brightnessMode = MANUAL;
		}
	}

	return PtzpDriver::SetAdvancedControl(context, request, response, cap);
}

void KayiDriver::timeout()
{
	mLog("Driver state: %d", state);
	switch (state) {
	case INIT:
		if(firmwareType == "absgs") {
			headModule->setProperty(37, 2);
			headModule->setProperty(5, 1);
			headModule->setProperty(37, 2);
			while (headModule->getProperty(61) != 2) {
				usleep(1000);
				headModule->setProperty(37, 1);
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
