#include "irdomemodule.h"

#include <errno.h>
#include <unistd.h>
#include <settings/applicationsettings.h>

#include <QEventLoop>

static void mySleep(int msec)
{
	QEventLoop l;
	QTimer::singleShot(msec, &l, SLOT(quit()));
	l.exec();
}

#define ARRAY_SIZE(X) (sizeof(X) / sizeof(X[0]))

const QByteArray IrDomeModule::charTable = "ABCDEFGHIJKLMNOPQRSTUVWXYZ& ?!1234567890ÀÈÌÒÙÁÉÍÓÚÂÊÔÆÃÕÑÇßÄÏÖÜÅ$¥£¿¡ø”:’.,/-";

/**
 * @brief IrDomeModule::IrDomeModule
 * @param port : pointer of Module serialport
 * @param readWrite : File RW state
 * @param presetFilename : saving file of preset setting
 * @param presetLimitNo : preset limit
 * @param pattFilename : saving file of pattern setting
 * @param pattLimit : pattern limit
 * @param parent
 */
IrDomeModule::IrDomeModule(QextSerialPort *port, int readWrite, QString presetFilename,
						   int presetLimitNo, QString pattFilename, int pattLimit, QObject *parent):
	Pattern(readWrite, pattFilename, pattLimit, parent)
{
	writeAble = readWrite;
	cmdInterval = 110;
	maskBits = 0;
	irPort = port;
	presetLimit = presetLimitNo;
	presetSaveFile = presetFilename;
	Positions temp = {0,0,0};
	homePos = temp;
	for(int i = 0; i < presetLimit; i++)
		preset.append(QPair<bool, Positions>(0, temp));
	seriPortElapse.start();
	memset(&lastV, 0, sizeof(LastVariables));
	if(writeAble) {
		connect(this, SIGNAL(nextCmd(const char *,int)), this ,SLOT(writePort(const char *,int)));
		file2SpecialPosition();
	}
}

const QByteArray IrDomeModule::getPelcod(uchar addr, PelcoDCommands cmd, uchar data1, uchar data2)
{
	QByteArray command;
	command.resize(7);
	uchar *buf = (uchar *) command.constData();
	buf[0] = 0xff;
	buf[1] = addr;
	buf[2] = 0x00;
	buf[3] = 0x00;
	buf[4] = data1;
	buf[5] = data2;
	if (cmd == PELCOD_CMD_PANR)
		buf[3] = 0x02;
	else if (cmd == PELCOD_CMD_PANL)
		buf[3] = 0x04;
	else if (cmd == PELCOD_CMD_TILTU)
		buf[3] = 0x08;
	else if (cmd == PELCOD_CMD_TILTD)
		buf[3] = 0x10;
	else if (cmd == PELCOD_CMD_PANR_TILTU)
		buf[3] = 0x0A;
	else if (cmd == PELCOD_CMD_PANR_TILTD)
		buf[3] = 0x12;
	else if (cmd == PELCOD_CMD_PANL_TILTU)
		buf[3] = 0x0C;
	else if (cmd == PELCOD_CMD_PANL_TILTD)
		buf[3] = 0x14;
	else if (cmd == PELCOD_CMD_SET_PRESET) {
		buf[3] = 0x03;
		buf[4] = 0x00;
		buf[5] = data2;
	} else if (cmd == PELCOD_CMD_CLEAR_PRESET) {
		buf[3] = 0x05;
		buf[4] = 0x00;
		buf[5] = data2;
	} else if (cmd == PELCOD_CMD_GOTO_PRESET) {
		buf[3] = 0x07;
		buf[4] = 0x00;
		buf[5] = data2;
	} else if (cmd == PELCOD_CMD_FLIP180) {
		buf[3] = 0x07;
		buf[4] = 0x00;
		buf[5] = 0x21;
	} else if (cmd == PELCOD_CMD_ZERO_PAN) {
		buf[3] = 0x07;
		buf[4] = 0x00;
		buf[5] = 0x22;
	} else if (cmd == PELCOD_CMD_RESET) {
		buf[3] = 0x0F;
		buf[4] = 0x00;
		buf[5] = 0x00;
	} else if (cmd == PELCOD_QUE_PAN_POS) {
		buf[3] = 0x51;
		buf[4] = 0x00;
		buf[5] = 0x00;
	} else if (cmd == PELCOD_QUE_TILT_POS) {
		buf[3] = 0x53;
		buf[4] = 0x00;
		buf[5] = 0x00;
	} else if (cmd == PELCOD_QUE_ZOOM_POS) {
		buf[3] = 0x55;
		buf[4] = 0x00;
		buf[5] = 0x00;
	} else if (cmd == PELCOD_SET_ZERO_POS) {
		buf[3] = 0x49;
		buf[4] = 0x00;
		buf[5] = 0x00;
	} else if (cmd == PELCOD_SET_PAN_POS) {
		buf[3] = 0x4B;
		buf[5] = data2;
	} else if (cmd == PELCOD_SET_TILT_POS) {
		buf[3] = 0x4D;
		buf[5] = data2;
	} else if (cmd == PELCOD_PATTERN_START) {
		buf[3] = 0x1F;
		buf[4] = 0x00;
		buf[5] = 0x0F & data2;
	} else if (cmd == PELCOD_PATTERN_STOP) {
		buf[3] = 0x21;
		buf[4] = 0x00;
		buf[5] = 0x00;
	} 	else if (cmd == PELCOD_PATTERN_RUN) {
		buf[3] = 0x23;
		buf[4] = 0x00;
		buf[5] = 0x0F & data2;
	} else if (cmd == PELCOD_CMD_STOP)
		buf[3] = 0;
	buf[6] = checksum(buf, 6);
	return command;
}

const QByteArray IrDomeModule::getSpecialCommand(SpecialCommands cmd, uint data1, uint data2)
{
	QByteArray command;
	command.resize(9);
	uchar *buf = (uchar *) command.constData();
	buf[0] = 0x3A;
	buf[1] = 0xFF;
	buf[2] = 0x00;
	buf[3] = (data1 >> 8) & 0xff;
	buf[4] = data1 & 0xff;
	buf[5] = (data2 >> 8) & 0xff;
	buf[6] = data2 & 0xff;
	buf[7] = 0x00;
	buf[8] = 0x5C;
	if (cmd == SPECIAL_SET_COOR)
		buf[2] = 0x81;
	else if (cmd == SPECIAL_GET_COOR)
		buf[2] = 0x82;
	else if (cmd == SPECIAL_ZOOM_POS)
		buf[2] = 0x91;
	else if (cmd == SPECIAL_CURR_POS)
		buf[2] = 0x92;
	buf[7] = checksum(buf, 7);
	return command;
}

const QByteArray IrDomeModule::getLedControl(LedControl cmd)
{
	QByteArray command;
	command.resize(7);
	uchar *buf = (uchar *) command.constData();
	buf[0] = 0xFF;
	buf[1] = 0xFF;
	buf[2] = 0x00;
	buf[3] = 0x9B;
	buf[4] = 0x01;
	if (cmd == AUTO) {
		buf[4] = 0x00;
		buf[5] = 0x00;
	} else if (cmd == MANUEL) {
		buf[4] = 0x00;
		buf[5] = 0x01;
	} else
		buf[5] = cmd - 1;
	buf[6] = checksum(buf, 6);
	return command;
}

uint IrDomeModule::checksum(const uchar *cmd, uint lenght)
{
	unsigned int sum = 0;
	for (uint i = 1; i < lenght; i++)
		sum += cmd[i];
	return sum & 0xff;
}

int IrDomeModule::powerOnOff(int on)
{
	char val = 0x03;
	if (on)
		val = 0x02;
	const char mes[] = { 0x81, 0x01, 0x04, 0x00, val, 0xFF };
	writePort(mes, sizeof(mes));
	return 0;
}

int IrDomeModule::initializeCameraLens()
{
	const char lens[] = { 0x81, 0x01, 0x04, 0x19, 0x01, 0xff };
	return writePort(lens, sizeof(lens));
}

int IrDomeModule::initializeCameraReset()
{
	const char camera[] = { 0x81, 0x01, 0x04, 0x19, 0x03, 0xff };
	return 	writePort(camera, sizeof(camera));
}

int IrDomeModule::initializeCamera()
{
	initializeCameraLens();
	return initializeCameraReset();
}

int IrDomeModule::vSetReg(int reg, int value)
{
	const char regCmd[] = { 0x81, 0x01, 0x04, 0x24, reg & 0xff, (value & 0xf0) >> 4, value & 0xf, 0xff };
	return writePort(regCmd, sizeof(regCmd));
}

int IrDomeModule::vQueReg(int reg)
{
	const char regCmd[] = { 0x81, 0x09, 0x04, 0x24, reg & 0xff, 0xff };
	QByteArray ba = readPort(regCmd, sizeof(regCmd), 5);
	if (ba.size() == 5)
		return ((ba.at(2) & 0x0F) << 4) + ((ba.at(3) & 0x0F));
	return -1;
}

