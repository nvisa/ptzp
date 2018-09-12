#ifndef PTZPDRIVER_H
#define PTZPDRIVER_H

#include <QObject>
#include <QVector>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <ecl/interfaces/keyvalueinterface.h>

#ifdef HAVE_PTZP_GRPC_API
#include <ecl/ptzp/grpc/ptzp.pb.h>
#include <ecl/ptzp/grpc/ptzp.grpc.pb.h>
#endif

class QTimer;
class PtzpHead;
class PatternNg;
class PtzpTransport;

#ifdef HAVE_PTZP_GRPC_API
class PtzpDriver : public QObject, public KeyValueInterface, public ptzp::PTZService::Service
#else
class PtzpDriver : public QObject, public KeyValueInterface
#endif
{
	Q_OBJECT
public:
	explicit PtzpDriver(QObject *parent = 0);

	virtual int setTarget(const QString &targetUri) = 0;
	virtual PtzpHead * getHead(int index) = 0;
	virtual int getHeadCount();

	void startSocketApi(quint16 port);
	int startGrpcApi(quint16 port);
	virtual QVariant get(const QString &key);
	virtual int set(const QString &key, const QVariant &value);

#ifdef HAVE_PTZP_GRPC_API
	struct Preset{
		QString name;
		float pan;
		float tilt;
		float zoom;
		bool empty;
		int pt_id;
		int zoom_id;
		Preset() : empty(true){}
		Preset(QString name,int pt_id,int zoom_id) : empty(false), name(name), pt_id(pt_id), zoom_id(zoom_id) {}
		Preset(QString name,float pan,float tilt,int pt_id=-1,float zoom=-1,int zoom_id=-1) : name(name),
			pan(pan), tilt(tilt),zoom(zoom),pt_id(pt_id),zoom_id(zoom_id){
			empty = false;
		}

		QJsonObject toJson(int id) const {
			if (empty)
				return {{"id", id},{"name", ""}};
			return {{"id", id},{"name", name}};
		}

	};

	struct Patrol{

		QString name;
		QList<int> intervals;
		QList<int> presetSequence;
		int currentIndex;
		bool empty;

		Patrol(): empty(true){}

		Patrol(QString n,QList<int> i, QList<int> p) : name(n),intervals(i),presetSequence(p),
			currentIndex(0),empty(false){}

		void inc(){
			if(currentIndex==intervals.size()) currentIndex=0;
			else currentIndex++;
		}

		QJsonObject toJson(int id) const {
			if (empty)
				return {{"id", id},{"name", ""},{"sequence",""},{"interval",""}};
			return {{"id", id},{"name", name}, {"sequence", listToComma(presetSequence)}, {"interval", listToComma(intervals)}};
		}
	};

	virtual int presetGo(int id);
	virtual int presetDelete(int id);
	virtual int presetSave(int id, Preset preset);

	virtual int patrolSave(int id,Patrol patrol);
	virtual int patrolDelete(int id);
	virtual int patrolRun(int id);
	virtual int patrolStop();

	static QList<int> commaToList(const QString& comma);
	static QString listToComma(const QList<int>& list);
	virtual QVariantMap jsonToMap(const QByteArray& arr);
	virtual QByteArray mapToJson(const QVariantMap& map);
private:
	bool isRunning;
	Patrol runningPatrol;
	QTimer *patrolTimer;
	QVector<Preset> presetList;
	QVector<Patrol> patrolList;
	int MAX_PRESET;
	int MAX_PATROL;
#endif

private slots:
	virtual void patrolTick();

#ifdef HAVE_PTZP_GRPC_API
public:
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
	// pattern
	grpc::Status PatternRun(grpc::ServerContext *context, const ptzp::PatternCmd *request, ptzp::PtzCommandResult *response);
	grpc::Status PatternStop(grpc::ServerContext *context, const ptzp::PatternCmd *request, ptzp::PtzCommandResult *response);
	grpc::Status PatternStartRecording(grpc::ServerContext *context, const ptzp::PatternCmd *request, ptzp::PtzCommandResult *response);
	grpc::Status PatternStopRecording(grpc::ServerContext *context, const ptzp::PatternCmd *request, ptzp::PtzCommandResult *response);
	grpc::Status PatternGetList(grpc::ServerContext *context, const ptzp::PatternCmd *request, ptzp::PtzCommandResult *response);
	// settings
	grpc::Status GetSettings(grpc::ServerContext *context, const ptzp::Settings *request, ptzp::Settings *response);
	grpc::Status SetSettings(grpc::ServerContext *context, const ptzp::Settings *request, ptzp::Settings *response);
	// focus
	grpc::Status FocusIn(grpc::ServerContext *context, const ptzp::PtzCmdPar *request, ptzp::PtzCommandResult *response);
	grpc::Status FocusOut(grpc::ServerContext *context, const ptzp::PtzCmdPar *request, ptzp::PtzCommandResult *response);
	grpc::Status FocusStop(grpc::ServerContext *context, const ptzp::PtzCmdPar *request, ptzp::PtzCommandResult *response);
#endif

protected slots:
	virtual void timeout();
	QVariant headInfo(const QString &key, PtzpHead *head);

protected:
	QTimer *timer;
	PatternNg *ptrn;
	PtzpHead *defaultPTHead;
	PtzpHead *defaultModuleHead;
};

#endif // PTZPDRIVER_H
