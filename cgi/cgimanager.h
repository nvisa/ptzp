#ifndef CGIMANAGER_H
#define CGIMANAGER_H

#include <QObject>
#include <QHash>

#include <memory>

#include "cgidevicedata.h"

class CgiRequestManager;

class CgiManager : public QObject
{
public:
	explicit CgiManager(CgiDeviceData const& devData, QObject* parent = nullptr);
	//Pan
	int getPan();
	int setPan(int panPos);
	int startPanLeft(int speed);
	int startPanRight(int speed);

	//Tilt
	int getTilt();
	int setTilt(int tiltPos);
	int startTiltUp(int speed);
	int startTiltDown(int speed);

	//Diagonal PT
	int startPTUpLeft(int speed);
	int startPTUpRight(int speed);
	int startPTLowLeft(int speed);
	int startPTLowRight(int speed);

	//Stop all PT Actions
	int stopPt();

	//Zoom
	int getZoom();
	int setZoom(int zoom);
	int startZoomIn(int speed);
	int startZoomOut(int speed);
	int stopZoom();

	//Focus
	int getFocus();
	int setFocus(int focus);
	int increaseFocus(int speed);
	int decreaseFocus(int speed);
	int stopFocus();

	//Aperture
	int increaseAperture(int speed);
	int decreaseAperture(int speed);
	int stopAperture();

	//Various Camera Settings
	QHash<QString, QString> getCamSettings();
	int setCamSettings(QHash<QString, QString> const& settings);
	QHash<QString, QString> getDeviceAbilities();

	//Upgrade
	void startUpgrade(QString const& filePath);
	int getUpgradePercentage();

	//Reboot
	int reboot();

	void setTimeout(int msec);

	~CgiManager();
private:
	std::unique_ptr<CgiRequestManager> requestManager;
	CgiDeviceData devData;
	int timeout = 50;
};
#endif // CGIMANAGER_H
