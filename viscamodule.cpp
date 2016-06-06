#include "debug.h"
#include "viscamodule.h"
#include "qextserialport/qextserialport.h"

#include <errno.h>

ViscaModule::ViscaModule(QextSerialPort *port, QObject *parent) :
	QObject(parent)
{
	this->port = port;
	serialTimer = new QTimer(this);
	connect(serialTimer, SIGNAL(timeout()), SLOT(timeout()));
}

ViscaModule::~ViscaModule()
{
}

int ViscaModule::writePort(const char *command, int len)
{
	usleep(20*1000);
	return port->write(command, len);
}

int ViscaModule::writePort(QByteArray command, int len)
{
	usleep(20*1000);
	return port->write(command, len);
}

QByteArray ViscaModule::readPort(QextSerialPort *port, const char *command, int wrlen, int rdlen)
{
	/* clear buffers */
	port->readAll();

	QByteArray readData;
	qint64 bw = 0;
	bw = writePort(port, command, wrlen);
	usleep(100 * 1000);
	//usleep(RW_DELAY_US);

	int timeout = 1000;
	while (port->bytesAvailable() < rdlen) {
		usleep(10000);
		timeout -= 10;
		if (timeout < 0)
			break;
	}
	//usleep(100 * 1000);
	readData = port->readAll();
	return readData;
}

int ViscaModule::writePort(QextSerialPort *port, const char *command, int len)
{
	usleep(20*1000);
	return port->write(command, len);
}

QByteArray ViscaModule::readPort(const char *command, int wrlen, int rdlen)
{
	return readPort(port, command, wrlen, rdlen);
}

ViscaModule::ModuleType ViscaModule::getModel(QextSerialPort *port)
{
	ModuleType modType = MODULE_TYPE_UNDEFINED;
	port->flush();
	port->readAll();
	const char buf[] = { 0x81, 0x09, 0x00, 0x02, 0xff };
	usleep(100 * 1000);
	QByteArray ba = readPort(port, buf, sizeof(buf), 10);
	if(ba.at(3) == 0x20)
		modType = MODULE_TYPE_CHINESE;
	return modType;
}

int ViscaModule::startZoomIn()
{
	//const char buf[] = { 0x81, 0x01, 0x04, 0x07, 0x02, 0xff };
	const char buf[] = { 0x81, 0x01, 0x04, 0x07, 0x25, 0xff };
	serialTimer->start(100);
	return writePort(buf, sizeof(buf));
}

int ViscaModule::startZoomOut()
{
	//const char buf[] = { 0x81, 0x01, 0x04, 0x07, 0x03, 0xff };
	const char buf[] = { 0x81, 0x01, 0x04, 0x07, 0x35, 0xff };
	serialTimer->start(100);
	return writePort(buf, sizeof(buf));
}

int ViscaModule::stopZoom()
{
	serialTimer->stop();
	const char buf[] = { 0x81, 0x01, 0x04, 0x07, 0x00, 0xff };
	return writePort(buf, sizeof(buf));
}

int ViscaModule::startFocusNear()
{
	const char buf[] = { 0x81, 0x01, 0x04, 0x08, 0x03, 0xff };
	return writePort(buf, sizeof(buf));
}

int ViscaModule::startFocusFar()
{
	const char buf[] = { 0x81, 0x01, 0x04, 0x08, 0x02, 0xff };
	return writePort(buf, sizeof(buf));
}

int ViscaModule::stopFocus()
{
	const char buf[] = { 0x81, 0x01, 0x04, 0x08, 0x00, 0xff };
	return writePort(buf, sizeof(buf));
}

int ViscaModule::setFocusMode(FocusMode mode)
{
	char focusMode[] = { 0x81, 0x01, 0x04, 0x38, 0x02, 0xff };
	if (mode == FOCUS_MANUAL)
		focusMode[4] = 0x03;
	return writePort(focusMode, sizeof(focusMode));
}

ViscaModule::FocusMode ViscaModule::getFocusMode()
{
	char buf[] = {0x81, 0x09, 0x04, 0x38, 0xff};
	QByteArray ba = readPort(buf, sizeof(buf), 4);
	if (ba.at(2) == 0x02)
		return FOCUS_AUTO;
	else if (ba.at(2) == 0x03)
		return FOCUS_MANUAL;
	else
		return FOCUS_UNDEFINED;
}