int IrDomeModule::startZoomIn(uint zoomSpeed)
{
	if (zoomSpeed < 8) {
		const char buf[] = { 0x81, 0x01, 0x04, 0x07, 0x20 + zoomSpeed, 0xff };
		return writePort(buf, sizeof(buf), true);
	} else
		return -ENODATA;
}

int IrDomeModule::startZoomOut(uint zoomSpeed)
{
	if (zoomSpeed < 8) {
		const char buf[] = { 0x81, 0x01, 0x04, 0x07, 0x30 + zoomSpeed, 0xff };
		return writePort(buf, sizeof(buf), true);
	} else
		return -ENODATA;
}

int IrDomeModule::stopZoom()
{
	const char opZoom[] = { 0x81, 0x01, 0x04, 0x07, 0x00, 0xff };
	return writePort(opZoom, sizeof(opZoom), true);
}

bool IrDomeModule::getDigiZoomState()
{
	const char digZoom[] = { 0x81, 0x09, 0x04, 0x06, 0xff };
	QByteArray ba = readPort(digZoom, sizeof(digZoom), 4);
	return (ba.at(2) == 0x02) ? true : false;
}

IrDomeModule::ZoomType IrDomeModule::getZoomType()
{
	const char zmType[] = { 0x81, 0x09, 0x04, 0x36, 0xff };
	QByteArray ba = readPort(zmType, sizeof(zmType), 4);
	if (ba.at(2) == 0x00)
		return COMBINE_MODE;
	else if (ba.at(2) == 0x01)
		return SEPARATE_MODE;
	else
		return ZM_UNDEFINED;
}

int IrDomeModule::selectZoomType(bool digiZoom, ZoomType zoomType)
{
	char digZoomT = 0x03;
	char zTypeT = 0x00;
	if (digiZoom)
		digZoomT = 0x02;
	if (zoomType == SEPARATE_MODE)
		zTypeT = 0x01;
	const char digZoom[] = { 0x81, 0x01, 0x04, 0x06, digZoomT, 0xff };
	const char zType[] = { 0x81, 0x01, 0x04, 0x36, zTypeT, 0xff };
	writePort(digZoom, sizeof(digZoom));
	return writePort(zType, sizeof(zType));
}

int IrDomeModule::setZoomFocusSettings()
{
	const char buf[] = { 0x81, 0x01, 0x04, 0x3f, 0x01, 0x7f, 0xff };
	return writePort(buf, sizeof(buf));
}

int IrDomeModule::startFocusNear()
{
	const char buf[] = { 0x81, 0x01, 0x04, 0x08, 0x03, 0xff };
	return writePort(buf, sizeof(buf));
}

int IrDomeModule::startFocusFar()
{
	const char buf[] = { 0x81, 0x01, 0x04, 0x08, 0x02, 0xff };
	return writePort(buf, sizeof(buf));
}

int IrDomeModule::stopFocus()
{
	const char buf[] = { 0x81, 0x01, 0x04, 0x08, 0x00, 0xff };
	return writePort(buf, sizeof(buf));
}

int IrDomeModule::setFocusMode(FocusMode mode)
{
	char focusModeT = 0x02;
	if (mode == FOCUS_MANUAL)
		focusModeT = 0x03;
	const char focusMode[] = { 0x81, 0x01, 0x04, 0x38, focusModeT, 0xff };

	return writePort(focusMode, sizeof(focusMode));
}

IrDomeModule::FocusMode IrDomeModule::getFocusMode()
{
	const char buf[] = {0x81, 0x09, 0x04, 0x38, 0xff};
	QByteArray ba = readPort(buf, sizeof(buf), 4);
	if (ba.at(2) == 0x02)
		return FOCUS_AUTO;
	else if (ba.at(2) == 0x03)
		return FOCUS_MANUAL;
	else
		return FOCUS_UNDEFINED;
}

int IrDomeModule::setZoomTrigger(bool stat)
{
	char mode = 0x01;
	if (stat == true)
		mode = 0x02;
	const char buf[] = { 0x81, 0x01, 0x04, 0x57,mode, 0xFF };
	return writePort(buf, sizeof(buf));
}

int IrDomeModule::getZoomTrigger()
{
	const char buf[] = {0x81, 0x09, 0x04, 0x57, 0xff};
	QByteArray ba = readPort(buf, sizeof(buf), 4);
	if (ba.at(2) == 0x01)
		return 0;
	else if (ba.at(2) == 0x02)
		return 1;
	else
		return -1;
}

int IrDomeModule::setOnePushAF()
{
	const char buf[] = { 0x81, 0x01, 0x04, 0x18, 0x01, 0xff };
	return writePort(buf, sizeof(buf));
}

int IrDomeModule::setBLCstatus(bool stat)
{
	char blcModeT = 0x03;
	if (stat == true)
		blcModeT = 0x02;
	const char blcMode[] = { 0x81, 0x01, 0x04, 0x33, blcModeT, 0xff };

	return writePort(blcMode, sizeof(blcMode));
}

bool IrDomeModule::getBLCstatus()
{
	const char blcMode[] = { 0x81, 0x09, 0x04, 0x33, 0xff };
	QByteArray ba = readPort(blcMode, sizeof(blcMode), 4);
	return (ba.at(2) == 0x02) ? true : false;
}

int IrDomeModule::setDisplayRotation(RotateMode val)
{
	char flipModeT = 0x03 ;
	char mirrorModeT = 0x03 ;
	if (val == REVERSE_MODE) {
		flipModeT = 0x02;
	} else if (val == MIRROR_MODE) {
		mirrorModeT = 0x02;
	} else if (val == VERT_INVER_MODE) {
		flipModeT = 0x02;
		mirrorModeT = 0x02;
	}
	const char mirrorMode[] = { 0x81, 0x01, 0x04, 0x61, mirrorModeT, 0xff };
	const char flipMode[] = { 0x81, 0x01, 0x04, 0x66, flipModeT, 0xff };

	writePort(mirrorMode, sizeof(mirrorMode));
	return writePort(flipMode, sizeof(flipMode));
}

IrDomeModule::RotateMode IrDomeModule::getDisplayRotation()
{
	bool flip = false, mirror = false;
	const char flipInq[] = { 0x81, 0x09, 0x04, 0x66, 0xff };
	const char mirrorInq[] = { 0x81, 0x09, 0x04, 0x61, 0xff };
	QByteArray ba = readPort(flipInq, sizeof(flipInq), 4);
	flip = (ba.at(2) == 0x02) ? true : false;
	ba = readPort(mirrorInq, sizeof(mirrorInq), 4);
	mirror = (ba.at(2) == 0x02) ? true : false;
	if (flip && !mirror)
		return REVERSE_MODE;
	else if (!flip && mirror)
		return MIRROR_MODE;
	else if (flip && mirror)
		return VERT_INVER_MODE;
	else
		return ROTATE_OFF;
}

int IrDomeModule::setIRCFstat(bool stat)
{
	char ircfModeT = 0x03;
	if (stat == true)
		ircfModeT = 0x02;
	const char ircfMode[] = { 0x81, 0x01, 0x04, 0x01, ircfModeT, 0xff };
	return writePort(ircfMode, sizeof(ircfMode), true);
}

bool IrDomeModule::getIRCFstat()
{
	const char ircfMode[] = { 0x81, 0x09, 0x04, 0x01, 0xff };
	QByteArray ba = readPort(ircfMode, sizeof(ircfMode), 4);
	lastV.IRCFstat = (ba.at(2) == 0x02) ? true : false;
	return lastV.IRCFstat;
}

int IrDomeModule::setAutoICR(bool stat)
{
	char autoICRt = 0x03;
	if (stat == true)
		autoICRt = 0x02;
	const char autoICR[] = { 0x81, 0x01, 0x04, 0x51, autoICRt, 0xff };
	return writePort(autoICR, sizeof(autoICR), true);
}

bool IrDomeModule::getAutoICR()
{
	const char autoICR[] = { 0x81, 0x09, 0x04, 0x51, 0xff };
	QByteArray ba = readPort(autoICR, sizeof(autoICR), 4);
	return (ba.at(2) == 0x02) ? true : false;
}

int IrDomeModule::setProgramAEmode(ProgramAEmode val)
{
	char progModeT = 0x00;
	if (val == MODE_FULL_AUTO)
		progModeT = 0x00;
	else if (val == MODE_MANUAL)
		progModeT = 0x03;
	else if (val == MODE_SHUTTER_PRIORITY)
		progModeT = 0x0a;
	else if (val == MODE_IRIS_PRIORITY)
		progModeT = 0x0b;
	else if (val == MODE_BRIGHT)
		progModeT = 0x0d;
	else
		return -EINVAL;
	const char progMode[] = { 0x81, 0x01, 0x04, 0x39, progModeT, 0xff };
	return writePort(progMode, sizeof(progMode));
}

IrDomeModule::ProgramAEmode IrDomeModule::getProgramAEmode()
{
	const char buf[] = {0x81, 0x09, 0x04, 0x39, 0xff};
	QByteArray ba = readPort(buf, sizeof(buf), 4);
	if (ba.at(2) == 0x00)
		return MODE_FULL_AUTO;
	else if (ba.at(2) == 0x03)
		return MODE_MANUAL;
	else if (ba.at(2) == 0x0a)
		return MODE_SHUTTER_PRIORITY;
	else if (ba.at(2) == 0x0b)
		return MODE_IRIS_PRIORITY;
	else if (ba.at(2) == 0x0d)
		return MODE_BRIGHT;
	else
		return MODE_INVALID;
}

