#include "oemmodulehead.h"
#include "debug.h"
#include "ptzptransport.h"
#include "ioerrors.h"

#include <QFile>
#include <QList>
#include <QMutex>
#include <QJsonArray>
#include <QJsonObject>
#include <QMutexLocker>
#include <QJsonDocument>

#include <drivers/hardwareoperations.h>

#include <errno.h>

#define dump() \
	for (int i = 0; i < len; i++) \
		qDebug("%s: %d: 0x%x", __func__, i, p[i]);

enum Commands {
	/* visca commands */
	C_VISCA_SET_EXPOSURE,		//14 //td:nd // exposure_value
	C_VISCA_SET_EXPOSURE_TARGET,
	C_VISCA_SET_GAIN,			//15 //td:nd // gainvalue
	C_VISCA_SET_ZOOM_POS,
	C_VISCA_SET_EXP_COMPMODE,	//16 //td:nd
	C_VISCA_SET_EXP_COMPVAL,	//17 //td:nd
	C_VISCA_SET_GAIN_LIM,		//18 //td:nd
	C_VISCA_SET_SHUTTER,		//19 //td:nd // getshutterspeed
	C_VISCA_SET_NOISE_REDUCT,	//22 //td:nd
	C_VISCA_SET_WDRSTAT,		//23 //td:nd
	//	C_VISCA_SET_WDRPARAM,		//24 //td:nd
	C_VISCA_SET_GAMMA,			//25 //td.nd
	C_VISCA_SET_AWB_MODE,		//26 //td:nd
	C_VISCA_SET_DEFOG_MODE,		//27 //td:nd
	C_VISCA_SET_DIGI_ZOOM_STAT,	//28 //td:nd
	C_VISCA_SET_ZOOM_TYPE,		//29 //td:nd
	C_VISCA_SET_FOCUS,
	C_VISCA_SET_FOCUS_MODE,		//30 //td:nd
	C_VISCA_SET_ZOOM_TRIGGER,	//31 //td:nd
	C_VISCA_SET_BLC_STATUS,		//32 //td:nd
	C_VISCA_SET_IRCF_STATUS,	//33 //td:nd
	C_VISCA_SET_AUTO_ICR,		//34 //td:nd
	C_VISCA_SET_PROGRAM_AE_MODE,//35 //td:nd
	C_VISCA_SET_FLIP_MODE,		//36
	C_VISCA_SET_MIRROR_MODE,	//37
	C_VISCA_SET_ONE_PUSH,
	C_VISCA_GET_ZOOM,			//10
	C_VISCA_ZOOM_IN,			//11
	C_VISCA_ZOOM_OUT,			//12
	C_VISCA_ZOOM_STOP,			//13
	C_VISCA_GET_EXPOSURE,		//14 //td:nd // exposure_value
	C_VISCA_GET_GAIN,			//15 //td:nd // gainvalue
	C_VISCA_GET_EXP_COMPMODE,	//16 //td:nd
	C_VISCA_GET_EXP_COMPVAL,	//17 //td:nd
	C_VISCA_GET_GAIN_LIM,		//18 //td:nd
	C_VISCA_GET_SHUTTER,		//19 //td:nd // getshutterspeed
	C_VISCA_GET_NOISE_REDUCT,	//22 //td:nd
	C_VISCA_GET_WDRSTAT,		//23 //td:nd
	//	C_VISCA_GET_WDRPARAM,		//24 //td:nd
	C_VISCA_GET_GAMMA,			//25 //td.nd
	C_VISCA_GET_AWB_MODE,		//26 //td:nd
	C_VISCA_GET_DEFOG_MODE,		//27 //td:nd
	C_VISCA_GET_DIGI_ZOOM_STAT,	//28 //td:nd
	C_VISCA_GET_ZOOM_TYPE,		//29 //td:nd
	C_VISCA_GET_FOCUS_MODE,		//30 //td:nd
	C_VISCA_GET_ZOOM_TRIGGER,	//31 //td:nd
	C_VISCA_GET_BLC_STATUS,		//32 //td:nd
	C_VISCA_GET_IRCF_STATUS,	//33 //td:nd
	C_VISCA_GET_AUTO_ICR,		//34 //td:nd
	C_VISCA_GET_PROGRAM_AE_MODE,//35 //td:nd
	C_VISCA_GET_FLIP_MODE,		//36
	C_VISCA_GET_MIRROR_MODE,	//37

	C_COUNT,
};

#define MAX_CMD_LEN	16

