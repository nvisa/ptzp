#ifndef IRDOMEMODULE_H
#define IRDOMEMODULE_H

#include <QTimer>
#include <QPair>
#include <QByteArray>

#include <ecl/drivers/pattern.h>
#include <ecl/drivers/qextserialport/qextserialport.h>

#define PELCOD_ADD 0x01
#define DEF_PRESET_LIMIT 8
#define DEF_PRESET_FILE "specialPos.bin"

class IrDomeModule : public Pattern
{
	Q_OBJECT
public:
	enum PelcoDCommands {
		PELCOD_CMD_STOP,
		PELCOD_CMD_PANR,
		PELCOD_CMD_PANL,
		PELCOD_CMD_TILTU,
		PELCOD_CMD_TILTD,
		PELCOD_CMD_PANR_TILTU,
		PELCOD_CMD_PANR_TILTD,
		PELCOD_CMD_PANL_TILTU,
		PELCOD_CMD_PANL_TILTD,
		PELCOD_CMD_SET_PRESET,
		PELCOD_CMD_CLEAR_PRESET,
		PELCOD_CMD_GOTO_PRESET,
		PELCOD_CMD_FLIP180,
		PELCOD_CMD_ZERO_PAN,
		PELCOD_CMD_RESET,
		PELCOD_QUE_PAN_POS,
		PELCOD_QUE_TILT_POS,
		PELCOD_QUE_ZOOM_POS,
		PELCOD_SET_ZERO_POS,
		PELCOD_SET_PAN_POS,
		PELCOD_SET_TILT_POS,
		PELCOD_PATTERN_START,
		PELCOD_PATTERN_STOP,
		PELCOD_PATTERN_RUN,
	};

	enum SpecialCommands {
		SPECIAL_SET_COOR,
		SPECIAL_GET_COOR,
		SPECIAL_ZOOM_POS,
		SPECIAL_CURR_POS,
	};

	enum LedControl {
		MANUEL,
		LEVEL0,
		LEVEL1,
		LEVEL2,
		LEVEL3,
		LEVEL4,
		LEVEL5,
		LEVEL6,
		AUTO
	};

	enum ModuleType {
		MODULE_TYPE_NO_VISCA_MODULE = -2,
		MODULE_TYPE_UNDEFINED = -1,
		MODULE_TYPE_PV6403_F12D,
		MODULE_TYPE_FCB_EV7500,
		MODULE_TYPE_PV8430_F2D,
		MODULE_TYPE_FCB_EV7520,
	};

	enum FocusMode {
		FOCUS_AUTO,
		FOCUS_MANUAL,
		FOCUS_UNDEFINED
	};

	enum RotateMode {
		REVERSE_MODE,
		MIRROR_MODE,
		VERT_INVER_MODE,
		ROTATE_OFF,
	};

	enum ProgramAEmode {
		MODE_FULL_AUTO,
		MODE_MANUAL,
		MODE_SHUTTER_PRIORITY,
		MODE_IRIS_PRIORITY,
		MODE_BRIGHT,
		MODE_INVALID
	};

	enum ShutterSpeed {
		SH_SPD_1_OV_1,
		SH_SPD_1_OV_2,
		SH_SPD_1_OV_4,
		SH_SPD_1_OV_8,
		SH_SPD_1_OV_15,
		SH_SPD_1_OV_30,
		SH_SPD_1_OV_60,
		SH_SPD_1_OV_90,
		SH_SPD_1_OV_100,
		SH_SPD_1_OV_125,
		SH_SPD_1_OV_180,
		SH_SPD_1_OV_250,
		SH_SPD_1_OV_350,
		SH_SPD_1_OV_500,
		SH_SPD_1_OV_725,
		SH_SPD_1_OV_1000,
		SH_SPD_1_OV_1500,
		SH_SPD_1_OV_2000,
		SH_SPD_1_OV_3000,
		SH_SPD_1_OV_4000,
		SH_SPD_1_OV_6000,
		SH_SPD_1_OV_10000,
		SH_SPD_UNDEFINED
	};

