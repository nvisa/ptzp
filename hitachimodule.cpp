#include "hitachimodule.h"
#include "qextserialport.h"

#include <debug.h>

#include <math.h>
#include <errno.h>
#include <unistd.h>

#include <QElapsedTimer>

#define speedText(__x) QString("1/").append(QString(#__x).split("_").last())
#define exposureText(__x) QString(#__x).split("HitachiModule::EXP_").last()
#define programText(__x) QString(#__x).split("HitachiModule::MODE_").last()
#define ircfText(__x) QString(#__x).split("HitachiModule::").last()

HitachiModule::HitachiModule(QextSerialPort *port, QObject *parent) :
	QObject(parent)
{
	hiPort = port;
	hiPort->setBaudRate(BAUD4800);
	hiPort->setParity(PAR_EVEN);
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), SLOT(timeout()));
	timer->start(600*1000);

	zoomFocusTimer = new QTimer(this);
	connect(zoomFocusTimer, SIGNAL(timeout()), SLOT(saveZoomFocus()));
}

HitachiModule::~HitachiModule()
{
}

int HitachiModule::writePort( const QString &command)
{
	if (!command.length())
		return -EIO;
	const int succ = hiPort->write(command.toAscii()) == command.size() ? 0 : -EIO;
	usleep(command.size() * 210);
	return succ;
}

QString HitachiModule::readPort( const QString &command)
{
	/* clear buffers */
	hiPort->readAll();

	QByteArray readData;
	hiPort->write(command.toAscii());
	QElapsedTimer t;
	t.start();
	while (hiPort->bytesAvailable() < command.size()) {
		usleep(210);
		if (t.elapsed() > 1000)
			break;
	}
	readData = hiPort->readAll();
	/* there may be some stray bytes before valid data */
	while (readData.size() && readData.at(0) == 0)
		readData.remove(0, 1);
	fDebug("Read data(%d) is '%s'", readData.size(), readData.data());
	return QString::fromAscii(readData);
}

int HitachiModule::writePort(QextSerialPort *port, const QString &command)
{
	if (!command.length())
		return -EIO;
	const int succ = port->write(command.toAscii()) == command.size() ? 0 : -EIO;
	usleep(command.size() * 300);
	return succ;
}

QString HitachiModule::readPort(QextSerialPort *port, const QString &command)
{
	/* clear buffers */
	port->readAll();

	QByteArray readData;
	port->write(command.toAscii());
	QElapsedTimer t;
	t.start();
	while (port->bytesAvailable() < command.size()) {
		usleep(210);
		if (t.elapsed() > 1000)
			break;
	}
	readData = port->readAll();
	/* there may be some stray bytes before valid data */
	while (readData.size() && readData.at(0) == 0)
		readData.remove(0, 1);
	fDebug("Read data(%d) is '%s'", readData.size(), readData.data());
	return QString::fromAscii(readData);
}

int HitachiModule::setIRCFInOutCtrl(HitachiModule::IRCFControl val)
{
	if (val == IRCF_IN)
		writeReg(hiPort, 0xffe6, 0xc0);
	if (val == IRCF_OUT)
		writeReg(hiPort, 0xffe6, 0x80);
	usleep(1000 * 1000);
	return writeReg(hiPort, 0xffe6, 0x00);
}

HitachiModule::IRCFControl HitachiModule::getIRCFInOutCtrl()
{
	int val = readReg(hiPort, 0xffe7);
	if (val == 0x00)
		return IRCF_OUT;
	if (val == 0x01)
		return IRCF_IN;
	return IRCF_UNDEFINED;
}

int HitachiModule::setIRCFOutColor(HitachiModule::IRCFOutColor val)
{
	uchar reg = readReg(hiPort,0x11e3) & 0xfd;
	if (val == IRCF_OUT_BW)
		return writeReg(hiPort, 0x11e3, reg);
	if (val == IRCF_OUT_COLOR)
		return writeReg(hiPort, 0x11e3, reg | 0x02);
	return -EINVAL;
}

