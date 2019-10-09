#include "oemmodulehead.h"
#include "debug.h"
#include "ioerrors.h"
#include "ptzptransport.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMutex>
#include <QMutexLocker>

#include <drivers/hardwareoperations.h>

#include <errno.h>
#include <unistd.h>

#define dump()                                                                 \
	for (int i = 0; i < len; i++)                                              \
		qDebug("%s: %d: 0x%x", __func__, i, p[i]);

enum Modules {
	SONY_FCB_CV7500 = 0x0641,
	OEM = 0x045F,
};

enum Commands {
	/* visca commands */
	C_VISCA_SET_EXPOSURE, // 14 //td:nd // exposure_value
	C_VISCA_SET_EXPOSURE_TARGET,
	C_VISCA_SET_GAIN, // 15 //td:nd // gainvalue
	C_VISCA_SET_ZOOM_POS,
	C_VISCA_SET_EXP_COMPMODE,	// 16 //td:nd
	C_VISCA_SET_EXP_COMPVAL,	// 17 //td:nd
	C_VISCA_SET_GAIN_LIM,		// 18 //td:nd
	C_VISCA_SET_SHUTTER,		// 19 //td:nd // getshutterspeed
	C_VISCA_SET_NOISE_REDUCT,	// 22 //td:nd
	C_VISCA_SET_WDRSTAT,		// 23 //td:nd
	C_VISCA_SET_GAMMA,			// 25 //td.nd
	C_VISCA_SET_AWB_MODE,		// 26 //td:nd
	C_VISCA_SET_DEFOG_MODE,		// 27 //td:nd
	C_VISCA_SET_DIGI_ZOOM_STAT, // 28 //td:nd
	C_VISCA_SET_ZOOM_TYPE,		// 29 //td:nd
	C_VISCA_SET_FOCUS,
	C_VISCA_SET_FOCUS_MODE,		 // 30 //td:nd
	C_VISCA_SET_ZOOM_TRIGGER,	 // 31 //td:nd
	C_VISCA_SET_BLC_STATUS,		 // 32 //td:nd
	C_VISCA_SET_IRCF_STATUS,	 // 33 //td:nd
	C_VISCA_SET_AUTO_ICR,		 // 34 //td:nd
	C_VISCA_SET_PROGRAM_AE_MODE, // 35 //td:nd
	C_VISCA_SET_FLIP_MODE,		 // 36
	C_VISCA_SET_MIRROR_MODE,	 // 37
	C_VISCA_SET_ONE_PUSH,
	C_VISCA_ZOOM_IN,   // 11
	C_VISCA_ZOOM_OUT,  // 12
	C_VISCA_ZOOM_STOP, // 13
	C_VISCA_SET_GAIN_LIMIT_OEM,
	C_VISCA_SET_SHUTTER_LIMIT_OEM,
	C_VISCA_SET_IRIS_LIMIT_OEM,
	C_VISCA_GET_ZOOM, // 10

	C_VISCA_GET_EXPOSURE,	  // 14 //td:nd // exposure_value
	C_VISCA_GET_GAIN,		  // 15 //td:nd // gainvalue
	C_VISCA_GET_EXP_COMPMODE, // 16 //td:nd
	C_VISCA_GET_EXP_COMPVAL,  // 17 //td:nd
	C_VISCA_GET_GAIN_LIM,	  // 18 //td:nd
	C_VISCA_GET_SHUTTER,	  // 19 //td:nd // getshutterspeed
	C_VISCA_GET_NOISE_REDUCT, // 22 //td:nd
	C_VISCA_GET_WDRSTAT,	  // 23 //td:nd
	C_VISCA_GET_GAMMA,		  // 25 //td.nd
	C_VISCA_GET_AWB_MODE,	  // 26 //td:nd
	C_VISCA_GET_DEFOG_MODE,	  // 27 //td:nd
	C_VISCA_GET_DEFOG_MODE_OEM,
	C_VISCA_GET_DIGI_ZOOM_STAT,	 // 28 //td:nd
	C_VISCA_GET_ZOOM_TYPE,		 // 29 //td:nd
	C_VISCA_GET_FOCUS_MODE,		 // 30 //td:nd
	C_VISCA_GET_ZOOM_TRIGGER,	 // 31 //td:nd
	C_VISCA_GET_BLC_STATUS,		 // 32 //td:nd
	C_VISCA_GET_IRCF_STATUS,	 // 33 //td:nd
	C_VISCA_GET_AUTO_ICR,		 // 34 //td:nd
	C_VISCA_GET_PROGRAM_AE_MODE, // 35 //td:nd
	C_VISCA_GET_FLIP_MODE,		 // 36
	C_VISCA_GET_MIRROR_MODE,	 // 37

	C_VISCA_GET_VERSION,
	C_VISCA_GET_GAIN_LIMIT_OEM,
	C_VISCA_GET_SHUTTER_LIMIT_OEM,
	C_VISCA_GET_IRIS_LIMIT_OEM,

	C_COUNT,
};

/* [CR] [yca] define'lar genelde include'lardan sonra oluyor */
#define MAX_CMD_LEN 16

/*
 * [CR] [yca] yaklasik 500 msec/1 saniyede bir IRCF okumak cok degil mi?
 * Bunun yerine bir timer ile birlikte IRCF okunamaz mi? Daha da az surecek
 * bir sekilde? 10 saniyede, 20, 30 saniyede 1 falan?
*/
const Commands queryPatternList[] = {
	C_VISCA_GET_ZOOM,
	C_VISCA_GET_ZOOM,
	C_VISCA_GET_ZOOM,
	C_VISCA_GET_ZOOM,
	C_VISCA_GET_IRCF_STATUS,
};