int IrDomeModule::setShutterSpeed(ShutterSpeed val)
{
	const char shSpd[] = { 0x81, 0x01, 0x04, 0x4a, 0x00, 0x00, ((int)val & 0xf0) >> 4, ((int)val & 0x0f), 0xff };
	return writePort(shSpd, sizeof(shSpd));
}

IrDomeModule::ShutterSpeed IrDomeModule::getShutterSpeed()
{
	const char shInq[] = { 0x81, 0x09, 0x04, 0x4a,0xff };
	QByteArray ba = readPort(shInq, sizeof(shInq), 7);
	return (ShutterSpeed)((ba.at(4) << 4) | (ba.at(5)));
}

int IrDomeModule::setShutterLimit(int upLim, int downLim)
{
	const char shutLim[] = { 0x81, 0x01, 0x04, 0x40, 0x06, (upLim & 0xf00) >> 8,
							 (upLim & 0x0f0) >> 4, (upLim & 0x00f), (downLim & 0xf00) >> 8,
							 (downLim & 0x0f0) >> 4, (downLim & 0x00f), 0xff };
	return 	writePort(shutLim, sizeof(shutLim));
}

/**
 * @brief IrDomeModule::setMinShutter
 * @param on
 * @return
 */
int IrDomeModule::setMinShutter(bool on)
{
	char state = 0x02;
	if (!on)
		state = 0x03;
	const char buf[] = { 0x81, 0x01, 0x04, 0x12, state, 0xff };
	return 	writePort(buf, sizeof(buf));
}

/**
 * @brief IrDomeModule::getMinShutter
 * @return
 */
int IrDomeModule::getMinShutter()
{
	const char inq[] = { 0x81, 0x09, 0x04, 0x12, 0xff };
	QByteArray ba = readPort(inq, sizeof(inq), 4);
	if(ba.size() != 4)
		return -2;
	if (ba.at(2) == 0x02)
		return 1;
	else
		return 0;
}

/**
 * @brief IrDomeModule::setMinShutterLimit
 * @param lim
 * @return
 */
int IrDomeModule::setMinShutterLimit(IrDomeModule::ShutterSpeed lim)
{
	if(lim < SH_SPD_1_OV_30 || lim > SH_SPD_1_OV_6000)
		return -1;
	const char minLimit[] = { 0x81, 0x01, 0x04, 0x13, 0x00, 0x00, (lim & 0xf0) >> 4, (lim & 0x0f), 0xff };
	return writePort(minLimit, sizeof(minLimit));

}

/**
 * @brief IrDomeModule::getMinShutterLimit
 * @return
 */
IrDomeModule::ShutterSpeed IrDomeModule::getMinShutterLimit()
{
	const char inq[] = { 0x81, 0x09, 0x04, 0x13, 0xff };
	QByteArray ba = readPort(inq, sizeof(inq), 7);
	if(ba.size() != 7)
		return SH_SPD_UNDEFINED;
	uchar p = ba.at(4);
	uchar q = ba.at(5);
	return (ShutterSpeed)((p << 4) + q);
}

int IrDomeModule::setExposureValue(ExposureValue val)
{
	int exVal = (int)val;
	if (val == EXP_CLOSE)
		exVal = 0;
	else
		exVal = EXP_UNDEFINED - exVal + 3;
	const char expVal[] = { 0x81, 0x01, 0x04, 0x4b, 0x00, 0x00, (exVal & 0xf0) >> 4, (exVal & 0x0f), 0xff };

	return writePort(expVal, sizeof(expVal));
}

IrDomeModule::ExposureValue IrDomeModule::getExposureValue()
{
	const char expInq[] = { 0x81, 0x09, 0x04, 0x4b, 0xff };
	QByteArray ba = readPort(expInq, sizeof(expInq), 7);
	int exVal = (ba.at(4) << 4) | (ba.at(5));
	if (exVal == 0)
		return EXP_CLOSE;
	else
		exVal = EXP_UNDEFINED - exVal + 3;
	return (ExposureValue)(exVal);
}

int IrDomeModule::setExposureTarget(int val)
{
	const char buf[] = { 0x81, 0x01, 0x04, 0x40, 0x04, (val & 0xf0) >> 4, (val & 0x0f), 0xff };
	return 	writePort(buf, sizeof(buf));
}

/**
 * @brief IrDomeModule::setExpCompMode
 * @param on
 * @return
 */
int IrDomeModule::setExpCompMode(bool on)
{
	char state = 0x02;
	if (!on)
		state = 0x03;
	const char buf[] = { 0x81, 0x01, 0x04, 0x3E, state, 0xff };
	return 	writePort(buf, sizeof(buf));
}

/**
 * @brief IrDomeModule::getExpCompMode
 * @return
 */
int IrDomeModule::getExpCompMode()
{
	const char inq[] = { 0x81, 0x09, 0x04, 0x3e, 0xff };
	QByteArray ba = readPort(inq, sizeof(inq), 4);
	if(ba.size() != 4)
		return -2;
	if (ba.at(2) == 0x02)
		return 1;
	else
		return 0;
}

/**
 * @brief IrDomeModule::setExpCompValue
 * @param on
 * @return
 */
int IrDomeModule::setExpCompValue(ExposureComp val)
{
	const char buf[] = { 0x81, 0x01, 0x04, 0x4E, 0x00, 0x00, (val & 0xf0) >> 4, (val & 0x0f), 0xff };
	return 	writePort(buf, sizeof(buf));
}

/**
 * @brief IrDomeModule::getExpCompValue
 * @return
 */
IrDomeModule::ExposureComp IrDomeModule::getExpCompValue()
{
	const char inq[] = { 0x81, 0x09, 0x04, 0x4e, 0xff };
	QByteArray ba = readPort(inq, sizeof(inq), 7);
	if(ba.size() != 7)
		return EXP_COMP_ERROR;
	uchar p = ba.at(4);
	uchar q = ba.at(5);
	return (ExposureComp)((p << 4) + q);
}

int IrDomeModule::setGainValue(GainValue val)
{
	const char gainVal[] = { 0x81, 0x01, 0x04, 0x4c, 0x00, 0x00, (val & 0xf0) >> 4, (val & 0x0f), 0xff };
	return writePort(gainVal, sizeof(gainVal));
}

IrDomeModule::GainValue IrDomeModule::getGainValue()
{
	const char gainInq[] = { 0x81, 0x09, 0x04, 0x4c, 0xff };
	QByteArray ba = readPort(gainInq, sizeof(gainInq), 7);
	if (ba.size() != 7)
		return GAIN_UNDEFINED;
	return (GainValue)((ba.at(4) << 4) | (ba.at(5)));
}

/**
 * @brief IrDomeModule::setGainLimit
 * @param limit
 * @return
 */
int IrDomeModule::setGainLimit(GainLimit limit)
{
	const char gainLim[] = { 0x81, 0x01, 0x04, 0x2c, (limit + 4) & 0x0f, 0xff };
	return writePort(gainLim, sizeof(gainLim));
}

/**
 * @brief IrDomeModule::getGainLimit
 * @return
 */
IrDomeModule::GainLimit IrDomeModule::getGainLimit()
{
	const char gainInq[] = { 0x81, 0x09, 0x04, 0x2c, 0xff };
	QByteArray ba = readPort(gainInq, sizeof(gainInq), 4);
	if (ba.size() != 4)
		return GAIN_LIM_ERROR;
	return GainLimit(ba[2] - 4);
}

/**
 * @brief IrDomeModule::setGainLimit [Sony modules don't support]
 * @param upLim
 * @param downLim
 * @return
 */
int IrDomeModule::setGainLimit(int upLim, int downLim)
{
	const char gainLimit[] = { 0x81, 0x01, 0x04, 0x40, 0x05,
							   (upLim & 0xf0) >> 4, (upLim & 0x0f),
							   (downLim & 0xf0) >> 4, (downLim & 0x0f), 0xff };
	return 	writePort(gainLimit, sizeof(gainLimit));
}

int IrDomeModule::setNoiseReduction(int val)
{
	if (val > 5)
		return -ENODATA;
	const char nred[] = { 0x81, 0x01, 0x04, 0x53, val & 0x0f, 0xff };
	return writePort(nred, sizeof(nred));
}

int IrDomeModule::getNoiseReduction()
{
	const char nred[] = { 0x81, 0x09, 0x04, 0x53, 0xff };
	QByteArray ba = readPort(nred, sizeof(nred), 4);
	return (int)(ba.at(2));
}

int IrDomeModule::setWDRstat(bool stat)
{
	char buft = 0x03;
	if (stat == true)
		buft = 0x02;
	const char buf[] = { 0x81, 0x01, 0x04, 0x3d, buft, 0xff };
	return writePort(buf, sizeof(buf));
}

