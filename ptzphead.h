#ifndef PTZPHEAD_H
#define PTZPHEAD_H

#include <ioerrors.h>

#include <QByteArray>
#include <QElapsedTimer>
#include <QHash>
#include <QJsonValue>
#include <QMutex>
#include <QVariantMap>

#include <algorithm>
#include <vector>

#include <grpc/ptzp.pb.h>

class PtzpTransport;

class PtzpHead : public QObject
{
public:
	PtzpHead();
	~PtzpHead() {}

	enum HeadStatus {
		ST_SYNCING,
		ST_NORMAL,
		ST_ERROR,
	};
	struct FovValues {
		float max;
		float min;
	};
	class RangeMapper
	{
	public:
		RangeMapper() {}

		bool isAvailable() { return maps.size() > 1 ? true : false; }

		std::vector<float> map(int value);

		void setLookUpValues(std::vector<int> v) { lookup = v; }

		void addMap(std::vector<float> m) { maps.push_back(m); }

	protected:
		std::vector<std::vector<float>> maps;
		std::vector<int> lookup;
	};

	virtual int setTransport(PtzpTransport *tport);
	virtual int syncRegisters();
	virtual int getHeadStatus();

	virtual int panLeft(float speed);
	virtual int panRight(float speed);
	virtual int tiltUp(float speed);
	virtual int tiltDown(float speed);
	virtual int panTiltAbs(float pan, float tilt);
	virtual int panTiltDegree(float pan, float tilt);
	virtual int panTiltStop();
	virtual int startZoomIn(int speed);
	virtual int startZoomOut(int speed);
	virtual int stopZoom();
	virtual int focusIn(int speed);
	virtual int focusOut(int speed);
	virtual int focusStop();
	virtual float getPanAngle();
	virtual float getTiltAngle();
	virtual float getAngle();
	virtual int getZoom();
	virtual int setZoom(uint pos);
	virtual int getFOV(float &hor, float &ver);
	virtual int panTiltGoPos(float ppos, float tpos);
	virtual uint getProperty(uint r);
	virtual void setProperty(uint r, uint x);
	virtual QVariant getProperty(const QString &key);
	virtual void setProperty(const QString &key, const QVariant &value);
	virtual QString whoAmI();
	virtual float getMaxPatternSpeed() const;
	virtual void screenClick(int x, int y, int action);
	int saveRegisters(const QString &filename);
	int loadRegisters(const QString &filename);
	int communicationElapsed();
	RangeMapper *getRangeMapper() { return &rmapper; }
	int getFovList(const QString &file, const QString &objName);
	virtual QJsonObject factorySettings(const QString &file);

	void setZoomRatios(std::vector<int> v) { zoomRatios = v; }
	int getZoomRatio();
	virtual void fillCapabilities(ptzp::PtzHead *head) = 0;
	virtual bool hasCapability(ptzp::PtzHead_Capability c);
	virtual QVariant getSetting(const QString &setting);
	virtual QVariantMap getSettings();
	virtual void setSetting(QString key, const QVariant &value);
	virtual void setSettings(QVariantMap key);
	QHash<QString, QPair<int, int>> settings {};
	QStringList nonRegisterSettings;
	ptzp::PtzHead *capcache;
	int getErrorCount(uint err);
	virtual void enableSyncing(bool en);
	virtual void setSyncInterval(int interval);

	static int dataReady(const unsigned char *bytes, int len, void *priv);
	static QByteArray transportReady(void *priv);
	void setTransportInterval(int interval);
	int getSystemStatus();
	virtual void setHeadName(const QString &name);
	QString getHeadName() { return headName; }
	virtual void initHead();
protected:
	virtual int dataReady(const unsigned char *bytes, int len);
	virtual QByteArray transportReady();
	virtual void setIOError(int err);
	int setRegister(uint reg, uint value);
	uint getRegister(uint reg);
	virtual QJsonValue marshallAllRegisters();
	virtual void unmarshallloadAllRegisters(const QJsonValue &node);

	PtzpTransport *transport;
	QHash<uint, uint> registers;
	QMutex rlock;
	QHash<uint, uint> errorCount;
	QElapsedTimer pingTimer;
	int systemChecker;
	RangeMapper rmapper;

	std::vector<int> zoomRatios;
	QString headName;
	FovValues fovValue;
};

#endif // PTZPHEAD_H