HitachiModule::IRCFOutColor HitachiModule::getIRCFOutColor()
{
	int val = readReg(hiPort, 0x11e3);
	if (val & 0x02)
		return IRCF_OUT_COLOR;
	return IRCF_OUT_BW;
}

int HitachiModule::setIRcurve(IRcurve val)
{
	if (val == IR_VISIBLE_RAY)
		return writeReg(hiPort, 0xf8a4, 0x00);
	else if (val == IR_950_NM)
		return writeReg(hiPort, 0xf8a4, 0xc1);
	else if (val == IR_850_NM)
		return writeReg(hiPort, 0xf8a4, 0xc2);
	return -EINVAL;
}

HitachiModule::IRcurve HitachiModule::getIRcurve()
{
	int val = readReg(hiPort, 0xf8a4);
	if (val == 0x00)
		return IR_VISIBLE_RAY;
	else if (val == 0xc1)
		return IR_950_NM;
	else if (val == 0xc2)
		return IR_850_NM;
	else
		return IR_CURVE_UNDEFINED;
}

int HitachiModule::setProgramAEmode(HitachiModule::ProgramAEmode val)
{
	int rval;
	if (val == MODE_PRG_AE)
		rval = 0x00;
	else if (val == MODE_PRG_AER_IR_RMV1)
		rval = 0x10;
	else if (val == MODE_PRG_AER_IR_RMV2)
		rval = 0x20;
	else if (val == MODE_PRG_AE_DSS)
		rval = 0x01;
	else if (val == MODE_PRG_AE_DSS_IR_RMV1)
		rval = 0x11;
	else if (val == MODE_PRG_AE_DSS_IR_RMV2)
		rval = 0x21;
	else if (val == MODE_PRG_AE_DSS_IR_RMV3)
		rval = 0x31;
	else if (val == MODE_SHUTTER_PRIORITY)
		rval = 0x07;
	else if (val == MODE_EXPOSURE_PRIORITY)
		rval = 0x08;
	else if (val == MODE_FULL_MANUAL_CTRL)
		rval = 0x0A;
	else
		return -EINVAL;
	return writeReg(hiPort, 0xfcc8, rval);
}

HitachiModule::ProgramAEmode HitachiModule::getProgramAEmode()
{
	int val = readReg(hiPort,0xfcc8);
	if (val == 0x00)
		return MODE_PRG_AE;
	if (val == 0x10)
		return MODE_PRG_AER_IR_RMV1;
	if (val == 0x20)
		return MODE_PRG_AER_IR_RMV2;
	if (val == 0x01)
		return MODE_PRG_AE_DSS;
	if (val == 0x11)
		return MODE_PRG_AE_DSS_IR_RMV1;
	if (val == 0x21)
		return MODE_PRG_AE_DSS_IR_RMV2;
	if (val == 0x31)
		return MODE_PRG_AE_DSS_IR_RMV3;
	if (val == 0x07)
		return MODE_SHUTTER_PRIORITY;
	if (val == 0x08)
		return MODE_EXPOSURE_PRIORITY;
	if (val == 0x0A)
		return MODE_FULL_MANUAL_CTRL;
	return MODE_INVALID;
}

