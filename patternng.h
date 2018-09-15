#ifndef PATTERNNG_H
#define PATTERNNG_H

#include <QFile>
#include <QMutex>
#include <QObject>
#include <QDataStream>
#include <QElapsedTimer>
#include <QStringList>

#include <ecl/debug.h>
#include <ecl/interfaces/ptzcontrolinterface.h>
#include <ecl/interfaces/ptzpatterninterface.h>

class PatternNg : public PtzPatternInterface
{
public:
	explicit PatternNg(PtzControlInterface *ctrl = 0);

	struct preset {
		float panPos;
		float tiltPos;
		int zoomPos;
		bool stat;
	};

	struct patrol
	{
		QStringList pSetOrder;
		QStringList timeOrder;
		bool stat;
		bool replay;
	};

	enum SyncMethod {
		SYNC_POSITION,
		SYNC_TIME,
	};

	enum ReplayState {
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
	void commandUpdate(int pan, int tilt, int zoom, int c, float par1, float par2);
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

	void addPreset(QString name, float panPos, float tiltPos, int zoomPos);
	void goToPreset(QString name);
	void deletePreset(QString name);
	void addPatrol(QString name, QString pSet, QString time);
//	void addPatrol(int i, const QList<QPair<QString, QString> > &pairList);
	void runPatrol(QString name);
	void deletePatrol(QString name);
	void stopPatrol(QString name);
	int currentPreset;
	QString currentPatrol;

protected:
	void replayCurrent(int pan, int tilt, int zoom);

	QHash<QString, preset>presetMap;
	QHash<QString, patrol>patrolMap;

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
};

#endif // PATTERNNG_H
