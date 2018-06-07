#ifndef PRESETNG_H
#define PRESETNG_H

#include <QMap>
#include <QObject>

class PresetNg : public QObject
{
	Q_OBJECT
public:
	explicit PresetNg(QObject *parent = 0);
	struct PTZLocation {
		int pan;
		int tilt;
		int zoom;
		bool status;
		QString tag;
	};

	int savePreset(int pan, int tilt, int zoom);
	int setIndex(int index);
	int getIndex();
	int save(QString filename);
	QString getPreset(int index);
	QString showPresets();
	int initPresets();
	int load(QString filename);
	int setPresetName(const QString name);
signals:

public slots:
private:
	int lastIndex;
	QString lastTag;
	QMap<int, PTZLocation> presets;
};

#endif // PRESETNG_H
