#ifndef VISCAMODULE_H
#define VISCAMODULE_H

#include <QObject>
#include <QTimer>

class QextSerialPort;

class ViscaModule : public QObject
{
	Q_OBJECT
public:
	enum ModuleType
	{
		MODULE_TYPE_UNDEFINED = -1,
		MODULE_TYPE_CHINESE
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

	enum ProgramAEmode
	{
		MODE_FULL_AUTO,
		MODE_MANUAL,
		MODE_SHUTTER_PRIORITY,
		MODE_IRIS_PRIORITY,
		MODE_BRIGHT,
		MODE_INVALID
	};

	enum ShutterSpeed
	{
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

	enum ExposureValue
	{
		EXP_F1_6,
		EXP_F2_0,
		EXP_F2_4,
		EXP_F2_8,
		EXP_F4_0,
		EXP_F4_4,
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

	enum GainValue
	{
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

	explicit ViscaModule(QextSerialPort *port, QObject *parent = 0);
	~ViscaModule();

	int writePort(const char *command, int len);
	int writePort(QByteArray command, int len);
	QByteArray readPort(const char *command, int wrlen, int rdlen);
	static QByteArray readPort(QextSerialPort *port, const char *command, int wrlen, int rdlen);
	static int writePort(QextSerialPort *port, const char *command, int len);

	static ModuleType getModel(QextSerialPort *port);

	int startZoomIn();
	int startZoomOut();
	int stopZoom();
	int startFocusNear();
	int startFocusFar();
	int stopFocus();
	int setFocusMode(FocusMode mode);
	FocusMode getFocusMode();
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
	int	setShutterSpeed(ShutterSpeed val);
	ShutterSpeed getShutterSpeed();
	int	setExposureValue(ExposureValue val);
	ExposureValue getExposureValue();
	int setGainValue(GainValue val);
	GainValue getGainValue();
	int	setZoomFocusSettings();
	int setNoiseReduction(int val);
	int getNoiseReduction();

	int setWDRstat(bool stat);
	bool getWDRstat();

	int setGamma(int val);
	int getGamma();

	int initializeCamera();
	int setAWBmode(AWBmode mode);
	AWBmode getAWBmode();

	int setDefogMode(bool stat);
	bool getDefogMode();
	int setExposureTarget(int val);
	int setGainLimit(int upLim, int downLim);
	int setShutterLimit(int upLim, int downLim);
	int setIrisLimit(int upLim, int downLim);
	int openMenu(bool stat);
	int menuGoTo(MenuDirection direct);
	int selectZoomType(bool digiZoom, ZoomType zoomType);
	bool getDigiZoomState();
	ZoomType getZoomType();

	int applyProgramAEmode(ProgramAEmode ae, ShutterSpeed shVal, ExposureValue exVal, GainValue gain, bool ircf, FocusMode focusMode);

protected:
	QextSerialPort *port;
};

#endif // VISCAMODULE_H

