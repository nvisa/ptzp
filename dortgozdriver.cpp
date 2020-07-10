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


#define zoomEntry(zoomv, h)                                                    \
	zooms.push_back((zoomv / (float)17703)* 100);                                                    \
	hmap.push_back(h);                                                         \
	vmap.push_back(h *(float)4 / 3)

DortgozDriver::DortgozDriver(QList<int> relayConfig, QObject *parent)
	: PtzpDriver(parent)
{
	headModule = new MgeoDortgozHead(relayConfig);
	headDome = new AryaPTHead;
	headDome->setMaxSpeed(888889);
	state = INIT;
	defaultPTHead = headDome;
	defaultModuleHead = headModule;


	std::vector<float> hmap, vmap;
	std::vector<int> zooms;
	zoomEntry(0, 17.2);
	zoomEntry(740, 16.24);
	zoomEntry(1397, 14.81);
	zoomEntry(1988, 12.08);
	zoomEntry(2778, 11.06);
	zoomEntry(3474, 9.87);
	zoomEntry(4252, 8.64);
	zoomEntry(4836, 7.87);
	zoomEntry(5647, 6.84);
	zoomEntry(6586, 5.83);
	zoomEntry(7115, 5.32);
	zoomEntry(7678, 4.84);
	zoomEntry(8665, 4.11);
	zoomEntry(9511, 3.56);
	zoomEntry(10250, 3.15);
	zoomEntry(10982, 2.76);
	zoomEntry(10790, 2.33);
	zoomEntry(12288, 2.19);
	zoomEntry(13199, 1.87);
	zoomEntry(13988, 1.63);
	zoomEntry(14684, 1.47);
	zoomEntry(15307, 1.32);
	zoomEntry(15870, 1.2);
	zoomEntry(16384, 1.11);
	zoomEntry(17080, 0.99);
	zoomEntry(17502, 0.93);
	zoomEntry(17703, 0.91);
	headModule->getRangeMapper()->setLookUpValues(zooms);
	headModule->getRangeMapper()->addMap(hmap);
	headModule->getRangeMapper()->addMap(vmap);
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
		_map[ptzp::PtzHead_Capability_KARDELEN_SHOW_HIDE_SYMBOLOGY] = "symbology";
		_map[ptzp::PtzHead_Capability_KARDELEN_NUC] = "one_point_nuc";
		_map[ptzp::PtzHead_Capability_KARDELEN_DIGITAL_ZOOM] = "digital_zoom";
		_map[ptzp::PtzHead_Capability_KARDELEN_POLARITY] = "polarity";
		_map[ptzp::PtzHead_Capability_KARDELEN_THERMAL_STANDBY_MODES] = "relay_control";
		_map[ptzp::PtzHead_Capability_KARDELEN_SHOW_RETICLE] = "reticle_mode";
		_map[ptzp::PtzHead_Capability_KARDELEN_BRIGHTNESS] = "brightness_change";
		_map[ptzp::PtzHead_Capability_KARDELEN_CONTRAST] = "contrast_change";
		_map[ptzp::PtzHead_Capability_KARDELEN_MENU_OVER_VIDEO] = "button";
	}

	return _map[cap];
}

grpc::Status DortgozDriver::GetAdvancedControl(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response, ptzp::PtzHead_Capability cap)
{
	if (cap == ptzp::PtzHead_Capability_KARDELEN_NIGHT_VIEW) {
		response->set_value(1);
		return grpc::Status::OK;
	}
	if (cap == ptzp::PtzHead_Capability_KARDELEN_BRIGHTNESS || cap == ptzp::PtzHead_Capability_BRIGHTNESS) {
		response->set_enum_field(true); // means we got auto-manual brightness for the kardelen dudes.
	}

	if (cap == ptzp::PtzHead_Capability_KARDELEN_CONTRAST || cap == ptzp::PtzHead_Capability_CONTRAST) {
		response->set_enum_field(true); // means we got auto-manual contrast for the kardelen dudes.
	}
	if (cap == ptzp::PtzHead_Capability_POLARITY) {
		response->add_supported_values(1);
		response->add_supported_values(2);
		response->add_supported_values(3);
		response->add_supported_values(4);
		response->add_supported_values(5);
		response->add_supported_values(6);
		response->add_supported_values(7);
	}
	if (cap == ptzp::PtzHead_Capability_IMAGE_PROCESS) {
		response->add_supported_values(3);
		response->add_supported_values(4);
		response->add_supported_values(5);
	}
	if (cap == ptzp::PtzHead_Capability_DIGITAL_ZOOM) {
		response->add_supported_values(1);
		response->add_supported_values(2);
		response->add_supported_values(3);
	}
	if (cap == ptzp::PtzHead_Capability_NUC_CHART) {
		response->add_supported_values(1);
		response->add_supported_values(2);
		response->add_supported_values(3);
	}
	if (cap == ptzp::PtzHead_Capability_VIDEO_FREEZE) {
		response->add_supported_values(1);
		response->add_supported_values(2);
	}
	return PtzpDriver::GetAdvancedControl(context, request, response, cap);
}

grpc::Status DortgozDriver::SetAdvancedControl(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response, ptzp::PtzHead_Capability cap)
{
	if(cap == ptzp::PtzHead_Capability_FOCUS || cap == ptzp::PtzHead_Capability_KARDELEN_FOCUS){
		if(request->raw_value())
			headModule->setProperty(16, 0); //auto 0 manuel 1
		else headModule->setProperty(16, 1);

		return grpc::Status::OK;
	}

	return PtzpDriver::SetAdvancedControl(context, request, response, cap);
}

grpc::Status DortgozDriver::SetImagingControl(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, google::protobuf::Empty *response)
{
	uint val = request->new_value();
	if (val == 1)
		headModule->setProperty(13, 1);
	else if (val == 0)
		headModule->setProperty(13, 2);
	return grpc::Status::OK;
}

grpc::Status DortgozDriver::GetImagingControl(grpc::ServerContext *context, const google::protobuf::Empty *request, ptzp::ImagingResponse *response)
{
	if (headModule->getProperty(18) == 1)
		response->set_status(true);
	else if (headModule->getProperty(18) == 2)
		response->set_status(false);
	return grpc::Status::OK;
}

grpc::Status DortgozDriver::GetZoom(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	Q_UNUSED(request);
	Q_UNUSED(context);

	response->add_supported_values(0);
	response->add_supported_values(1);
	response->add_supported_values(2);

	response->set_value(headModule->getProperty(MgeoDortgozHead::R_FOV));
	return grpc::Status::OK;
}

grpc::Status DortgozDriver::SetZoom(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response)
{
	Q_UNUSED(response);
	Q_UNUSED(context);
	float value = request->new_value();

	headModule->setProperty(17, value);

	return grpc::Status::OK;
}

void DortgozDriver::timeout()
{
	mLog("Driver state: %d", state);
	switch (state) {
	case INIT:
		headModule->setProperty(16, 0);
		headModule->setProperty(16, 0);
		while (headModule->getProperty(20) != 0) {
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

