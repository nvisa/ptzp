#ifndef MGEOTHERMALHEAD_H
#define MGEOTHERMALHEAD_H

#include <ecl/ptzp/ptzphead.h>

#include <QElapsedTimer>
#include <QStringList>

class MgeoThermalHead : public PtzpHead
{
public:
	MgeoThermalHead();

	int getCapabilities();

	bool isAlive();

	virtual int syncRegisters();
	virtual int startZoomIn(int speed);
	virtual int startZoomOut(int speed);
	virtual int stopZoom();
	virtual int getZoom();
	virtual int focusIn(int speed);
	virtual int focusOut(int speed);
	virtual int focusStop();
	virtual int getHeadStatus();
	virtual void setProperty(uint r, uint x);
	virtual uint getProperty(uint r);
	virtual int setZoom(uint pos);
	int headSystemChecker();
	int getAngle();

protected:
	int sendCommand(const QString &key);
	int dataReady(const unsigned char *bytes, int len);
	QByteArray transportReady();
	int syncNext();
	QJsonValue marshallAllRegisters();
	void unmarshallloadAllRegisters(const QJsonValue &node);

	QStringList ptzCommandList;
	int panPos;
	int tiltPos;
	int nextSync;
	QList<uint> syncList;
	bool alive;

	QList<int> loadDefaultRegisterValues();

private:
	/* mgeo/thermal API */
	int sendCommand(uint index, uchar data1 = 0x00, uchar data2 = 0x00);
};

#endif // MGEOTHERMALHEAD_H