static unsigned char protoBytes[C_COUNT][MAX_CMD_LEN] = {
	/* visca commands */
	{0x09, 0x00, 0x81, 0x01, 0x04, 0x4b, 0x00, 0x00, 0x00, 0x0f,
	 0xff}, // set_exposure_value
	{0x08, 0x00, 0x81, 0x01, 0x04, 0x40, 0x04, 0x00, 0x00,
	 0xff}, // set exposure target
	{0x09, 0x00, 0x81, 0x01, 0x04, 0x4c, 0x00, 0x00, 0xf0, 0x0f,
	 0xff}, // set gain value
	{0x09, 0x00, 0x81, 0x01, 0x04, 0x47, 0x00, 0x00, 0x00, 0x00,
	 0xff},											  // set zoom pos
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x3E, 0x00, 0xff}, // set exp comp mode
	{0x09, 0x00, 0x81, 0x01, 0x04, 0x4E, 0x00, 0x00, 0xf0, 0x0f,
	 0xff},											  // set exp comp val
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x2c, 0x0f, 0xff}, // set gain limit
	{0x09, 0x00, 0x81, 0x01, 0x04, 0x4a, 0x00, 0x00, 0x00, 0x0f,
	 0xff},													// set shutter speed
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x53, 0x0f, 0xff},		// set noise reduct
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x3d, 0x00, 0xff},		// set wdr stat
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x5b, 0x0f, 0xff},		// set gamma
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x35, 0x00, 0xff},		// set awb mode
	{0x07, 0x00, 0x81, 0x01, 0x04, 0x37, 0x00, 0x00, 0xff}, // set defog mode
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x06, 0x00, 0xff},		// digizoom
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x36, 0x00, 0xff},		// zoom type
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x08, 0x00, 0xff}, // set focus far-near
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x38, 0x00, 0xff}, // set focus mode
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x57, 0x00, 0xFF}, // set zoom trigger
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x33, 0x00, 0xff}, // set BLC stat
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x01, 0x00, 0xff}, // set IRCF stst
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x51, 0x00, 0xff}, // set Auto ICR
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x39, 0x00, 0xff}, // set program AE mode
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x66, 0x00, 0xff}, // set flip mode
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x61, 0x00, 0xff}, // set Mirror mode
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x18, 0x01, 0xff}, // set One Push
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x07, 0x20, 0xff}, // C_VISCA_ZOOM_IN
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x07, 0x30, 0xff}, // C_VISCA_ZOOM_OUT
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x07, 0x00, 0xff}, // C_VISCA_ZOOM_STOP
	{0x10, 0x00, 0x81, 0x01, 0x04, 0x40, 0x05, 0x00, 0x00, 0x00, 0x00,
	 0xff}, // C_VISCA_SET_GAIN_LIMIT_OEM
	{0x12, 0x00, 0x81, 0x01, 0x04, 0x40, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0xff}, // C_VISCA_SET_SHUTTER_LIMIT_OEM
	{0x12, 0x00, 0x81, 0x01, 0x04, 0x40, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0xff}, // C_VISCA_SET_IRIS_LIMIT_OEM

	{0x05, 0x07, 0x81, 0x09, 0x04, 0x47, 0xff, 0x00}, // C_VISCA_GET_ZOOM

	{0x05, 0x07, 0x81, 0x09, 0x04, 0x4b, 0xff, 0x00}, // get_exposure_value
	{0x05, 0x07, 0x81, 0x09, 0x04, 0x4c, 0xff, 0x00}, // get_gain_value
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x3e, 0xff, 0x01}, // getExpCompMode
	{0x05, 0x07, 0x81, 0x09, 0x04, 0x4e, 0xff, 0x01}, // getExpCompval
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x2c, 0xff, 0x01}, // getGainLimit
	{0x05, 0x07, 0x81, 0x09, 0x04, 0x4a, 0xff, 0x00}, // getsutterSpeed 19
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x53, 0xff, 0x00}, // getNoiseReduct
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x3d, 0xff, 0x00}, // getWDRstat
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x5b, 0xff, 0x00}, // getGamma
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x35, 0xff, 0x00}, // getAWBmode
	{0x05, 0x05, 0x81, 0x09, 0x04, 0x37, 0xff, 0x01}, // getDefogMode
	{0x04, 0x05, 0x81, 0x09, 0x04, 0x37, 0xff,
	 0x02}, // C_VISCA_GET_DEFOG_MODE_OEM
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x06, 0xff, 0x00}, // getDigiZoomStat
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x36, 0xff, 0x00}, // getZoomType
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x38, 0xff, 0x00}, // getFocusMode
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x57, 0xff, 0x00}, // getZoomTrigger
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x33, 0xff, 0x00}, // getBLCstatus
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x01, 0xff, 0x00}, // getIRCFstat
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x51, 0xff, 0x00}, // getAutoICR
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x39, 0xff, 0x00}, // getProgramAEmode
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x66, 0xff, 0x00}, // getFlipMode
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x61, 0xff, 0x00}, // getMirrorMode

	{0x05, 0x0A, 0x81, 0x09, 0x00, 0x02, 0xff, 0x00}, // C_VISCA_GET_VERSION
	{0x06, 0x07, 0x81, 0x09, 0x04, 0x40, 0x05, 0xff,
	 0x02}, // C_VISCA_GET_GAIN_LIMIT_OEM
	{0x06, 0x09, 0x81, 0x09, 0x04, 0x40, 0x06, 0xff,
	 0x02}, // C_VISCA_GET_SHUTTER_LIMIT_OEM
	{0x06, 0x09, 0x81, 0x09, 0x04, 0x40, 0x07, 0xff,
	 0x02}, // C_VISCA_GET_IRIS_LIMIT_OEM

};

class CommandHistory
{
public:
	void add(Commands c)
	{
		QMutexLocker l(&m);
		commands << c;
	}

	Commands takeFirst()
	{
		QMutexLocker l(&m);
		if (commands.size())
			return commands.takeFirst();
		return C_COUNT;
	}

	Commands peek()
	{
		QMutexLocker l(&m);
		if (commands.size())
			return commands.first();
		return C_COUNT;
	}

protected:
	QMutex m;
	QList<Commands> commands;
};

