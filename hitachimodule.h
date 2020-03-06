#ifndef HITACHIMODULE_H
#define HITACHIMODULE_H

#include <QObject>
#include <QTimer>

class QextSerialPort;

class HitachiModule : public QObject
{
	Q_OBJECT
public:
	explicit HitachiModule(QextSerialPort *hiPort, QObject *parent = 0);

	~HitachiModule();

	enum ModuleType
	{
		MODULE_TYPE_SC110,
		MODULE_TYPE_SC120,
		MODULE_TYPE_UNDEFINED
	};

	enum IRCFControl
	{
		IRCF_IN,
		IRCF_OUT,
		IRCF_NEUTRAL,
		IRCF_UNDEFINED
	};

	enum IRCFOutColor
	{
		IRCF_OUT_BW,
		IRCF_OUT_COLOR,
		IRCF_OUT_UNDEFINED_COLOR
	};

	enum IRcurve
	{
		IR_VISIBLE_RAY,
		IR_950_NM,
		IR_850_NM,
		IR_CURVE_UNDEFINED
	};

	enum ProgramAEmode
	{
		MODE_PRG_AE,
		MODE_PRG_AER_IR_RMV1,
		MODE_PRG_AER_IR_RMV2,
		MODE_PRG_AE_DSS,
		MODE_PRG_AE_DSS_IR_RMV1,
		MODE_PRG_AE_DSS_IR_RMV2,
		MODE_PRG_AE_DSS_IR_RMV3,
		MODE_SHUTTER_PRIORITY,
		MODE_EXPOSURE_PRIORITY,
		MODE_FULL_MANUAL_CTRL,
		MODE_AGC_PRIORITY,
		MODE_INVALID
	};

	enum ShutterSpeed
	{
		SH_SPD_1_OV_4,
		SH_SPD_1_OV_8,
		SH_SPD_1_OV_15,
		SH_SPD_1_OV_30,
		SH_SPD_1_OV_50,
		SH_SPD_1_OV_60,
		SH_SPD_1_OV_100,
		SH_SPD_1_OV_120,
		SH_SPD_1_OV_180,
		SH_SPD_1_OV_250,
		SH_SPD_1_OV_500,
		SH_SPD_1_OV_1000,
		SH_SPD_1_OV_2000,
		SH_SPD_1_OV_4000,
		SH_SPD_1_OV_10000,
		SH_SPD_UNDEFINED
	};

	enum ShutterSpeedManual
	{
		MANUAL_1_OV_4 = 0x193,
		MANUAL_1_OV_8 = 0x253,
		MANUAL_1_OV_15 = 0x302,
		MANUAL_1_OV_30 = 0x3c0,
		MANUAL_1_OV_50 = 0x44d,
		MANUAL_1_OV_60 = 0x480,
		MANUAL_1_OV_100 = 0x50E,
		MANUAL_1_OV_120 = 0x541,
		MANUAL_1_OV_180 = 0x5b2,
		MANUAL_1_OV_250 = 0x60d,
		MANUAL_1_OV_500 = 0x6cd,
		MANUAL_1_OV_1000 = 0x78e,
		MANUAL_1_OV_2000 = 0x84f,
		MANUAL_1_OV_4000 = 0x90f,
		MANUAL_1_OV_10000 = 0xa0e,
		MANUAL_SH_UNDEFINED
	};

	enum ExposureValue
	{
		EXP_F1_4,
		EXP_F2_0,
		EXP_F2_8,
		EXP_F4_0,
		EXP_F5_6,
		EXP_F8_0,
		EXP_F11_0,
		EXP_F16_0,
		EXP_F22_0,
		EXP_F32_0,
		EXP_UNDEFINED
	};

	enum ExposureValueManual
	{
		MANUAL_F1_4 = 0x0,
		MANUAL_F2_0 = 0xc6,
		MANUAL_F2_8 = 0x181,
		MANUAL_F4_0 = 0x247,
		MANUAL_F5_6 = 0x302,
		MANUAL_F8_0 = 0x3c8,
		MANUAL_F11_0 = 0x479,
		MANUAL_F16_0 = 0x54a,
		MANUAL_F22_0 = 0x5fb,
		MANUAL_F32_0 = 0x6cb,
		MANUAL_UNDEFINED
	};

	enum FocusMode {
		FOCUS_AUTO,
		FOCUS_MANUAL
	};

	enum ZoomSpeed {
		ZOOM_SPEED_HIGH,
		ZOOM_SPEED_SUPER_HIGH,
		ZOOM_SPEED_NORMAL,
		ZOOM_SPEED_SLOW
	};

	enum RotateMode {
		REVERSE_MODE,
		MIRROR_MODE,
		VERT_INVER_MODE,
		ROTATE_OFF,
	};

	enum irisOffset {
		IRIS_AVERAGE,
		IRIS_PEAK
	};

	enum TypeAB {
		TYPE_A,
		TYPE_B
	};

	enum FNRtuningVal {
		FNR_WEAK,
		FNR_MEDIUM,
		FNR_STRONG,
		FNR_MANUAL,
		FNR_UNDEFINED
	};

