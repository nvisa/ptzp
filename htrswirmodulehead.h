#ifndef HTRSWIRMODULEHEAD_H
#define HTRSWIRMODULEHEAD_H

#include <ecl/ptzp/ptzphead.h>

#include <QElapsedTimer>
#include <QStringList>

class HtrSwirModuleHead : public PtzpHead
{
public:
	HtrSwirModuleHead();

	enum Registers {
		R_ZOOM_LEVEL,
		R_DGAIN,
		R_AUTO_GAIN,

		R_COUNT
	};

	int getCapabilities();
	virtual int getHeadStatus();
	virtual int focusIn(int speed);
	virtual int focusOut(int speed);
	virtual void setProperty(uint r, uint x);
	virtual uint getProperty(uint r);
//	/*
//	 * NOTE: useless because device not support.
//	 */
//	virtual int focusStop();
//	virtual int syncRegisters();
//	virtual int startZoomIn(int speed);
//	virtual int startZoomOut(int speed);
//	virtual int stopZoom();
//	virtual int getZoom();

//protected:
	int sendCommand(const QString &key);
	QJsonValue marshallAllRegisters();
	void unmarshallloadAllRegisters(const QJsonValue &node);
	/*
	 * NOTE: useless because device not support.
	 */
//	uint nextSync;
//	int syncNext();
//	QByteArray transportReady();
//	virtual int dataReady(const unsigned char *bytes, int len);
};

#endif // HTRSWIRMODULEHEAD_H
