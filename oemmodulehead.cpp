#include "oemmodulehead.h"
#include "debug.h"
#include "ptzptransport.h"
#include "ioerrors.h"

#include <QList>
#include <QMutex>
#include <QMutexLocker>

#include <errno.h>

#define dump() \
	for (int i = 0; i < len; i++) \
		qDebug("%s: %d: 0x%x", __func__, i, p[i]);

enum Commands {
	/* visca commands */
	C_VISCA_SET_EXPOSURE,		//14 //td:nd // exposure_value
	C_VISCA_SET_GAIN,			//15 //td:nd // gainvalue
	C_VISCA_SET_ZOOM_POS,
	C_VISCA_SET_EXP_COMPMODE,	//16 //td:nd
	C_VISCA_SET_EXP_COMPVAL,	//17 //td:nd
	C_VISCA_SET_GAIN_LIM,		//18 //td:nd
	C_VISCA_SET_SHUTTER,		//19 //td:nd // getshutterspeed
	//	C_VISCA_SET_MIN_SHUTTER,	//20 //td:nd
	//	C_VISCA_SET_MIN_SHUTTER_LIM,//21 //td:nd
	C_VISCA_SET_NOISE_REDUCT,	//22 //td:nd
	C_VISCA_SET_WDRSTAT,		//23 //td:nd
	//	C_VISCA_SET_WDRPARAM,		//24 //td:nd
	C_VISCA_SET_GAMMA,			//25 //td.nd
	C_VISCA_SET_AWB_MODE,		//26 //td:nd
	C_VISCA_SET_DEFOG_MODE,		//27 //td:nd
	C_VISCA_SET_DIGI_ZOOM_STAT,	//28 //td:nd
	C_VISCA_SET_ZOOM_TYPE,		//29 //td:nd
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
	//	C_VISCA_GET_MIN_SHUTTER,	//20 //td:nd
	//	C_VISCA_GET_MIN_SHUTTER_LIM,//21 //td:nd
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

	//	C_VISCA_SET_EXPOSURE_TARGET,//19 //td:nd

	C_COUNT,
};

#define MAX_CMD_LEN	16

static unsigned char protoBytes[C_COUNT][MAX_CMD_LEN] = {
	/* visca commands */
	{0x09, 0x00, 0x81, 0x01, 0x04, 0x4b, 0x00, 0x00, 0x00, 0x0f, 0xff},	//set_exposure_value
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

protected:
	QMutex m;
	QList<Commands> commands;
};

OemModuleHead::OemModuleHead()
{
	hist = new CommandHistory;
	nextSync = C_COUNT;
	syncEnabled = true;
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
	};
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
	unsigned char *p = protoBytes[C_VISCA_ZOOM_IN];
	hist->add(C_VISCA_ZOOM_IN);
	p[4 + 2] = 0x20 + speed;
	return transport->send((const char *)p + 2, p[0]);
}

int OemModuleHead::startZoomOut(int speed)
{
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
}

int OemModuleHead::syncNext()
{
	const unsigned char *p = protoBytes[nextSync];
	hist->add((Commands)nextSync);
	return transport->send((const char *)p + 2, p[0]);
}