int HitachiModule::setShutterSpeed(HitachiModule::ShutterSpeed val, HitachiModule::ProgramAEmode ae)
{
	if (ae == MODE_SHUTTER_PRIORITY) {
		return writeReg(hiPort, 0xfcc9, (val + 2));
	} else if (ae == MODE_FULL_MANUAL_CTRL) {
		if (val == SH_SPD_1_OV_4) {
			return writeRegW(hiPort, 0xffda, MANUAL_1_OV_4);
		} else if (val == SH_SPD_1_OV_8) {
			return writeRegW(hiPort, 0xffda, MANUAL_1_OV_8);
		} else if (val == SH_SPD_1_OV_15) {
			return writeRegW(hiPort, 0xffda, MANUAL_1_OV_15);
		} else if (val == SH_SPD_1_OV_30) {
			return writeRegW(hiPort, 0xffda, MANUAL_1_OV_30);
		} else if (val == SH_SPD_1_OV_50) {
			return writeRegW(hiPort, 0xffda, MANUAL_1_OV_50);
		} else if (val == SH_SPD_1_OV_60) {
			return writeRegW(hiPort, 0xffda, MANUAL_1_OV_60);
		} else if (val == SH_SPD_1_OV_100) {
			return writeRegW(hiPort, 0xffda, MANUAL_1_OV_100);
		} else if (val == SH_SPD_1_OV_120) {
			return writeRegW(hiPort, 0xffda, MANUAL_1_OV_120);
		} else if (val == SH_SPD_1_OV_180) {
			return writeRegW(hiPort, 0xffda, MANUAL_1_OV_180);
		} else if (val == SH_SPD_1_OV_250) {
			return writeRegW(hiPort, 0xffda, MANUAL_1_OV_250);
		} else if (val == SH_SPD_1_OV_500) {
			return writeRegW(hiPort, 0xffda, MANUAL_1_OV_500);
		} else if (val == SH_SPD_1_OV_1000) {
			return writeRegW(hiPort, 0xffda, MANUAL_1_OV_1000);
		} else if (val == SH_SPD_1_OV_2000) {
			return writeRegW(hiPort, 0xffda, MANUAL_1_OV_2000);
		} else if (val == SH_SPD_1_OV_4000) {
			return writeRegW(hiPort, 0xffda, MANUAL_1_OV_4000);
		} else if (val == SH_SPD_1_OV_10000) {
			return writeRegW(hiPort, 0xffda, MANUAL_1_OV_10000);
		} else if (val == SH_SPD_UNDEFINED) {
			return -1;
		}
	}
	return 0;
}

HitachiModule::ShutterSpeed HitachiModule::getShutterSpeed(HitachiModule::ProgramAEmode ae)
{
	if (ae == MODE_SHUTTER_PRIORITY)
		return (HitachiModule::ShutterSpeed(readReg(hiPort,0xfcc9) - 2));
	else if (ae == MODE_FULL_MANUAL_CTRL) {
		const int manSh = readRegW(hiPort, 0xffda);
		if (manSh == MANUAL_1_OV_4) {
			return SH_SPD_1_OV_4;
		} else if (manSh == MANUAL_1_OV_8) {
			return SH_SPD_1_OV_8;
		} else if (manSh == MANUAL_1_OV_15) {
			return SH_SPD_1_OV_15;
		} else if (manSh == MANUAL_1_OV_30) {
			return SH_SPD_1_OV_30;
		} else if (manSh == MANUAL_1_OV_50) {
			return SH_SPD_1_OV_50;
		} else if (manSh == MANUAL_1_OV_60) {
			return SH_SPD_1_OV_60;
		} else if (manSh == MANUAL_1_OV_100) {
			return SH_SPD_1_OV_100;
		} else if (manSh == MANUAL_1_OV_120) {
			return SH_SPD_1_OV_120;
		} else if (manSh == MANUAL_1_OV_180) {
			return SH_SPD_1_OV_180;
		} else if (manSh == MANUAL_1_OV_250) {
			return SH_SPD_1_OV_250;
		} else if (manSh == MANUAL_1_OV_500) {
			return SH_SPD_1_OV_500;
		} else if (manSh ==  MANUAL_1_OV_1000) {
			return SH_SPD_1_OV_1000;
		} else if (manSh == MANUAL_1_OV_2000) {
			return SH_SPD_1_OV_2000;
		} else if (manSh == MANUAL_1_OV_4000) {
			return SH_SPD_1_OV_4000;
		} else if (manSh == MANUAL_1_OV_10000) {
			return SH_SPD_1_OV_10000;
		}
	}
	return SH_SPD_UNDEFINED;
}

