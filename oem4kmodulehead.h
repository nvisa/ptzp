#ifndef OEM4KMODULEHEAD_H
#define OEM4KMODULEHEAD_H

#include <ptzphead.h>
#include <ptzpserialtransport.h>

class Oem4kModuleHead : public PtzpHead
{
public:
	Oem4kModuleHead();

	enum Registers {
		R_ZOOM,
		R_BRIGHTNESS,
		R_CONTRAST,
		R_HUE,
		R_SATURATION,
		R_SHARPNESS,
		R_CAMMODE
	};

	void fillCapabilities(ptzp::PtzHead *head);

	int focusIn(int speed);
	int focusOut(int speed);
	int focusStop();
	int startZoomIn(int speed);
	int startZoomOut(int speed);
	int stopZoom();
	int getZoom();
	int setZoom(uint pos);
	virtual void setProperty(uint r, uint x);
	virtual uint getProperty(uint r);

	int syncRegisters();
	virtual int getHeadStatus();


protected:
	int sendCommand(const QString &key);
	QByteArray transportReady();
	int dataReady(const unsigned char *bytes, int len);
private:
	QHash <QString, int> camModes;
	PtzpSerialTransport *tpIRC;
	bool switchCmd;
	void manageIRC();

	QStringList commandList;
	int nextSync;

	uint mapZoom(uint x);
};

#endif // OEM4KMODULEHEAD_H
