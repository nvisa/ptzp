#ifndef PATTERNNG_H
#define PATTERNNG_H

#include <QFile>
#include <QMutex>
#include <QObject>
#include <QDataStream>
#include <QElapsedTimer>

#include <ecl/debug.h>
#include <ecl/interfaces/ptzcontrolinterface.h>
#include <ecl/interfaces/ptzpatterninterface.h>

class PatternNg : public PtzPatternInterface
{
public:
	explicit PatternNg(PtzControlInterface *ctrl = 0);

	struct ReplayParameters {
		int panResolution;		//hundreds of 1 degree
		int tiltResolution;		//hundreds of 1 degree
		int approachSpeedHigh;
		int approachSpeedMedium;
		int approachSpeedLow;
		int approachSpeedUltraLow;
		int approachPointHigh;
		int approachPointMedium;
		int approachPointLow;
		int approachPointUltraLow;
	};

	enum SyncMethod {
		SYNC_POSITION,
		SYNC_TIME,
	};

	enum ReplayState {
		RS_PAN_INIT,
		RS_TILT_INIT,
		RS_ZOOM_INIT,
		RS_RUN,
		RS_FINALIZE,
	};

	struct SpaceTime
	{
		int pan;
		int tilt;
		int zoom;
		qint64 time;
		int cmd;
		int par1;
		int par2;
	};

	void positionUpdate(int pan, int tilt, int zoom);
	void commandUpdate(int pan, int tilt, int zoom, int c, int par1, int par2);
	//void viscaCommand(int c, int par1, int par2);
	//void customCommand(int c, int par1, int par2);
	int start(int pan, int tilt, int zoom);
	void stop(int pan, int tilt, int zoom);
	int replay();
	void space();
	bool isRecording();
	bool isReplaying();

	int save(const QString &filename);
	int load(const QString &filename);

	ReplayParameters rpars;
protected:
	void replayCurrent(int pan, int tilt, int zoom);

	QElapsedTimer ptime;
	QList<SpaceTime> geometry;
	bool recording;
	bool replaying;
	QMutex mutex;
	SyncMethod sm;
	int current;
	ReplayState rs;
	PtzControlInterface *ptzctrl;
};

#endif // PATTERNNG_H