int HitachiModule::setExposureValue(ExposureValue val, ProgramAEmode ae)
{
	if (ae == MODE_EXPOSURE_PRIORITY) {
		return writeReg(hiPort, 0xfcc9, val);
	} else if (ae == MODE_FULL_MANUAL_CTRL) {
		if (val == EXP_F1_4) {
			return writeRegW(hiPort, 0xffdc, MANUAL_F1_4);
		} else if (val == EXP_F2_0) {
			return writeRegW(hiPort, 0xffdc, MANUAL_F2_0);
		} else if (val == EXP_F2_8) {
			return writeRegW(hiPort, 0xffdc, MANUAL_F2_8);
		} else if (val == EXP_F4_0) {
			return writeRegW(hiPort, 0xffdc, MANUAL_F4_0);
		} else if (val == EXP_F5_6) {
			return writeRegW(hiPort, 0xffdc, MANUAL_F5_6);
		} else if (val == EXP_F8_0) {
			return writeRegW(hiPort, 0xffdc, MANUAL_F8_0);
		} else if (val == EXP_F11_0) {
			return writeRegW(hiPort, 0xffdc, MANUAL_F11_0);
		} else if (val == EXP_F16_0) {
			return writeRegW(hiPort, 0xffdc, MANUAL_F16_0);
		} else if (val == EXP_F22_0) {
			return writeRegW(hiPort, 0xffdc, MANUAL_F22_0);
		} else if (val == EXP_F32_0) {
			return writeRegW(hiPort, 0xffdc, MANUAL_F32_0);
		} else if (val == EXP_UNDEFINED) {
			return -1;
		}
	}
	return 0;
}

HitachiModule::ExposureValue HitachiModule::getExposureValue(ProgramAEmode ae)
{
	int manExp;
	if (ae == MODE_EXPOSURE_PRIORITY)
		return HitachiModule::ExposureValue(readReg(hiPort,0xfcc9));
	else if (ae == MODE_FULL_MANUAL_CTRL) {
		manExp = readRegW(hiPort, 0xffdc);
		switch(manExp) {
		case MANUAL_F1_4:
			return EXP_F1_4;
		case MANUAL_F2_0:
			return EXP_F2_0;
		case MANUAL_F2_8:
			return EXP_F2_8;
		case MANUAL_F4_0:
			return EXP_F4_0;
		case MANUAL_F5_6:
			return EXP_F5_6;
		case MANUAL_F8_0:
			return EXP_F8_0;
		case MANUAL_F11_0:
			return EXP_F11_0;
		case MANUAL_F16_0:
			return EXP_F16_0;
		case MANUAL_F22_0:
			return EXP_F22_0;
		case MANUAL_F32_0:
			return EXP_F32_0;
		}
	}
	return EXP_UNDEFINED;
}

int HitachiModule::setAGCgain(float valdB)
{
	return writeRegW(hiPort, 0xffde, 32 * valdB);
}

float HitachiModule::getAGCgain()
{
	return float(readRegW(hiPort, 0xffde)/32);
}

int HitachiModule::getFocusPosition()
{
	return readReg(hiPort,0x7100);
}

int HitachiModule::applyProgramAEmode(ProgramAEmode ae, ShutterSpeed shVal, ExposureValue exVal, float gain, IRCFControl ircf, FocusMode focusMode)
{
	static ProgramAEmode prevAE = getProgramAEmode();
	setProgramAEmode(ae);
	if (prevAE != ae)
		usleep(1000*1000);
	prevAE = ae;
	if (ae == MODE_SHUTTER_PRIORITY) {
		setShutterSpeed(shVal, ae);
	} else if (ae == MODE_EXPOSURE_PRIORITY) {
		setExposureValue(exVal, ae);
	} else if (ae == MODE_FULL_MANUAL_CTRL) {
		setShutterSpeed(shVal, ae);
		setExposureValue(exVal, ae);
		setAGCgain(gain);
	}
	if (getIRCFInOutCtrl() != ircf)
		setIRCFInOutCtrl(ircf);
	if (getFocusMode() != focusMode)
		setFocusMode(focusMode);
	return 0;
}

int HitachiModule::setDisplayRotation(RotateMode val)
{
	if (val == ROTATE_OFF)
		return writeReg(hiPort, 0xff30, false);
	else {
		writeReg(hiPort, 0xff30, true);
		writeReg(hiPort, 0x11f7, val);
		return writeReg(hiPort, 0xff31, val);
	}
}

