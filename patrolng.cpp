#include "patrolng.h"

#include "debug.h"

#include <QFile>
#include <QFileInfo>
#include <QDataStream>
#include <QMutexLocker>

#include <errno.h>
#include <presetng.h>

PatrolNg::PatrolNg()
{
	currentPatrol = new PatrolInfo();
	currentPatrol->patrolName = "";
	currentPatrol->state = STOP;
	mInfo("Registered patrols '%s'", qPrintable(getList()));
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
		if (PresetNg::getInstance()->getPreset(preset).isEmpty())
			return -ENODATA;
		patrol  << QPair<QString, int>(preset, 0);
	}
	patrols.insert(name, patrol);
	return save();
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
	return save();
}

int PatrolNg::deletePatrol(const QString &name)
{
	if (getCurrentPatrol()->state != STOP && getCurrentPatrol()->patrolName == name)
		return -EBUSY;
	if(name.isEmpty())
		return -EINVAL;
	patrols.remove(name);
	mInfo("'%s' patrol deleted", qPrintable(name));
	return save();
}

int PatrolNg::save()
{
	QByteArray ba;
	QDataStream out(&ba, QIODevice::WriteOnly);
	out.setByteOrder(QDataStream::LittleEndian);
	out.setFloatingPointPrecision(QDataStream::SinglePrecision);
	out << (qint32)0x77177177; //key
	out << (qint32)1; //version
	out << patrols;

	QFile f("patrols.bin");
	if (!f.open(QIODevice::WriteOnly)) {
		mDebug("Preset save error");
		return -EPERM;
	}
	f.write(ba);
	f.close();
	return 0;
}

int PatrolNg::load()
{
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
		mInfo("Wrong key '0x%x'", key);
		return -1;
	}
	qint32 ver; in >> ver;
	if (ver != 1) {
		mInfo("Unsupported version '0x%x'", ver);
		return -2;
	}
	patrols.clear();
	in >> patrols;
	return 0;
}

int PatrolNg::setPatrolStateRun(const QString &name)
{
	if (name.isEmpty())
		return -EINVAL;
	PresetNg *preset = PresetNg::getInstance();
	QStringList ls = getPresetList(name);
	foreach (QString p, ls) {
		if (preset->getPreset(p).isEmpty())
			return -ENODATA;
	}
	currentPatrol->patrolName = name;
	currentPatrol->state = RUN;
	currentPatrol->list = patrols.value(name);
	mInfo("This patrol '%s' is running", qPrintable(name));
	return 0;
}

int PatrolNg::setPatrolStateStop(const QString &name)
{
	if (name.isEmpty())
		return -EINVAL;
	currentPatrol->patrolName = name;
	currentPatrol->state = STOP;
	mInfo("This patrol '%s' is stopping", qPrintable(name));
	return 0;
}

QStringList PatrolNg::getPresetList(const QString &name)
{
	QPair<QString, int> p;
	QStringList presets;
	foreach (p, patrols[name]) {
		presets << p.first;
	}
	return presets;
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
	patrol = patrol.left(patrol.length() - 1);
	mInfo("Patrol list '%s'", qPrintable(patrol));
	return patrol;
}

PatrolNg::patrolType PatrolNg::getPatrolDef(const QString &name)
{
	if (patrols.contains(name))
		return patrols[name];
	return patrolType();
}
