#ifndef TBGTHDRIVER_H
#define TBGTHDRIVER_H

#include <ecl/ptzp/ptzpdriver.h>
#include <ecl/ptzp/ptzptcptransport.h>

class YamanoLensHead;
class EvpuPTHead;
class PtzpSerialTransport;
class MgeoThermalHead;

class TbgthDriver : public PtzpDriver, public PtzpTcpTransport::TransportFilterInteface
{
	Q_OBJECT
public:
	explicit TbgthDriver(bool useThermal, QObject *parent = 0);

	PtzpHead * getHead(int index);
	int setTarget(const QString &targetUri);
	QVariant get(const QString &key);
	int set(const QString &key, const QVariant &value);
	void configLoad(const QJsonObject &obj);

	QByteArray sendFilter(const char *bytes, int len);
	int readFilter(QAbstractSocket *sock, QByteArray &ba);

	void enableDriver(bool value) override;
	virtual grpc::Status SetIO(grpc::ServerContext *context, const ptzp::IOCmdPar *request, ptzp::IOCmdPar *response);
	virtual QJsonObject doExtraDeviceTests();
protected slots:
	void timeout();

protected:
	enum DriverState {
		INIT,
		SYNC_HEAD_LENS,
		SYNC_HEAD_THERMAL,
		NORMAL,
		EVPU_RELEASE,
		EVPU_GRAB,
		EVPU_GRAB_WAIT,
	};

	class FilteringState {
	public:
		FilteringState()
		{
			payloadLen = 0;
		}

		int payloadLen;
		QByteArray payload;
		QString prefix;
		QStringList fields;
	};
	FilteringState fstate;

	YamanoLensHead *headLens;
	EvpuPTHead *headEvpuPt;
	MgeoThermalHead *headThermal;
	PtzpTransport *tp;
	PtzpTransport *tp1;
	DriverState state;
	bool yamanoActive;
	bool evpuActive;
	bool controlThermal;
	QString tp1ConnectionString;
};

#endif // TBGTHDRIVER_H