HitachiModule::RotateMode HitachiModule::getDisplayRotation()
{
	int rotStat = readReg(hiPort, 0xff30);
	if (rotStat == false) {
		return ROTATE_OFF;
	} else {
		return HitachiModule::RotateMode(readReg(hiPort, 0xff31));
	}
}

float HitachiModule::getZoomPositionF()
{
	uint val = readReg(hiPort, 0xfc91);
	while (val != 0xff)
		val = readReg(hiPort, 0xfc91);
	//return 18 * 256 / (val + 1);
	val = readRegW(hiPort, 0xf720);
	int interval = 0;
	for (int i = xPoints.size() - 1; i > 0; i--) {
		if (val > xPoints[i]) {
			interval = i;
			break;
		}
	}
	return val * m[interval] + c[interval];
}

int HitachiModule::getZoomPosition()
{
	int val = readReg(hiPort, 0xfc91);
	while (val != 0xff)
		val = readReg(hiPort, 0xfc91);
	//return 18 * 256 / (val + 1);
	val = readRegW(hiPort, 0xf720);
	if (val <= 0x17b3)
		return 1;
	if (val <= 0x2f0a)
		return 2;
	if (val <= 0x3b56)
		return 3;
	if (val <= 0x435e)
		return 4;
	if (val <= 0x4934)
		return 5;
	if (val <= 0x4dba)
		return 6;
	if (val <= 0x516b)
		return 7;
	if (val <= 0x5484)
		return 8;
	if (val <= 0x5737)
		return 9;
	if (val <= 0x5999)
		return 10;
	if (val <= 0x5bb3)
		return 11;
	if (val <= 0x5d9b)
		return 12;
	if (val <= 0x5f46)
		return 13;
	if (val <= 0x60c8)
		return 14;
	if (val <= 0x6218)
		return 15;
	if (val <= 0x632a)
		return 16;
	if (val <= 0x63f5)
		return 17;
	if (val < 0x7000)
		return 18;
	return 1;
}

int HitachiModule::setZoomPosition(int val)
{
	QElapsedTimer t; t.start();
	int curr = getZoomPosition();
	int last = curr;
	if (val > curr) {
		writeReg(hiPort, 0xfcbb, 0x99);
		while (curr < val) {
			curr = getZoomPosition();
			if (curr != last) {
				last = curr;
				t.restart();
			}
			if (t.elapsed() > 1000)
				break;
		}
		writeReg(hiPort, 0xfcbb, 0xfe);
	} else {
		writeReg(hiPort, 0xfcbb, 0x9b);
		while (curr > val) {
			curr = getZoomPosition();
			if (curr != last) {
				last = curr;
				t.restart();
			}
			if (t.elapsed() > 1000)
				break;
		}
		writeReg(hiPort, 0xfcbb, 0xfe);
	}
	return 0;
}

#define SH_DIFF 4.0
#define H_DIFF 0.2
int HitachiModule::setZoomPositionF(float val)
{
	QElapsedTimer t; t.start();
	setZoomSpeed(ZOOM_SPEED_SUPER_HIGH);
	zoomAuto(val, 1000, 4.0);
	zoomAuto(val, 500, 2.0);
	setZoomSpeed(ZOOM_SPEED_HIGH);
	zoomAuto(val, 1000, 1.0);
	zoomAuto(val, 500, 0.5);
	setZoomSpeed(ZOOM_SPEED_NORMAL);
	zoomAuto(val, 75, 0.15);
	mDebug("zooming finished in %lld msec", t.elapsed());
	return 0;
}

float HitachiModule::zoomAuto(float val, int step, float tolerance)
{
	QElapsedTimer t; t.start();
	float curr = getZoomPositionF();
	if (qAbs(val - curr) < tolerance)
		return curr;
	if (val > curr) {
		while (val - curr > tolerance) {
			zoomIn(step);
			curr = getZoomPositionF();
			if (t.elapsed() > 30000)
				break;
		}
	} else {
		while (curr - val > tolerance) {
			zoomOut(step);
			curr = getZoomPositionF();
			if (t.elapsed() > 30000)
				break;
		}
	}
	return curr;
}