	enum ExposureValue {
		EXP_F1_6,
		EXP_F2_0,
		EXP_F2_4,
		EXP_F2_8,
		EXP_F3_4,
		EXP_F4_0,
		EXP_F4_8,
		EXP_F5_6,
		EXP_F6_8,
		EXP_F8_0,
		EXP_F9_6,
		EXP_F11_0,
		EXP_F14_0,
		EXP_CLOSE,
		EXP_UNDEFINED
	};

	enum GainValue {
		GAIN_MIN3_DB,
		GAIN_0_DB,
		GAIN_2_DB,
		GAIN_4_DB,
		GAIN_6_DB,
		GAIN_8_DB,
		GAIN_10_DB,
		GAIN_12_DB,
		GAIN_14_DB,
		GAIN_16_DB,
		GAIN_18_DB,
		GAIN_20_DB,
		GAIN_22_DB,
		GAIN_24_DB,
		GAIN_26_DB,
		GAIN_28_DB,
		GAIN_UNDEFINED
	};

	enum AWBmode {
		AWB_AUTO,
		AWB_INDOOR,
		AWB_OUTDOOR,
		AWB_ATW,
		AWB_MANUAL,
		AWB_UNDEFINED
	};

	enum MenuDirection {
		DR_UP,
		DR_LEFT,
		DR_RIGHT,
		DR_DOWN,
		DR_UNDEFINED
	};

	enum ZoomType {
		COMBINE_MODE,
		SEPARATE_MODE,
		ZM_UNDEFINED
	};

	enum Effect {
		EFF_OFF,
		EFF_NEG,
		EFF_BLACK_WHITE
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

	enum WhiteBalance {
		WB_AUTO,
		WB_INDOOR,
		WB_OUTDOOR,
		WB_ATW,
		WB_MANUAL,
		WB_ONE_PUSH_TRIGGER,
		WB_OUTDOOR_AUTO,
		WB_SODIUM_LAMP_AUTO,
		WB_SODIUM_LAMP
	};

	enum TitleColor {
		TITLE_WHITE,
		TITLE_YELLOW,
		TITLE_VIOLET,
		TITLE_RED,
		TITLE_CYAN,
		TITLE_GREEN,
		TITLE_BLUE
	};

	enum ExposureComp {
		EXP_COMP_N10_5,
		EXP_COMP_N9,
		EXP_COMP_N7_5,
		EXP_COMP_N6,
		EXP_COMP_N4_5,
		EXP_COMP_N3,
		EXP_COMP_N1_5,
		EXP_COMP_0,
		EXP_COMP_P1_5,
		EXP_COMP_P3,
		EXP_COMP_P4_5,
		EXP_COMP_P6,
		EXP_COMP_P7_5,
		EXP_COMP_P9,
		EXP_COMP_P10_5,
		EXP_COMP_ERROR,
	};

	enum GainLimit {
		GAIN_LIM_10_7,
		GAIN_LIM_14_3,
		GAIN_LIM_17_8,
		GAIN_LIM_21_4,
		GAIN_LIM_25,
		GAIN_LIM_28_6,
		GAIN_LIM_32_1,
		GAIN_LIM_35_7,
		GAIN_LIM_39_3,
		GAIN_LIM_42_8,
		GAIN_LIM_46_4,
		GAIN_LIM_50,
		GAIN_LIM_ERROR
	};

	enum SupportDevice {
		PANTILT = 1,
		NIGHTVISION
	};

	struct Positions {
		int posHor;
		int posVer;
		int posZoom;
	};

	struct LastVariables {
		int nightVisionSupport;
		int panTitlSupport;

		QPair<int, int> position;
		int zoom;
		int irLedstate;
		bool IRCFstat;
	};


	explicit IrDomeModule(QextSerialPort *port, int readWrite, QString presetFilename = DEF_PRESET_FILE, int presetLimitNo = DEF_PRESET_LIMIT,
						  QString pattFilename = DEF_PATT_FILENAME, int pattLimit = DEF_PATT_LIM, QObject *parent = 0);