static unsigned char protoBytes[C_COUNT][MAX_CMD_LEN] = {
	/* visca commands */
	{0x09, 0x00, 0x81, 0x01, 0x04, 0x4b, 0x00, 0x00, 0x00, 0x0f, 0xff},	//set_exposure_value
	{0x08, 0x00, 0x81, 0x01, 0x04, 0x40, 0x04, 0x00, 0x00, 0xff },	//set exposure target
	{0x09, 0x00, 0x81, 0x01, 0x04, 0x4c, 0x00, 0x00, 0xf0, 0x0f, 0xff },	//set gain value
	{0x09, 0x00, 0x81, 0x01, 0x04, 0x47, 0x00, 0x00, 0x00, 0x00, 0xff },	//set zoom pos
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x3E,0x00, 0xff },	//set exp comp mode
	{0x09, 0x00, 0x81, 0x01, 0x04, 0x4E, 0x00, 0x00, 0xf0, 0x0f, 0xff },	//set exp comp val
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x2c, 0x0f, 0xff },	//set gain limit
	{0x09, 0x00, 0x81, 0x01, 0x04, 0x4a, 0x00, 0x00, 0x00, 0x0f, 0xff },//set shutter speed
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x53, 0x0f, 0xff },	//set noise reduct
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x3d, 0x00, 0xff },	// set wdr stat
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x5b, 0x0f, 0xff },	//set gamma
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x35, 0x00, 0xff },	//set awb mode
	{0x07, 0x00, 0x81, 0x01, 0x04, 0x37, 0x00, 0x00, 0xff },	//set defog mode
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x06, 0x00, 0xff },//digizoom
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x36, 0x00, 0xff },//zoom type
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x08, 0x00, 0xff },//set focus far-near
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x38, 0x00, 0xff },	//set focus mode
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x57, 0x00, 0xFF },	//set zoom trigger
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x33, 0x00, 0xff },//se BLC stat
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x01, 0x00, 0xff },//set IRCF stst
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x51, 0x00, 0xff },//set Auto ICR
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x39, 0x00, 0xff },//set program AE mode
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x66, 0x00, 0xff }, //set flip mode
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x61, 0x00, 0xff },//set Mirror mode
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x18, 0x01, 0xff },//set One Push
	{0x05, 0x07, 0x81, 0x09, 0x04, 0x47, 0xff},
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x07, 0x20, 0xff},
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x07, 0x30, 0xff},
	{0x06, 0x00, 0x81, 0x01, 0x04, 0x07, 0x00, 0xff},
	{0x05, 0x07, 0x81, 0x09, 0x04, 0x4b, 0xff}, //get_exposure_value
	{0x05, 0x07, 0x81, 0x09, 0x04, 0x4c, 0xff}, //get_gain_value
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x3e, 0xff}, //getExpCompMode
	{0x05, 0x07, 0x81, 0x09, 0x04, 0x4e, 0xff}, //getExpCompval
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x2c, 0xff}, //getGainLimit
	{0x05, 0x07, 0x81, 0x09, 0x04, 0x4a, 0xff}, //getsutterSpeed 19
	//	{0x05, 0x04, 0x81, 0x09, 0x04, 0x12, 0xff}, //getMinShutter
	//	{0x05, 0x07, 0x81, 0x09, 0x04, 0x13, 0xff}, //getMinShutterLimit
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x53, 0xff}, //getNoiseReduct
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x3d, 0xff}, //getWDRstat
	//	{0x05, 0x08, 0x81, 0x09, 0x04, 0x2D, 0xff}, //getWDRparameter
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x5b, 0xff}, //getGamma
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x35, 0xff}, //getAWBmode
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x37, 0xff}, //getDefogMode
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x06, 0xff}, //getDigiZoomStat
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x36, 0xff}, //getZoomType
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x38, 0xff}, //getFocusMode
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x57, 0xff}, //getZoomTrigger
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x33, 0xff}, //getBLCstatus
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x01, 0xff}, //getIRCFstat
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x51, 0xff}, //getAutoICR
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x39, 0xff}, //getProgramAEmode
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x66, 0xff}, //getFlipMode
	{0x05, 0x04, 0x81, 0x09, 0x04, 0x61, 0xff}, //getMirrorMode

	//	{0x08, 0x07, 0x81, 0x01, 0x04, 0x40, 0x04, 0x, 0x, 0xff }, //set_exposure_target
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
	hist = new CommandHistory;
	nextSync = C_COUNT;
	syncEnabled = true;
	syncInterval = 40;
	syncTime.start();
	irLedLevel = 0;
#ifdef HAVE_PTZP_GRPC_API
	settings = {
		{"exposure_value", {C_VISCA_SET_EXPOSURE, R_EXPOSURE_VALUE}},
		{"gain_value", {C_VISCA_SET_GAIN, R_GAIN_VALUE}},
		{"exp_comp_mode", {C_VISCA_SET_EXP_COMPMODE, R_EXP_COMPMODE}},
		{"exp_comp_val", {C_VISCA_SET_EXP_COMPVAL, R_EXP_COMPVAL}},
		{"gain_limit", {C_VISCA_SET_GAIN_LIM,R_GAIN_LIM}},
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
		{"focus_in", {C_VISCA_SET_FOCUS, NULL}},
		{"focus_out", {C_VISCA_SET_FOCUS, NULL}},
		{"focus_stop", {C_VISCA_SET_FOCUS, NULL}}
	};
