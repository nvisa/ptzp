#include "patrolng.h"

#include "ecl/debug.h"

#include <QFile>
#include <QFileInfo>
#include <QDataStream>
#include <QMutexLocker>

#include <QJsonArray>
#include <QJsonObject>
#include <errno.h>

PatrolNg::PatrolNg()
{
	load();
	currentPatrol = new PatrolInfo();
	currentPatrol->patrolName = "";
	currentPatrol->state = 0;
}

PatrolNg *PatrolNg::getInstance()
{
	static PatrolNg* instance = new PatrolNg();
	if (instance == 0)
		instance = new PatrolNg();
	return instance;
}

int PatrolNg::addPatrol(const QString &name, const QStringList presets)
{
	if (presets.isEmpty() | name.isEmpty())
		return -EINVAL;
	patrolType patrol;
	foreach (QString preset, presets) {
		patrol  << QPair<QString, int>(preset, 0);
	}
	patrols.insert(name, patrol);
	save();
	return 0;
}

int PatrolNg::addInterval(const QString &name, const QStringList intervals)
{
	if (intervals.isEmpty() || name.isEmpty())
		return -EINVAL;
	patrolType patrol = patrols[name];
	for (int i = 0; i < intervals.size(); i++) {
		if (i < patrol.size())
			patrol[i].second = intervals[i].toInt();
	}
	patrols.insert(name, patrol);
	save();
	return 0;
}

int PatrolNg::deletePatrol(const QString &name)
{
	if(name.isEmpty())
		return -EINVAL;
	patrols.remove(name);
	return save();
}

int PatrolNg::save()
{
//	QMutexLocker ml(&mutex);
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
//	QMutexLocker ml(&mutex);
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
	patrols.clear();
	in >> patrols;
	return 0;
}

int PatrolNg::setPatrolStateRun(const QString &name)
{
	qDebug() << name << "patrol name";
	if (name.isEmpty())
		return -EINVAL;
	currentPatrol->patrolName = name;
	currentPatrol->state = RUN;
	currentPatrol->list = patrols.value(name);
	return 0;
}

int PatrolNg::setPatrolStateStop(const QString &name)
{
	if (name.isEmpty())
		return -EINVAL;
	currentPatrol->patrolName = name;
	currentPatrol->state = STOP;
	return 0;
}

PatrolNg::PatrolInfo* PatrolNg::getCurrentPatrol()
{
	return currentPatrol;
}

QString PatrolNg::getList()
{
	load();
	QString patrol;
	foreach (QString st, patrols.keys()) {
		QString tmp = st + ",";
		patrol = patrol + tmp;
	}
	return patrol;
}