OemModuleHead::OemModuleHead()
{
	oldZoomValue = 0;
	/* [CR] [yca] Memory leak... */
	hist = new CommandHistory;
	nextSync = C_COUNT;
	syncEnabled = true;
	syncInterval = 40;
	syncTime.start();
#ifdef HAVE_PTZP_GRPC_API
	settings = {
		{"exposure_value", {C_VISCA_SET_EXPOSURE, R_EXPOSURE_VALUE}},
		{"gain_value", {C_VISCA_SET_GAIN, R_GAIN_VALUE}},
		{"exp_comp_mode", {C_VISCA_SET_EXP_COMPMODE, R_EXP_COMPMODE}},
		{"exp_comp_val", {C_VISCA_SET_EXP_COMPVAL, R_EXP_COMPVAL}},
		{"gain_limit", {C_VISCA_SET_GAIN_LIM, R_GAIN_LIM}},
		{"shutter", {C_VISCA_SET_SHUTTER, R_SHUTTER}},
		{"noise_reduct", {C_VISCA_SET_NOISE_REDUCT, R_NOISE_REDUCT}},
		{"wdr_stat", {C_VISCA_SET_WDRSTAT, R_WDRSTAT}},
		{"gamma", {C_VISCA_SET_GAMMA, R_GAMMA}},
		{"awb_mode", {C_VISCA_SET_AWB_MODE, R_AWB_MODE}},
		{"defog_mode", {C_VISCA_SET_DEFOG_MODE, R_DEFOG_MODE}},
		{"digi_zoom_on", {C_VISCA_SET_DIGI_ZOOM_STAT, R_DIGI_ZOOM_STAT}},
		{"digi_zoom_off", {C_VISCA_SET_DIGI_ZOOM_STAT, R_DIGI_ZOOM_STAT}},
		{"zoom_type", {C_VISCA_SET_ZOOM_TYPE, R_ZOOM_TYPE}},
		{"auto_focus_on", {C_VISCA_SET_FOCUS_MODE, R_FOCUS_MODE}},
		{"auto_focus_off", {C_VISCA_SET_FOCUS_MODE, R_FOCUS_MODE}},
		{"zoom_trigger", {C_VISCA_SET_ZOOM_TRIGGER, R_ZOOM_TRIGGER}},
		{"blc_status", {C_VISCA_SET_BLC_STATUS, R_BLC_STATUS}},
		{"ircf_status", {C_VISCA_SET_IRCF_STATUS, R_IRCF_STATUS}},
		{"auto_icr", {C_VISCA_SET_AUTO_ICR, R_AUTO_ICR}},
		{"program_ae_mode", {C_VISCA_SET_PROGRAM_AE_MODE, R_PROGRAM_AE_MODE}},
		{"flip", {C_VISCA_SET_FLIP_MODE, R_FLIP}},
		{"mirror", {C_VISCA_SET_MIRROR_MODE, R_MIRROR}},
		{"one_push_af", {C_VISCA_SET_ONE_PUSH, NULL}},
		{"display_rotation", {NULL, R_DISPLAY_ROT}},
		{"digi_zoom_pos", {NULL, R_DIGI_ZOOM_POS}},
		{"optic_zoom_pos", {NULL, R_OPTIC_ZOOM_POS}},
		{"focus", {C_VISCA_SET_FOCUS, NULL}},
		{"cam_model", {NULL, R_VISCA_MODUL_ID}},
	};
#endif
	setRegister(R_VISCA_MODUL_ID, 0);
}

int OemModuleHead::getCapabilities()
{
	return PtzpHead::CAP_ZOOM | PtzpHead::CAP_ADVANCED;
}

int OemModuleHead::syncRegisters()
{
	if (!transport)
		return -ENOENT;
	nextSync = C_VISCA_GET_EXPOSURE;
	return syncNext();
}

int OemModuleHead::getHeadStatus()
{
	if (nextSync == C_COUNT) {
		if (getRegister(R_VISCA_MODUL_ID) == 0)
			return ST_ERROR;
		else
			return ST_NORMAL;
	}
	return ST_SYNCING;
}

int OemModuleHead::startZoomIn(int speed)
{
	zoomSpeed = speed;
	unsigned char *p = protoBytes[C_VISCA_ZOOM_IN];
	p[4 + 2] = 0x20 + speed;
	return transport->send((const char *)p + 2, p[0]);
}

int OemModuleHead::startZoomOut(int speed)
{
	zoomSpeed = speed;
	unsigned char *p = protoBytes[C_VISCA_ZOOM_OUT];
	p[4 + 2] = 0x30 + speed;
	return transport->send((const char *)p + 2, p[0]);
}

int OemModuleHead::stopZoom()
{
	const unsigned char *p = protoBytes[C_VISCA_ZOOM_STOP];
	return transport->send((const char *)p + 2, p[0]);
}

int OemModuleHead::getZoom()
{
	return getRegister(R_ZOOM_POS);
}

int OemModuleHead::setZoom(uint pos)
{
	unsigned char *p = protoBytes[C_VISCA_SET_ZOOM_POS];
	p[4 + 2] = (pos & 0XF000) >> 12;
	p[4 + 3] = (pos & 0XF00) >> 8;
	p[4 + 4] = (pos & 0XF0) >> 4;
	p[4 + 5] = pos & 0XF;
	return transport->send((const char *)p + 2, p[0]);
}

int OemModuleHead::focusIn(int speed)
{
	unsigned char *p = protoBytes[C_VISCA_SET_FOCUS];
	p[4 + 2] = 0x20 + speed;
	return transport->send((const char *)p + 2, p[0]);
}

int OemModuleHead::focusOut(int speed)
{
	unsigned char *p = protoBytes[C_VISCA_SET_FOCUS];
	p[4 + 2] = 0x30 + speed;
	return transport->send((const char *)p + 2, p[0]);
}

int OemModuleHead::focusStop()
{
	unsigned char *p = protoBytes[C_VISCA_SET_FOCUS];
	p[4 + 2] = 0x00;
	return transport->send((const char *)p + 2, p[0]);
}

void OemModuleHead::enableSyncing(bool en)
{
	/* [CR] [yca] syncEnabled'a cok rastliyorum, bunu PtzpHead'e
	 * tasimak epey bir kafa icin faydali olacak sanirim?
	 */
	syncEnabled = en;
	mInfo("Sync status: %d", (int)syncEnabled);
}

void OemModuleHead::setSyncInterval(int interval)
{
	/* [CR] [yca] syncEnabled gibi... */
	syncInterval = interval;
	mInfo("Sync interval: %d", syncInterval);
}