bool IrDomeModule::getWDRstat()
{
	const char wdr[] = { 0x81, 0x09, 0x04, 0x3d, 0xff };
	QByteArray ba = readPort(wdr, sizeof(wdr), 4);
	return (ba.at(2) == 0x02) ? true : false;
}

/**
 * @brief IrDomeModule::setWDRParameter
 * @param level : Display Brightness Level
 * @param selection : Brightness compensation selection
 * @param compen : Compensation Level
 * @return
 */
int IrDomeModule::setWDRParameter(int level, int selection, int compen)
{
	if (level > 6 || level < 0)
		return -1;
	if (selection > 3 || selection < 0)
		return -1;
	if (compen > 2 || compen < 0)
		return -1;
	const char buf[] = { 0x81, 0x01, 0x04, 0x2d, 0x00,
						 level, selection, compen,
						 0x00, 0x00, 0x00, 0x00, 0xff };
	return writePort(buf, sizeof(buf));
}

/**
 * @brief IrDomeModule::getWDRParameter
 * @return <Display Brightness Level>,<Brightness compensation selection>,<Compensation Level>
 */
QString IrDomeModule::getWDRParameter()
{
	const char inq[] = { 0x81, 0x09, 0x04, 0x2D, 0xff };
	QByteArray ba = readPort(inq, sizeof(inq), 11);
	if (ba.size() != 11)
		return QString();
	return QString("%1,%2,%3").arg(int(ba.at(3)))
			.arg(int(ba.at(4))).arg(int(ba.at(5)));
}

int IrDomeModule::setGamma(int val)
{
	if (val > 4)
		return -ENODATA;
	const char gamma[] = { 0x81, 0x01, 0x04, 0x5b, val & 0x0f, 0xff };
	return writePort(gamma, sizeof(gamma));
}

int IrDomeModule::getGamma()
{
	const char gamma[] = { 0x81, 0x09, 0x04, 0x5b, 0xff };
	QByteArray ba = readPort(gamma, sizeof(gamma), 4);
	return (int)(ba.at(2));
}

int IrDomeModule::setAWBmode(AWBmode mode)
{
	char whBalt= mode & 0x0f;
	if ((mode == AWB_MANUAL) || (mode == AWB_ATW))
		whBalt = (mode + 1) & 0x0f;
	const char whBal[] = { 0x81, 0x01, 0x04, 0x35, whBalt, 0xff };
	return 	writePort(whBal, sizeof(whBal));
}

IrDomeModule::AWBmode IrDomeModule::getAWBmode()
{
	const char whBal[] = { 0x81, 0x09, 0x04, 0x35, 0xff };
	QByteArray ba = readPort(whBal, sizeof(whBal), 4);
	int val = ba.at(2);
	if (val > 0x03)
		val -= 1;
	return (AWBmode)val;
}

int IrDomeModule::setDefogMode(bool stat)
{
	char statT = 0x03;
	if (stat == true)
		statT = 0x02;
	const char buf[] = { 0x81, 0x01, 0x04, 0x37, statT, 0x00, 0xff };
	return 	writePort(buf, sizeof(buf));
}

bool IrDomeModule::getDefogMode()
{
	const char defog[] = { 0x81, 0x09, 0x04, 0x37, 0xff };
	QByteArray ba = readPort(defog, sizeof(defog), 4);
	if (ba.at(2) == 0x02)
		return true;
	else
		return false;
}

int IrDomeModule::setIrisLimit(int upLim, int downLim)
{
	const char irisLim[] = { 0x81, 0x01, 0x04, 0x40, 0x07, (upLim & 0xf00) >> 8,
							 (upLim & 0x0f0) >> 4, (upLim & 0x00f),
							 (downLim & 0xf00) >> 8, (downLim & 0x0f0) >> 4,
							 (downLim & 0x00f), 0xff };
	return 	writePort(irisLim, sizeof(irisLim));
}

int IrDomeModule::openMenu(bool stat)
{
	char statT = 0x02;
	if (stat == true)
		statT = 0x01;
	const char buf[] = { 0x81, 0x01, 0x04, 0xdf, statT, 0xff };
	return 	writePort(buf, sizeof(buf));
}

int IrDomeModule::menuGoTo(MenuDirection direct)
{
	const char up[] = { 0x81, 0x01, 0x04, 0x07, 0x02, 0xff };
	const char down[] = { 0x81, 0x01, 0x04, 0x07, 0x03, 0xff };
	const char left[] = { 0x81, 0x01, 0x04, 0x08, 0x02, 0xff };
	const char right[] = { 0x81, 0x01, 0x04, 0x08, 0x03, 0xff };

	if (direct == DR_UP)
		return 	writePort(up, sizeof(up));
	else if (direct == DR_DOWN)
		return 	writePort(down, sizeof(down));
	else if (direct == DR_LEFT)
		return 	writePort(left, sizeof(left));
	else if (direct == DR_RIGHT)
		return 	writePort(right, sizeof(right));
	else
		return -ENOENT;
}

int IrDomeModule::applyProgramAEmode(ProgramAEmode ae, ShutterSpeed shVal, ExposureValue exVal, GainValue gain, bool ircf, FocusMode focusMode)
{
	static ProgramAEmode prevAE = MODE_INVALID;
	setProgramAEmode(ae);
	prevAE = ae;
	if (ae == MODE_SHUTTER_PRIORITY) {
		setShutterSpeed(shVal);
	} else if (ae == MODE_IRIS_PRIORITY) {
		setExposureValue(exVal);
	} else if (ae == MODE_MANUAL) {
		setShutterSpeed(shVal);
		setExposureValue(exVal);
		setGainValue(gain);
	}
	setIRCFstat(ircf);
	setFocusMode(focusMode);
	return 0;
}

int IrDomeModule::vGetZoom()
{
	const char opZoom[] = { 0x81, 0x09, 0x04, 0x47, 0xff };
	QByteArray ba = readPort(opZoom, sizeof(opZoom), 7);
	if (ba.size() == 7) {
		lastV.zoom = ((ba.at(2) & 0x0F) << 12) + ((ba.at(3) & 0x0F) << 8) + ((ba.at(4) & 0x0F) << 4) + (ba.at(5) & 0x0F);
		emit zoomPositionReady();
		return lastV.zoom;
	}
	return -1;
}

int IrDomeModule::vSetZoom(uint zoomPos)
{
	const char opZoom[] = { 0x81, 0x01, 0x04, 0x47, ((zoomPos & 0XF000) >> 12),
							((zoomPos & 0XF00) >> 8), ((zoomPos & 0XF0) >> 4),
							(zoomPos & 0XF), 0xff };
	QByteArray ba = readPort(opZoom, sizeof(opZoom), 20, true);
	if (ba.length() == 20) {
		char* baData = ba.data();
		lastV.zoom = ((baData[15] & 0x0f) << 4) + ((baData[17] & 0x0f) << 4) +
				((baData[18] & 0x0f) << 4) + (baData[19] & 0x0f);
		setAutoIRLed();
		return lastV.zoom;
	}
	return -1;
}

int IrDomeModule::pPanLeft(uchar speedPan)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	if (speedPan > 0x3f)
		speedPan = 0x40;
	return writePort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_CMD_PANL, speedPan, 0), 7, true);
}

int IrDomeModule::pPanRight(uchar speedPan)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	if (speedPan > 0x3f)
		speedPan = 0x40;
	return writePort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_CMD_PANR, speedPan, 0), 7, true);
}

int IrDomeModule::pTiltUp(uchar speedTilt)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	if (speedTilt > 0x3f)
		speedTilt = 0x3f;
	return writePort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_CMD_TILTU, 0, speedTilt), 7, true);
}

int IrDomeModule::pTiltDown(uchar speedTilt)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	if (speedTilt > 0x3f)
		speedTilt = 0x3f;
	return writePort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_CMD_TILTD, 0, speedTilt), 7, true);
}

int IrDomeModule::pPanLeftTiltUp(uchar speedPan, uchar speedTilt)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	if (speedTilt > 0x3f)
		speedTilt = 0x3f;
	if (speedPan > 0x3f)
		speedPan = 0x40;
	return writePort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_CMD_PANL_TILTU, speedPan, speedTilt), 7, true);
}

int IrDomeModule::pPanLeftTiltDown(uchar speedPan, uchar speedTilt)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	if (speedTilt > 0x3f)
		speedTilt = 0x3f;
	if (speedPan > 0x3f)
		speedPan = 0x40;
	return writePort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_CMD_PANL_TILTD, speedPan, speedTilt), 7, true);
}

int IrDomeModule::pPanRightTiltUp(uchar speedPan, uchar speedTilt)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	if (speedTilt > 0x3f)
		speedTilt = 0x3f;
	if (speedPan > 0x3f)
		speedPan = 0x40;
	return writePort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_CMD_PANR_TILTU, speedPan, speedTilt), 7, true);
}

int IrDomeModule::pPanRightTiltDown(uchar speedPan, uchar speedTilt)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	if (speedTilt > 0x3f)
		speedTilt = 0x3f;
	if (speedPan > 0x3f)
		speedPan = 0x40;
	return writePort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_CMD_PANR_TILTD, speedPan, speedTilt), 7, true);
}

