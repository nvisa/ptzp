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
		R_EXPOSURE_VALUE,		//14 //td:nd // exposure_value
		R_GAIN_VALUE,			//15 //td:nd // gainvalue
		R_ZOOM_POS,
		R_EXP_COMPMODE,	//16 //td:nd
		R_EXP_COMPVAL,	//17 //td:nd
		R_GAIN_LIM,		//18 //td:nd
		R_SHUTTER,		//19 //td:nd // getshutterspeed
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
		R_EXPOSURE_TARGET,
		R_BOT_SHUTTER,
		R_TOP_SHUTTER,
		R_BOT_IRIS,
		R_TOP_IRIS,
		R_TOP_GAIN,
		R_BOT_GAIN,

		R_COUNT
	};

	enum MaskColor {
		COLOR_BLACK,
		COLOR_GRAY1,
		COLOR_GRAY2,
		COLOR_GRAY3,
		COLOR_GRAY4,
		COLOR_GRAY5,
		COLOR_GRAY6,
		COLOR_WHITE,
		COLOR_RED,
		COLOR_GREEN,
		COLOR_BLUE,
		COLOR_CYAN,
		COLOR_YELLOW,
		COLOR_MAGENTA,
		COLOR_BLACK_TRANSP = 16,
		COLOR_GRAY1_TRANSP,
		COLOR_GRAY2_TRANSP,
		COLOR_GRAY3_TRANSP,
		COLOR_GRAY4_TRANSP,
		COLOR_GRAY5_TRANSP,
		COLOR_GRAY6_TRANSP,
		COLOR_WHITE_TRANSP,
		COLOR_RED_TRANSP,
		COLOR_GREEN_TRANSP,
		COLOR_BLUE_TRANSP,
		COLOR_CYAN_TRANSP,
		COLOR_YELLOW_TRANSP,
		COLOR_MAGENTA_TRANSP,
		COLOR_MOSAIC = 127,
	};

	enum MaskGrid {
		GRID_ON = 2,
		GRID_OFF,
		GRID_CENTER_LINE
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
	void setProperty(uint r, uint x);

	int setIRLed(int led);
	int getIRLed();
	void enableSyncing(bool en);
	void setSyncInterval(int interval);
	void setDeviceDefinition(QString definition);
	QString getDeviceDefinition();
	int getZoomRatio();
	void clockInvert(bool st);

	/**
	 * @brief maskeleme işlemleri
	 */
	int maskSet(uint maskID, int width, int height, bool nn = 1);
	int maskSetNoninterlock(uint maskID, int x, int y, int width, int height);
	int maskSetPTZ(uint maskID, int pan, int tilt, uint zoom);
	int maskSetPanTiltAngle(int pan, int tilt);
	int maskSetRanges(int panMax, int panMin, int xMax, int xMin, int tiltMax, int tiltMin, int yMax, int yMin, bool hConvert, bool vConvert);
	int maskDisplay(uint maskID, bool onOff);
	int getDisplayMask() { return maskBits; }
	void updateMaskPosition();
	int maskColor(uint maskID, MaskColor color_0, MaskColor color_1, bool colorChoose = 0);
	int maskGrid(MaskGrid onOff);

	int setShutterLimit(uint topLim, uint botLim);
	QString getShutterLimit();
	int setIrisLimit(uint topLim, uint botLim);
	QString getIrisLimit();
	int setGainLimit(uchar topLim, uchar botLim);
	QString getGainLimit();
private:
	struct maskRange {
		int pMin;
		int pMax;
		int tMin;
		int tMax;
		int xMin;
		int xMax;
		int yMin;
		int yMax;
	};
	maskRange maskRanges;
	uint maskBits;

	bool hPole;
	bool vPole;
	float xPanRate;
	float yTiltRate;

protected:
	int syncNext();
	int dataReady(const unsigned char *bytes, int len);
	QByteArray transportReady();
	QJsonValue marshallAllRegisters();
	void unmarshallloadAllRegisters(const QJsonValue &node);

	bool syncEnabled;
	int syncInterval;
	int irLedLevel;
	CommandHistory *hist;
	uint nextSync;
	QElapsedTimer syncTime;
	QString deviceDefinition;
	int zoomRatio;
};

#endif // OEMMODULEHEAD_H