	const QByteArray getPelcod(uchar addr, PelcoDCommands cmd, uchar data1 , uchar data2);
	const QByteArray getSpecialCommand(SpecialCommands cmd, uint data1, uint data2);
	const QByteArray getLedControl(LedControl cmd);
	uint checksum(const uchar *cmd, uint lenght);

	int powerOnOff(int on);

	int initializeCameraLens();
	int initializeCameraReset();
	int initializeCamera();

	int vSetReg(int reg, int value);
	int vQueReg(int reg);

	int startZoomIn(uint zoomSpeed);
	int startZoomOut(uint zoomSpeed);
	int stopZoom();
	bool getDigiZoomState();
	ZoomType getZoomType();
	int selectZoomType(bool digiZoom, ZoomType zoomType);

	int setZoomFocusSettings();

	int startFocusNear();
	int startFocusFar();
	int stopFocus();
	int setFocusMode(FocusMode mode);
	FocusMode getFocusMode();
	int setZoomTrigger(bool stat);
	int getZoomTrigger();

	int setOnePushAF();

	int setBLCstatus(bool stat);
	bool getBLCstatus();

	int setDisplayRotation(RotateMode val);
	RotateMode getDisplayRotation();

	int setIRCFstat(bool stat);
	bool getIRCFstat();

	int setAutoICR(bool stat);
	bool getAutoICR();

	int setProgramAEmode(ProgramAEmode val);
	ProgramAEmode getProgramAEmode();

	int setShutterSpeed(ShutterSpeed val);
	ShutterSpeed getShutterSpeed();
	int setShutterLimit(int upLim, int downLim);
	int setMinShutter(bool on);
	int getMinShutter();
	int setMinShutterLimit(ShutterSpeed lim);
	ShutterSpeed getMinShutterLimit();

	int setExposureValue(ExposureValue val);
	ExposureValue getExposureValue();
	int setExposureTarget(int val);
	int setExpCompMode(bool on);
	int getExpCompMode();
	int setExpCompValue(ExposureComp val);
	ExposureComp getExpCompValue();

	int setGainValue(GainValue val);
	GainValue getGainValue();
	int setGainLimit(GainLimit limit);
	GainLimit getGainLimit();
	int setGainLimit(int upLim, int downLim);

	int setNoiseReduction(int val);
	int getNoiseReduction();

	int setWDRstat(bool stat);
	bool getWDRstat();
	int setWDRParameter(int level, int selection, int compen);
	QString getWDRParameter();

	int setGamma(int val);
	int getGamma();

	int setAWBmode(AWBmode mode);
	AWBmode getAWBmode();

	int setDefogMode(bool stat);
	bool getDefogMode();

	int setIrisLimit(int upLim, int downLim);

	int openMenu(bool stat);
	int menuGoTo(MenuDirection direct);

	int applyProgramAEmode(ProgramAEmode ae, ShutterSpeed shVal, ExposureValue exVal, GainValue gain, bool ircf, FocusMode focusMode);

	int vGetZoom();
	int vSetZoom(uint zoomPos);
	uint getZoomRatio();
	float getZoomRatioFloat();

	int pPanLeft(uchar speedPan);
	int pPanRight(uchar speedPan);
	int pTiltUp(uchar speedTilt);
	int pTiltDown(uchar speedTilt);
	int pPanLeftTiltUp(uchar speedPan, uchar speedTilt);
	int pPanLeftTiltDown(uchar speedPan, uchar speedTilt);
	int pPanRightTiltUp(uchar speedPan, uchar speedTilt);
	int pPanRightTiltDown(uchar speedPan, uchar speedTilt);

	int pSetPreset(uchar saveNo);
	int pClearPreset(uchar saveNo);
	int pGotoPreset(uchar saveNo);
	int pFlip180();
	int pZeroPan();
	int pReset();
	int pSetPan(uint pos);
	int pSetTilt(uint pos);
	int pSetZero();
	int pPatternStart(int patternID);
	int pPatternStop();
	int pPatternRun(int patternID);
	int pGetPosPan();
	int pGetPosTilt();
	int pGetPosZoom();