int OemModuleHead::syncNext()
{
	unsigned char const *p = protoBytes[nextSync];
	char modulFlag = p[2 + p[0]];
	/* [CR] [yca] [fo] Bu while burada tehlikeli bunun uzerinde
	 * konusup duzeltebilir miyiz?
	 */
	while (modulFlag > 0) {
		if (getRegister(R_VISCA_MODUL_ID) == SONY_FCB_CV7500 && modulFlag == 1)
			break;
		else if (getRegister(R_VISCA_MODUL_ID) == OEM && modulFlag == 2)
			break;
		else {
			if (++nextSync == C_COUNT) {
				fDebug("Visca register syncing completed, activating auto-sync "
					   "%d %d",
					   nextSync, C_COUNT);
				return 0;
			}
			p = protoBytes[nextSync];
			modulFlag = p[2 + p[0]];
		}
	}
	hist->add((Commands)nextSync);
	return transport->send((const char *)p + 2, p[0]);
}

int OemModuleHead::dataReady(const unsigned char *bytes, int len)
{
	for (int i = 0; i < len; i++)
		mLogv("%d", bytes[i]);
	const unsigned char *p = bytes;
	if (p[0] != 0x90)
		return -ENOENT;
	/* sendcmd can be C_COUNT, beware! */
	Commands sendcmd = hist->peek();
	if (sendcmd == C_COUNT) {
		setIOError(IOE_NO_LAST_WRITTEN);
		return -ENOENT;
	}
	const unsigned char *sptr = protoBytes[sendcmd];
	int expected = sptr[1];

	// fDebug("%s %d %d", qPrintable(QByteArray((char*)bytes, len).toHex()),
	// len, expected);

	if (p[1] == 0x41 || p[1] == 0x51) {
		mLogv("acknowledge");
		return 3;
	} else if (p[1] == 0x61 && len == 4 && p[3] == 0xff) {
		mLogv("error message: &d", p[2]);
		pingTimer.restart();
		hist->takeFirst();
		syncNext();
		return 4;
	} else if (expected == 0x00) {
		for (int i = 0; i < len; i++) {
			mInfo("%s: %d: 0x%x", __func__, i, bytes[i]);
			if (bytes[i] == 0xff)
				return i + 1;
		}
		return len;
	}
	if (len < expected)
		return -EAGAIN;
	if (sptr[2] != 0x81) {
		setIOError(IOE_VISCA_INVALID_ADDR);
		return expected;
	}
	if (p[expected - 1] != 0xff) {
		setIOError(IOE_VISCA_LAST);
		return expected;
	}

	pingTimer.restart();
	/* register sync support */
	if (nextSync != C_COUNT) {
		/* we are in sync mode, let's sync next */
		mInfo("Next sync property: %d", nextSync);
		if (++nextSync == C_COUNT) {
			fDebug("Visca register syncing completed, activating auto-sync");
		} else
			syncNext();
	}
	hist->takeFirst();

	if (sendcmd == C_VISCA_GET_EXPOSURE) {
		uint exVal = ((p[4] << 4) | p[5]);
		if (exVal == 0) // Açıklama için setProperty'de exposure kısmına bakın!
			exVal = 13;
		else
			exVal = 17 - exVal;
		mInfo("Exposure value synced");
		setRegister(R_EXPOSURE_VALUE, exVal);
	} else if (sendcmd == C_VISCA_GET_GAIN) {
		mInfo("Gain Value synced");
		setRegister(R_GAIN_VALUE, (p[4] << 4 | p[5]));
	} else if (sendcmd == C_VISCA_GET_EXP_COMPMODE) {
		mInfo("Ex Comp Mode synced");
		setRegister(R_EXP_COMPMODE, (p[2] == 0x02));
	} else if (sendcmd == C_VISCA_GET_EXP_COMPVAL) {
		mInfo("Ex Comp Value synced");
		setRegister(R_EXP_COMPVAL, ((p[4] << 4) + p[5]));
	} else if (sendcmd == C_VISCA_GET_GAIN_LIM) {
		mInfo("Gain Limit synced");
		setRegister(R_GAIN_LIM, (p[2] - 4));
	} else if (sendcmd == C_VISCA_GET_SHUTTER) {
		mInfo("Shutter synced");
		setRegister(R_SHUTTER, (p[4] << 4 | p[5]));
	} else if (sendcmd == C_VISCA_GET_NOISE_REDUCT) {
		mInfo("Noise Reduct synced");
		setRegister(R_NOISE_REDUCT, p[2]);
	} else if (sendcmd == C_VISCA_GET_WDRSTAT) {
		mInfo("Wdr Status synced");
		setRegister(R_WDRSTAT, (p[2] == 0x02));
	} else if (sendcmd == C_VISCA_GET_GAMMA) {
		mInfo("Gamma synced");
		setRegister(R_GAMMA, p[2]);
	} else if (sendcmd == C_VISCA_GET_AWB_MODE) {
		mInfo("Awb Mode synced");
		char val = p[2];
		if (val > 0x03)
			val -= 1;
		setRegister(R_AWB_MODE, val);
	} else if (sendcmd == C_VISCA_GET_DEFOG_MODE) {
		mInfo("Defog Mode synced");
		setRegister(R_DEFOG_MODE, (p[2] == 0x02));
	} else if (sendcmd == C_VISCA_GET_DIGI_ZOOM_STAT) {
		mInfo("Digi Zoom Status synced");
		setRegister(R_DIGI_ZOOM_STAT, (p[2] == 0x02) ? true : false);
	} else if (sendcmd == C_VISCA_GET_ZOOM_TYPE) {
		mInfo("Zoom Type synced");
		setRegister(R_ZOOM_TYPE, p[2]);
	} else if (sendcmd == C_VISCA_GET_FOCUS_MODE) {
		mInfo("Focus mode synced");
		setRegister(R_FOCUS_MODE, p[2]);
	} else if (sendcmd == C_VISCA_GET_ZOOM_TRIGGER) {
		mInfo("Zoom Trigger synced");
		if (p[2] == 0x02)
			setRegister(R_ZOOM_TRIGGER, 1);
		else
			setRegister(R_ZOOM_TRIGGER, 0);
	} else if (sendcmd == C_VISCA_GET_BLC_STATUS) {
		mInfo("BLC status synced");
		setRegister(R_BLC_STATUS, (p[2] == 0x02) ? true : false);
	} else if (sendcmd == C_VISCA_GET_IRCF_STATUS) {
		mInfo("Ircf Status synced");
		setRegister(R_IRCF_STATUS, (p[2] == 0x02) ? true : false);
	} else if (sendcmd == C_VISCA_GET_AUTO_ICR) {
		mInfo("Auto Icr synced");
		setRegister(R_AUTO_ICR, (p[2] == 0x02) ? true : false);
	} else if (sendcmd == C_VISCA_GET_PROGRAM_AE_MODE) {
		mInfo("Program AE Mode synced");
		setRegister(R_PROGRAM_AE_MODE, p[2]);
	} else if (sendcmd == C_VISCA_GET_ZOOM) {
		if (p[1] != 0x50 || p[6] != 0xFF) {
			mInfo("Zoom response err: wrong mesg[%s]",
				  QByteArray((char *)p, len).toHex().data());
			return expected;
		}

		uint regValue = getRegister(R_ZOOM_POS);
		uint value = ((p[2] & 0x0F) << 12) | ((p[3] & 0x0F) << 8) |
					 ((p[4] & 0x0F) << 4) | ((p[5] & 0x0F) << 0);
		if (value > 0x7AC0) {
			mInfo("Zoom response err: big value [%d]", value);
			return expected;
		}

		if (regValue * 1.1 < value || value < regValue * 0.9) {
			if (oldZoomValue * 1.1 < value || value < oldZoomValue * 0.9) {
				oldZoomValue = value;
				mInfo("Zoom response err: value peak [%d]", value);
				return expected;
			}
		}
		oldZoomValue = value;

		mLogv("Zoom Position synced");
		setRegister(R_ZOOM_POS, value);
		if (value >= 16384) {
			mLogv("Optic and digital zoom position synced");
			setRegister(R_DIGI_ZOOM_POS, getRegister(R_ZOOM_POS) - 16384);
			setRegister(R_OPTIC_ZOOM_POS, 16384);
		} else if (value < 16384) {
			mLogv("Optic and digital zoom position synced");
			setRegister(R_OPTIC_ZOOM_POS, getRegister(R_ZOOM_POS));
			setRegister(R_DIGI_ZOOM_POS, 0);
		}
	} else if (sendcmd == C_VISCA_GET_FLIP_MODE) {
		mInfo("Flip Status synced");
		setRegister(R_FLIP, (p[2] == 0x02) ? true : false);
	} else if (sendcmd == C_VISCA_GET_MIRROR_MODE) {
		mInfo("Mirror status synced");
		setRegister(R_MIRROR, (p[2] == 0x02) ? true : false);
	} else if (sendcmd == C_VISCA_GET_VERSION) {
		mInfo("Version synced");
		if (p[1] == 0x50 && p[2] == 0x00 && p[3] == 0x20) {
			setRegister(R_VISCA_MODUL_ID, (uint(p[4]) << 8) + p[5]);
		}
	} else if (sendcmd == C_VISCA_GET_GAIN_LIMIT_OEM) {
		mInfo("Oem Gain Limit synced");
		if (p[1] != 50) {
			setRegister(R_TOP_GAIN, (p[2] << 4) + p[3]);
			setRegister(R_BOT_GAIN, (p[4] << 4) + p[5]);
		}
	} else if (sendcmd == C_VISCA_GET_SHUTTER_LIMIT_OEM) {
		mInfo("Oem Shutter Limit synced");
		if (p[1] != 50) {
			setRegister(R_TOP_SHUTTER, (uint(p[2]) << 8) + (p[3] << 4) + p[4]);
			setRegister(R_BOT_SHUTTER, (uint(p[5]) << 8) + (p[6] << 4) + p[7]);
		}
	} else if (sendcmd == C_VISCA_GET_IRIS_LIMIT_OEM) {
		mInfo("Oem Iris Limit synced");
		if (p[1] != 50) {
			setRegister(R_TOP_IRIS, (uint(p[2]) << 8) + (p[3] << 4) + p[4]);
			setRegister(R_BOT_IRIS, (uint(p[5]) << 8) + (p[6] << 4) + p[7]);
		}
	}

	if (getRegister(R_FLIP) == 1 && getRegister(R_MIRROR) == 0) {
		setRegister(R_DISPLAY_ROT, 0);
	} else if (getRegister(R_FLIP) == 0 && getRegister(R_MIRROR) == 1) {
		setRegister(R_DISPLAY_ROT, 1);
	} else if (getRegister(R_FLIP) == 1 && getRegister(R_MIRROR) == 1) {
		setRegister(R_DISPLAY_ROT, 2);
	} else if (getRegister(R_FLIP) == 0 && getRegister(R_MIRROR) == 0) {
		setRegister(R_DISPLAY_ROT, 3);
		getRegister(R_DISPLAY_ROT);
	}
	return expected;
}