int OemModuleHead::dataReady(const unsigned char *bytes, int len)
{
	const unsigned char *p = bytes;
	if (p[0] != 0x90)
		return -1;
	/* sendcmd can be C_COUNT, beware! */
	Commands sendcmd = hist->takeFirst();
	if (sendcmd == C_COUNT) {
		setIOError(IOE_NO_LAST_WRITTEN);
		return -1;
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
		ffDebug() << "acknowledge";
		return 3;
	}
	if (len < expected)
		return -1;
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
		setRegister(R_EXPOSURE_VALUE , exVal);
	} else if(sendcmd == C_VISCA_GET_GAIN) {
		setRegister(R_GAIN_VALUE , (p[4] << 4 | p[5]));
	} else if(sendcmd == C_VISCA_GET_EXP_COMPMODE){
		setRegister(R_EXP_COMPMODE , (p[2] == 0x02));
	} else if(sendcmd == C_VISCA_GET_EXP_COMPVAL){
		setRegister(R_EXP_COMPVAL , ((p[4] << 4) + p[5]));
	} else if(sendcmd == C_VISCA_GET_GAIN_LIM){
		setRegister(R_GAIN_LIM , (p[2] - 4));
	} else if(sendcmd == C_VISCA_GET_SHUTTER){
		setRegister(R_SHUTTER , (p[4] << 4 | p[5]));
	} else if(sendcmd == C_VISCA_GET_NOISE_REDUCT){
		setRegister(R_NOISE_REDUCT , p[2]);
	} else if(sendcmd == C_VISCA_GET_WDRSTAT){
		setRegister(R_WDRSTAT , (p[2] == 0x02));
	} else if(sendcmd == C_VISCA_GET_GAMMA){
		setRegister(R_GAMMA , p[2]);
	} else if(sendcmd == C_VISCA_GET_AWB_MODE){
		char val = p[2];
		if (val > 0x03)
			val -= 1;
		setRegister(R_AWB_MODE , val);
	} else if(sendcmd == C_VISCA_GET_DEFOG_MODE){
		setRegister(R_DEFOG_MODE , (p[2] == 0x02));
	} else if(sendcmd == C_VISCA_GET_DIGI_ZOOM_STAT){
		setRegister(R_DIGI_ZOOM_STAT,(p[2] == 0x02) ? true : false);
	} else if(sendcmd == C_VISCA_GET_ZOOM_TYPE){
		setRegister(R_ZOOM_TYPE,p[2]);
	} else if(sendcmd == C_VISCA_GET_FOCUS_MODE){
		setRegister(R_FOCUS_MODE,p[2]);
	} else if(sendcmd == C_VISCA_GET_ZOOM_TRIGGER){
		if (p[2] == 0x01)
			setRegister(R_ZOOM_TRIGGER,0);
		else if (p[2] == 0x02)
			setRegister(R_ZOOM_TRIGGER,1);
	} else if(sendcmd == C_VISCA_GET_BLC_STATUS){
		setRegister(R_BLC_STATUS,(p[2] == 0x02) ? true : false);
	} else if(sendcmd == C_VISCA_GET_IRCF_STATUS){
		setRegister(R_IRCF_STATUS,(p[2] == 0x02) ? true : false);
	} else if(sendcmd == C_VISCA_GET_AUTO_ICR){
		setRegister(R_AUTO_ICR,(p[2] == 0x02) ? true : false);
	} else if(sendcmd == C_VISCA_GET_PROGRAM_AE_MODE){
		setRegister(R_PROGRAM_AE_MODE,p[2]);
	} else if(sendcmd == C_VISCA_GET_ZOOM){
		uint value =
				((p[2] & 0x0F) << 12) +
				((p[3] & 0x0F) << 8) +
				((p[4] & 0x0F) << 4) +
				((p[5] & 0x0F) << 0);
		setRegister(R_ZOOM_POS, value);
	} else if(sendcmd == C_VISCA_GET_FLIP_MODE){
		setRegister(R_FLIP,(p[2] == 0x02) ? true : false);
	} else if(sendcmd == C_VISCA_GET_MIRROR_MODE){
		setRegister(R_MIRROR,(p[2] == 0x02) ? true : false);
	}

	if(getRegister(R_ZOOM_POS) >= 16384){
		setRegister(R_DIGI_ZOOM_POS,getRegister(R_ZOOM_POS)-16384);
		setRegister(R_OPTIC_ZOOM_POS,16384);
	} else if (getRegister(R_ZOOM_POS) < 16384){
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
	if (syncEnabled && syncTime.elapsed() > 40) {
		syncTime.restart();
		const unsigned char *p = protoBytes[C_VISCA_GET_ZOOM];
		hist->add(C_VISCA_GET_ZOOM);
		return QByteArray((const char *)p + 2, p[0]);
	}
	return QByteArray();
}
void OemModuleHead::setProperty(int r,uint x)
{
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
	} else if (r == 22){
		setRegister(R_PT_SPEED, (int)x);
	} else if (r == 23){
		setRegister(R_ZOOM_SPEED, (int)x);
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
