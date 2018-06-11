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
	//	C_VISCA_SET_DIGI_ZOOM_STAT,	//28 //td:nd
	//	C_VISCA_SET_ZOOM_TYPE,		//29 //td:nd
	C_VISCA_SET_FOCUS_MODE,		//30 //td:nd
	C_VISCA_SET_ZOOM_TRIGGER,	//31 //td:nd
	C_VISCA_SET_BLC_STATUS,		//32 //td:nd
	C_VISCA_SET_IRCF_STATUS,	//33 //td:nd
	C_VISCA_SET_AUTO_ICR,		//34 //td:nd
	C_VISCA_SET_PROGRAM_AE_MODE,//35 //td:nd
	C_VISCA_SET_FLIP_MODE,		//36
	C_VISCA_SET_MIRROR_MODE,	//37
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
	{0x09, 0x07, 0x81, 0x01, 0x04, 0x4b, 0x00, 0x00, 0x00, 0x0f, 0xff},	//set_exposure_value
	{0x09, 0x07, 0x81, 0x01, 0x04, 0x4c, 0x00, 0x00, 0xf0, 0x0f, 0xff },	//set gain value
	{0x06, 0x07, 0x81, 0x01, 0x04, 0x3E,0x00, 0xff },	//set exp comp mode
	{0x09, 0x07, 0x81, 0x01, 0x04, 0x4E, 0x00, 0x00, 0xf0, 0x0f, 0xff },	//set exp comp val
	{0x06, 0x04, 0x81, 0x01, 0x04, 0x2c, 0x0f, 0xff },	//set gain limit
	{0x09, 0x07, 0x81, 0x01, 0x04, 0x4a, 0x00, 0x00, 0x00, 0x0f, 0xff },//set shutter speed
	{0x06, 0x07, 0x81, 0x01, 0x04, 0x53, 0x0f, 0xff },	//set noise reduct
	{0x06, 0x04, 0x81, 0x01, 0x04, 0x3d, 0x00, 0xff },	// set wdr stat
	{0x06, 0x04, 0x81, 0x01, 0x04, 0x5b, 0x0f, 0xff },	//set gamma
	{0x06, 0x04, 0x81, 0x01, 0x04, 0x35, 0x00, 0xff },	//set awb mode
	{0x07, 0x05, 0x81, 0x01, 0x04, 0x37, 0x00, 0x00, 0xff },	//set defog mode
	//	digizoom
	//	zoom type
	{0x06, 0x04, 0x81, 0x01, 0x04, 0x38, 0x00, 0xff },	//set focus mode
	{0x06, 0x04, 0x81, 0x01, 0x04, 0x57, 0x00, 0xFF },	//set zoom trigger
	{0x06, 0x04, 0x81, 0x01, 0x04, 0x33, 0x00, 0xff },//se BLC stat
	{0x06, 0x04, 0x81, 0x01, 0x04, 0x01, 0x00, 0xff },//set IRCF stst
	{0x06, 0x04, 0x81, 0x01, 0x04, 0x51, 0x00, 0xff },//set Auto ICR
	{0x06, 0x04, 0x81, 0x01, 0x04, 0x39, 0x00, 0xff },//set program AE mode
	{0x06, 0x04, 0x81, 0x01, 0x04, 0x66, 0x00, 0xff }, //set flip mode
	{0x06, 0x04, 0x81, 0x01, 0x04, 0x61, 0x00, 0xff },//set Mirror mode
	{0x05, 0x07, 0x81, 0x09, 0x04, 0x47, 0xff},
	{0x06, 0x07, 0x81, 0x01, 0x04, 0x07, 0x20, 0xff},
	{0x06, 0x07, 0x81, 0x01, 0x04, 0x07, 0x30, 0xff},
	{0x06, 0x07, 0x81, 0x01, 0x04, 0x07, 0x00, 0xff},
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
	return getRegister(C_VISCA_GET_ZOOM);
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

