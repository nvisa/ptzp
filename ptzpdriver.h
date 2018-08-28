#ifndef PTZPDRIVER_H
#define PTZPDRIVER_H

#include <QObject>

#include <ecl/interfaces/keyvalueinterface.h>
#include <ecl/interfaces/ptzcontrolinterface.h>

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
class PtzpDriver : public QObject,
		public KeyValueInterface,
		public PtzControlInterface
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
	void goToPosition(float p, float t, int z);
	void sendCommand(int c, float par1, int par2);

#ifdef HAVE_PTZP_GRPC_API
	grpc::Status GetHeads(grpc::ServerContext *context, const google::protobuf::Empty *request, ptzp::PtzHeadInfo *response);
#endif

protected slots:
	virtual void timeout();
	QVariant headInfo(const QString &key, PtzpHead *head);

protected:
	QTimer *timer;
	PtzpHead *defaultPTHead;
	PtzpHead *defaultModuleHead;
	PatternNg *ptrn;
};

#endif // PTZPDRIVER_H