#endif
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
	if (nextSync == C_COUNT)
		return ST_NORMAL;
	return ST_SYNCING;
}

int OemModuleHead::startZoomIn(int speed)
{
	zoomRatio = speed;
	unsigned char *p = protoBytes[C_VISCA_ZOOM_IN];
	hist->add(C_VISCA_ZOOM_IN);
	p[4 + 2] = 0x20 + speed;
	return transport->send((const char *)p + 2, p[0]);
}

int OemModuleHead::startZoomOut(int speed)
{
	zoomRatio = speed;
	unsigned char *p = protoBytes[C_VISCA_ZOOM_OUT];
	hist->add(C_VISCA_ZOOM_OUT);
	p[4 + 2] = 0x30 + speed;
	return transport->send((const char *)p + 2, p[0]);
}

int OemModuleHead::stopZoom()
{
	const unsigned char *p = protoBytes[C_VISCA_ZOOM_STOP];
	hist->add(C_VISCA_ZOOM_IN);
	return transport->send((const char *)p + 2, p[0]);
}

int OemModuleHead::getZoom()
{
	return getRegister(R_ZOOM_POS);
}

int OemModuleHead::setZoom(uint pos)
{
	unsigned char *p = protoBytes[C_VISCA_SET_ZOOM_POS];
	hist->add(C_VISCA_SET_ZOOM_POS);
	p[4 + 2] = (pos & 0XF000) >> 12;
	p[4 + 3] = (pos & 0XF00) >> 8;
	p[4 + 4] = (pos & 0XF0) >> 4;
	p[4 + 5] = pos & 0XF;
	return transport->send((const char *)p + 2, p[0]);
}

void OemModuleHead::enableSyncing(bool en)
{
	syncEnabled = en;
	mInfo("Sync status: %d", (int)syncEnabled);
}

void OemModuleHead::setSyncInterval(int interval)
{
	syncInterval = interval;
	mInfo("Sync interval: %d", syncInterval);
}

int OemModuleHead::syncNext()
{
	const unsigned char *p = protoBytes[nextSync];
	hist->add((Commands)nextSync);
	return transport->send((const char *)p + 2, p[0]);
}

