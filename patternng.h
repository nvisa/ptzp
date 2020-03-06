#ifndef PATTERNNG_H
#define PATTERNNG_H

#include <QFile>
#include <QMutex>
#include <QObject>
#include <QDataStream>
#include <QElapsedTimer>

#include <debug.h>
#include <ptzcontrolinterface.h>
#include <ptzpatterninterface.h>

class PatternNg: QObject
{
public:
	explicit PatternNg(PtzControlInterface *ctrl = 0);

	enum SyncMethod {
		SYNC_POSITION,
		SYNC_TIME,
	};

	enum ReplayState {
		RS_PTZ_WAIT,
		RS_PTZ_INIT,
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
		float par1;
		float par2;
	};

	void positionUpdate(int pan, int tilt, int zoom);
	virtual void commandUpdate(int pan, int tilt, int zoom, int c, float par1, float par2);
	int start(int pan, int tilt, int zoom);
	void stop(int pan, int tilt, int zoom);
	int replay();
	bool isRecording();
	bool isReplaying();

	int save(const QString &filename);
	int load(const QString &filename);
	int deletePattern(const QString &name);
	QString getList();

protected:
	void replayCurrent(int pan, int tilt, int zoom);

	QElapsedTimer ptime;
	QElapsedTimer rtime;
	QList<SpaceTime> geometry;
	bool recording;
	bool replaying;
	QMutex mutex;
	SyncMethod sm;
	int current;
	ReplayState rs;
	PtzControlInterface *ptzctrl;
	QString currentPattern;
};

#endif // PATTERNNG_H