void HitachiModule::zoomInOut(float val, float curr, int step)
{
	if (val > curr)
		zoomIn(step);
	else
		zoomOut(step);
}

int HitachiModule::zoomIn(int msec)
{
	writeReg(hiPort, 0xfcbb, 0x99);
	usleep(msec * 1000);
	writeReg(hiPort, 0xfcbb, 0xfe);
	return 0;
}

int HitachiModule::zoomOut(int msec)
{
	writeReg(hiPort, 0xfcbb, 0x9b);
	usleep(msec * 1000);
	writeReg(hiPort, 0xfcbb, 0xfe);
	return 0;
}

int HitachiModule::setZoomSpeed(HitachiModule::ZoomSpeed s)
{
	uchar reg = readReg(hiPort, 0xfdfc) & 0xf3;
	if (s == ZOOM_SPEED_HIGH)
		writeReg(hiPort, 0xfdfc, reg);
	if (s == ZOOM_SPEED_SUPER_HIGH)
		writeReg(hiPort, 0xfdfc, (reg | 0x04));
	if (s == ZOOM_SPEED_NORMAL)
		writeReg(hiPort, 0xfdfc, (reg | 0x08));
	if (s == ZOOM_SPEED_SLOW)
		writeReg(hiPort, 0xfdfc, (reg | 0x0c));
	return 0;
}

HitachiModule::ZoomSpeed HitachiModule::getZoomSpeed()
{
	uchar reg = readReg(hiPort, 0xfdfc);
	if ((reg & 0x0c) == 0x08) {
		return ZOOM_SPEED_NORMAL;
	}
	if ((reg & 0x0c) == 0x04) {
		return ZOOM_SPEED_SUPER_HIGH;
	}
	if ((reg & 0x0c) == 0x0c) {
		return ZOOM_SPEED_SLOW;
	}
	return ZOOM_SPEED_HIGH;
}

int HitachiModule::startZoomIn()
{
	writeReg(hiPort, 0xfcbb, 0x99);
	return 0;
}

int HitachiModule::stopZoom()
{
	writeReg(hiPort, 0xfcbb, 0xfe);
	zoomFocusTimer->start(5*1000);
	return 0;
}

int HitachiModule::startZoomOut()
{
	writeReg(hiPort, 0xfcbb, 0x9b);
	return 0;
}

int HitachiModule::startFocusNear()
{
	writeReg(hiPort, 0xfcbb, 0xaa);
	return 0;
}

int HitachiModule::startFocusFar()
{
	writeReg(hiPort, 0xfcbb, 0xa9);
	return 0;
}

int HitachiModule::stopFocus()
{
	writeReg(hiPort, 0xfcbb, 0xfe);
	zoomFocusTimer->start(5*1000);
	return 0;
}

int HitachiModule::setCameraMode(CameraMode mode)
{
	if (mode == MODE_720P_25)
		writeReg(hiPort, 0x1701, 0x04);
	else if (mode == MODE_720P_30)
		writeReg(hiPort, 0x1701, 0x14);
	return 0;
}

HitachiModule::CameraMode HitachiModule::getCameraMode() // this function is wrong for sc110
{
	int val = readReg(hiPort, 0x1701);
	if (val == 0x04)
		return MODE_720P_25;
	else if (val == 0x14)
		return MODE_720P_30;
	else
		return MODE_UNDEFINED;
}

int HitachiModule::setFocusMode(FocusMode mode)
{
	int reg = readReg(hiPort, 0xff0e);
	if (mode == FOCUS_AUTO)
		writeReg(hiPort, 0xff0e, reg & ~0x8);
	else
		writeReg(hiPort, 0xff0e, reg | 0x8);
	return 0;
}

HitachiModule::FocusMode HitachiModule::getFocusMode()
{
	return (readReg(hiPort, 0xff0e) & 0x8) ? FOCUS_MANUAL : FOCUS_AUTO;
}

QString HitachiModule::getModelString(QextSerialPort *port)
{
	return readPort(port, ":RE1ED00");
}