int OemModuleHead::dataReady(const unsigned char *bytes, int len)
{
	for (int i = 0 ; i < len; i++)
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
	if (expected == 0x00) {
		for (int i = 0; i < len; i++) {
			ffDebug() << i << bytes[i];
			if (bytes[i] == 0xff)
				return i+1;
		}
		return len;
	} else if (p[1] == 0x41) {
		mLogv("acknowledge");
		return 3;
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

	/* register sync support */
	if (nextSync != C_COUNT) {
		/* we are in sync mode, let's sync next */
		mInfo("Next sync property: %d",nextSync);
		hist->takeFirst();
		if (++nextSync == C_COUNT) {
			fDebug("Visca register syncing completed, activating auto-sync");
		} else
			syncNext();
	}

	if (sendcmd == C_VISCA_GET_EXPOSURE) {
		uint exVal = ((p[4] << 4) | p[5]);
		if (exVal == 0)			//Açıklama için setProperty'de exposure kısmına bakın!
			exVal = 13;
		else exVal = 17 - exVal;
		mInfo("Exposure value synced");
		setRegister(R_EXPOSURE_VALUE , exVal);
	} else if(sendcmd == C_VISCA_GET_GAIN) {
		mInfo("Gain Value synced");
		setRegister(R_GAIN_VALUE , (p[4] << 4 | p[5]));
	} else if(sendcmd == C_VISCA_GET_EXP_COMPMODE){
		mInfo("Ex Comp Mode synced");
		setRegister(R_EXP_COMPMODE , (p[2] == 0x02));
	} else if(sendcmd == C_VISCA_GET_EXP_COMPVAL){
		mInfo("Ex Comp Value synced");
		setRegister(R_EXP_COMPVAL , ((p[4] << 4) + p[5]));
	} else if(sendcmd == C_VISCA_GET_GAIN_LIM){
		mInfo("Gain Limit synced");
		setRegister(R_GAIN_LIM , (p[2] - 4));
	} else if(sendcmd == C_VISCA_GET_SHUTTER){
		mInfo("Shutter synced");
		setRegister(R_SHUTTER , (p[4] << 4 | p[5]));
	} else if(sendcmd == C_VISCA_GET_NOISE_REDUCT){
		mInfo("Noise Reduct synced");
		setRegister(R_NOISE_REDUCT , p[2]);
	} else if(sendcmd == C_VISCA_GET_WDRSTAT){
		mInfo("Wdr Status synced");
		setRegister(R_WDRSTAT , (p[2] == 0x02));
	} else if(sendcmd == C_VISCA_GET_GAMMA){
		mInfo("Gamma synced");
		setRegister(R_GAMMA , p[2]);
	} else if(sendcmd == C_VISCA_GET_AWB_MODE){
		mInfo("Awb Mode synced");
		char val = p[2];
		if (val > 0x03)
			val -= 1;
		setRegister(R_AWB_MODE , val);
	} else if(sendcmd == C_VISCA_GET_DEFOG_MODE){
		mInfo("Defog Mode synced");
		setRegister(R_DEFOG_MODE , (p[2] == 0x02));
	} else if(sendcmd == C_VISCA_GET_DIGI_ZOOM_STAT){
		mInfo("Digi Zoom Status synced");
		setRegister(R_DIGI_ZOOM_STAT,(p[2] == 0x02) ? true : false);
	} else if(sendcmd == C_VISCA_GET_ZOOM_TYPE){
		mInfo("Zoom Type synced");
		setRegister(R_ZOOM_TYPE,p[2]);
	} else if(sendcmd == C_VISCA_GET_FOCUS_MODE){
		mInfo("Focus mode synced");
		setRegister(R_FOCUS_MODE,p[2]);
	} else if(sendcmd == C_VISCA_GET_ZOOM_TRIGGER){
		mInfo("Zoom Trigger synced");
		if (p[2] == 0x01)
			setRegister(R_ZOOM_TRIGGER,0);
		else if (p[2] == 0x02)
			setRegister(R_ZOOM_TRIGGER,1);
	} else if(sendcmd == C_VISCA_GET_BLC_STATUS){
		mInfo("BLC status synced");
		setRegister(R_BLC_STATUS,(p[2] == 0x02) ? true : false);
	} else if(sendcmd == C_VISCA_GET_IRCF_STATUS){
		mInfo("Ircf Status synced");
		setRegister(R_IRCF_STATUS,(p[2] == 0x02) ? true : false);
	} else if(sendcmd == C_VISCA_GET_AUTO_ICR){
		mInfo("Auto Icr synced");
		setRegister(R_AUTO_ICR,(p[2] == 0x02) ? true : false);
	} else if(sendcmd == C_VISCA_GET_PROGRAM_AE_MODE){
		mInfo("Program AE Mode synced");
		setRegister(R_PROGRAM_AE_MODE,p[2]);
	} else if(sendcmd == C_VISCA_GET_ZOOM){
		mLogv("Zoom Position synced");
		uint value =
				((p[2] & 0x0F) << 12) +
				((p[3] & 0x0F) << 8) +
				((p[4] & 0x0F) << 4) +
				((p[5] & 0x0F) << 0);
		setRegister(R_ZOOM_POS, value);
	} else if(sendcmd == C_VISCA_GET_FLIP_MODE){
		mInfo("Flip Status synced");
		setRegister(R_FLIP,(p[2] == 0x02) ? true : false);
	} else if(sendcmd == C_VISCA_GET_MIRROR_MODE){
		mInfo("Mirror status synced");
		setRegister(R_MIRROR,(p[2] == 0x02) ? true : false);
	}

	if(getRegister(R_ZOOM_POS) >= 16384){
		mLogv("Optic and digital zoom position synced");
		setRegister(R_DIGI_ZOOM_POS,getRegister(R_ZOOM_POS)-16384);
		setRegister(R_OPTIC_ZOOM_POS,16384);
	} else if (getRegister(R_ZOOM_POS) < 16384){
		mLogv("Optic and digital zoom position synced");
		setRegister(R_OPTIC_ZOOM_POS,getRegister(R_ZOOM_POS));
		setRegister(R_DIGI_ZOOM_POS,0);
	}

	if (getRegister(R_FLIP) == 1 && getRegister(R_MIRROR) == 0){
		setRegister(R_DISPLAY_ROT,0);
	} else if (getRegister(R_FLIP) == 0 && getRegister(R_MIRROR) == 1){
		setRegister(R_DISPLAY_ROT,1);
	} else if (getRegister(R_FLIP) == 1 && getRegister(R_MIRROR) == 1){
		setRegister(R_DISPLAY_ROT,2);
	} else if (getRegister(R_FLIP) == 0 && getRegister(R_MIRROR) == 0){
		setRegister(R_DISPLAY_ROT,3);
		getRegister(R_DISPLAY_ROT);
	}
	return expected;
}

QByteArray OemModuleHead::transportReady()
{
	if (syncEnabled && syncTime.elapsed() > syncInterval) {
		mLogv("Syncing zoom positon");
		syncTime.restart();
		const unsigned char *p = protoBytes[C_VISCA_GET_ZOOM];
		hist->add(C_VISCA_GET_ZOOM);
		return QByteArray((const char *)p + 2, p[0]);
	}
	return QByteArray();
}

QJsonValue OemModuleHead::marshallAllRegisters()
{
	QJsonObject json;
	for(int i = 0; i < R_COUNT; i++)
		json.insert(QString("reg%1").arg(i), (int)getRegister(i));

	json.insert(QString("deviceDefiniton"), (QString)deviceDefinition);
	json.insert(QString("zoomRatio"),(int)zoomRatio);
	json.insert(QString("irLedLevel"), (int)irLedLevel);
	return json;
}

void OemModuleHead::unmarshallloadAllRegisters(const QJsonValue &node)
{
	QJsonObject root = node.toObject();
	QString key = "reg%1";
	for(int i = 0 ; i <= 21; i++) {
		if(i == 2)
			setZoom((uint)root.value(key.arg(i)).toInt());
		setProperty(i,root.value(key.arg(i)).toInt());
	}
	deviceDefinition = root.value("deviceDefiniton").toString();
	zoomRatio = root.value("zoomRatio").toInt();
	setIRLed(root.value("irLedLevel").toInt());
}

void OemModuleHead::setProperty(uint r, uint x)
{
	mInfo("Set Property %d , value: %d", r, x);
	if (r == C_VISCA_SET_EXPOSURE) {
		unsigned char *p = protoBytes[C_VISCA_SET_EXPOSURE];
		hist->add(C_VISCA_SET_EXPOSURE );
		uint val;
		/***
		 * FCB-EV7500 dökümanında(Visca datasheet) sayfa 9'da bulunan Iris priority bölümünde bulunan tabloya göre
		 * exposure (iris priorty) değerleri 0 ile 17 arasında değişmekte fakat 14 adet bilgi bulunmaktadır.
		 * Bu değerlerin 0-13 arasına çekilip web sayfası tarafındada düzgün bir görüntüleme sağmabilmesi açısından
		 * aşağıdaki işlem uygulanmıştır.
		 */
		if (x == 13)
			val = 0;
		else val = 17- x;
		p[6 + 2] = val >> 4;
		p[7 + 2] = val & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_EXPOSURE_VALUE, (int)x);
	} else if (r == C_VISCA_SET_NOISE_REDUCT) {
		unsigned char *p = protoBytes[C_VISCA_SET_NOISE_REDUCT];
		hist->add(C_VISCA_SET_NOISE_REDUCT );
		p[4 + 2] = x & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_NOISE_REDUCT, (int)x);
	} else if (r == C_VISCA_SET_GAIN) {
		unsigned char *p = protoBytes[C_VISCA_SET_GAIN];
		hist->add(C_VISCA_SET_GAIN );
		p[6 + 2] = (x & 0xf0) >> 4;
		p[7 + 2] = x & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_GAIN_VALUE, (int)x);
	} else if (r == C_VISCA_SET_EXP_COMPMODE) {
		unsigned char *p = protoBytes[C_VISCA_SET_EXP_COMPMODE];
		hist->add(C_VISCA_SET_EXP_COMPMODE );
		p[4 + 2] = x ? 0x02 :0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_EXP_COMPMODE, (int)x);
	} else if (r == C_VISCA_SET_EXP_COMPVAL) {
		unsigned char *p = protoBytes[C_VISCA_SET_EXP_COMPVAL];
		hist->add(C_VISCA_SET_EXP_COMPVAL );
		p[6 + 2] = (x & 0xf0) >> 4;
		p[7 + 2] = x & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_EXP_COMPVAL, (int)x);
	} else if (r == C_VISCA_SET_GAIN_LIM) {
		unsigned char *p = protoBytes[C_VISCA_SET_GAIN_LIM];
		hist->add(C_VISCA_SET_GAIN_LIM );
		p[4 + 2] = (x + 4) & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_GAIN_LIM, (int)x);
	} else if (r == C_VISCA_SET_SHUTTER) {
		unsigned char *p = protoBytes[C_VISCA_SET_SHUTTER];
		hist->add(C_VISCA_SET_SHUTTER );
		p[6 + 2] = x >> 4;
		p[7 + 2] = x & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_SHUTTER, (int)x);
	} else if (r == C_VISCA_SET_WDRSTAT) {
		unsigned char *p = protoBytes[C_VISCA_SET_WDRSTAT];
		hist->add(C_VISCA_SET_WDRSTAT );
		p[4 + 2] = x ? 0x02 :0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_WDRSTAT, (int)x);
	} else if (r == C_VISCA_SET_GAMMA) {
		unsigned char *p = protoBytes[C_VISCA_SET_GAMMA];
		hist->add(C_VISCA_SET_GAMMA );
		p[4 + 2] = x & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_GAMMA, (int)x);
	} else if (r == C_VISCA_SET_AWB_MODE) {
		unsigned char *p = protoBytes[C_VISCA_SET_AWB_MODE];
		hist->add(C_VISCA_SET_AWB_MODE );
		if (x <0x04)
			p[4 + 2] = x;
		else p[4 + 2] = x + 0x01;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_AWB_MODE, (int)x);
	} else if (r == C_VISCA_SET_DEFOG_MODE) {
		unsigned char *p = protoBytes[C_VISCA_SET_DEFOG_MODE];
		hist->add(C_VISCA_SET_DEFOG_MODE );
		p[4 + 2] = x ? 0x02 :0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_DEFOG_MODE, (int)x);
	} else if (r == C_VISCA_SET_FOCUS_MODE) {
		unsigned char *p = protoBytes[C_VISCA_SET_FOCUS_MODE];
		hist->add(C_VISCA_SET_FOCUS_MODE );
		p[4 + 2] = x ? 0x03 :0x02;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_FOCUS_MODE, (int)x);
	} else if (r == C_VISCA_SET_ZOOM_TRIGGER) {
		unsigned char *p = protoBytes[C_VISCA_SET_ZOOM_TRIGGER];
		hist->add(C_VISCA_SET_ZOOM_TRIGGER );
		p[4 + 2] = x ? 0x02 :0x01;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_ZOOM_TRIGGER, (int)x);
	} else if (r == C_VISCA_SET_BLC_STATUS) {
		unsigned char *p = protoBytes[C_VISCA_SET_BLC_STATUS];
		hist->add(C_VISCA_SET_BLC_STATUS );
		p[4 + 2] = x ? 0x02 :0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_BLC_STATUS, (int)x);
	} else if (r == C_VISCA_SET_IRCF_STATUS) {
		unsigned char *p = protoBytes[C_VISCA_SET_IRCF_STATUS];
		hist->add(C_VISCA_SET_IRCF_STATUS );
		p[4 + 2] = x ? 0x02 :0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_IRCF_STATUS, (int)x);
	} else if (r == C_VISCA_SET_AUTO_ICR) {
		unsigned char *p = protoBytes[C_VISCA_SET_AUTO_ICR];
		hist->add(C_VISCA_SET_AUTO_ICR );
		p[4 + 2] = x ? 0x02 :0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_AUTO_ICR, (int)x);
	} else if (r == C_VISCA_SET_PROGRAM_AE_MODE) {
		unsigned char *p = protoBytes[C_VISCA_SET_PROGRAM_AE_MODE];
		hist->add(C_VISCA_SET_PROGRAM_AE_MODE );
		p[4 + 2] = x;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_PROGRAM_AE_MODE, (int)x);
	} else if (r == C_VISCA_SET_FLIP_MODE) {
		unsigned char *p = protoBytes[C_VISCA_SET_FLIP_MODE];
		hist->add(C_VISCA_SET_FLIP_MODE );
		p[4 + 2] = x ? 0x02 :0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_FLIP, (int)x);
	} else if (r == C_VISCA_SET_MIRROR_MODE) {
		unsigned char *p = protoBytes[C_VISCA_SET_MIRROR_MODE];
		hist->add(C_VISCA_SET_MIRROR_MODE );
		p[4 + 2] = x ? 0x02 :0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_MIRROR, (int)x);
	} else if (r == C_VISCA_SET_DIGI_ZOOM_STAT) {
		unsigned char *p = protoBytes[C_VISCA_SET_DIGI_ZOOM_STAT];
		hist->add(C_VISCA_SET_DIGI_ZOOM_STAT );
		p[4 + 2] = x ? 0x02 :0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_DIGI_ZOOM_STAT, (int)x);
	} else if (r == C_VISCA_SET_ZOOM_TYPE) {
		unsigned char *p = protoBytes[C_VISCA_SET_ZOOM_TYPE];
		hist->add(C_VISCA_SET_ZOOM_TYPE );
		p[4 + 2] = x ? 0x00 :0x01;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_ZOOM_TYPE, (int)x);
	} else if (r == C_VISCA_SET_ONE_PUSH) {
		unsigned char *p = protoBytes[C_VISCA_SET_ONE_PUSH];
		hist->add(C_VISCA_SET_ONE_PUSH );
		p[4 + 2] = 0x01;
		transport->send((const char *)p + 2, p[0]);
	} else if (r == C_VISCA_SET_FOCUS) {
		unsigned char *p = protoBytes[C_VISCA_SET_FOCUS];
		hist->add(C_VISCA_SET_FOCUS);
		p[4 + 2] = x;
		transport->send((const char *)p + 2, p[0]);
	} else if (r == C_VISCA_SET_EXPOSURE_TARGET){
		unsigned char *p = protoBytes[C_VISCA_SET_EXPOSURE_TARGET];
		hist->add(C_VISCA_SET_EXPOSURE_TARGET);
		p[5 + 2] = (x & 0xf0) >> 4 ;
		p[6 + 2] = x & 0x0f ;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_EXPOSURE_TARGET, (int)x);
	}

	if (getRegister(R_FLIP) == 1 && getRegister(R_MIRROR) == 0){
		setRegister(R_DISPLAY_ROT,0);
	} else if (getRegister(R_FLIP) == 0 && getRegister(R_MIRROR) == 1){
		setRegister(R_DISPLAY_ROT,1);
	} else if (getRegister(R_FLIP) == 1 && getRegister(R_MIRROR) == 1){
		setRegister(R_DISPLAY_ROT,2);
	} else if (getRegister(R_FLIP) == 0 && getRegister(R_MIRROR) == 0){
		setRegister(R_DISPLAY_ROT,3);
		getRegister(R_DISPLAY_ROT);
	}
}

