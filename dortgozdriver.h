#ifndef DORTGOZDRIVER_H
#define DORTGOZDRIVER_H

#include <ptzpdriver.h>

class MgeoDortgozHead;
class AryaPTHead;
class PtzpSerialTransport;

class DortgozDriver : public PtzpDriver
{
		Q_OBJECT
public:
	explicit DortgozDriver(QList<int> relayConfig, QObject *parent = 0);
	PtzpHead *getHead(int index);
	int setTarget(const QString &targetUri);
	QString getCapString(ptzp::PtzHead_Capability cap);
	grpc::Status GetAdvancedControl(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response, ptzp::PtzHead_Capability cap);
	grpc::Status SetAdvancedControl(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response, ptzp::PtzHead_Capability cap);
	grpc::Status SetImagingControl(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, google::protobuf::Empty *response);
	grpc::Status GetImagingControl(grpc::ServerContext *context, const google::protobuf::Empty *request, ptzp::ImagingResponse *response);
	grpc::Status GetZoom(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response);
	grpc::Status SetZoom(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response);

	void screenClick(int x, int y, int action);
	void buttonClick(int b, int action);

protected slots:
	void timeout();

protected:
	enum DriverState {
		INIT,
		WAIT_ALIVE,
		SYNC_HEAD_MODULE,
		NORMAL,
	};

	MgeoDortgozHead *headModule;
	AryaPTHead *headDome;
	DriverState state;
	PtzpTransport *tp1;
	PtzpTransport *tp2;
};

#endif // DORTGOZDRIVER_H
