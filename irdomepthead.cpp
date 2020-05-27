#include "irdomepthead.h"
#include "debug.h"
#include "ioerrors.h"
#include "ptzptransport.h"

#include <QJsonObject>

#include <unistd.h>

enum Commands {
	C_CUSTOM_PAN_TILT_POS, // 0
	C_CUSTOM_GO_TO_POS,

	/* pelco-d commands */
	C_PAN_LEFT,			   // 1
	C_PAN_RIGHT,		   // 2
	C_TILT_UP,			   // 3
	C_TILT_DOWN,		   // 4
	C_PAN_LEFT_TILT_UP,	   // 5
	C_PAN_LEFT_TILT_DOWN,  // 6
	C_PAN_RIGHT_TILT_UP,   // 7
	C_PAN_RIGHT_TILT_DOWN, // 8
	C_PELCOD_STOP,		   // 9

	C_COUNT,
};

#define MAX_CMD_LEN 16

static uchar protoBytes[C_COUNT][MAX_CMD_LEN] = {
	/* custom commands */
	{0x09, 0x09, 0x3a, 0xff, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c},
	{0x09, 0x09, 0x3a, 0xff, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c},

	/* pelco-d commands */
	{0x07, 0x00, 0xff, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00},
	{0x07, 0x00, 0xff, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00},
	{0x07, 0x00, 0xff, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00},
	{0x07, 0x00, 0xff, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00},
	{0x07, 0x00, 0xff, 0x01, 0x00, 0x0c, 0x00, 0x00, 0x00},
	{0x07, 0x00, 0xff, 0x01, 0x00, 0x14, 0x00, 0x00, 0x00},
	{0x07, 0x00, 0xff, 0x01, 0x00, 0x0a, 0x00, 0x00, 0x00},
	{0x07, 0x00, 0xff, 0x01, 0x00, 0x12, 0x00, 0x00, 0x00},
	{0x07, 0x07, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}, // pelcod_stop
};

static uint checksum(const uchar *cmd, uint lenght)
{
	unsigned int sum = 0;
	for (uint i = 1; i < lenght; i++)
		sum += cmd[i];
	return sum & 0xff;
}

IRDomePTHead::IRDomePTHead()
{
	devvar = BOTAS_DOME;
	maxPatternSpeed = 30;
	syncEnabled = true;
	syncInterval = 20;
	syncTime.start();

	/* [CR] [yca] Hiz kontrolunu PtzpHead API'ye tasimaliyiz */
	speedTableMapping.push_back(0.1);
	speedTableMapping.push_back(0.3);
	speedTableMapping.push_back(1);
	speedTableMapping.push_back(2);
	speedTableMapping.push_back(4);
	speedTableMapping.push_back(7);
	speedTableMapping.push_back(10);
	speedTableMapping.push_back(15);
	speedTableMapping.push_back(20);
	speedTableMapping.push_back(25);
	speedTableMapping.push_back(30);
	speedTableMapping.push_back(35);
}

void IRDomePTHead::fillCapabilities(ptzp::PtzHead *head)
{
	head->add_capabilities(ptzp::PtzHead_Capability_PAN);
	head->add_capabilities(ptzp::PtzHead_Capability_TILT);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_PT);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_JOYSTICK_CONTROL);
}

int IRDomePTHead::panLeft(float speed)
{
	/* [CR] [yca] maximum hizi define haline getirebiliriz... */
	int pspeed = qAbs(speed) * 0x3f;
	return panTilt(C_PAN_LEFT, pspeed, 0);
}

int IRDomePTHead::panRight(float speed)
{
	int pspeed = qAbs(speed) * 0x3f;
	return panTilt(C_PAN_RIGHT, pspeed, 0);
}

int IRDomePTHead::tiltUp(float speed)
{
	int pspeed = qAbs(speed) * 0x3f;
	return panTilt(C_TILT_UP, 0, pspeed);
}

int IRDomePTHead::tiltDown(float speed)
{
	int pspeed = qAbs(speed) * 0x3f;
	return panTilt(C_TILT_DOWN, 0, pspeed);
}