/**
 * @brief IrDomeModule::pSetPreset
 * @param saveNo
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::pSetPreset(uchar saveNo)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	if (saveNo > 15)
		return -1;
	return writePort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_CMD_SET_PRESET, 0, saveNo), 7);
}

/**
 * @brief IrDomeModule::pClearPreset
 * @param saveNo
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::pClearPreset(uchar saveNo)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	if (saveNo > 15)
		return -1;
	return writePort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_CMD_CLEAR_PRESET, 0, saveNo), 7);
}

/**
 * @brief IrDomeModule::pGotoPreset
 * @param saveNo
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::pGotoPreset(uchar saveNo)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	if (saveNo > 15)
		return -1;
	return writePort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_CMD_GOTO_PRESET, 0, saveNo), 7, true);
}

/**
 * @brief IrDomeModule::pFlip180
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::pFlip180()
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	return writePort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_CMD_FLIP180, 0, 0), 7, true);
}

/**
 * @brief IrDomeModule::pZeroPan
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::pZeroPan()
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	return writePort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_CMD_ZERO_PAN, 0, 0), 7, true);
}

/**
 * @brief IrDomeModule::pReset
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::pReset()
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	return writePort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_CMD_RESET, 0, 0), 7, true);
}

/**
 * @brief IrDomeModule::pSetPan
 * @param pos
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::pSetPan(uint pos)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	return writePort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_SET_PAN_POS, (pos >> 8) & 0xFF, pos & 0xFF), 7, true);
}

/**
 * @brief IrDomeModule::pSetTilt
 * @param pos
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::pSetTilt(uint pos)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	return writePort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_SET_TILT_POS, (pos >> 8) & 0xFF, pos & 0xFF), 7, true);
}

/**
 * @brief IrDomeModule::pSetZero
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::pSetZero()
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	return writePort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_SET_ZERO_POS, 0, 0), 7);
}

/**
 * @brief IrDomeModule::pPatternStart
 * @param patternID
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::pPatternStart(int patternID)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	if (patternID > 15)
		return -1;
	return writePort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_PATTERN_START, 0, patternID), 7);
}

/**
 * @brief IrDomeModule::pPatternStop
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::pPatternStop()
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	return writePort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_PATTERN_STOP, 0, 0), 7);
}

/**
 * @brief IrDomeModule::pPatternRun
 * @param patternID
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::pPatternRun(int patternID)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	if (patternID > 15)
		return -1;
	return writePort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_PATTERN_RUN, 0, patternID), 7);
}

/**
 * @brief IrDomeModule::pGetPosPan
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::pGetPosPan()
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	const QByteArray ba = readPort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_QUE_PAN_POS, 0, 0), 7, 7);
	if (ba.size() == 7) {
		uchar *bytes = (uchar*) ba.constData();
		if (bytes[4] == 0x59 || bytes[7] == checksum(bytes, 7)) {
			lastV.position.first  = (bytes[4] << 8 ) + bytes[5];
			emit panTiltPositionReady();
			return lastV.position.first;
		}
	}
	return -1;
}

/**
 * @brief IrDomeModule::pGetPosTilt
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::pGetPosTilt()
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	const QByteArray ba = readPort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_QUE_TILT_POS, 0, 0), 7, 7);
	if (ba.size() == 7) {
		uchar *bytes = (uchar*) ba.constData();
		if (bytes[4] == 0x5B || bytes[7] == checksum(bytes, 7)) {
			lastV.position.second  = (bytes[4] << 8 ) + bytes[5];
			emit panTiltPositionReady();
			return lastV.position.second;
		}
	}
	return -1;
}

/**
 * @brief IrDomeModule::pGetPosZoom
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::pGetPosZoom()
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	const QByteArray ba = readPort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_QUE_ZOOM_POS, 0, 0), 7, 7);
	if (ba.size() == 7) {
		uchar *bytes = (uchar*) ba.constData();
		if (bytes[4] == 0x5D || bytes[7] == checksum(bytes, 7)) {
			lastV.zoom  = (bytes[4] << 8 ) + bytes[5];
			emit zoomPositionReady();
			setAutoIRLed();
			return lastV.zoom;
		}
	}
	return -1;
}

int IrDomeModule::pStop()
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	return writePort(getPelcod(PELCOD_ADD, IrDomeModule::PELCOD_CMD_STOP, 0, 0), 7, true);
}

int IrDomeModule::sSetPos(uint posH, uint posV)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	return writePort(getSpecialCommand(IrDomeModule::SPECIAL_SET_COOR, posH, posV), 9, true);
	/* that command dont responsed  by dome camera which firmware versionu V1.0 2016 12 03,
	QByteArray ba = readPort(getSpecialCommand(IrDomeModule::SPECIAL_SET_COOR, posH, posV), 9, 9, true);
	if (ba.size() == 9) {
		uchar *bytes = (uchar*) ba.constData();
		if (bytes[2] == 0x82 || bytes[8] == checksum(bytes, 7)) {
			QPair <int,int> rPos = QPair <int,int> ((bytes[3] << 8 ) + bytes[4], (bytes[5] << 8 )+ bytes[6]);
			lastV.position = rPos;
			emit panTiltPositionReady();
			return lastV.position;
		}
	}
	return QPair <int,int> (-1, -1);
	*/
}

QPair <int,int> IrDomeModule::sGetPos()
{
	if (!lastV.panTitlSupport)
		return QPair <int,int> (-EPROTONOSUPPORT, -EPROTONOSUPPORT);
	const QByteArray &ba = readPort(getSpecialCommand(IrDomeModule::SPECIAL_GET_COOR, 0, 0), 9, 9);
	if (ba.size() == 9) {
		const uchar * const bytes = (uchar*) ba.constData();
		if (bytes[2] == 0x82 || bytes[8] == checksum(bytes, 7)) {
			QPair <int,int> rPos ((bytes[3] << 8 ) + bytes[4], (bytes[5] << 8 )+ bytes[6]);
			lastV.position = rPos;
			emit panTiltPositionReady();
			return lastV.position;
		}
	}
	return QPair <int,int> (-1, -1);
}

QPair<int, int> IrDomeModule::getPosMem()
{
	return lastV.position;
}

/**
 * @brief IrDomeModule::sSetZoom
 * @param zoomPos
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::sSetZoom(uint zoomPos)
{
	QByteArray ba = readPort(getSpecialCommand(IrDomeModule::SPECIAL_ZOOM_POS, zoomPos, 0), 9, 9);
	if (ba.size() == 9) {
		uchar *bytes = (uchar*) ba.constData();
		if (bytes[2] == 0x92 || bytes[8] == checksum(bytes, 7)) {
			lastV.zoom = (bytes[3] << 8 ) + bytes[4];
			setAutoIRLed();
			return lastV.zoom;
		}
	}
	return -1;
}

/**
 * @brief IrDomeModule::sGetZoom
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::sGetZoom()
{
	QByteArray ba = readPort(getSpecialCommand(IrDomeModule::SPECIAL_CURR_POS, 0, 0), 9, 9);
	if (ba.size() == 9) {
		uchar *bytes = (uchar*) ba.constData();
		if (bytes[2] == 0x92 || bytes[8] == checksum(bytes, 7)){
			lastV.zoom = (bytes[3] << 8 ) + bytes[4];
			emit zoomPositionReady();
			return lastV.zoom;
		}
	}
	return -1;
}

int IrDomeModule::getZoomMem()
{
	return lastV.zoom;
}

void IrDomeModule::setZoomMem(int z)
{
	lastV.zoom = z;
}

/**
 * @brief IrDomeModule::getZoomRatio
 * @return
 */
uint IrDomeModule::getZoomRatio ()
{
	if (lastV.zoom == 0 || zoomRatio.size() == 0)
		return 0;

	for (int i = 1; i < zoomRatio.size(); i++ )
		if ((zoomRatio.at(i-1) < lastV.zoom) && (zoomRatio.at(i) >= lastV.zoom))
			return (uint)i;
	return zoomRatio.size();
}

/**
 * @brief IrDomeModule::getZoomRatioFloat
 * @return
 */
float IrDomeModule::getZoomRatioFloat()
{
	if (lastV.zoom == 0 || zoomRatio.size() == 0)
		return 1;

	for (int i = 1; i < zoomRatio.size(); i++ )
		if ((zoomRatio.at(i-1) < lastV.zoom) && (zoomRatio.at(i) >= lastV.zoom))
			return float(i) + float((lastV.zoom - zoomRatio.at(i-1))) / (zoomRatio.at(i) - zoomRatio.at(i-1));
	return float(zoomRatio.size());
}

int IrDomeModule::sSetAbsolute(uint posH, uint posV, uint zoomPos)
{
	vSetZoom(zoomPos);
	sSetPos(posH, posV);
	return 1;
}

