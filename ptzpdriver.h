#ifndef PTZPDRIVER_H
#define PTZPDRIVER_H

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QVector>

#include <ecl/interfaces/keyvalueinterface.h>
#include <ecl/interfaces/ptzcontrolinterface.h>

#ifdef HAVE_PTZP_GRPC_API
#include <ecl/drivers/gpiocontroller.h>
#include <ecl/ptzp/grpc/ptzp.grpc.pb.h>
#include <ecl/ptzp/grpc/ptzp.pb.h>
#endif

class QTimer;
class QElapsedTimer;
class PtzpHead;
class PatrolNg;
class PatternNg;
class PtzpTransport;
#ifdef HAVE_PTZP_GRPC_API
class PtzpDriver : public QObject,
				   public KeyValueInterface,
				   public PtzControlInterface,
				   public ptzp::PTZService::Service
#else
class PtzpDriver : public QObject,
				   public KeyValueInterface,
				   public PtzControlInterface
#endif
{
	Q_OBJECT
public:
	struct SpeedRegulation {
		enum Interpolation { LINEAR, CUSTOM, ARYA };

		bool enable;
		int minZoom;
		int maxZoom;
		float minSpeed;
		Interpolation ipol;
		PtzpHead *zoomHead;
		PtzpHead *secondZoomHead;
		typedef float (*interOp)(float, float[]);
		interOp interFunc;
	};

	explicit PtzpDriver(QObject *parent = 0);
	enum StopProcess { PATTERN, PATROL, ANY };

	virtual int setTarget(const QString &targetUri) = 0;
	virtual PtzpHead *getHead(int index) = 0;
	virtual int getHeadCount();

	void startSocketApi(quint16 port);
	int startGrpcApi(quint16 port);
	virtual QVariant get(const QString &key);
	virtual int set(const QString &key, const QVariant &value);
	float getPanAngle();
	float getTiltAngle();
	void goToPosition(float p, float t, int z);
	void sendCommand(int c, float par1, float par2);
	void setSpeedRegulation(SpeedRegulation r);
	SpeedRegulation getSpeedRegulation();
	virtual void enableDriver(bool value);
	void manageRegisterSaving();
	void setRegisterSaving(bool enabled, int intervalMsecs);

	void setPatternHandler(PatternNg *p);
	virtual int setZoomOverlay();
	virtual int setOverlay(QString data);
	virtual QJsonObject doExtraDeviceTests();
	void getStartupProcess();
	void addStartupProcess(const QString &type, const QString name);
	void removeStartupProcess();
	void setChangeOverlayState(bool state);
	bool getChangeOverlayState();
	virtual bool isReady();

#ifdef HAVE_PTZP_GRPC_API
public:
	static QStringList commaToList(const QString &comma);
	static QString listToComma(const QStringList &list);
	virtual QVariantMap jsonToMap(const QByteArray &arr);
	virtual QByteArray mapToJson(const QVariantMap &map);
	// grpc start
	grpc::Status GetHeads(grpc::ServerContext *context,
						  const google::protobuf::Empty *request,
						  ptzp::PtzHeadInfo *response);
	// pan
	grpc::Status PanLeft(grpc::ServerContext *context,
						 const ::ptzp::PtzCmdPar *request,
						 ::ptzp::PtzCommandResult *response);
	grpc::Status PanRight(grpc::ServerContext *context,
						  const ::ptzp::PtzCmdPar *request,
						  ::ptzp::PtzCommandResult *response);
	grpc::Status PanStop(grpc::ServerContext *context,
						 const ::ptzp::PtzCmdPar *request,
						 ::ptzp::PtzCommandResult *response);
	// zoom
	grpc::Status ZoomIn(grpc::ServerContext *context,
						const ::ptzp::PtzCmdPar *request,
						::ptzp::PtzCommandResult *response);
	grpc::Status ZoomOut(grpc::ServerContext *context,
						 const ::ptzp::PtzCmdPar *request,
						 ::ptzp::PtzCommandResult *response);
	grpc::Status ZoomStop(grpc::ServerContext *context,
						  const ::ptzp::PtzCmdPar *request,
						  ::ptzp::PtzCommandResult *response);
	// tilt
	grpc::Status TiltUp(grpc::ServerContext *context,
						const ptzp::PtzCmdPar *request,
						ptzp::PtzCommandResult *response);
	grpc::Status TiltDown(grpc::ServerContext *context,
						  const ptzp::PtzCmdPar *request,
						  ptzp::PtzCommandResult *response);
	grpc::Status TiltStop(grpc::ServerContext *context,
						  const ::ptzp::PtzCmdPar *request,
						  ::ptzp::PtzCommandResult *response);
	// pt
	grpc::Status PanTilt2Pos(grpc::ServerContext *context,
							 const ptzp::PtzCmdPar *request,
							 ptzp::PtzCommandResult *response);
	grpc::Status PanTiltAbs(grpc::ServerContext *context,
							const ptzp::PtzCmdPar *request,
							ptzp::PtzCommandResult *response);
	grpc::Status GetPTZPosInfo(grpc::ServerContext *context,
							   const ptzp::PtzCmdPar *request,
							   ptzp::PTZPosInfo *response);
	// preset
	grpc::Status PresetGo(grpc::ServerContext *context,
						  const ptzp::PresetCmd *request,
						  ptzp::PtzCommandResult *response);
	grpc::Status PresetDelete(grpc::ServerContext *context,
							  const ptzp::PresetCmd *request,
							  ptzp::PtzCommandResult *response);
	grpc::Status PresetSave(grpc::ServerContext *context,
							const ptzp::PresetCmd *request,
							ptzp::PtzCommandResult *response);
	grpc::Status PresetGetList(grpc::ServerContext *context,
							   const ptzp::PresetCmd *request,
							   ptzp::PresetList *response);
	// patrol
	grpc::Status PatrolSave(grpc::ServerContext *context,
							const ptzp::PatrolCmd *request,
							ptzp::PtzCommandResult *response);
	grpc::Status PatrolRun(grpc::ServerContext *context,
						   const ptzp::PatrolCmd *request,
						   ptzp::PtzCommandResult *response);
	grpc::Status PatrolDelete(grpc::ServerContext *context,
							  const ptzp::PatrolCmd *request,
							  ptzp::PtzCommandResult *response);
	grpc::Status PatrolStop(grpc::ServerContext *context,
							const ptzp::PatrolCmd *request,
							ptzp::PtzCommandResult *response);
	grpc::Status PatrolGetList(grpc::ServerContext *context,
							   const ptzp::PatrolCmd *request,
							   ptzp::PresetList *response);
	grpc::Status PatrolGetDetails(grpc::ServerContext *context,
								  const ptzp::PatrolCmd *request,
								  ptzp::PatrolDefinition *response);
	// pattern
	grpc::Status PatternRun(grpc::ServerContext *context,
							const ptzp::PatternCmd *request,
							ptzp::PtzCommandResult *response);
	grpc::Status PatternStop(grpc::ServerContext *context,
							 const ptzp::PatternCmd *request,
							 ptzp::PtzCommandResult *response);
	grpc::Status PatternStartRecording(grpc::ServerContext *context,
									   const ptzp::PatternCmd *request,
									   ptzp::PtzCommandResult *response);
	grpc::Status PatternStopRecording(grpc::ServerContext *context,
									  const ptzp::PatternCmd *request,
									  ptzp::PtzCommandResult *response);
	grpc::Status PatternDelete(grpc::ServerContext *context,
							   const ptzp::PatternCmd *request,
							   ptzp::PtzCommandResult *response);
	grpc::Status PatternGetList(grpc::ServerContext *context,
								const ptzp::PatternCmd *request,
								ptzp::PresetList *response);
	// settings
	virtual grpc::Status GetSettings(grpc::ServerContext *context,
							 const ptzp::Settings *request,
							 ptzp::Settings *response);
	virtual grpc::Status SetSettings(grpc::ServerContext *context,
							 const ptzp::Settings *request,
							 ptzp::Settings *response);
	// focus
	grpc::Status FocusIn(grpc::ServerContext *context,
						 const ptzp::PtzCmdPar *request,
						 ptzp::PtzCommandResult *response);
	grpc::Status FocusOut(grpc::ServerContext *context,
						  const ptzp::PtzCmdPar *request,
						  ptzp::PtzCommandResult *response);
	grpc::Status FocusStop(grpc::ServerContext *context,
						   const ptzp::PtzCmdPar *request,
						   ptzp::PtzCommandResult *response);
	// IO
	grpc::Status GetIO(grpc::ServerContext *context,
					   const ptzp::IOCmdPar *request, ptzp::IOCmdPar *response);
	grpc::Status SetIO(grpc::ServerContext *context,
					   const ptzp::IOCmdPar *request, ptzp::IOCmdPar *response);
#endif

protected:
	int createHeadMaps();
	float regulateSpeed(float raw, int zoom);
	QJsonObject readHeadMaps();
	int denormalizeValue(int head, const QString &key, const float &value, QVariant &resMap);
	int normalizeValue(int head, const QString &key, const QVariant &value, float &normalized);
	int normalizeValues(int head, const QVariantMap &map, QVariantMap *resMap);
	PtzpHead * findHead(ptzp::PtzHead_Capability cap, int id = -1);
	PtzpHead * findHeadNull(ptzp::PtzHead_Capability cap, int id = -1);
	int findHeadIndex(QString capstring, int id = -1);

protected slots:
	virtual void timeout();
	QVariant headInfo(const QString &key, PtzpHead *head);

protected:
	QTimer *timer;
	PtzpHead *defaultPTHead;
	PtzpHead *defaultModuleHead;
	PatternNg *ptrn;
	QElapsedTimer *elaps; // patrol timer
	SpeedRegulation sreg;
	int patrolListPos;
	bool driverEnabled;
	GpioController *gpiocont;
	bool registerSavingEnabled;
	int registerSavingIntervalMsecs;
	QElapsedTimer *regsavet;
	bool doStartupProcess;

	void commandUpdate(int c, float arg1 = 0, float arg2 = 0);
	int runPatrol(QString name);
	bool changeOverlay;
	void stopAnyProcess(StopProcess stop = ANY);
	QJsonObject headMaps;

	// Service interface
public:
	virtual grpc::Status GetAdvancedControl(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response, ptzp::PtzHead_Capability cap);
	virtual grpc::Status SetAdvancedControl(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response, ptzp::PtzHead_Capability cap);
	grpc::Status GetExposure(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status SetExposure(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status GetWhiteBalance(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status SetWhiteBalance(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status GetSaturation(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status SetSaturation(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status GetAntiFlicker(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status SetAntiFlicker(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status GetGain(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status SetGain(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status GetShutterSpeed(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status SetShutterSpeed(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status GetIris(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status SetIris(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status GetSharpness(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status SetSharpness(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status GetBrightness(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status SetBrightness(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status GetContrast(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status SetContrast(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status GetHue(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status SetHue(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status GetDefog(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status SetDefog(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status GetVideoRotate(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status SetVideoRotate(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status GetBacklight(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status SetBacklight(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status GetDayNight(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status SetDayNight(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status GetFocus(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status SetFocus(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status GetZoom(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status SetZoom(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status RebootSystem(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status PoweroffSystem(grpc::ServerContext *context, const ptzp::AdvancedCmdRequest *request, ptzp::AdvancedCmdResponse *response) override;
	grpc::Status ScreenClick(grpc::ServerContext *context, const ptzp::ClickParameter *request, google::protobuf::Empty *response) override;
	// Service interface
public:
	grpc::Status GetCapabilityValues(grpc::ServerContext *context, const ptzp::CapabilityValuesReq *request, ptzp::CapabilityValuesResponse *response) override;
	grpc::Status SetCapabilityValues(grpc::ServerContext *context, const ptzp::CapabilityValuesReq *request, ptzp::CapabilityValuesResponse *response) override;
};

#endif // PTZPDRIVER_H