	enum CameraMode {
		MODE_720P_25,
		MODE_720P_30,
		MODE_UNDEFINED
	};

	enum AWBmode {
		AWB_NORMAL,
		AWB_SODIUM,
		AWB_MERCURY,
		AWB_UNDEFINED
	};

	enum DefogMode {
		DEFOG_OFF,
		DEFOG_WEAK,
		DEFOG_MEDIUM,
		DEFOG_STRONG,
		DEFOG_UNDEFINED
	};

	virtual int setIRCFInOutCtrl(IRCFControl val);
	virtual IRCFControl getIRCFInOutCtrl();
	virtual int setIRCFOutColor(IRCFOutColor val);
	virtual IRCFOutColor getIRCFOutColor();
	virtual int setIRcurve(IRcurve val);
	virtual IRcurve getIRcurve();
	virtual int setProgramAEmode(ProgramAEmode val);
	virtual ProgramAEmode getProgramAEmode();
	virtual int setShutterSpeed(ShutterSpeed val, ProgramAEmode ae);
	virtual ShutterSpeed getShutterSpeed(ProgramAEmode ae);
	virtual int setExposureValue(ExposureValue val, ProgramAEmode ae);
	virtual ExposureValue getExposureValue(ProgramAEmode ae);
	virtual int setAGCgain(float valdB);
	virtual float getAGCgain();
	virtual int setDisplayRotation(RotateMode val);
	virtual RotateMode getDisplayRotation();
	virtual int applyProgramAEmode(ProgramAEmode ae, ShutterSpeed shVal, ExposureValue exVal, float gain, IRCFControl ircf, FocusMode focusMode);
	virtual int getFocusPosition();
	virtual float getZoomPositionF();
	virtual int getZoomPosition();
	virtual int setZoomPosition(int val);
	virtual int setZoomPositionF(float val);
	virtual int zoomIn(int msec);
	virtual int zoomOut(int msec);
	virtual int setZoomSpeed(ZoomSpeed s);
	virtual ZoomSpeed getZoomSpeed();
	virtual int startZoomIn();
	virtual int stopZoom();
	virtual int startZoomOut();
	virtual int startFocusNear();
	virtual int startFocusFar();
	virtual int stopFocus();
	virtual int setCameraMode(CameraMode mode);
	virtual CameraMode getCameraMode();
	virtual int setFocusMode(FocusMode mode);
	virtual FocusMode getFocusMode();
	virtual int setZoomFocusSettings(FocusMode val);
	virtual int setAutoIrisLevel(irisOffset mode,bool wdr, int val);
	virtual int getAutoIrisLevel(irisOffset mode, bool wdr);
	virtual int setMaxAGCgain(float dBval);
	virtual int getMaxAGCgain();
	virtual int setFNRstatus(bool stat);
	virtual bool getFNRstatus();
	virtual int setFNRtuningVal(FNRtuningVal val);
	virtual FNRtuningVal getFNRtuningVal();
	virtual int setFNRlevel(int val);
	virtual int getFNRlevel();
	virtual int setOnePushAF();
	virtual int setBLCstatus(bool stat);
	virtual bool getBLCstatus();
	virtual int setBLCtuningVal(int val);
	virtual int getBLCtuningVal();
	virtual int setAWBmode(AWBmode val);
	virtual AWBmode getAWBmode();
	virtual int setDefogMode(DefogMode val);
	virtual DefogMode getDefogMode();
	virtual int setDefogColor(int val);
	virtual int getDefogColor();

	virtual int initializeRAM();

	/* virtual members */
	virtual ModuleType model() { return MODULE_TYPE_UNDEFINED;}

	int writePort( const QString &command);
	QString readPort( const QString &command);

	static int writePort(QextSerialPort *Port, const QString &command);
	static QString readPort(QextSerialPort *Port, const QString &command);

	static QString getModelString(QextSerialPort *port);
	static ModuleType getModel(QextSerialPort *port);
	static int readRegW(QextSerialPort *port, int reg);
	static int writeRegW(QextSerialPort *port, int reg, int val);
	static int readReg(QextSerialPort *port, int reg);
	static int writeReg(QextSerialPort *port, int reg, int val);

protected:
	QextSerialPort *hiPort;
	QList<float> yPoints;
	QList<uint> xPoints;
	QList<float> m;
	QList<float> c;

	struct settings {
		int FNRlevel;
		int BLClevel;
		HitachiModule::ProgramAEmode progAE;
		HitachiModule::IRCFControl ircfControl;
		int shSpd;
		int expVal;
		float agcVal;
		int focusMode;
		QString manCmd;
		int readCmd;
	};
	settings s;

	static QextSerialPort * initPort();
	float zoomAuto(float val, int step, float tolerance = 0);
	void zoomInOut(float val, float curr, int step);
	int setZoomFocusType(TypeAB val);
private:
	QTimer *timer;
	QTimer *zoomFocusTimer;
private slots:
	void timeout();
	int saveZoomFocus();
};

#endif // HITACHIMODULE_H