int IRDomePTHead::panTiltAbs(float pan, float tilt)
{
	int pspeed = qAbs(pan) * 0x3f;
	int tspeed = qAbs(tilt) * 0x3f;
	if (pspeed == 0 && tspeed == 0) {
		panTilt(C_PELCOD_STOP, 0, 0);
		return 0;
	}
	if (pan < 0) {
		// left
		if (tilt < 0) {
			// up
			panTilt(C_PAN_LEFT_TILT_UP, pspeed, tspeed);
		} else if (tilt > 0) {
			// down
			panTilt(C_PAN_LEFT_TILT_DOWN, pspeed, tspeed);
		} else
			panTilt(C_PAN_LEFT, pspeed, tspeed);
	} else {
		// right
		if (tilt < 0) {
			// up
			panTilt(C_PAN_RIGHT_TILT_UP, pspeed, tspeed);
		} else if (tilt > 0) {
			// down
			panTilt(C_PAN_RIGHT_TILT_DOWN, pspeed, tspeed);
		} else
			panTilt(C_PAN_RIGHT, pspeed, tspeed);
	}
	return 0;
}

int IRDomePTHead::panTiltDegree(float pan, float tilt)
{
	auto lowerp = std::lower_bound(speedTableMapping.begin(),
								   speedTableMapping.end(), qAbs(pan)) -
				  speedTableMapping.begin();
	auto lowert = std::lower_bound(speedTableMapping.begin(),
								   speedTableMapping.end(), qAbs(tilt)) -
				  speedTableMapping.begin();

	int speed_pan = lowerp * 3 + 2;
	int speed_tilt = lowert * 3 + 2;
	if (pan < 0)
		speed_pan *= -1;
	if (tilt < 0)
		speed_tilt *= -1;
	/* [CR] [yca] Maximum degeri bir define'a baglayabiliriz */
	return panTiltAbs((float)speed_pan / 63.0, (float)speed_tilt / 63.0);
}

int IRDomePTHead::panTiltStop()
{
	return panTilt(C_PELCOD_STOP, 0, 0);
}

static void setCustomCommandParameters(uchar *p, uint cmd, int arg1, int arg2)
{
	if (cmd == C_CUSTOM_GO_TO_POS) {
		p[5] = (arg1 >> 8) & 0xff;
		p[6] = arg1 & 0xff;
		p[7] = (arg2 >> 8) & 0xff;
		p[8] = arg2 & 0xff;
		p[9] = checksum(p + 2, 7);
	}
}

int IRDomePTHead::panTilt(uint cmd, int pspeed, int tspeed)
{
	/*
	 * our dome has one serial channel and it is a slow one,
	 * we disable other readers, zoom reading esspecially,
	 * so that communication is not adversely affected
	 *
	 * TODO: connect this to a config option
	 */
	if (0) {
		if (cmd == C_PELCOD_STOP)
			transport->setQueueFreeCallbackMask(0xffffffff);
		else
			transport->setQueueFreeCallbackMask(0xfffffffe);
		transport->setQueueFreeCallbackMask(0xffffffff);
	}

	unsigned char *p = protoBytes[cmd];
	if (p[2] == 0x3a) {
		setCustomCommandParameters(p, cmd, pspeed, tspeed);
	} else {
		p[4 + 2] = pspeed;
		p[5 + 2] = tspeed;
		p[6 + 2] = checksum(p + 2, 6);
	}
	return transport->send(QByteArray((const char *)p + 2, p[0]));
}

QJsonValue IRDomePTHead::marshallAllRegisters()
{
	QJsonObject json;
	for (int i = 0; i < R_COUNT; i++)
		json.insert(QString("reg%1").arg(i), (int)getRegister(i));
	/* [CR] [yca] IR led seviyesi neden bir register degil */
	json.insert("irLedLevel", irLedLevel);
	/*
	 * [CR] [yca] bu json ayarlari kaydetmek icin uygun bir yer degil. Eger
	 * max pattern hizini ayarlanabilir bir deger yapmak istiyorsak, (ki
	 * su ana kadar gordugum kadari ile oyle bir gereksinim yok) baska bir
	 * yere kaydetmemiz lazim.
	 */
	json.insert("maxPatternSpeed", maxPatternSpeed);
	return json;
}

void IRDomePTHead::unmarshallloadAllRegisters(const QJsonValue &node)
{
	/* [CR] [yca] Fazla bosluk, style dosyamiz bunu neden
	 * yakalamamis bakmak lazim (yca)...
	 */
	QJsonObject root = node.toObject();
	QString key = "reg%1";
	panTilt(C_CUSTOM_GO_TO_POS, root.value(key.arg(R_PAN_ANGLE)).toInt(),
			root.value(key.arg(R_TILT_ANGLE)).toInt());
	/* [CR] [yca] oops fix sleep suresi. Bir kac sakinca soz konusu:
	 *	1. 2 saniye uyuma suresi fazla garantili olmus gibi,
	 *	   gerekli sure test edilip 100/200 milisaniye tolerans
	 *	   ile buraya girilmeli.
	 *	2. Buranin bu sekilde sleep icermesi bu fonskiyonu cagiran
	 *	   katmanlarda soruna neden olabilir mi? (IRDomeDriver)
	 *	   Eger sorun teskil etmiyorsa buraya uygun bir yorum
	 *	   yazmak iyi olur.
	 */
	sleep(2);
	irLedLevel = root.value("irLedLevel").toInt();
	if (irLedLevel != 8)
		setIRLed(0);

	QJsonValue val = root.value("maxPatternSpeed");
	if (!val.isUndefined())
		maxPatternSpeed = val.toInt();
}

