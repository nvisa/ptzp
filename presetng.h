#ifndef PRESETNG_H
#define PRESETNG_H

#include <QHash>
#include <QMutex>
#include <QJsonObject>

class PresetNg
{
public:
	static PresetNg *getInstance();
	int save();
	int load();
	int addPreset(const QString &name, float panPos, float tiltPos, int zoomPos);
	QStringList getPreset(const QString &name);
	int deletePreset(const QString &name);
	QJsonObject getList();
private:
	PresetNg();
	QMutex mutex;
	QHash<QString, QString> presets;
};

#endif // PRESETNG_H