QByteArray OemModuleHead::transportReady()
{
	if (syncEnabled && syncTime.elapsed() > syncInterval) {
		mLogv("Syncing zoom positon");
		syncTime.restart();
		/* [CR] [yca] ooops, static degisken :( */
		static uint ind = 0;
		Commands q = queryPatternList[ind++];
		if (ind >= sizeof(queryPatternList) / sizeof(queryPatternList[0]))
			ind = 0;
		const unsigned char *p = protoBytes[q];
		hist->add(q);
		return QByteArray((const char *)p + 2, p[0]);
	}
	return QByteArray();
}

QJsonValue OemModuleHead::marshallAllRegisters()
{
	QJsonObject json;
	for (int i = 0; i < R_COUNT; i++)
		json.insert(QString("reg%1").arg(i), (int)getRegister(i));

	json.insert(QString("deviceDefiniton"), (QString)deviceDefinition);
	return json;
}

void OemModuleHead::unmarshallloadAllRegisters(const QJsonValue &node)
{
	QJsonObject root = node.toObject();
	QString key = "reg%1";
	/*
	 * according to our command table visca set commands doesn't get any
	 * response from module, in this case ordinary sleep should work here
	 */
	int sleepDur = 1000 * 100;
	setProperty(C_VISCA_SET_IRCF_STATUS,
				root.value(key.arg(R_IRCF_STATUS)).toInt());
	usleep(sleepDur);
	setProperty(C_VISCA_SET_NOISE_REDUCT,
				root.value(key.arg(R_NOISE_REDUCT)).toInt());
	usleep(sleepDur);
	setProperty(C_VISCA_SET_WDRSTAT, root.value(key.arg(R_WDRSTAT)).toInt());
	usleep(sleepDur);
	setProperty(C_VISCA_SET_AWB_MODE, root.value(key.arg(R_AWB_MODE)).toInt());
	usleep(sleepDur);
	setProperty(C_VISCA_SET_GAMMA, root.value(key.arg(R_GAMMA)).toInt());
	usleep(sleepDur);
	setProperty(C_VISCA_SET_BLC_STATUS,
				root.value(key.arg(R_BLC_STATUS)).toInt());
	usleep(sleepDur);
	setProperty(C_VISCA_SET_DEFOG_MODE,
				root.value(key.arg(R_DEFOG_MODE)).toInt());
	usleep(sleepDur);

	setProperty(C_VISCA_SET_ZOOM_TYPE,
				root.value(key.arg(R_ZOOM_TYPE)).toInt());
	usleep(sleepDur);
	setProperty(C_VISCA_SET_DIGI_ZOOM_STAT,
				root.value(key.arg(R_DIGI_ZOOM_STAT)).toInt());
	usleep(sleepDur);

	if (getRegister(R_VISCA_MODUL_ID) == SONY_FCB_CV7500) {
		// SONY Limits
		setProperty(C_VISCA_SET_GAIN_LIM,
					root.value(key.arg(R_GAIN_LIM)).toInt());
		usleep(sleepDur);
		setProperty(C_VISCA_SET_EXP_COMPMODE,
					root.value(key.arg(R_EXP_COMPMODE)).toInt());
		usleep(sleepDur);
		setProperty(C_VISCA_SET_EXP_COMPVAL,
					root.value(key.arg(R_EXP_COMPVAL)).toInt());
		usleep(sleepDur);
	} else if (getRegister(R_VISCA_MODUL_ID) != OEM) {
		// OEM Limits
		setProperty(C_VISCA_SET_EXPOSURE_TARGET,
					root.value(key.arg(R_EXPOSURE_TARGET)).toInt());
		usleep(sleepDur);
		uint gainLimit = (root.value(key.arg(R_TOP_GAIN)).toInt() << 8) +
						 root.value(key.arg(R_BOT_GAIN)).toInt();
		if (gainLimit == 0) {
			gainLimit = 0x7000; // 112,0
		}
		setProperty(C_VISCA_GET_GAIN_LIMIT_OEM, gainLimit);
		uint shutterLimit = (root.value(key.arg(R_TOP_SHUTTER)).toInt() << 12) +
							root.value(key.arg(R_BOT_SHUTTER)).toInt();
		if (shutterLimit == 0) {
			shutterLimit = 0x45f000; // 1119,0
		}
		setProperty(C_VISCA_GET_SHUTTER_LIMIT_OEM, shutterLimit);
		uint irisLimit = (root.value(key.arg(R_TOP_IRIS)).toInt() << 12) +
						 root.value(key.arg(R_BOT_IRIS)).toInt();
		if (shutterLimit == 0) {
			shutterLimit = 0x2BC000; // 700,0
		}
		setProperty(C_VISCA_GET_IRIS_LIMIT_OEM, irisLimit);
	}

	uint ae_mode = root.value(key.arg(R_PROGRAM_AE_MODE)).toInt();
	setProperty(C_VISCA_SET_PROGRAM_AE_MODE, ae_mode);
	sleep(1);

	if (ae_mode == 0x0a) {
		// Shutter priority
		setProperty(C_VISCA_SET_SHUTTER,
					root.value(key.arg(R_SHUTTER)).toInt());
		usleep(sleepDur);
	} else if (ae_mode == 0x0b) {
		// Iris priority
		setProperty(C_VISCA_SET_EXPOSURE,
					root.value(key.arg(R_EXPOSURE_VALUE)).toInt());
		usleep(sleepDur);
	} else if (ae_mode == 0x03) {
		// Manuel
		setProperty(C_VISCA_SET_SHUTTER,
					root.value(key.arg(R_SHUTTER)).toInt());
		usleep(sleepDur);
		setProperty(C_VISCA_SET_EXPOSURE,
					root.value(key.arg(R_EXPOSURE_VALUE)).toInt());
		usleep(sleepDur);
		setProperty(C_VISCA_SET_GAIN,
					root.value(key.arg(R_GAIN_VALUE)).toInt());
		usleep(sleepDur);
	}

	setProperty(C_VISCA_SET_AUTO_ICR, root.value(key.arg(R_AUTO_ICR)).toInt());
	usleep(sleepDur);
	setProperty(C_VISCA_SET_FOCUS_MODE,
				root.value(key.arg(R_FOCUS_MODE)).toInt());
	usleep(sleepDur);
	setProperty(C_VISCA_SET_ZOOM_TRIGGER,
				root.value(key.arg(R_ZOOM_TRIGGER)).toInt());
	usleep(sleepDur);

	setProperty(C_VISCA_SET_ZOOM_TRIGGER,
				root.value(key.arg(R_ZOOM_TRIGGER)).toInt());
	usleep(sleepDur);
	setProperty(C_VISCA_SET_FLIP_MODE, root.value(key.arg(R_FLIP)).toInt());
	usleep(sleepDur);
	setProperty(C_VISCA_SET_MIRROR_MODE, root.value(key.arg(R_MIRROR)).toInt());
	usleep(sleepDur);

	setZoom(root.value(key.arg(R_ZOOM_POS)).toInt());
	usleep(sleepDur);
	deviceDefinition = root.value("deviceDefiniton").toString();
}