int ViscaModule::setOnePushAF()
{
	const char buf[] = { 0x81, 0x01, 0x04, 0x18, 0x01, 0xff };
	return writePort(buf, sizeof(buf));
}

int ViscaModule::setBLCstatus(bool stat)
{
	char blcMode[] = { 0x81, 0x01, 0x04, 0x33, 0x03, 0xff };
	if (stat == true)
		blcMode[4] = 0x02;
	return writePort(blcMode, sizeof(blcMode));
}

bool ViscaModule::getBLCstatus()
{
	char blcMode[] = { 0x81, 0x09, 0x04, 0x33, 0xff };
	QByteArray ba = readPort(blcMode, sizeof(blcMode), 4);
	if (ba.at(2) == 0x02)
		return true;
	else
		return false;
}

int ViscaModule::setDisplayRotation(RotateMode val)
{
	char mirrorMode[] = { 0x81, 0x01, 0x04, 0x61, 0x03, 0xff };
	char flipMode[] = { 0x81, 0x01, 0x04, 0x66, 0x03, 0xff };

	if (val == REVERSE_MODE) {
		flipMode[4] = 0x02;
	} else if (val == MIRROR_MODE) {
		mirrorMode[4] = 0x02;
	} else if (val == VERT_INVER_MODE) {
		flipMode[4] = 0x02;
		mirrorMode[4] = 0x02;
	}
	writePort(mirrorMode, sizeof(mirrorMode));
	return writePort(flipMode, sizeof(flipMode));
}

ViscaModule::RotateMode ViscaModule::getDisplayRotation()
{
	bool flip = false, mirror = false;
	char flipInq[] = { 0x81, 0x09, 0x04, 0x66, 0xff };
	char mirrorInq[] = { 0x81, 0x09, 0x04, 0x61, 0xff };
	QByteArray ba = readPort(flipInq, sizeof(flipInq), 4);
	if(ba.at(2) == 0x02)
		flip = true;

	ba = readPort(mirrorInq, sizeof(mirrorInq), 4);
	if(ba.at(2) == 0x02)
		mirror = true;

	if(flip && !mirror)
		return REVERSE_MODE;
	else if (!flip && mirror)
		return MIRROR_MODE;
	else if (flip && mirror)
		return VERT_INVER_MODE;
	else
		return ROTATE_OFF;
}

int ViscaModule::setIRCFstat(bool stat)
{
	char ircfMode[] = { 0x81, 0x01, 0x04, 0x01, 0x03, 0xff };
	if (stat == true)
		ircfMode[4] = 0x02;
	return writePort(ircfMode, sizeof(ircfMode));
}

bool ViscaModule::getIRCFstat()
{
	char ircfMode[] = { 0x81, 0x09, 0x04, 0x01, 0xff };
	QByteArray ba = readPort(ircfMode, sizeof(ircfMode), 4);
	if (ba.at(2) == 0x02)
		return true;
	else
		return false;
}

int ViscaModule::setAutoICR(bool stat)
{
	char autoICR[] = { 0x81, 0x01, 0x04, 0x51, 0x03, 0xff };
	if (stat == true)
		autoICR[4] = 0x02;
	return writePort(autoICR, sizeof(autoICR));
}

bool ViscaModule::getAutoICR()
{
	char autoICR[] = { 0x81, 0x09, 0x04, 0x51, 0xff };
	QByteArray ba = readPort(autoICR, sizeof(autoICR), 4);
	if (ba.at(2) == 0x02)
		return true;
	else
		return false;
}

int ViscaModule::setProgramAEmode(ProgramAEmode val)
{
	char progMode[] = { 0x81, 0x01, 0x04, 0x39, 0x00, 0xff };

	if (val == MODE_FULL_AUTO)
		progMode[4] = 0x00;
	else if (val == MODE_MANUAL)
		progMode[4] = 0x03;
	else if (val == MODE_SHUTTER_PRIORITY)
		progMode[4] = 0x0a;
	else if (val == MODE_IRIS_PRIORITY)
		progMode[4] = 0x0b;
	else if (val == MODE_BRIGHT)
		progMode[4] = 0x0d;
	else
		return -EINVAL;
	return writePort(progMode, sizeof(progMode));
}

