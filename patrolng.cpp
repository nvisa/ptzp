#include "patrolng.h"

#include "ecl/debug.h"

#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>

#include <errno.h>

PatrolNg::PatrolNg()
{
	load();
	currentPatrolIndex = 0;
	currentPatrol.patrolId = 0;
	currentPatrol.state = 0;
}

PatrolNg *PatrolNg::getInstance()
{
	static PatrolNg* instance = new PatrolNg();
	if (instance == 0)
		instance = new PatrolNg();
	return instance;
}

int PatrolNg::addPatrol(const QStringList presets)
{
	if (presets.isEmpty())
		return -EINVAL;
	patrolType patrol;
	foreach (QString preset, presets) {
		patrol  << QPair<QString, int>(preset, 0);
	}
	patrols.insert(currentPatrolIndex, patrol);
	save();
	return 0;
}

int PatrolNg::addInterval(const QStringList intervals)
{
	if (intervals.isEmpty())
		return -EINVAL;
	patrolType patrol = patrols[currentPatrolIndex];
	for (int i = 0; i < intervals.size(); i++) {
		if (i < patrol.size())
			patrol[i].second = intervals[i].toInt();
	}
	patrols.insert(currentPatrolIndex, patrol);
	save();
	return 0;
}

int PatrolNg::deletePatrol()
{
	patrols.remove(currentPatrolIndex);
	save();
	return 0;
}

int PatrolNg::save()
{
	QMutexLocker ml(&mutex);
	qDebug() << "Saving patrols" << patrols;
	QByteArray ba;
	QDataStream out(&ba, QIODevice::WriteOnly);
	out.setByteOrder(QDataStream::LittleEndian);
	out.setFloatingPointPrecision(QDataStream::SinglePrecision);
	out << (qint32)0x77177177; //key
	out << (qint32)1; //version
	out << patrols;

	QFile f("patrols.bin");
	if (!f.open(QIODevice::WriteOnly)) {
		fDebug("Preset save error");
		return -EPERM;
	}
	f.write(ba);
	f.close();
	return 0;
}

int PatrolNg::load()
{
	QMutexLocker ml(&mutex);
	if (!QFileInfo("patrols.bin").exists()) {
		mDebug("The file doesn't existed");
		return -1;
	}
	QFile f("patrols.bin");
	if (!f.open(QIODevice::ReadOnly))
		return -EPERM;
	QByteArray ba = f.readAll();
	f.close();

	QDataStream in(ba);
	in.setByteOrder(QDataStream::LittleEndian);
	in.setFloatingPointPrecision(QDataStream::SinglePrecision);
	qint32 key; in >> key;
	if (key != 0x77177177) {
		fDebug("Wrong key '0x%x'", key);
		return -1;
	}
	qint32 ver; in >> ver;
	if (ver != 1) {
		fDebug("Unsupported version '0x%x'", ver);
		return -2;
	}
	in >> patrols;
	return 0;
}

int PatrolNg::setPatrolIndex(int index)
{
	currentPatrolIndex = index;
	return 0;
}

int PatrolNg::setPatrolName(const QString &name)
{
	currentPatrolName = name;
	return 0;
}

int PatrolNg::setPatrolStateRun(int index)
{
	QMutexLocker ml(&mutex);
	qDebug() << index << "index";
	int ind = currentPatrolIndex;
	if (index >= 0)
		ind = index;
	currentPatrol.patrolId = ind;
	currentPatrol.state = RUN;
	currentPatrol.list = patrols.value(ind);
	return 0;
}

int PatrolNg::setPatrolStateStop(int index)
{
	QMutexLocker ml(&mutex);
	int ind = currentPatrolIndex;
	if (index >= 0)
		ind = index;
	currentPatrol.patrolId = ind;
	currentPatrol.state = STOP;
}

PatrolNg::PatrolInfo PatrolNg::getCurrentPatrol()
{
	return currentPatrol;
}