	int pStop();
	int sSetPos(uint posH, uint posV);
	QPair<int, int> sGetPos();
	QPair<int, int> getPosMem();

	int sSetZoom(uint zoomPos);
	int sGetZoom();
	int getZoomMem();
	void setZoomMem(int z);

	int sSetAbsolute(uint posH, uint posV, uint zoomPos);

	QString dVersion();

	int irLedControl(LedControl cmd);
	int setAutoIRLed();

	int presetCall(int ind);
	int presetSave(int ind);
	int presetDelete(int ind);
	int presetState(int ind);
	Positions presetPos(int ind);
	QString presetPosString(int ind);
	int getPresetLimit();

	int patternStart(int ind);
	int patternStop(int ind);
	int patternRun(int ind);
	int patternDelete(int ind);
	int patternState(int ind);
	int patternCancel(int ind);
	int getPatternLimit();

	struct Positions getAllPos ();

	int homeSave();
	int homeGoto();
	const Positions getHomePos();
	const QString getHomePosString();

	int addSupport(SupportDevice dev, int state);
	bool isSupport(SupportDevice dev);

	const QByteArray readPort(const char *command, int wrlen, int rdlen, bool rec = false);

	static int writePort(QextSerialPort* port, const char *command, int len);
	static QByteArray readPort(QextSerialPort* port, const char *command, int wrlen, int rdlen);
	static ModuleType getModel(QextSerialPort* port);

	int pictureEffect(Effect eff);
	int continuousReplyFocus(bool state);
	int continuousReplyZoom(bool state);
	int maskDisplay(uint maskID, bool onOff);
	int maskSet(uint maskID, int width, int height, bool nn = 1);
	int maskSetNoninterlock(uint maskID, int x, int y, int width, int height);
	int maskSetPTZ(uint maskID, int pan, int tilt, uint zoom);
	int maskSetPanTiltAngle(int pan, int tilt);
	int maskSetRanges(int panMax, int panMin, int xMax, int xMin, int tiltMax, int tiltMin, int yMax, int yMin, bool hConvert, bool vConvert);
	int getDisplayMask() { return maskBits; }

	int maskColor(uint maskID, MaskColor color_0, MaskColor color_1, bool colorChoose = 0);
	int maskGrid(MaskGrid onOff);
	int freeze(bool state);
	int setWhiteBalance(WhiteBalance WB);
	int titleSet1(uint lineNumber, uint hPosition, TitleColor color, bool blink);
	int titleSet2(uint lineNumber, const char *characters, uint len);
	int titleSet3(uint lineNumber, const char *characters, uint len);
	int titleClear(uint lineNumber);
	int titleDisplay(uint lineNumber, bool onOff);
	int titleWrite(uint lineNumber, const QByteArray str, uint hPosition, TitleColor color, bool blink);

	void setSeriPortState(QIODevice::OpenMode mod);
	QIODevice::OpenMode getSeriPortState();

	void setCmdInterval(int interval);
	int getCmdInterval();

	int setZoomLookUp (const QStringList &zoomRatioList);

public slots:
	int writePort(const char *command, int len, bool rec = false);
	void updatePosition();

signals:
	void postionsUpdated();
	void panTiltPositionReady();
	void zoomPositionReady();
	void seriPortWrote();

protected:
	QextSerialPort *irPort;

	struct LastVariables lastV;

	QString presetSaveFile;
	int presetLimit;
	QList<QPair<bool, Positions> > preset;
	struct Positions homePos;

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

private:
	int specialPosition2File();
	int file2SpecialPosition();

	uint maskBits;
	maskRange maskRanges;
	float xPanRate;
	float yTiltRate;
	bool hPole;
	bool vPole;

	QList<int> zoomRatio;

	QElapsedTimer seriPortElapse;
	int cmdInterval;

	static const QByteArray charTable;
};

#endif // IRDOMEMODULE_H
