#ifndef THERMALMODULEDRIVER_H
#define THERMALMODULEDRIVER_H
#include "qextserialport.h"

enum ezoom_level{
	ZOOM_x1,
	ZOOM_x2,
	ZOOM_x4
};

enum live_video{
	FREEZE,
	UNFREEZE
};

enum polarity_hot{
	HOT_BLACK,
	HOT_WHITE
};

enum auto_manual{
	AUTO,
	MANUAL
};

enum inc_dec{
	INCREASE,
	DECREASE
};

enum buttons{
	L0_BUTTON,
	ENTER_BUTTON,
	L2_BUTTON
};

enum buttons_state{
	RELEASED,
	PRESSED
};

class ThermalModuleDriver
{
public:
	explicit ThermalModuleDriver();
	int init(QextSerialPort *port);
	void ezoom(ezoom_level electronicZoom);
	void liveVideo(live_video selection);
	void polaritySet(polarity_hot polaritySelect);
	void OffsetContrastMode(auto_manual selection);
	void brightnessSet(inc_dec selection);
	void contrastSet(inc_dec selection);
	void buttonsCommand(buttons buttonSelect, buttons_state state);
	void normalization();
	void shutterOnOff();
	void symbolgyOnOff();
	void normalizationCompleted();

private:
	unsigned char checksum(char *data);
	QextSerialPort *driverport;
};


#endif // THERMALMODULEDRIVER_H