uint OemModuleHead::getProperty(uint r)
{
	return getRegister(r);
}

void OemModuleHead::setDeviceDefinition(QString definition)
{
	deviceDefinition = definition;
}

QString OemModuleHead::getDeviceDefinition()
{
	return deviceDefinition;
}

int OemModuleHead::getZoomRatio()
{
	return zoomRatio;
}

void OemModuleHead::clockInvert(bool st)
{
	if (st)
		HardwareOperations::writeRegister(0x1c40044, 0x1c);
	else HardwareOperations::writeRegister(0x1c40044, 0x18);
}

/**
 * @brief OemModuleHead::maskSet
 * @param maskID : visca is have 24 mask ID, from 0 to 23
 * @param nn : if 0 ,this mask is used one time after reset setting, if 1, setting is record
 * @param width : width +-0x50
 * @param height : height +-0x2d
 * @return
 */

int OemModuleHead::maskSet(uint maskID, int width, int height, bool nn)
{
	if((maskID > 0x17) || (width > 0x50) || (width < -0x50) || (height > 0x2D) || (height < -0x2D))
		return -ENODATA;
	const uchar cmd[] = {0x81, 0x01, 0x04, 0x76, maskID & 0xFF,
						 nn, (uchar(width) & 0xF0) >> 4, uchar(width) & 0x0F,
						 (uchar(height) & 0xF0) >> 4, uchar(height) & 0x0F, 0xFF };
	return transport->send((const char *)cmd, sizeof(cmd));
}

