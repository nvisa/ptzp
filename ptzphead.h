#ifndef PTZPHEAD_H
#define PTZPHEAD_H

#include <ecl/ptzp/ioerrors.h>

#include <QHash>
#include <QMutex>
#include <QByteArray>
#include <QVariantMap>
#include <QElapsedTimer>

class PtzpTransport;

class PtzpHead
{
public:
	PtzpHead();
	~PtzpHead() {}

	enum PtzpCapabilities {
		CAP_PAN			= 0x01,
		CAP_TILT		= 0x02,
		CAP_ZOOM		= 0x04,
		CAP_ADVANCED	= 0x08,
	};
	enum HeadStatus {
		ST_SYNCING,
		ST_NORMAL,
		ST_ERROR,
	};

	virtual int getCapabilities() = 0;
	virtual int setTransport(PtzpTransport *tport);
	virtual int syncRegisters();
	virtual int getHeadStatus();

	virtual int panLeft(float speed);
	virtual int panRight(float speed);
	virtual int tiltUp(float speed);
	virtual int tiltDown(float speed);
	virtual int panTiltAbs(float pan, float tilt);
	virtual int panTiltStop();
	virtual int startZoomIn(int speed);
	virtual int startZoomOut(int speed);
	virtual int stopZoom();
	virtual float getPanAngle();
	virtual float getTiltAngle();
	virtual int getZoom();
	virtual int setZoom(uint pos);
	virtual int panTiltGoPos(float ppos, float tpos);
	virtual uint getProperty(uint r);
	virtual void setProperty(uint r, uint x);
#ifdef HAVE_PTZP_GRPC_API
	virtual QVariantMap getSettings();
	virtual void setSettings(QVariantMap key);
#endif
	int getErrorCount(uint err);
	virtual void enableSyncing(bool en);
	virtual void setSyncInterval(int interval);

	static int dataReady(const unsigned char *bytes, int len, void *priv);
	static QByteArray transportReady(void *priv);
	QHash<QString, QPair<int, int> > settings;

protected:
	virtual int dataReady(const unsigned char *bytes, int len);
	virtual QByteArray transportReady();
	virtual void setIOError(int err);
	int setRegister(uint reg, uint value);
	uint getRegister(uint reg);

	PtzpTransport *transport;
	QHash<uint, uint> registers;
	QMutex rlock;
	QHash<uint, uint> errorCount;
	QElapsedTimer pingTimer;
};

#endif // PTZPHEAD_H
