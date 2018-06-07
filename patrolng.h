#ifndef PATROLNG_H
#define PATROLNG_H

#include <QHash>
#include <QObject>
#include <QElapsedTimer>

class PresetNg;
class PatrolNg : public QObject
{
	Q_OBJECT
public:
	explicit PatrolNg(QObject *parent = 0);
	int save(const QString &filename);
	int load(const QString &filename);
	int addPatrol(const QString &data);
	int addInterval(const QString &data);
	int setIndex(int index);
	int deletePatrol(int index);
	int setPatrolState(const QString &data);
	int getIndex();
	QString getPatrolList();
	QString getPatrolInterval();
	QString getPatrol(int index);
signals:

public slots:
private:
	QHash<int, QString> presets;
	typedef QList<QPair<int, int> > patrolType;
	QHash<int, patrolType> patrols;
	int currentPatrolIndex;
	struct PatrolInfo {
		int patrolId;
		int state; //0: stopped, 1: running
		patrolType list;
		int listPos;
		QElapsedTimer t;
	};
	QString targetFileName;
	PatrolInfo currentPatrol;
	PresetNg *prst;
};

#endif // PATROLNG_H
