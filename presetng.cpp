#include "presetng.h"

#include "ecl/debug.h"

#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>

#include <errno.h>

PresetNg::PresetNg()
{
	load();
}

int PresetNg::addPreset(const QString &name, float panPos, float tiltPos, int zoomPos)
{
	QMutexLocker ml(&mutex);
	presets.insert(name, QString("%1;%2;%3").arg(panPos).arg(tiltPos).arg(zoomPos));
	return save();
}

PresetNg *PresetNg::getInstance()
{
	static PresetNg* instance = new PresetNg();
	if (instance == 0)
		instance = new PresetNg();
	return instance;
}

QStringList PresetNg::getPreset(const QString &name)
{
	QMutexLocker ml(&mutex);
	if (!presets.keys().contains(name))
		load();
	QString pos = presets.value(name);
	qDebug() << "go to preset" << pos;
	if (pos.isEmpty())
		return QStringList();
	return pos.split(";");
}

int PresetNg::deletePreset(const QString &name)
{
	QMutexLocker ml(&mutex);
	presets.remove(name);
	save();
}

int PresetNg::save()
{
	QMutexLocker ml(&mutex);
	qDebug() << "saving " << presets;
	QByteArray ba;
	QDataStream out(&ba, QIODevice::WriteOnly);
	out.setByteOrder(QDataStream::LittleEndian);
	out.setFloatingPointPrecision(QDataStream::SinglePrecision);
	out << (qint32)0x77177177; //key
	out << (qint32)1; //version
	out << presets;

	QFile f("presets.bin");
	if (!f.open(QIODevice::WriteOnly)) {
		fDebug("Preset save error");
		return -EPERM;
	}
	f.write(ba);
	f.close();
	return 0;
}

int PresetNg::load()
{
	QMutexLocker ml(&mutex);
	if (!QFileInfo("presets.bin").exists()) {
		mDebug("The file doesn't existed");
		return -1;
	}
	QFile f("presets.bin");
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
	in >> presets;
	return 0;
}