QString IrDomeModule::dVersion()
{
	const char version[] = {0xFF, 0xFF, 0x00, 0xC2, 0x00, 0x00, 0xC1 };
	QByteArray ba = readPort(version, sizeof(version), 14);
	if (ba.size() == 14) {
		const uchar* const reply1 = (const uchar*)ba.data();
		const uchar* const reply2 = reply1 + 7;
		if (reply1[6] == checksum((uchar*)reply1, 6) && reply2[6] == checksum((uchar*)reply2, 6)) {
			QString verInfo = QString("V%1.%2_%3%4%5")
					.arg((reply1[4] & 0xF0) >> 4)
					.arg(reply1[4] & 0x0F, 2, 10, QChar('0'))
					.arg(reply1[5] + 2000)
					.arg(reply2[4], 2, 10, QChar('0'))
					.arg(reply2[5], 2, 10, QChar('0'));
			return verInfo;
		}
	}
	return QString();
}

int IrDomeModule::irLedControl(LedControl cmd)
{	
	if (!lastV.nightVisionSupport)
		return -EPROTONOSUPPORT;

	if (cmd == AUTO) {
		lastV.irLedstate = AUTO;
		return 0;
	}

	writePort(getLedControl(cmd), 7, true);
	if ( cmd == IrDomeModule::MANUEL) {
		lastV.irLedstate = cmd;
		writePort(getLedControl(IrDomeModule::LEVEL0), 7, true);
		return 0;
	}
	return -EROFS;
}

int IrDomeModule::setAutoIRLed() {

	if (!lastV.nightVisionSupport)
		return -EPROTONOSUPPORT;
	if (lastV.irLedstate != AUTO)
		return -1;
	if (lastV.IRCFstat) {
		uint zoomRatio = getZoomRatio();
		if(zoomRatio <= 5)
			return irLedControl(LEVEL1);
		else if(zoomRatio <= 10)
			return irLedControl(LEVEL2);
		else if(zoomRatio <= 15)
			return irLedControl(LEVEL3);
		else if(zoomRatio <= 20)
			return irLedControl(LEVEL4);
		else if(zoomRatio <= 25)
			return irLedControl(LEVEL5);
		else if(zoomRatio <= 30)
			return irLedControl(LEVEL6);
		else
			return -1;
	} else
		return irLedControl(LEVEL0);
}

int IrDomeModule::presetCall(int ind)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	if (ind < presetLimit && preset.at(ind).first == 1) {
		return sSetAbsolute(preset.at(ind).second.posHor,
							preset.at(ind).second.posVer, preset.at(ind).second.posZoom);
	}
	return -1;
}

int IrDomeModule::presetSave(int ind)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	if (ind < presetLimit) {
		Positions preSave = getAllPos();
		preset.replace(ind,QPair<bool, Positions>(1, preSave));
		return specialPosition2File();
	}
	return -1;
}

int IrDomeModule::presetDelete(int ind)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	Positions temp = {0,0,0};
	if (ind < presetLimit) {
		preset.replace(ind, QPair<bool, Positions>(0, temp));
		return specialPosition2File();
	}
	return -1;
}

int IrDomeModule::presetState(int ind)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	if (ind > -1 && ind < presetLimit)
		return preset.at(ind).first;
	return -1;
}

IrDomeModule::Positions IrDomeModule::presetPos(int ind)
{
	const Positions error = {-1, -1, -1};
	if (ind < presetLimit)
		return preset.at(ind).second;
	return error;
}

QString IrDomeModule::presetPosString(int ind)
{
	if (ind < presetLimit)
		return " p:" + QString::number(preset.at(ind).second.posHor) +
				", t:" + QString::number(preset.at(ind).second.posVer) +
				", z:" + QString::number(preset.at(ind).second.posZoom);
	return QString();
}

int IrDomeModule::getPresetLimit()
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	return presetLimit;
}

int IrDomeModule::patternStart(int ind)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	sGetPos();
	vGetZoom();
	int r = start(ind);
	sSetPos(lastV.position.first, lastV.position.second);
	vSetZoom(lastV.zoom);
	return r;
}

int IrDomeModule::patternStop(int ind)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	return stop(ind);
}

int IrDomeModule::patternRun(int ind)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	return run(ind);
}

int IrDomeModule::patternDelete(int ind)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	return deletePat(ind);
}

int IrDomeModule::patternCancel(int ind)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	pStop();
	stopZoom();
	return cancelPat(ind);
}

int IrDomeModule::patternState(int ind)
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	if (ind > -1 && ind < patternLimit)
		return patternStates[ind];
	return -1;
}

int IrDomeModule::getPatternLimit()
{
	if (!lastV.panTitlSupport)
		return -EPROTONOSUPPORT;
	return patternLimit;
}

IrDomeModule::Positions IrDomeModule::getAllPos()
{
	sGetPos();
	vGetZoom();
	struct Positions posAll;
	posAll.posHor = lastV.position.first;
	posAll.posVer = lastV.position.second;
	posAll.posZoom = lastV.zoom;
	return posAll;
}

int IrDomeModule::homeSave()
{
	sGetPos();
	vGetZoom();
	homePos.posHor = lastV.position.first;
	homePos.posVer = lastV.position.second;
	homePos.posZoom = lastV.zoom;
	return specialPosition2File();
}

int IrDomeModule::homeGoto()
{
	return sSetAbsolute(homePos.posHor, homePos.posVer, homePos.posZoom);
}

const struct IrDomeModule::Positions IrDomeModule::getHomePos()
{
	return homePos;
}

const QString IrDomeModule::getHomePosString()
{
	return " p:" + QString::number(homePos.posHor) +
			", t:" + QString::number(homePos.posVer) +
			", z:" + QString::number(homePos.posZoom);
}

int IrDomeModule::addSupport(IrDomeModule::SupportDevice dev, int state)
{
	if (dev == PANTILT)
		return lastV.panTitlSupport = state;
	else if (dev == NIGHTVISION)
		return lastV.nightVisionSupport = state;
	else
		return -ENOSTR;
}

bool IrDomeModule::isSupport(IrDomeModule::SupportDevice dev)
{
	if (dev == PANTILT)
		return lastV.panTitlSupport;
	else if (dev == NIGHTVISION)
		return lastV.nightVisionSupport;
	else
		return -ENOSTR;
}

const QByteArray IrDomeModule::readPort(const char *command, int wrlen, int rdlen, bool rec)
{
	/* clear buffers */
	irPort->readAll();

	QByteArray readData;
	writePort(command, wrlen, rec);
	QElapsedTimer t;
	t.start();
	if (irPort->queryMode() == QextSerialPort::EventDriven) {
		QTimer timeBreak;
		timeBreak.setSingleShot(true);
		QEventLoop l;
		connect(&timeBreak, SIGNAL(timeout()), &l , SLOT(quit()));
		connect(irPort, SIGNAL(readyRead()), &l , SLOT(quit()));
		timeBreak.start(1000);
		l.exec();
		if (timeBreak.isActive())
			timeBreak.stop();
	}
	t.start();
	while (irPort->bytesAvailable() < rdlen) {
		mySleep(2);
		if (t.elapsed() > 1000)
			break;
	}
	readData = irPort->readAll();
	if(rec)
		mInfo("Read Command: %s", readData.toHex().data());
	return readData;
}

int IrDomeModule::writePort(QextSerialPort* port, const char *command, int len)
{
	port->write(command, len);
	usleep(10000000 / port->baudRate() * len);
	return len;
}

QByteArray IrDomeModule::readPort(QextSerialPort *port, const char *command, int wrlen, int rdlen)
{
	/* clear buffers */
	port->readAll();

	QByteArray readData;
	writePort(port, command, wrlen);
	QElapsedTimer t;
	if (port->queryMode() == QextSerialPort::EventDriven) {
		QTimer timeBreak;
		timeBreak.setSingleShot(true);
		QEventLoop l;
		connect(&timeBreak, SIGNAL(timeout()), &l , SLOT(quit()));
		connect(port, SIGNAL(readyRead()), &l , SLOT(quit()));
		timeBreak.start(1000);
		l.exec();
		if (timeBreak.isActive())
			timeBreak.stop();
	}
	t.start();
	while (port->bytesAvailable() < rdlen) {
		mySleep(2);
		if (t.elapsed() > 1000)
			break;
	}
	readData = port->readAll();
	return readData;
}

IrDomeModule::ModuleType IrDomeModule::getModel(QextSerialPort *port)
{
	ModuleType modType = MODULE_TYPE_UNDEFINED;
	port->flush();
	port->readAll();
	const char buf[] = {0x81, 0x09, 0x00, 0x02, 0xff};
	const QByteArray ba = readPort(port, buf, sizeof(buf), 10);
	if (ba.at(3) == 0x20) {
		if ((ba.at(4) == 0x04) && (ba.at(5) == 0x5f)) {
			modType = MODULE_TYPE_PV6403_F12D;
		} else if ((ba.at(4) == 0x04) && (ba.at(5) == 0x6f)) {
			modType = MODULE_TYPE_FCB_EV7500;
		} else if ((ba.at(4) == 0x04) && (ba.at(5) == 0x5f)) {
			modType = MODULE_TYPE_FCB_EV7520;
		} else if ((ba.at(4) == 0x04) && (ba.at(5) == 0x67)) {
			modType = MODULE_TYPE_PV8430_F2D;
		}
	} else
		modType = MODULE_TYPE_NO_VISCA_MODULE;
	return modType;
}