void OemModuleHead::setProperty(uint r, uint x)
{
	mInfo("Set Property %d , value: %d", r, x);
	if (r == C_VISCA_SET_EXPOSURE) {
		unsigned char *p = protoBytes[C_VISCA_SET_EXPOSURE];
		uint val;
		/***
		 * FCB-EV7500 dökümanında(Visca datasheet) sayfa 9'da bulunan Iris
		 * priority bölümünde bulunan tabloya göre exposure (iris priorty)
		 * değerleri 0 ile 17 arasında değişmekte fakat 14 adet bilgi
		 * bulunmaktadır. Bu değerlerin 0-13 arasına çekilip web sayfası
		 * tarafındada düzgün bir görüntüleme sağmabilmesi açısından aşağıdaki
		 * işlem uygulanmıştır.
		 */
		if (x == 13)
			val = 0;
		else
			val = 17 - x;
		p[6 + 2] = val >> 4;
		p[7 + 2] = val & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_EXPOSURE_VALUE, (int)x);
	} else if (r == C_VISCA_SET_NOISE_REDUCT) {
		unsigned char *p = protoBytes[C_VISCA_SET_NOISE_REDUCT];
		p[4 + 2] = x & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_NOISE_REDUCT, (int)x);
	} else if (r == C_VISCA_SET_GAIN) {
		unsigned char *p = protoBytes[C_VISCA_SET_GAIN];
		p[6 + 2] = (x & 0xf0) >> 4;
		p[7 + 2] = x & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_GAIN_VALUE, (int)x);
	} else if (r == C_VISCA_SET_EXP_COMPMODE) {
		unsigned char *p = protoBytes[C_VISCA_SET_EXP_COMPMODE];
		p[4 + 2] = x ? 0x02 : 0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_EXP_COMPMODE, (int)x);
	} else if (r == C_VISCA_SET_EXP_COMPVAL) {
		unsigned char *p = protoBytes[C_VISCA_SET_EXP_COMPVAL];
		p[6 + 2] = (x & 0xf0) >> 4;
		p[7 + 2] = x & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_EXP_COMPVAL, (int)x);
	} else if (r == C_VISCA_SET_GAIN_LIM) {
		unsigned char *p = protoBytes[C_VISCA_SET_GAIN_LIM];
		p[4 + 2] = (x + 4) & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_GAIN_LIM, (int)x);
	} else if (r == C_VISCA_SET_SHUTTER) {
		unsigned char *p = protoBytes[C_VISCA_SET_SHUTTER];
		p[6 + 2] = x >> 4;
		p[7 + 2] = x & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_SHUTTER, (int)x);
	} else if (r == C_VISCA_SET_WDRSTAT) {
		unsigned char *p = protoBytes[C_VISCA_SET_WDRSTAT];
		p[4 + 2] = x ? 0x02 : 0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_WDRSTAT, (int)x);
	} else if (r == C_VISCA_SET_GAMMA) {
		unsigned char *p = protoBytes[C_VISCA_SET_GAMMA];
		p[4 + 2] = x & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_GAMMA, (int)x);
	} else if (r == C_VISCA_SET_AWB_MODE) {
		unsigned char *p = protoBytes[C_VISCA_SET_AWB_MODE];
		if (x < 0x04)
			p[4 + 2] = x;
		else
			p[4 + 2] = x + 0x01;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_AWB_MODE, (int)x);
	} else if (r == C_VISCA_SET_DEFOG_MODE) {
		unsigned char *p = protoBytes[C_VISCA_SET_DEFOG_MODE];
		p[4 + 2] = x ? 0x02 : 0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_DEFOG_MODE, (int)x);
	} else if (r == C_VISCA_SET_FOCUS_MODE) {
		unsigned char *p = protoBytes[C_VISCA_SET_FOCUS_MODE];
		p[4 + 2] = x ? 0x03 : 0x02;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_FOCUS_MODE, (int)x);
	} else if (r == C_VISCA_SET_ZOOM_TRIGGER) {
		unsigned char *p = protoBytes[C_VISCA_SET_ZOOM_TRIGGER];
		p[4 + 2] = x ? 0x02 : 0x01;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_ZOOM_TRIGGER, (int)x);
	} else if (r == C_VISCA_SET_BLC_STATUS) {
		unsigned char *p = protoBytes[C_VISCA_SET_BLC_STATUS];
		p[4 + 2] = x ? 0x02 : 0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_BLC_STATUS, (int)x);
	} else if (r == C_VISCA_SET_IRCF_STATUS) {
		unsigned char *p = protoBytes[C_VISCA_SET_IRCF_STATUS];
		p[4 + 2] = x ? 0x02 : 0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_IRCF_STATUS, (int)x);
	} else if (r == C_VISCA_SET_AUTO_ICR) {
		unsigned char *p = protoBytes[C_VISCA_SET_AUTO_ICR];
		p[4 + 2] = x ? 0x02 : 0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_AUTO_ICR, (int)x);
	} else if (r == C_VISCA_SET_PROGRAM_AE_MODE) {
		unsigned char *p = protoBytes[C_VISCA_SET_PROGRAM_AE_MODE];
		p[4 + 2] = x;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_PROGRAM_AE_MODE, (int)x);
	} else if (r == C_VISCA_SET_FLIP_MODE) {
		unsigned char *p = protoBytes[C_VISCA_SET_FLIP_MODE];
		p[4 + 2] = x ? 0x02 : 0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_FLIP, (int)x);
	} else if (r == C_VISCA_SET_MIRROR_MODE) {
		unsigned char *p = protoBytes[C_VISCA_SET_MIRROR_MODE];
		p[4 + 2] = x ? 0x02 : 0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_MIRROR, (int)x);
	} else if (r == C_VISCA_SET_DIGI_ZOOM_STAT) {
		unsigned char *p = protoBytes[C_VISCA_SET_DIGI_ZOOM_STAT];
		p[4 + 2] = x ? 0x02 : 0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_DIGI_ZOOM_STAT, (int)x);
	} else if (r == C_VISCA_SET_ZOOM_TYPE) {
		unsigned char *p = protoBytes[C_VISCA_SET_ZOOM_TYPE];
		p[4 + 2] = x ? 0x00 : 0x01;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_ZOOM_TYPE, (int)x);
	} else if (r == C_VISCA_SET_ONE_PUSH) {
		unsigned char *p = protoBytes[C_VISCA_SET_ONE_PUSH];
		p[4 + 2] = 0x01;
		transport->send((const char *)p + 2, p[0]);
	} else if (r == C_VISCA_SET_EXPOSURE_TARGET) {
		unsigned char *p = protoBytes[C_VISCA_SET_EXPOSURE_TARGET];
		p[5 + 2] = (x & 0xf0) >> 4;
		p[6 + 2] = x & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_EXPOSURE_TARGET, (int)x);
	} else if (r == C_VISCA_GET_GAIN_LIMIT_OEM) {
		unsigned char *p = protoBytes[r];
		p[5 + 2] = (x & 0xf000) >> 12;
		p[6 + 2] = (x & 0x0f00) >> 8;
		p[7 + 2] = (x & 0x00f0) >> 4;
		p[8 + 2] = (x & 0x000f);
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_TOP_SHUTTER, (int)((x & 0xff00) >> 8));
		setRegister(R_BOT_SHUTTER, (int)(x & 0x00ff));
	} else if (r == C_VISCA_GET_SHUTTER_LIMIT_OEM) {
		unsigned char *p = protoBytes[r];
		p[5 + 2] = (x & 0xf00000) >> 20;
		p[6 + 2] = (x & 0x0f0000) >> 16;
		p[7 + 2] = (x & 0x00f000) >> 12;
		p[8 + 2] = (x & 0x000f00) >> 8;
		p[9 + 2] = (x & 0x0000f0) >> 4;
		p[10 + 2] = (x & 0x00000f);
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_TOP_GAIN, (int)((x & 0xfff000) >> 12));
		setRegister(R_BOT_GAIN, (int)(x & 0x000fff));
	} else if (r == C_VISCA_GET_IRIS_LIMIT_OEM) {
		unsigned char *p = protoBytes[r];
		p[5 + 2] = (x & 0xf00000) >> 20;
		p[6 + 2] = (x & 0x0f0000) >> 16;
		p[7 + 2] = (x & 0x00f000) >> 12;
		p[8 + 2] = (x & 0x000f00) >> 8;
		p[9 + 2] = (x & 0x0000f0) >> 4;
		p[10 + 2] = (x & 0x00000f);
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_TOP_IRIS, (int)((x & 0xfff000) >> 12));
		setRegister(R_BOT_IRIS, (int)(x & 0x000fff));
	}

	if (getRegister(R_FLIP) == 1 && getRegister(R_MIRROR) == 0) {
		setRegister(R_DISPLAY_ROT, 0);
	} else if (getRegister(R_FLIP) == 0 && getRegister(R_MIRROR) == 1) {
		setRegister(R_DISPLAY_ROT, 1);
	} else if (getRegister(R_FLIP) == 1 && getRegister(R_MIRROR) == 1) {
		setRegister(R_DISPLAY_ROT, 2);
	} else if (getRegister(R_FLIP) == 0 && getRegister(R_MIRROR) == 0) {
		setRegister(R_DISPLAY_ROT, 3);
		getRegister(R_DISPLAY_ROT);
	}
}

