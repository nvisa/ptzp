#ifndef KAYIDRIVER_H
#define KAYIDRIVER_H

#include <ecl/ptzp/ptzpdriver.h>

class MgeoFalconEyeHead;
class AryaPTHead;
class PtzpSerialTransport;

class KayiDriver : public PtzpDriver
{
	Q_OBJECT
public:
	explicit KayiDriver(QList<int> relayConfig, bool gps, QString type, QObject *parent = 0);

	PtzpHead *getHead(int index);
	int setTarget(const QString &targetUri);
	int set(const QString &key, const QVariant &value);
	bool isReady();

	grpc::Status GetZoom(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response);
	grpc::Status SetZoom(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response);
	grpc::Status GetAdvancedControl(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response, ptzp::PtzHead_Capability cap);
	grpc::Status SetAdvancedControl(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response, ptzp::PtzHead_Capability cap);

protected slots:
	void timeout();

protected:
	enum DriverState {
		INIT,
		WAIT_ALIVE,
		SYNC_HEAD_MODULE,
		NORMAL,
	};

	enum Modes {
		MANUAL,
		AUTO
	};

	Modes brightnessMode; // brightness and contrast modes respectively.

	MgeoFalconEyeHead *headModule;
	AryaPTHead *headDome;
	DriverState state;
	PtzpTransport *tp1;
	PtzpTransport *tp2;
	QString firmwareType;
	QVector<float> fovValues;
};

#endif // KAYIDRIVER_H