/**
 * @brief OemModuleHead::maskSetNoninterlock
 * @param  : visca is have 24 mask ID, from 0 to 23
 * @param x : horizonal +-0x50
 * @param y : vertical +-0x2d
 * @param width : width +-0x50
 * @param height : height +-0x2d
 * @param nn : if 0 ,this mask is used one time after reset setting, if 1, setting is record
 * @return
 */

int OemModuleHead::maskSetNoninterlock(uint maskID, int x, int y, int width, int height)
{
	if((maskID > 0x17) || (width > 0x50) || (width < -0x50) ||
	   (height > 0x2D) || (height < -0x2D)||
	   (x > 0x50) || (x < -0x50) ||
	   (y > 0x2D) || (y < -0x2D))
		return -ENODATA;
	const uchar cmd[] = {0x81, 0x01, 0x04, 0x6F, maskID & 0xFF,
						 (x & 0xF0) >> 4, x & 0x0F,
						 (y & 0xF0) >> 4, y & 0x0F,
						 (uchar(width) & 0xF0) >> 4, uchar(width) & 0x0F,
						 (uchar(height) & 0xF0) >> 4, uchar(height) & 0x0F, 0xFF };
	return transport->send((const char *)cmd, sizeof(cmd));
}

int OemModuleHead::maskSetPTZ(uint maskID, int pan, int tilt, uint zoom)
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
	const uchar cmd[] = {0x81, 0x01, 0x04, 0x7B, maskID,
						 uchar((0xf00 & pan) >> 8), uchar((0xf0 & pan) >> 4), uchar(0xf & pan),
						 uchar((0xf00 & tilt) >> 8), uchar((0xf0 & tilt) >> 4), uchar(0xf & tilt),
						 uchar((0xf000 & zoom) >> 16),uchar((0xf00 & zoom) >> 8), uchar((0xf0 & zoom) >> 4),
						 uchar((0xf & zoom)), 0xFF};
	return transport->send((const char *)cmd, sizeof(cmd));
}

