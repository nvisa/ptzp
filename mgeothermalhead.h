#ifndef MGEOTHERMALHEAD_H
#define MGEOTHERMALHEAD_H

#include <ecl/ptzp/ptzphead.h>

#include <QElapsedTimer>
#include <QStringList>

class MgeoThermalHead : public PtzpHead
{
	Q_OBJECT
public:
	MgeoThermalHead(const QString &type = "");

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
	float getAngle();
	QJsonObject factorySettings(const QString &file);
	float getFovMax();
	void initHead();
protected slots:
	void timeout();
protected:
	int syncNext();
	int sendCommand(const QString &key);
	QList<int> loadDefaultRegisterValues();
	QByteArray transportReady();
	int dataReady(const unsigned char *bytes, int len);
	QJsonValue marshallAllRegisters();
	void unmarshallloadAllRegisters(const QJsonValue &node);

private:
	bool alive;
	int panPos;
	int tiltPos;
	int nextSync;
	QList<uint> syncList;
	QStringList ptzCommandList;

private:
	/* mgeo/thermal API */
	int sendCommand(uint index, uchar data1 = 0x00, uchar data2 = 0x00);
};

#endif // MGEOTHERMALHEAD_H
