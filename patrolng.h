#ifndef PATROLNG_H
#define PATROLNG_H

#include <QHash>
#include <QMutex>
#include <QByteArray>
#include <QElapsedTimer>
#include <QJsonObject>
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
		QString patrolName;
		int state; //0: stopped, 1: running
		patrolType list;
	};

	int addPatrol(const QString &name, const QStringList presets);
	int addInterval(const QString &name, const QStringList intervals);
	int deletePatrol(const QString &name);
	int save();
	int load();
	int setPatrolName(const QString &name);
	int setPatrolStateRun(const QString &name);
	int setPatrolStateStop(const QString &name);
	PatrolInfo* getCurrentPatrol();
	QString getList();
private:
	PatrolNg();
	QMutex mutex;
	QHash<QString, patrolType> patrols;

	PatrolInfo *currentPatrol;

};

#endif // PATROLNG_H