int OemModuleHead::maskSetPanTiltAngle(int pan, int tilt)
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

	const uchar cmd[] = {0x81, 0x01, 0x04, 0x79,
						 uchar((0xf00 & pan) >> 8), uchar((0xf0 & pan) >> 4), uchar(0xf & pan),
						 uchar((0xf00 & tilt) >> 8), uchar((0xf0 & tilt) >> 4), uchar((0xf & tilt)), 0xFF};
	return transport->send((const char *)cmd, sizeof(cmd));
}

int OemModuleHead::maskSetRanges(int panMax, int panMin, int xMax, int xMin, int tiltMax, int tiltMin, int yMax, int yMin, bool hConvert, bool vConvert)
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

int OemModuleHead::maskDisplay(uint maskID, bool onOff)
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
	const uchar cmd[] = {0x81, 0x01, 0x04, 0x77, (maskBits & 0xff000000) >> 24,
						 (maskBits & 0xff0000) >> 16, (maskBits & 0xff00) >> 8,
						 (maskBits & 0xff), 0xff };
	return transport->send((const char *)cmd, sizeof(cmd));
}

void OemModuleHead::updateMaskPosition()
{
//	vGetZoom();
//	if (!lastV.panTitlSupport)
//		return;
//	sGetPos();
//	if (maskBits)
//		maskSetPanTiltAngle(lastV.position.first, lastV.position.second);
}