HitachiModule::ModuleType HitachiModule::getModel(QextSerialPort *port)
{
	port->flush();
	port->readAll();
	/* some modules may be in AFP command mode */
	writePort(port, "SFD0101T");
	port->flush();
	usleep(100000);
	int model = readReg(port, 0xe1ed);
	if (model == 0x21)
		return MODULE_TYPE_SC110;
	if (model == 0x61)
		return MODULE_TYPE_SC120;
	return MODULE_TYPE_UNDEFINED;
}

int HitachiModule::setZoomFocusSettings(FocusMode val)
{
	if (val == FOCUS_AUTO) {
		writeRegW(hiPort, 0x11e6, readRegW(hiPort, 0xf714));
		usleep(50*1000);
		setZoomFocusType(TYPE_A);
		return 0;
	} else if (val == FOCUS_MANUAL) {
		writeRegW(hiPort, 0x1800, readRegW(hiPort, 0xf710));
		usleep(50*1000);
		writeRegW(hiPort, 0x1802, readRegW(hiPort, 0xf714));
		usleep(50*1000);
		writeRegW(hiPort, 0x1804, readRegW(hiPort, 0xf718));
		usleep(50*1000);
		writeRegW(hiPort, 0x1806, readRegW(hiPort, 0xf71A));
		usleep(50*1000);
		writeRegW(hiPort, 0x1808, readRegW(hiPort, 0xf7be));
		usleep(50*1000);
		writeRegW(hiPort, 0x180A, readRegW(hiPort, 0xf7c2));
		usleep(50*1000);
		writeRegW(hiPort, 0x180C, readRegW(hiPort, 0xf7c4));
		usleep(50*1000);
		writeReg(hiPort, 0x1810, readReg(hiPort, 0xf895));
		setZoomFocusType(TYPE_B);
		return 0;
	}
	return -EINVAL;
}

int HitachiModule::saveZoomFocus()
{
	zoomFocusTimer->stop();
	return setZoomFocusSettings(getFocusMode());
}

int HitachiModule::setZoomFocusType(TypeAB val)
{
	uchar reg = readReg(hiPort, 0x11e1) & ~0x80;
	if(val == TYPE_A)
		return writeReg(hiPort, 0x11e1, reg);
	else if (val == TYPE_B)
		return writeReg(hiPort, 0x11e1, reg | 0x80);
	return -EINVAL;
}

int HitachiModule::setAutoIrisLevel(irisOffset mode, bool wdr, int val)
{
	if (mode == IRIS_AVERAGE) {
		if (wdr == false) {
			return writeReg(hiPort, 0xfd9e, val);
		} else if (wdr == true) {
			return writeReg(hiPort, 0xfd90, val);
		}
	} else if (mode == IRIS_PEAK) {
		if (wdr == false) {
			return writeReg(hiPort, 0xfd9f, val);
		} else if (wdr == true) {
			return writeReg(hiPort, 0x13cf, val);
		}
	}
	return -EINVAL;
}

int HitachiModule::getAutoIrisLevel(irisOffset mode, bool wdr)
{
	if (mode == IRIS_AVERAGE) {
		if (wdr == false)
			return readReg(hiPort, 0xfd9e);
		else if (wdr == true)
			return readReg(hiPort, 0xfd90);
	} else if (mode == IRIS_PEAK) {
		if (wdr == false)
			return readReg(hiPort, 0xfd9f);
		else if (wdr == true)
			return readReg(hiPort, 0x13cf);
	}
	return -EINVAL;
}

int HitachiModule::setMaxAGCgain(float dBval)
{
	float step = 0.03125;
	float tuneX, tuneY;

	tuneX = dBval / step;
	tuneY = tuneX / 8;
	writeRegW(hiPort, 0xfd46, tuneX);
	writeReg(hiPort, 0xfa20, tuneY);
	return 0;
}

int HitachiModule::getMaxAGCgain()
{
	float step = 0.03125;
	return (readRegW(hiPort, 0xfd46) * step);
}

int HitachiModule::setFNRstatus(bool stat)
{
	return writeReg(hiPort, 0xff4a, stat);
}