ViscaModule::ProgramAEmode ViscaModule::getProgramAEmode()
{
	char buf[] = {0x81, 0x09, 0x04, 0x39, 0xff};
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

int ViscaModule::setShutterSpeed(ShutterSpeed val)
{
	char shSpd[] = { 0x81, 0x01, 0x04, 0x4a, 0x00, 0x00, 0x00, 0x00, 0xff };
	shSpd[6] = (val & 0xf0) >> 4;
	shSpd[7] = (val & 0x0f);
	return writePort(shSpd, sizeof(shSpd));
}

ViscaModule::ShutterSpeed ViscaModule::getShutterSpeed()
{
	char shInq[] = { 0x81, 0x09, 0x04, 0x4a,0xff };
	QByteArray ba = readPort(shInq, sizeof(shInq), 7);
	return (ShutterSpeed)((ba.at(4) << 4) | (ba.at(5)));
}

int ViscaModule::setExposureValue(ExposureValue val)
{
	int exVal = (int)val;
	char expVal[] = { 0x81, 0x01, 0x04, 0x4b, 0x00, 0x00, 0x00, 0x00, 0xff };
	if (val == EXP_CLOSE)
		exVal = 0;
	else
		exVal = EXP_UNDEFINED - exVal + 3;
	expVal[6] = (exVal & 0xf0) >> 4;
	expVal[7] = (exVal & 0x0f);
	return writePort(expVal, sizeof(expVal));
}

ViscaModule::ExposureValue ViscaModule::getExposureValue()
{
	char expInq[] = { 0x81, 0x09, 0x04, 0x4b, 0xff };
	QByteArray ba = readPort(expInq, sizeof(expInq), 7);
	int exVal = (ba.at(4) << 4) | (ba.at(5));
	if (exVal == 0)
		return EXP_CLOSE;
	else
		exVal = EXP_UNDEFINED - exVal + 3;
	return (ExposureValue)(exVal);
}

int ViscaModule::setGainValue(GainValue val)
{
	char gainVal[] = { 0x81, 0x01, 0x04, 0x4c, 0x00, 0x00, 0x00, 0x00, 0xff };
	gainVal[6] = (val & 0xf0) >> 4;
	gainVal[7] = (val & 0x0f);
	return writePort(gainVal, sizeof(gainVal));
}

ViscaModule::GainValue ViscaModule::getGainValue()
{
	char gainInq[] = { 0x81, 0x09, 0x04, 0x4c, 0xff };
	QByteArray ba = readPort(gainInq, sizeof(gainInq), 7);
	return (GainValue)((ba.at(4) << 4) | (ba.at(5)));
}

int ViscaModule::setZoomFocusSettings()
{
	const char buf[] = { 0x81, 0x01, 0x04, 0x3f, 0x01, 0x7f, 0xff };
	return writePort(buf, sizeof(buf));
}

int ViscaModule::setNoiseReduction(int val)
{
	if (val > 5)
		return -ENODATA;
	char nred[] = { 0x81, 0x01, 0x04, 0x53, 0x00, 0xff };
	nred[4] = val & 0x0f;
	return writePort(nred, sizeof(nred));
}

int ViscaModule::getNoiseReduction()
{
	char nred[] = { 0x81, 0x09, 0x04, 0x53, 0xff };
	QByteArray ba = readPort(nred, sizeof(nred), 4);
	return (int)(ba.at(2));
}

int ViscaModule::setWDRstat(bool stat)
{
	char buf[] = { 0x81, 0x01, 0x04, 0x3d, 0x03, 0xff }; //off 0x03 mu dokumanda hatali?
	if (stat == true)
		buf[4] = 0x02;
	return writePort(buf, sizeof(buf));
}

bool ViscaModule::getWDRstat()
{
	char wdr[] = { 0x81, 0x09, 0x04, 0x3d, 0xff };
	QByteArray ba = readPort(wdr, sizeof(wdr), 4);
	if(ba.at(2) == 0x02)
		return true;
	else
		return false;
}

int ViscaModule::setGamma(int val)
{
	if (val > 4)
		return -ENODATA;
	char gamma[] = { 0x81, 0x01, 0x04, 0x5b, 0x00, 0xff };
	gamma[4] = val & 0x0f;
	return writePort(gamma, sizeof(gamma));
}

int ViscaModule::getGamma()
{
	char gamma[] = { 0x81, 0x09, 0x04, 0x5b, 0xff };
	QByteArray ba = readPort(gamma, sizeof(gamma), 4);
	return (int)(ba.at(2));
}

int ViscaModule::initializeCamera()
{
	const char lens[] = { 0x81, 0x01, 0x04, 0x19, 0x01, 0xff };
	const char camera[] = { 0x81, 0x01, 0x04, 0x19, 0x03, 0xff };
	writePort(lens, sizeof(lens));
	usleep (2000 * 1000);
	return 	writePort(camera, sizeof(camera));
}

int ViscaModule::setAWBmode(AWBmode mode)
{
	char whBal[] = { 0x81, 0x01, 0x04, 0x35, 0x00, 0xff };
	if((mode == AWB_MANUAL) || (mode == AWB_ATW))
		whBal[4] = (mode + 1) & 0x0f;
	else
		whBal[4] = mode & 0x0f;
	return 	writePort(whBal, sizeof(whBal));
}

ViscaModule::AWBmode ViscaModule::getAWBmode()
{
	char whBal[] = { 0x81, 0x09, 0x04, 0x35, 0xff };
	QByteArray ba = readPort(whBal, sizeof(whBal), 4);
	int val = ba.at(2);
	if (val > 0x03)
		val -= 1;
	return (AWBmode)val;
}

int ViscaModule::setDefogMode(bool stat)
{
	char buf[] = { 0x81, 0x01, 0x04, 0x37, 0x03, 0x00, 0xff };
	if (stat == true)
		buf[4] = 0x02;
	return 	writePort(buf, sizeof(buf));
}

bool ViscaModule::getDefogMode()
{
	char defog[] = { 0x81, 0x09, 0x04, 0x37, 0xff };
	QByteArray ba = readPort(defog, sizeof(defog), 4);
	if(ba.at(2) == 0x02)
		return true;
	else
		return false;}

int ViscaModule::setExposureTarget(int val)
{
	char buf[] = { 0x81, 0x01, 0x04, 0x40, 0x04, 0x00, 0x00, 0xff };
	buf[5] = (val & 0xf0) >> 4;
	buf[6] = (val & 0x0f);
	return 	writePort(buf, sizeof(buf));
}

int ViscaModule::setGainLimit(int upLim, int downLim)
{
	char gainLimit[] = { 0x81, 0x01, 0x04, 0x40, 0x05, 0x00, 0x00, 0x00, 0x00, 0xff };
	gainLimit[5] = (upLim & 0xf0) >> 4;
	gainLimit[6] = (upLim & 0x0f);
	gainLimit[7] = (downLim & 0xf0) >> 4;
	gainLimit[8] = (downLim & 0x0f);
	return 	writePort(gainLimit, sizeof(gainLimit));
}

int ViscaModule::setShutterLimit(int upLim, int downLim)
{
	char shutLim[] = { 0x81, 0x01, 0x04, 0x40, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff };
	shutLim[5] = (upLim & 0xf00) >> 8;
	shutLim[6] = (upLim & 0x0f0) >> 4;
	shutLim[7] = (upLim & 0x00f);
	shutLim[8] = (downLim & 0xf00) >> 8;
	shutLim[9] = (downLim & 0x0f0) >> 4;
	shutLim[10] = (downLim & 0x00f);
	return 	writePort(shutLim, sizeof(shutLim));
}

int ViscaModule::setIrisLimit(int upLim, int downLim)
{
	char irisLim[] = { 0x81, 0x01, 0x04, 0x40, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff };
	irisLim[5] = (upLim & 0xf00) >> 8;
	irisLim[6] = (upLim & 0x0f0) >> 4;
	irisLim[7] = (upLim & 0x00f);
	irisLim[8] = (downLim & 0xf00) >> 8;
	irisLim[9] = (downLim & 0x0f0) >> 4;
	irisLim[10] = (downLim & 0x00f);

	return 	writePort(irisLim, sizeof(irisLim));
}

int ViscaModule::openMenu(bool stat)
{
	char buf[] = { 0x81, 0x01, 0x04, 0xdf, 0x02, 0xff };
	if (stat == true)
		buf[4] = 0x01;
	return 	writePort(buf, sizeof(buf));
}

int ViscaModule::menuGoTo(MenuDirection direct)
{
	char up[] = { 0x81, 0x01, 0x04, 0x07, 0x02, 0xff };
	char down[] = { 0x81, 0x01, 0x04, 0x07, 0x03, 0xff };
	char left[] = { 0x81, 0x01, 0x04, 0x08, 0x02, 0xff };
	char right[] = { 0x81, 0x01, 0x04, 0x08, 0x03, 0xff };

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

int ViscaModule::selectZoomType(bool digiZoom, ZoomType zoomType)
{
	char digZoom[] = { 0x81, 0x01, 0x04, 0x06, 0x03, 0xff };
	char zType[] = { 0x81, 0x01, 0x04, 0x36, 0x00, 0xff };
	if(digiZoom)
		digZoom[4] = 0x02;

	if (zoomType == SEPARATE_MODE)
		zType[4] = 0x01;

	writePort(digZoom, sizeof(digZoom));
	return writePort(zType, sizeof(zType));
}

bool ViscaModule::getDigiZoomState()
{
	char digZoom[] = { 0x81, 0x09, 0x04, 0x06, 0xff };
	QByteArray ba = readPort(digZoom, sizeof(digZoom), 4);
	if(ba.at(2) == 0x02)
		return true;
	else
		return false;
}

ViscaModule::ZoomType ViscaModule::getZoomType()
{
	char zmType[] = { 0x81, 0x09, 0x04, 0x36, 0xff };
	QByteArray ba = readPort(zmType, sizeof(zmType), 4);
	if(ba.at(2) == 0x00)
		return COMBINE_MODE;
	else if (ba.at(2) == 0x01)
		return SEPARATE_MODE;
	else
		return ZM_UNDEFINED;
}

int ViscaModule::applyProgramAEmode(ProgramAEmode ae, ShutterSpeed shVal, ExposureValue exVal, GainValue gain, bool ircf, FocusMode focusMode)
{
	ProgramAEmode prevAE = MODE_INVALID;
	setProgramAEmode(ae);
	if (prevAE != ae)
		usleep(1000*1000);
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

void ViscaModule::timeout()
{
	if (port->bytesAvailable() == 0)
		return;
	static QByteArray buf;
	buf.append(port->readAll());
	while (buf.size()) {
		if (buf.at(0) == 0x90) {
			if (buf.size() < 2)
				break;
			if (buf.at(1) == 0x41 || buf.at(1) == 0x51) {
				if (buf.size() < 3)
					break;
				buf.remove(0, 3);
			} else if (buf.at(1) == 0x07) {
				if (buf.size() < 11)
					break;
				currentZoomPos = buf[6] * 16 * 16 * 16 + buf[7] * 16 * 16 + buf[8] * 16+ buf[9];
			} else
				buf.clear();
		} else
			buf.clear();
	}
}

#if 0
+#include "qextserialport.h"
+static int serTest(const QMap<QString, QString> &args)
+{
+	QextSerialPort *port = new QextSerialPort("/dev/ttyS0", QextSerialPort::Polling);
+	port->setBaudRate(BAUD9600);
+	port->setFlowControl(FLOW_OFF);
+	port->setParity(PAR_NONE);
+	port->setDataBits(DATA_8);
+	port->setStopBits(STOP_1);
+	port->open(QIODevice::ReadWrite);
+
+	QByteArray ba;
+	QStringList list = args["--mes"].split(",");
+	for (int i = 0; i < list.size(); i++)
+		ba.append((uchar)list[i].toInt(0, 16));
+	port->write(ba);
+	port->close();
+
+	return 0;
+}
+
#endif
