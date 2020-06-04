#ifndef IRDOMEDRIVER_H
#define IRDOMEDRIVER_H

#include <ptzpdriver.h>

class IRDomePTHead;
class OemModuleHead;
class PtzpSerialTransport;

class IRDomeDriver : public PtzpDriver
{
	Q_OBJECT
public:
	explicit IRDomeDriver(QObject *parent = 0);

	PtzpHead *getHead(int index);
	int setTarget(const QString &targetUri);
	QString getCapString(ptzp::PtzHead_Capability cap);

	grpc::Status GetAdvancedControl(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response, ptzp::PtzHead_Capability cap);
	grpc::Status SetAdvancedControl(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response, ptzp::PtzHead_Capability cap);
	grpc::Status SetImagingControl(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ::google::protobuf::Empty *response);
	grpc::Status GetImagingControl(grpc::ServerContext *context, const google::protobuf::Empty *request, ptzp::ImagingResponse *response);

protected slots:
	void timeout();

protected:
	enum DriverState {
		UNINIT,
		INIT,
		SYNC_HEAD_MODULE,
		SYNC_HEAD_DOME,
		NORMAL,
	};

	OemModuleHead *headModule;
	IRDomePTHead *headDome;
	DriverState state;
	PtzpTransport *tp;
	PtzpTransport *tp1;
	bool ptSupport;

	void autoIRcontrol();
};

#endif // IRDOMEDRIVER_H