int IrDomeModule::writePort(const char *command, int len, bool rec)
{
	int t = seriPortElapse.elapsed();
	while (t < cmdInterval) {
		mySleep(cmdInterval - t);
		t = seriPortElapse.elapsed();
	}
	seriPortElapse.restart();
	if(rec)
		mInfo("Send Command: %s", QByteArray(command, len).toHex().data());
	if(writeAble && rec)
		record(command, len);
	irPort->write(command, len);
	emit seriPortWrote();
	return len;
}

void IrDomeModule::updatePosition()
{
	vGetZoom();
	if (lastV.nightVisionSupport)
		setAutoIRLed();
	if (lastV.panTitlSupport) {
		sGetPos();
		if (maskBits)
			maskSetPanTiltAngle(lastV.position.first, lastV.position.second);
	}
	emit postionsUpdated();
}

int IrDomeModule::specialPosition2File()
{
	if (!writeAble)
		return -ENODATA;
	QFile specialPosFile;
	specialPosFile.setFileName(presetSaveFile);
	if (specialPosFile.open(QFile::ReadWrite)) {
		specialPosFile.resize(0);
		QByteArray rec;
		for (int i = 0; i < presetLimit; i++) {
			for (uint k = 0; k < sizeof(preset.at(i).second.posHor); k++)
				rec.append((preset.at(i).second.posHor & (0xFF << (k*8))) >> (k*8));
			for (uint k = 0; k < sizeof(preset.at(i).second.posVer); k++)
				rec.append((preset.at(i).second.posVer & (0xFF << (k*8))) >> (k*8));
			for (uint k = 0; k < sizeof(preset.at(i).second.posZoom); k++)
				rec.append((preset.at(i).second.posZoom & (0xFF << (k*8))) >> (k*8));
			rec.append((char)preset.at(i).first);
			rec.append("\n");
		}
		for (uint k = 0; k < sizeof(homePos.posHor); k++)
			rec.append((homePos.posHor & (0xFF << (k*8))) >> (k*8));
		for (uint k = 0; k < sizeof(homePos.posVer); k++)
			rec.append((homePos.posVer & (0xFF << (k*8))) >> (k*8));
		for (uint k = 0; k < sizeof(homePos.posZoom); k++)
			rec.append((homePos.posZoom & (0xFF << (k*8))) >> (k*8));
		rec.append(1);
		rec.append("\n");
		specialPosFile.write(rec);
		specialPosFile.close();
		return 1;
	}
	return -1;
}

#define EACH_POS_LENGHT (sizeof(IrDomeModule::Positions) + 2)
int IrDomeModule::file2SpecialPosition()
{
	if (!writeAble)
		return -ENODATA;
	mDebug("Last position record is been reading.");
	QFile specialPosFile;
	specialPosFile.setFileName(presetSaveFile);
	if (specialPosFile.open(QIODevice::ReadWrite)) {
		if (specialPosFile.size() == (presetLimit + 1) * EACH_POS_LENGHT) {
			QByteArray ba;
			Positions tmp;
			for (int i = 0; i < presetLimit; i++) {
				ba = specialPosFile.read(EACH_POS_LENGHT);
				const char *baData = ba.constData();
				tmp.posHor = baData[0] + (baData[1] << 8) +
						(baData[2] << 16) + (baData[3] << 24);
				tmp.posVer = baData[4] + (baData[5] << 8) +
						(baData[6] << 16) + (baData[7] << 24);
				tmp.posZoom = baData[8] + (baData[9] << 8) +
						(baData[10] << 16) + (baData[11] << 24);
				preset.replace(i,QPair<bool, Positions> (baData[12], tmp));
			}
			ba = specialPosFile.read(EACH_POS_LENGHT);
			const char *baData = ba.constData();
			if (baData[12]) {
				homePos.posHor = baData[0] + (baData[1] << 8) +
						(baData[2] << 16) + (baData[3] << 24);
				homePos.posVer = baData[4] + (baData[5] << 8) +
						(baData[6] << 16) + (baData[7] << 24);
				homePos.posZoom = baData[8] + (baData[9] << 8) +
						(baData[10] << 16) + (baData[11] << 24);
			}
			specialPosFile.close();
			return 1;
		}
	}
	return -1;
}

int IrDomeModule::pictureEffect(Effect eff)
{
	const char effect[] = {0x81, 0x01, 0x04, 0x63, (int)eff * 2, 0xFF};
	return writePort(effect, sizeof(effect));
}

int IrDomeModule::continuousReplyZoom (bool state)
{
	char onOff = 0x03;
	if (state)
		onOff = 0x02; // on
	const char reply[] = {0x81, 0x01, 0x04, 0x69, onOff, 0xFF};
	return writePort(reply, sizeof(reply));
}

int IrDomeModule::continuousReplyFocus (bool state)
{
	char onOff = 0x03;
	if (state)
		onOff = 0x02; // on
	const char reply[] = {0x81, 0x01, 0x04, 0x16, onOff, 0xFF};
	return writePort(reply, sizeof(reply));
}

/**
 * @brief IrDomeModule::maskSet Mask Setting
 * @param maskID : visca is have 24 mask ID, from 0 to 23
 * @param nn : if 0 ,this mask is used one time after reset setting, if 1, setting is record
 * @param width : width +-0x50
 * @param height : height +-0x2d
 * @return
 */
int IrDomeModule::maskSet (uint maskID, int width, int height, bool nn)
{
	if((maskID > 0x17) || (width > 0x50) || (width < -0x50) || (height > 0x2D) || (height < -0x2D))
		return -1;
	const char buf[] = {0x81, 0x01, 0x04, 0x76, maskID & 0xFF,
						nn, (width & 0xF0) >> 4, width & 0x0F,
						(height & 0xF0) >> 4, height & 0x0F, 0xFF };
	return writePort(buf, sizeof(buf));
}

/**
 * @brief IrDomeModule::maskSetNoninterlock
 * @param  : visca is have 24 mask ID, from 0 to 23
 * @param x : horizonal +-0x50
 * @param y : vertical +-0x2d
 * @param width : width +-0x50
 * @param height : height +-0x2d
 * @param nn : if 0 ,this mask is used one time after reset setting, if 1, setting is record
 * @return
 */
int IrDomeModule::maskSetNoninterlock (uint maskID, int x, int y, int width, int height)
{
	if((maskID > 0x17) || (width > 0x50) || (width < -0x50) ||
			(height > 0x2D) || (height < -0x2D)||
			(x > 0x50) || (x < -0x50) ||
			(y > 0x2D) || (y < -0x2D))
		return -1;
	const char buf[] = {0x81, 0x01, 0x04, 0x6F, maskID & 0xFF,
						(x & 0xF0) >> 4, x & 0x0F,
						(y & 0xF0) >> 4, y & 0x0F,
						(width & 0xF0) >> 4, width & 0x0F,
						(height & 0xF0) >> 4, height & 0x0F, 0xFF };
	return writePort(buf, sizeof(buf));
}

int IrDomeModule::maskSetPTZ(uint maskID, int pan, int tilt, uint zoom)
{
	if (maskRanges.pMax == maskRanges.pMin)
		return -ENODATA;
	if (maskRanges.tMax == maskRanges.tMin)
		return -ENODATA;
	if (maskRanges.xMax == maskRanges.xMin)
		return -ENODATA;
	if (maskRanges.yMax == maskRanges.yMin)
		return -ENODATA;

	if (hPole)
		pan = maskRanges.pMax - pan;
	if (vPole)
		tilt = maskRanges.tMax - tilt;

	if (tilt > 9000)
		tilt += 18000;

	tilt = tilt * yTiltRate + maskRanges.yMin;
	pan = pan * xPanRate + maskRanges.xMin;
	const char cmdMask[] = {0x81, 0x01, 0x04, 0x7B, maskID,
							(0xf00 & pan) >> 8, (0xf0 & pan) >> 4, (0xf & pan),
							(0xf00 & tilt) >> 8, (0xf0 & tilt) >> 4, (0xf & tilt),
							(0xf000 & zoom) >> 16,(0xf00 & zoom) >> 8, (0xf0 & zoom) >> 4,
							(0xf & zoom), 0xFF};
	return writePort(cmdMask, sizeof(cmdMask));
}

int IrDomeModule::maskSetPanTiltAngle(int pan, int tilt)
{
	if (maskRanges.pMax == maskRanges.pMin)
		return -ENODATA;
	if (maskRanges.tMax == maskRanges.tMin)
		return -ENODATA;

	if (hPole)
		pan = maskRanges.pMax - pan;
	if (vPole)
		tilt = maskRanges.tMax - tilt;

	if (tilt > 9000)
		tilt += 18000;

	tilt = tilt * yTiltRate + maskRanges.yMin;
	pan = pan * xPanRate + maskRanges.xMin;

	const char cmdMask[] = {0x81, 0x01, 0x04, 0x79,
							(0xf00 & pan) >> 8, (0xf0 & pan) >> 4, (0xf & pan),
							(0xf00 & tilt) >> 8, (0xf0 & tilt) >> 4, (0xf & tilt), 0xFF};
	return writePort(cmdMask, sizeof(cmdMask));
}

