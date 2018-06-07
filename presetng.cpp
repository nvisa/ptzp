#include "presetng.h"

#include <QFile>
#include <errno.h>
#include <debug.h>

PresetNg::PresetNg(QObject *parent)
	: QObject(parent)
{
	lastIndex = 0;
	initPresets();
}

int PresetNg::initPresets()
{
	PTZLocation position;
	position.pan = 0;
	position.tilt = 0;
	position.zoom = 0;
	position.status = false;
	position.tag = "";
	for (int i = 0; i < 256; i++)
		presets.insert(i, position);
	load("presets.bin");
	return 0;
}

int PresetNg::savePreset(int pan, int tilt, int zoom)
{
	PTZLocation position;
	position.pan = pan;
	position.tilt = tilt;
	position.zoom = zoom;
	position.status = true;
	position.tag = lastTag;

	presets.insert(lastIndex, position);
	int ret = save("presets.bin");
	if (ret)
		return ret;
	return 0;
}

int PresetNg::setIndex(int index)
{
	lastIndex = index;
	return 0;
}

int PresetNg::setPresetName(const QString name)
{
	lastTag = name;
	return 0;
}

int PresetNg::getIndex()
{
	return lastIndex;
}

int PresetNg::save(QString filename)
{
	QFile f(filename);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
		mDebug("Error, This file didn't open. %s", qPrintable(f.fileName()));
		return -EINVAL;
	}
	QString presetValues = showPresets();
	f.write(presetValues.toLatin1());
	f.close();
	return 0;
}

int PresetNg::load(QString filename)
{
	QFile f(filename);
	if (!f.open(QIODevice::ReadOnly)) {
		mDebug("Error, This file didn't open. %s", qPrintable(f.fileName()));
		return -EINVAL;
	}

	QTextStream in(&f);
	QString line = in.readLine();
	PTZLocation location;
	while (!line.isNull()) {
		if (!line.contains("."))
			continue;
		int index = line.split(".").first().toInt();
		QString positions = line.split(".").last();
		positions.remove(" ");
		if (!positions.contains(","))
			continue;
		QStringList ptz = positions.split(",");
		if (ptz.size() < 4)
			continue;
		int pan = ptz.at(0).split(":").last().toInt();
		int tilt = ptz.at(1).split(":").last().toInt();
		int zoom = ptz.at(2).split(":").last().toInt();
		QString tag = ptz.at(3).split(":").last();
		location.pan = pan;
		location.tilt = tilt;
		location.zoom = zoom;
		location.tag = tag;
		presets.insert(index, location);
		line = in.readLine();
	}
	f.close();
	return 0;
}

QString PresetNg::getPreset(int index)
{
	if (presets.value(index).status == false) {
		mDebug("This index dont set. %d", index);
		return QString();
	}
	PTZLocation position = presets.value(index);
	QString value = "%1, %2, %3";
	return value.arg(position.pan).arg(position.tilt).arg(position.zoom);
}

QString PresetNg::showPresets()
{
	QString info;
	QString ptz = "%1. p:%2, t:%3, z:%4, n:%5";
	for (int i = 0; i < 256; i++) {
		PTZLocation position = presets.value(i);
		info = info + ptz.arg(i).arg(position.pan).arg(position.tilt).arg(position.zoom).arg(position.tag) + "\n";
	}
	return info;
}