uint OemModuleHead::getProperty(uint r)
{
	return getRegister(r);
}

/* [CR] [yca] galiba bu API kalkabilir? */
void OemModuleHead::setDeviceDefinition(QString definition)
{
	deviceDefinition = definition;
}

QString OemModuleHead::getDeviceDefinition()
{
	return deviceDefinition;
}

int OemModuleHead::getZoomSpeed()
{
	return zoomSpeed;
}

void OemModuleHead::clockInvert(bool st)
{
	if (st)
		HardwareOperations::writeRegister(0x1c40044, 0x1c);
	else
		HardwareOperations::writeRegister(0x1c40044, 0x18);
}

int OemModuleHead::setShutterLimit(uint topLim, uint botLim)
{
	const uchar cmd[] = {0x81,
						 0x01,
						 0x04,
						 0x40,
						 0x06,
						 uchar((topLim & 0xf00) >> 8),
						 uchar((topLim & 0xf0) >> 4),
						 uchar(topLim & 0xf),
						 uchar((botLim & 0xf00) >> 8),
						 uchar((botLim & 0xf0) >> 4),
						 uchar(botLim & 0xf),
						 0xff};
	setRegister(R_TOP_SHUTTER, topLim);
	setRegister(R_BOT_SHUTTER, botLim);
	return transport->send((const char *)cmd, sizeof(cmd));
}