float IRDomePTHead::getPanAngle()
{
	/* [CR] [yca] neden 100? */
	return getRegister(R_PAN_ANGLE) / 100.0;
}

float IRDomePTHead::getTiltAngle()
{
	/* [CR] [yca] neden 100? */
	return getRegister(R_TILT_ANGLE) / 100.0;
}

int IRDomePTHead::dataReady(const unsigned char *bytes, int len)
{
	mLogv("%s %d", qPrintable(QByteArray((char*)bytes, len).toHex()), len);

	const unsigned char *p = bytes;

	/* If content is not match with our format,
	 * you should return ENOENT. So other drivers can use it.
	 * We cannot make sure there is a wrong message.
	 * If message have been useless for all driver,
	 * ptzptransport will drop the first byte.
	 */
	if (p[0] != 0x3a)
		return -ENOENT;
	if (len < 9)
		return -EAGAIN;

	/* messages errors */
	IOErr err = IOE_NONE;
	if (p[1] != 0xff || p[2] != 0x82) {
		err = IOE_UNSUPP_SPECIAL;
	} else if (p[7] != checksum(p, 7)) {
		err = IOE_SPECIAL_CHKSUM;
	} else if (p[8] != 0x5c) {
		err = IOE_SPECIAL_LAST;
	}

	if (err != IOE_NONE) {
		setIOError(err);
		return -ENOENT;
	}

	setRegister(R_PAN_ANGLE, (p[3] << 8) + p[4]);
	setRegister(R_TILT_ANGLE, (p[5] << 8) + p[6]);
	pingTimer.restart();
	return 9;
}

QByteArray IRDomePTHead::transportReady()
{
	if (syncEnabled && syncTime.elapsed() > syncInterval) {
		mLogv("Syncing pan and tilt pos");
		unsigned char *p = protoBytes[C_CUSTOM_PAN_TILT_POS];
		p[2 + 7] = checksum(p + 2, 7);
		syncTime.restart();
		return QByteArray((const char *)p + 2, p[0]);
	}
	return QByteArray();
}

int IRDomePTHead::panTiltGoPos(float ppos, float tpos)
{
	panTilt(C_CUSTOM_GO_TO_POS, ppos * 100, tpos * 100);
	return 0;
}

void IRDomePTHead::enableSyncing(bool en)
{
	syncEnabled = en;
	mInfo("Sync status: %d", (int)syncEnabled);
}

void IRDomePTHead::setSyncInterval(int interval)
{
	syncInterval = interval;
	mInfo("Sync interval: %d", syncInterval);
}

float IRDomePTHead::getMaxPatternSpeed() const
{
	/*
	 * [CR] [yca] burada istenen degeri degisken tanimladan dondurebiliriz sanki,
	 * zaten maxPatternSpeed degistirilebilir degil?
	 */
	return float(maxPatternSpeed) / 0x3F;
}

int IRDomePTHead::setIRLed(int led)
{
	uchar cmd[] = {0xff, 0xff, 0x00, 0x9b, 0x00, 0x00, 0x00};
	if (led == 8) {
		cmd[4] = 0x00;
		cmd[5] = 0x00;
	} else if (led == 0) {
		cmd[4] = 0x00;
		cmd[5] = 0x01;
	} else {
		cmd[4] = 0x01;
		cmd[5] = (uint)led - 1;
	}
	cmd[6] = checksum(cmd, 6);
	irLedLevel = led;
	return transport->send((const char *)cmd, sizeof(cmd));
}

int IRDomePTHead::getIRLed()
{
	return irLedLevel;
}

void IRDomePTHead::setDeviceVariant(IRDomePTHead::DeviceVariant v)
{
	devvar = v;
}

QVariant IRDomePTHead::getCapabilityValues(ptzp::PtzHead_Capability c)
{
	return QVariant();
}

void IRDomePTHead::setCapabilityValues(ptzp::PtzHead_Capability c, uint val)
{
	//TODO
}