bool HitachiModule::getFNRstatus()
{
	return readReg(hiPort, 0xff4a);
}

int HitachiModule::setFNRtuningVal(FNRtuningVal val)
{
	return writeReg(hiPort, 0xff4b, val);
}

HitachiModule::FNRtuningVal HitachiModule::getFNRtuningVal()
{
	return FNRtuningVal(readReg(hiPort, 0xff4b));
}

int HitachiModule::setFNRlevel(int val)
{
	if (val > 15)
		val = 15;
	else if (val < 0)
		val = 0;
	return writeReg(hiPort, 0xff4f, val);
}

int HitachiModule::getFNRlevel()
{
	return readReg(hiPort, 0xff4f);
}

int HitachiModule::setOnePushAF()
{
	zoomFocusTimer->start(10 * 1000);
	return writeReg(hiPort, 0xf73b,1);
}

int HitachiModule::setBLCstatus(bool stat)
{
	if (stat == false)
		return writeReg(hiPort, 0xfece, 0x00);
	else if(stat == true)
		return writeReg(hiPort, 0xfece, 0x02);
	return 0;
}

bool HitachiModule::getBLCstatus()
{
	int reg = readReg(hiPort, 0xfece);
	if (reg == 0x00)
		return false;
	else if (reg == 0x02)
		return true;
	return 0;
}

int HitachiModule::setBLCtuningVal(int val)
{
	return writeReg(hiPort, 0xfd8e, val);
}

int HitachiModule::getBLCtuningVal()
{
	return readReg(hiPort, 0xfd8e);
}

int HitachiModule::setAWBmode(AWBmode val)
{
	if (val == AWB_NORMAL)
		return writeReg(hiPort, 0xf9f2, val);
	else
		return writeReg(hiPort, 0xf9f2, val+1);
}

HitachiModule::AWBmode HitachiModule::getAWBmode()
{
	int reg = readReg(hiPort, 0xf9f2);
	if (reg == 0)
		return AWB_NORMAL;
	else
		return AWBmode(reg - 1);
}

int HitachiModule::setDefogMode(DefogMode val)
{
	return writeReg(hiPort, 0xff80, val);
}

HitachiModule::DefogMode HitachiModule::getDefogMode()
{
	return DefogMode(readReg(hiPort, 0xff80));
}

int HitachiModule::setDefogColor(int val)
{
	return writeReg(hiPort, 0xff81, val);
}

int HitachiModule::getDefogColor()
{
	return readReg(hiPort, 0xff81);
}

int HitachiModule::initializeRAM()
{
	writeReg(hiPort, 0xfcac, 0);
	usleep(5000*1000);
	return 0;
}

void HitachiModule::timeout()
{
	qDebug() << "current shutter value: " << readRegW(hiPort, 0xffda);
	qDebug() << "current exposure value: " << readRegW(hiPort, 0xffdc);
	qDebug() << "current gain value: " << getAGCgain();
}

int HitachiModule::readRegW(QextSerialPort *port, int reg)
{
	QString str = QString(":R%1%2").arg(reg, 4, 16, QChar('0')).arg("0000").toUpper();
	str = readPort(port, str.replace(":R", ":r"));
	return str.mid(str.size() - 4).toInt(0, 16);
}

int HitachiModule::writeRegW(QextSerialPort *port,int reg, int val)
{
	QString str = QString(":W%1%2").arg(reg, 4, 16, QChar('0')).arg(val, 4, 16, QChar('0')).toUpper();
	return writePort(port, str.replace(":W", ":w"));
}

int HitachiModule::readReg(QextSerialPort *port, int reg)
{	QString str = QString(":R%1%2").arg(reg, 4, 16, QChar('0')).arg("00").toUpper();
	str = readPort(port, str);
	return str.mid(str.size() - 2).toInt(0, 16);
}

int HitachiModule::writeReg(QextSerialPort *port, int reg, int val)
{
	QString str = QString(":W%1%2").arg(reg, 4, 16, QChar('0')).arg(val, 2, 16, QChar('0')).toUpper();
	return writePort(port, str);
}