QString OemModuleHead::getShutterLimit()
{
	QString mes = QString("%1,%2")
					  .arg(getProperty(R_TOP_SHUTTER))
					  .arg(getProperty(R_BOT_SHUTTER));
	return mes;
}

int OemModuleHead::setIrisLimit(uint topLim, uint botLim)
{
	const uchar cmd[] = {0x81,
						 0x01,
						 0x04,
						 0x40,
						 0x07,
						 uchar((topLim & 0xf00) >> 8),
						 uchar((topLim & 0xf0) >> 4),
						 uchar(topLim & 0xf),
						 uchar((botLim & 0xf00) >> 8),
						 uchar((botLim & 0xf0) >> 4),
						 uchar(botLim & 0xf),
						 0xff};
	setRegister(R_TOP_IRIS, topLim);
	setRegister(R_BOT_IRIS, botLim);
	return transport->send((const char *)cmd, sizeof(cmd));
}

QString OemModuleHead::getIrisLimit()
{
	int top = getProperty(R_TOP_IRIS);
	int bot = getProperty(R_BOT_IRIS);
	QString mes = QString("%1,%2").arg(top).arg(bot);
	return mes;
}

int OemModuleHead::setGainLimit(uchar topLim, uchar botLim)
{
	const uchar cmd[] = {0x81,
						 0x01,
						 0x04,
						 0x40,
						 0x05,
						 uchar(topLim >> 4),
						 uchar(topLim & 0xf),
						 uchar(botLim >> 4),
						 uchar(botLim & 0xf),
						 0xff};
	setRegister(R_TOP_GAIN, topLim);
	setRegister(R_BOT_GAIN, botLim);
	return transport->send((const char *)cmd, sizeof(cmd));
}

QString OemModuleHead::getGainLimit()
{
	QString mes = QString("%1,%2")
					  .arg(getProperty(R_TOP_GAIN))
					  .arg(getProperty(R_BOT_GAIN));
	return mes;
}
