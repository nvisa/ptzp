#include "patrolng.h"
#include "debug.h"

#include <QFile>

#include <errno.h>
#include <ecl/drivers/presetng.h>

PatrolNg::PatrolNg(QObject *parent)
	: QObject(parent)
{
	targetFileName = "patrol.bin";
	currentPatrolIndex = 0;
	currentPatrol.patrolId = 0;
	currentPatrol.listPos = 0;
	currentPatrol.state = 0;
	load(targetFileName);

	prst = new PresetNg();
}

int PatrolNg::setPatrolState(const QString &data)
{
	QString pstring = data;
	QStringList l = pstring.remove(" ").split("_", QString::SkipEmptyParts);
	int ind = currentPatrolIndex;
	if (l.size() == 2)
		ind = l[1].toInt();
	currentPatrol.patrolId = ind;
	currentPatrol.list = patrols[ind];
	if (currentPatrol.list.isEmpty()) {
		mDebug("Patrol list is empty");
		return -EACCES;
	}
	QPair<int, int> pp = currentPatrol.list[currentPatrol.listPos];

	if (l[0] == "run") {
		currentPatrol.listPos = 0;
		currentPatrol.state = 1;
		currentPatrol.t.start();
	} else currentPatrol.state = 0;
	return 0;
}

int PatrolNg::addPatrol(const QString &data)
{
	if (!data.contains(","))
		return -EINVAL;
	QStringList plist = data.split(",");
	mDebug("Adding new patrol, index value is %d, preset values are %s", currentPatrolIndex, qPrintable(data));
	patrolType patrol;
	foreach (QString pl, plist)
		patrol << QPair<int, int>(pl.toInt(), 0);
	patrols.insert(currentPatrolIndex, patrol);
	save(targetFileName);
	return 0;
}

int PatrolNg::addInterval(const QString &data)
{
	if (!data.contains(","))
		return -EINVAL;
	QStringList plist = data.split(",");
	mDebug("Adding patrol interval, index value is %d, interval value is %s", currentPatrolIndex, qPrintable(data));
	patrolType patrol = patrols[currentPatrolIndex];
	for (int i = 0; i < plist.size(); i++) {
		if (i < patrol.size())
			patrol[i].second = plist[i].toInt();
	}
	patrols.insert(currentPatrolIndex, patrol);
	save(targetFileName);
	return 0;
}

int PatrolNg::setIndex(int index)
{
	currentPatrolIndex = index;
	return 0;
}

int PatrolNg::getIndex()
{
	return currentPatrolIndex;
}

QString PatrolNg::getPatrol(int index)
{
	qDebug() << "This is my patrol" << patrols[index] << patrols[index].size();
}

QString PatrolNg::getPatrolList()
{
	QStringList list;
	qDebug() << "SIZE PATROL LIST" << patrols[currentPatrolIndex].size();
	for	(int i = 0; i < patrols[currentPatrolIndex].size(); i++) {
		QPair<int, int> p = patrols[currentPatrolIndex][i];
		list << QString::number(p.first);
	}
	qDebug() << "GET PATROL LIST " << list;
	return list.join(",");
}

QString PatrolNg::getPatrolInterval()
{
	QStringList list;
	for	(int i = 0; i < patrols[currentPatrolIndex].size(); i++) {
		QPair<int, int> p = patrols[currentPatrolIndex][i];
		list << QString::number(p.second);
	}
	return list.join(",");
}

int PatrolNg::deletePatrol(int index)
{
	patrols.remove(currentPatrolIndex);
	save(targetFileName);
	return 0;
}

int PatrolNg::save(const QString &filename)
{
	QByteArray ba;
	QDataStream out(&ba, QIODevice::WriteOnly);
	out.setByteOrder(QDataStream::LittleEndian);
	out.setFloatingPointPrecision(QDataStream::SinglePrecision);
	out << (qint32)0x77177177; //key
	out << (qint32)1; //version
	out << presets;
	out << patrols;

	QFile f(filename);
	if (!f.open(QIODevice::WriteOnly)) {
		mDebug("error saving presets and patrols");
		return -EPERM;
	}
	f.write(ba);
	f.close();
	return 0;
}

int PatrolNg::load(const QString &filename)
{
	QFile f(filename);
	if (!f.open(QIODevice::ReadOnly)) {
		mDebug("No such file");
		return -EPERM;
	}
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
	presets.clear();
	patrols.clear();
	in >> presets;
	in >> patrols;
	return 0;
}


