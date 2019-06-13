#ifndef PTZPDRIVER_H
#define PTZPDRIVER_H

#include <QObject>
#include <QVector>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <ecl/interfaces/keyvalueinterface.h>
#include <ecl/interfaces/ptzcontrolinterface.h>

#ifdef HAVE_PTZP_GRPC_API
#include <ecl/ptzp/grpc/ptzp.pb.h>
#include <ecl/ptzp/grpc/ptzp.grpc.pb.h>
#endif

class QTimer;
class QElapsedTimer;
class PtzpHead;
class PatternNg;
class PtzpTransport;

struct conf
{
	QString model;
	QString type;
	QString cam_module;
	bool ptSupport;
	bool irLedSupport;
};

#ifdef HAVE_PTZP_GRPC_API
class PtzpDriver : public QObject, public KeyValueInterface, public PtzControlInterface, public ptzp::PTZService::Service
#else
class PtzpDriver : public QObject,
		public KeyValueInterface,
		public PtzControlInterface
#endif
{
	Q_OBJECT
public:

	struct SpeedRegulation
	{
		enum Interpolation {
			LINEAR,
			CUSTOM
		};

		bool enable;
		int minZoom;
		int maxZoom;
		float minSpeed;
		Interpolation ipol;
		PtzpHead *zoomHead;
		typedef float (*interOp)(float, int);
		interOp interFunc;
	};

	explicit PtzpDriver(QObject *parent = 0);

	virtual int setTarget(const QString &targetUri) = 0;
	virtual PtzpHead * getHead(int index) = 0;
	virtual int getHeadCount();
	virtual void configLoad(const QString filename);

	void startSocketApi(quint16 port);
	int startGrpcApi(quint16 port);
	virtual QVariant get(const QString &key);
	virtual int set(const QString &key, const QVariant &value);
	float getPanAngle();
	void goToPosition(float p, float t, int z);
	void sendCommand(int c, float par1, float par2);
	virtual void sleepMode(bool stat);
	void setSpeedRegulation(SpeedRegulation r);
	SpeedRegulation getSpeedRegulation();
	virtual void enableDriver(bool value);

	void setPatternHandler(PatternNg *p);
	virtual int setZoomOverlay();
	virtual int setOverlay(QString data);
	bool getDriverUsability() { return usability;}

#ifdef HAVE_PTZP_GRPC_API
public:
	static QStringList commaToList(const QString& comma);
	static QString listToComma(const QStringList& list);
	virtual QVariantMap jsonToMap(const QByteArray& arr);
	virtual QByteArray mapToJson(const QVariantMap& map);
	// grpc start
	grpc::Status GetHeads(grpc::ServerContext *context, const google::protobuf::Empty *request, ptzp::PtzHeadInfo *response);
	// pan
	grpc::Status PanLeft(grpc::ServerContext *context, const::ptzp::PtzCmdPar *request, ::ptzp::PtzCommandResult *response);
	grpc::Status PanRight(grpc::ServerContext *context, const::ptzp::PtzCmdPar *request, ::ptzp::PtzCommandResult *response);
	grpc::Status PanStop(grpc::ServerContext *context, const::ptzp::PtzCmdPar *request, ::ptzp::PtzCommandResult *response);
	// zoom
	grpc::Status ZoomIn(grpc::ServerContext *context, const::ptzp::PtzCmdPar *request, ::ptzp::PtzCommandResult *response);
	grpc::Status ZoomOut(grpc::ServerContext *context, const::ptzp::PtzCmdPar *request, ::ptzp::PtzCommandResult *response);
	grpc::Status ZoomStop(grpc::ServerContext *context, const::ptzp::PtzCmdPar *request, ::ptzp::PtzCommandResult *response);
	// tilt
	grpc::Status TiltUp(grpc::ServerContext *context, const ptzp::PtzCmdPar *request, ptzp::PtzCommandResult *response);
	grpc::Status TiltDown(grpc::ServerContext *context, const ptzp::PtzCmdPar *request, ptzp::PtzCommandResult *response);
	grpc::Status TiltStop(grpc::ServerContext *context, const::ptzp::PtzCmdPar *request, ::ptzp::PtzCommandResult *response);
	// pt
	grpc::Status PanTilt2Pos(grpc::ServerContext *context, const ptzp::PtzCmdPar *request, ptzp::PtzCommandResult *response);
	grpc::Status PanTiltAbs(grpc::ServerContext *context, const ptzp::PtzCmdPar *request, ptzp::PtzCommandResult *response);
	grpc::Status GetPTZPosInfo(grpc::ServerContext *context, const ptzp::PtzCmdPar *request, ptzp::PTZPosInfo *response);
	// preset
	grpc::Status PresetGo(grpc::ServerContext *context, const ptzp::PresetCmd *request, ptzp::PtzCommandResult *response);
	grpc::Status PresetDelete(grpc::ServerContext *context, const ptzp::PresetCmd *request, ptzp::PtzCommandResult *response);
	grpc::Status PresetSave(grpc::ServerContext *context, const ptzp::PresetCmd *request, ptzp::PtzCommandResult *response);
	grpc::Status PresetGetList(grpc::ServerContext *context, const ptzp::PresetCmd *request, ptzp::PresetList *response);
	// patrol
	grpc::Status PatrolSave(grpc::ServerContext *context, const ptzp::PatrolCmd *request, ptzp::PtzCommandResult *response);
	grpc::Status PatrolRun(grpc::ServerContext *context, const ptzp::PatrolCmd *request, ptzp::PtzCommandResult *response);
	grpc::Status PatrolDelete(grpc::ServerContext *context, const ptzp::PatrolCmd *request, ptzp::PtzCommandResult *response);
	grpc::Status PatrolStop(grpc::ServerContext *context, const ptzp::PatrolCmd *request, ptzp::PtzCommandResult *response);
	grpc::Status PatrolGetList(grpc::ServerContext *context, const ptzp::PatrolCmd *request, ptzp::PresetList *response);
	grpc::Status PatrolGetDetails(grpc::ServerContext *context, const ptzp::PatrolCmd *request, ptzp::PatrolDefinition *response);
	// pattern
	grpc::Status PatternRun(grpc::ServerContext *context, const ptzp::PatternCmd *request, ptzp::PtzCommandResult *response);
	grpc::Status PatternStop(grpc::ServerContext *context, const ptzp::PatternCmd *request, ptzp::PtzCommandResult *response);
	grpc::Status PatternStartRecording(grpc::ServerContext *context, const ptzp::PatternCmd *request, ptzp::PtzCommandResult *response);
	grpc::Status PatternStopRecording(grpc::ServerContext *context, const ptzp::PatternCmd *request, ptzp::PtzCommandResult *response);
	grpc::Status PatternDelete(grpc::ServerContext *context, const ptzp::PatternCmd *request, ptzp::PtzCommandResult *response);
	grpc::Status PatternGetList(grpc::ServerContext *context, const ptzp::PatternCmd *request, ptzp::PresetList *response);
	// settings
	grpc::Status GetSettings(grpc::ServerContext *context, const ptzp::Settings *request, ptzp::Settings *response);
	grpc::Status SetSettings(grpc::ServerContext *context, const ptzp::Settings *request, ptzp::Settings *response);
	// focus
	grpc::Status FocusIn(grpc::ServerContext *context, const ptzp::PtzCmdPar *request, ptzp::PtzCommandResult *response);
	grpc::Status FocusOut(grpc::ServerContext *context, const ptzp::PtzCmdPar *request, ptzp::PtzCommandResult *response);
	grpc::Status FocusStop(grpc::ServerContext *context, const ptzp::PtzCmdPar *request, ptzp::PtzCommandResult *response);
#endif

protected:
	float regulateSpeed(float raw, int zoom);

protected slots:
	virtual void timeout();
	QVariant headInfo(const QString &key, PtzpHead *head);

protected:
	bool sleep;
	bool usability;
	QTimer *timer;
	QElapsedTimer *time;
	QElapsedTimer *timeSettingsLoad;
	PtzpHead *defaultPTHead;
	PtzpHead *defaultModuleHead;
	PatternNg *ptrn;
	QElapsedTimer *elaps;
	SpeedRegulation sreg;
	int patrolListPos;
	bool driverEnabled;
	conf config;

	void commandUpdate(int c, float arg1 = 0, float arg2 = 0);

};

#endif // PTZPDRIVER_H
