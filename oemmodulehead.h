#ifndef OEMMODULEHEAD_H
#define OEMMODULEHEAD_H

#include <ecl/ptzp/ptzphead.h>

#include <QElapsedTimer>

class CommandHistory;

class OemModuleHead : public PtzpHead
{
public:
	OemModuleHead();

	enum Registers {
		R_PAN_ANGLE,
		R_TILT_ANGLE,
		R_ZOOM_POS,
		R_EXPOSURE_VALUE,		//14 //td:nd // exposure_value
		R_GAIN_VALUE,			//15 //td:nd // gainvalue
		R_EXP_COMPMODE,	//16 //td:nd
		R_EXP_COMPVAL,	//17 //td:nd
		R_GAIN_LIM,		//18 //td:nd
		R_SHUTTER,		//19 //td:nd // getshutterspeed
	//	R_MIN_SHUTTER,	//20 //td:nd
	//	R_MIN_SHUTTER_LIM,//21 //td:nd
		R_NOISE_REDUCT,	//22 //td:nd
		R_WDRSTAT,		//23 //td:nd
	//	R_WDRPARAM,		//24 //td:nd
		R_GAMMA,			//25 //td.nd
		R_AWB_MODE,		//26 //td:nd
		R_DEFOG_MODE,		//27 //td:nd
		R_DIGI_ZOOM_STAT,	//28 //td:nd
		R_ZOOM_TYPE,		//29 //td:nd
		R_FOCUS_MODE,		//30 //td:nd
		R_ZOOM_TRIGGER,	//31 //td:nd
		R_BLC_STATUS,		//32 //td:nd
		R_IRCF_STATUS,	//33 //td:nd
		R_AUTO_ICR,		//34 //td:nd
		R_PROGRAM_AE_MODE,//35 //td:nd
		R_FLIP,
		R_MIRROR,
		R_DISPLAY_ROT,
		R_DIGI_ZOOM_POS,
		R_OPTIC_ZOOM_POS,
		R_PT_SPEED,
		R_ZOOM_SPEED,

		R_COUNT
	};

	int getCapabilities();
	int syncRegisters();
	int getHeadStatus();

	int startZoomIn(int speed);
	int startZoomOut(int speed);
	int stopZoom();
	int getZoom();
	int setZoom(uint pos);
	uint getProperty(uint r);
	void setProperty(int r, uint x);


	void enableSyncing(bool en);
	void setDeviceDefinition(QString definition);
	QString getDeviceDefinition();

protected:
	int syncNext();
	int dataReady(const unsigned char *bytes, int len);
	QByteArray transportReady();

	CommandHistory *hist;
	uint nextSync;
	QElapsedTimer syncTime;
	bool syncEnabled;
	QString deviceDefinition;
};

#endif // OEMMODULEHEAD_H
