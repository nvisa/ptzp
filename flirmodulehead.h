#ifndef FLIRMODULEHEAD_H
#define FLIRMODULEHEAD_H

#include <ecl/ptzp/ptzphead.h>

class FlirModuleHead : public PtzpHead
{
public:
	FlirModuleHead();

	void fillCapabilities(ptzp::PtzHead *head);
	virtual int startZoomIn(int speed);
	virtual int startZoomOut(int speed);
	virtual int stopZoom();
	virtual int getZoom();
	virtual int focusIn(int speed);
	virtual int focusOut(int speed);
	virtual int focusStop();
	virtual void setProperty(uint r, uint x);
	virtual uint getProperty(uint r);

	int setFocusMode(uint x);
	int setFocusValue(uint x);
	int getFocusMode();
	int getFocus();
	int getLaserState();

	bool getLaserCIT();
	int getLaserTimer();
	int setIRCState(int type);
protected:
	int getNext();
	QByteArray transportReady();
	int sendCommand(const QString &key);
	int dataReady(const unsigned char *bytes, int len);
private:
	int next;
	int zoomPos;
	int focusPos;
	bool laserCIT;
	QList<uint> syncList;
	QStringList commandList;
	QElapsedTimer laserTimer;
	QHash<QString, QString> camModes;
};

#endif // FLIRMODULEHEAD_H