int IrDomeModule::maskSetRanges(int panMax, int panMin, int xMax, int xMin, int tiltMax, int tiltMin, int yMax, int yMin, bool hConvert, bool vConvert)
{
	if (panMax == panMin)
		return -ENODATA;
	maskRanges.pMax = panMax;
	maskRanges.pMin = panMin;

	if (tiltMax == tiltMin)
		return -ENODATA;
	maskRanges.tMax = tiltMax;
	maskRanges.tMin = tiltMin;

	if (xMax == xMin)
		return -ENODATA;
	maskRanges.xMax = xMax;
	maskRanges.xMin = xMin;

	if (yMax == yMin)
		return -ENODATA;
	maskRanges.yMax = yMax;
	maskRanges.yMin = yMin;

	hPole = hConvert;
	vPole = vConvert;

	xPanRate = (1.0 * maskRanges.xMax - 1.0 * maskRanges.xMin) / (1.0 * maskRanges.pMax - 1.0 * maskRanges.pMin);
	yTiltRate = (1.0 * maskRanges.yMax - 1.0 * maskRanges.yMin) / (1.0 * maskRanges.tMax - 1.0 * maskRanges.tMin);
	return 0;
}

int IrDomeModule::maskDisplay (uint maskID, bool onOff)
{

	if (maskID > 17)
		maskID += 6;
	else if (maskID > 11)
		maskID += 4;
	else if (maskID > 5)
		maskID += 2;

	if (onOff)
		maskBits |= (1 << maskID);
	else
		maskBits &= ~(1 << maskID);
	maskBits &= 0x3F3F3F3F;
	const char buf[] = {0x81, 0x01, 0x04, 0x77, (maskBits & 0xff000000) >> 24,
						(maskBits & 0xff0000) >> 16, (maskBits & 0xff00) >> 8,
						(maskBits & 0xff), 0xff };
	return writePort(buf, sizeof(buf));
}

/**
 * @brief IrDomeModule::maskColor
 * @param maskID
 * @param color_0
 * @param color_1
 * @param colorChoose
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::maskColor (uint maskID, MaskColor color_0, MaskColor color_1, bool colorChoose)
{
	uint maskBits = 0;
	if (colorChoose == true)
		maskBits |= (1 << maskID);
	else
		maskBits &= ~(1 << maskID);
	const char buf[] = {0x81, 0x01, 0x04, 0x78, (maskBits & 0xff000000) >> 24,
						(maskBits & 0xff0000) >> 16, (maskBits & 0xff00) >> 8,
						(maskBits & 0xff),(((int)color_0) & 0xff),
						(((int)color_1) & 0xff), 0xff };
	return writePort(buf, sizeof(buf));
}

/**
 * @brief IrDomeModule::maskGrid
 * @param onOffCenter
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::maskGrid(MaskGrid onOffCenter)
{
	const char buf[] = {0x81, 0x01, 0x04, 0x7C, onOffCenter, 0xff };
	return writePort(buf, sizeof(buf));
}

/**
 * @brief IrDomeModule::freeze
 * @param state
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::freeze(bool state)
{
	char onOff = 0x03;
	if (state)
		onOff = 0x02; // on
	const char freezee[] = {0x81, 0x01, 0x04, 0x62, onOff, 0xFF};
	return writePort(freezee, sizeof(freezee));
}

/**
 * @brief IrDomeModule::setWhiteBalance
 * @param WB
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::setWhiteBalance(WhiteBalance WB)
{
	const char buf[] = {0x81, 0x01, 0x04, 0x35, (int)WB, 0xff };
	return writePort(buf, sizeof(buf));
}

/**
 * @brief IrDomeModule::titleSet1
 * @param lineNumber
 * @param hPosition
 * @param color
 * @param blink
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::titleSet1(uint lineNumber, uint hPosition, TitleColor color, bool blink)
{
	if (lineNumber > 0x0A || hPosition > 0x1F)
		return -1;
	const char buf[] = {0x81, 0x01, 0x04, 0x73, 0x10 | lineNumber, 0x00, hPosition,
						(int) color, blink, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff};
	return writePort(buf, sizeof(buf));
}

/**
 * @brief IrDomeModule::titleSet2 set 0-10 character of that one line
 * @param lineNumber 0-10 line id
 * @param characters
 * @param len : number of characters
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::titleSet2(uint lineNumber, const char* characters, uint len)
{
	if (lineNumber > 10 || len > 10)
		return -1;
	char charBuf[10];
	memset(charBuf, ' ', 10);
	if (len)
		memcpy(charBuf, characters, len);
	const char buf[] = {0x81, 0x01, 0x04, 0x73, 0x20 | lineNumber,
						charTable.indexOf(charBuf[0]),
						charTable.indexOf(charBuf[1]),
						charTable.indexOf(charBuf[2]),
						charTable.indexOf(charBuf[3]),
						charTable.indexOf(charBuf[4]),
						charTable.indexOf(charBuf[5]),
						charTable.indexOf(charBuf[6]),
						charTable.indexOf(charBuf[7]),
						charTable.indexOf(charBuf[8]),
						charTable.indexOf(charBuf[9]), 0xff};
	return writePort(buf, sizeof(buf));
}

/**
 * @brief IrDomeModule::titleSet3 set 11-20 character of that one line
 * @param lineNumber 0-10 line id
 * @param characters
 * @param len : number of characters
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::titleSet3(uint lineNumber, const char* characters, uint len)
{
	if (lineNumber > 10 || len > 10)
		return -1;
	char charBuf[10];
	memset(charBuf, ' ', 10);
	if (len)
		memcpy(charBuf, characters, len);
	const char buf[] = {0x81, 0x01, 0x04, 0x73, 0x30 | lineNumber,
						charTable.indexOf(charBuf[0]),
						charTable.indexOf(charBuf[1]),
						charTable.indexOf(charBuf[2]),
						charTable.indexOf(charBuf[3]),
						charTable.indexOf(charBuf[4]),
						charTable.indexOf(charBuf[5]),
						charTable.indexOf(charBuf[6]),
						charTable.indexOf(charBuf[7]),
						charTable.indexOf(charBuf[8]),
						charTable.indexOf(charBuf[9]), 0xff};
	return writePort(buf, sizeof(buf));
}

/**
 * @brief IrDomeModule::titleClear [china dome is not supported]
 * @param lineNumber : 0-10 for single line, 0x0f(15) for all line
 * @return
 */
int IrDomeModule::titleClear(uint lineNumber)
{
	if ((lineNumber > 10) && (lineNumber != 0x0f))
		return -1;
	const char buf[] = {0x81, 0x01, 0x04, 0x74, 0x10 | lineNumber, 0xff};
	return writePort(buf, sizeof(buf));
}

/**
 * @brief IrDomeModule::titleDisplay
 * @param lineNumber : 0-10 for single line, 0x0f(15) for all line
 * @param onOff
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::titleDisplay(uint lineNumber, bool onOff)
{
	if ((lineNumber > 10) && (lineNumber != 0x0f))
		return -1;

	const char buf[] = {0x81, 0x01, 0x04, 0x74, (0x30 - onOff * 0x10) | lineNumber, 0xff};
	return writePort(buf, sizeof(buf));
}

/**
 * @brief IrDomeModule::titleWrite displayed one line title
 * @param lineNumber
 * @param str
 * @param hPosition
 * @param color
 * @param blink
 * @return
 * [china dome is not supported]
 */
int IrDomeModule::titleWrite(uint lineNumber,const QByteArray str, uint hPosition, TitleColor color, bool blink)
{
	if (lineNumber > 10)
		return -1;
	titleClear(lineNumber);
	titleSet1(lineNumber, hPosition, color,blink);
	int strLen = str.size();
	const char* strData = str.constData();
	if (strLen > 10) {
		titleSet2(lineNumber, strData, 10);
		if (strLen > 20 )
			titleSet3(lineNumber, strData + 10, 10);
		else
			titleSet3(lineNumber, strData + 10, strLen -10);
	} else
		titleSet2(lineNumber, strData, strLen);
	return titleDisplay(lineNumber, 1);
}

void IrDomeModule::setSeriPortState(QIODevice::OpenMode mod)
{
	if (!irPort->isOpen())
		irPort->open(mod);
}

QIODevice::OpenMode IrDomeModule::getSeriPortState()
{
	return irPort->openMode();
}

void IrDomeModule::setCmdInterval(int interval)
{
	cmdInterval = interval;
}

int IrDomeModule::getCmdInterval()
{
	return cmdInterval;
}

int IrDomeModule::setZoomLookUp(const QStringList &zoomRatioList)
{
	if (!zoomRatioList.size())
		return -EINVAL;
	zoomRatio.clear();
	bool ok;
	int zRatio;
	foreach (QString s, zoomRatioList) {
		zRatio = s.toInt(&ok, 16);
		if (ok)
			zoomRatio.append(zRatio);
		else {
			zoomRatio.clear();
			return -EINVAL;
		}
	}
	return 0;
}
