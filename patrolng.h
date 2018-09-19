#ifndef PATROLNG_H
#define PATROLNG_H

#include <QHash>
#include <QMutex>
#include <QByteArray>
#include <QElapsedTimer>
class PatrolNg
{
public:
	static PatrolNg* getInstance();
	enum PatrolMode  {
		STOP,
		RUN
	};

	typedef QList<QPair<QString, int> > patrolType;
	struct PatrolInfo {
		int patrolId;
		int state; //0: stopped, 1: running
		patrolType list;
	};

	int addPatrol(const QStringList presets);
	int addInterval(const QStringList intervals);
	int deletePatrol();
	int save();
	int load();
	int setPatrolIndex(int index);
	int setPatrolName(const QString &name);
	int setPatrolStateRun(int index = -1);
	int setPatrolStateStop(int index = -1);
	PatrolInfo getCurrentPatrol();
private:
	PatrolNg();
	QMutex mutex;
	QHash<int, patrolType> patrols;

	int currentPatrolIndex;
	QString currentPatrolName;
	PatrolInfo currentPatrol;

};

#endif // PATROLNG_H