/**
 * @brief OemModuleHead::maskColor
 * @param maskID
 * @param color_0
 * @param color_1
 * @param colorChoose
 * @return
 * [china dome is not supported]
 */

int OemModuleHead::maskColor(uint maskID, OemModuleHead::MaskColor color_0, OemModuleHead::MaskColor color_1, bool colorChoose)
{
	uint maskBits = 0;
	if (colorChoose == true)
		maskBits |= (1 << maskID);
	else
		maskBits &= ~(1 << maskID);
	const uchar cmd[] = {0x81, 0x01, 0x04, 0x78, (maskBits & 0xff000000) >> 24,
						 (maskBits & 0xff0000) >> 16, (maskBits & 0xff00) >> 8,
						 (maskBits & 0xff),(((int)color_0) & 0xff),
						 (((int)color_1) & 0xff), 0xff };
	return transport->send((const char *)cmd, sizeof(cmd));
}

/**
 * @brief OemModuleHead::maskGrid
 * @param onOffCenter
 * [china dome is not supported]
 * @return
 */

int OemModuleHead::maskGrid(OemModuleHead::MaskGrid onOffCenter)
{
	const uchar cmd[] = {0x81, 0x01, 0x04, 0x7C, onOffCenter, 0xff };
	return transport->send((const char *)cmd, sizeof(cmd));
}

int OemModuleHead::setShutterLimit(uint topLim, uint botLim)
{
	const uchar cmd[] = { 0x81, 0x01, 0x04, 0x40, 0x06,
						  uchar((topLim & 0xf00) >> 8), uchar((topLim & 0xf0) >> 4), uchar(topLim & 0xf),
						  uchar((botLim & 0xf00) >> 8), uchar((botLim & 0xf0) >> 4), uchar(botLim & 0xf),
						  0xff };
	setRegister(R_TOP_SHUTTER, topLim);
	setRegister(R_BOT_SHUTTER, botLim);
	return transport->send((const char *)cmd, sizeof(cmd));
}

QString OemModuleHead::getShutterLimit()
{
	QString mes = QString("%1,%2").arg(getProperty(R_TOP_SHUTTER)).arg(getProperty(R_BOT_SHUTTER));
	return mes;
}

int OemModuleHead::setIrisLimit(uint topLim, uint botLim)
{
	const uchar cmd[] = { 0x81, 0x01, 0x04, 0x40, 0x07,
						  uchar((topLim & 0xf00) >> 8), uchar((topLim & 0xf0) >> 4), uchar(topLim & 0xf),
						  uchar((botLim & 0xf00) >> 8), uchar((botLim & 0xf0) >> 4), uchar(botLim & 0xf),
						  0xff };
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
	const uchar cmd[] = { 0x81, 0x01, 0x04, 0x40, 0x05,
						  topLim >> 4, topLim & 0xf,
						  botLim >> 4, botLim & 0xf, 0xff };
	setRegister(R_TOP_GAIN, topLim);
	setRegister(R_BOT_GAIN, botLim);
	return transport->send((const char *)cmd, sizeof(cmd));
}

QString OemModuleHead::getGainLimit()
{
	QString mes = QString("%1,%2").arg(getProperty(R_TOP_GAIN)).arg(getProperty(R_BOT_GAIN));
	return mes;
}

static uint checksum(const uchar *cmd, uint lenght)
{
	unsigned int sum = 0;
	for (uint i = 1; i < lenght; i++)
		sum += cmd[i];
	return sum & 0xff;
}

int OemModuleHead::setIRLed(int led)
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

int OemModuleHead::getIRLed()
{
	return irLedLevel;
}
