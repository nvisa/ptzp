#include "oemmodulehead.h"
#include "debug.h"
#include "ioerrors.h"
#include "ptzptransport.h"
#include "simplemetrics.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMutex>
#include <QMutexLocker>

#include <hardwareoperations.h>

#include <errno.h>
#include <unistd.h>

#define dump()                                                                 \
	for (int i = 0; i < len; i++)                                              \
		qDebug("%s: %d: 0x%x", __func__, i, p[i]);

enum Modules {
	SONY_FCB_CV7500 = 0x0641,
	OEM = 0x045F,
	POWERVIEW_36X = 0x10C25b,
};

enum Commands {
	/* visca commands */
	C_VISCA_SET_EXPOSURE,			// 0x00~0x13
	C_VISCA_SET_EXPOSURE_TARGET,	// 0x00~0xFF [oem]
	C_VISCA_SET_GAIN,				// 0x00~0x0F
	C_VISCA_SET_ZOOM_POS,			//
	C_VISCA_SET_EXP_COMPMODE,		// 1:On, 0:Off
	C_VISCA_SET_EXP_COMPVAL,		// 0x00~0x0E
	C_VISCA_SET_GAIN_LIM,			// 0x00~0x0b [sony]
	C_VISCA_SET_SHUTTER,			// 0x00~0x15
	C_VISCA_SET_NOISE_REDUCT,		// 0x00~0x05
	C_VISCA_SET_WDRSTAT,			// 1:On, 0:Off
	C_VISCA_SET_GAMMA,				// 0x00~0x01[sony] 0x00~0x05[oem]
	C_VISCA_SET_AWB_MODE,			// 0x00~0x09
	C_VISCA_SET_DEFOG_MODE,			// 1:On, 0:Off
	C_VISCA_SET_DIGI_ZOOM_STAT,		// 1:On, 0:Off
	C_VISCA_SET_ZOOM_TYPE,			// 1:Combine 0:Seperated
	C_VISCA_SET_FOCUS,				// 0x20~0x27 Far 0x30~0x37 Near
	C_VISCA_SET_FOCUS_MODE,			// 1:Manual, 0:Auto
	C_VISCA_SET_ZOOM_TRIGGER,		// 0:Interval, 1:Zoom Trigger
	C_VISCA_SET_BLC_STATUS,			// 1:On, 0:Off
	C_VISCA_SET_IRCF_STATUS,		// 1:On, 0:Off
	C_VISCA_SET_AUTO_ICR,			// 1:On, 0:Off
	C_VISCA_SET_PROGRAM_AE_MODE,	// 0:Auto 3:manual 10:shutterP 11:irisP 13:Brigth
	C_VISCA_SET_FLIP_MODE,			// 1:On, 0:Off
	C_VISCA_SET_MIRROR_MODE,		// 1:On, 0:Off
	C_VISCA_SET_ONE_PUSH,			//
	C_VISCA_ZOOM_IN,				// 0x00~0x07
	C_VISCA_ZOOM_OUT,				// 0x00~0x07
	C_VISCA_ZOOM_STOP,				//
	C_VISCA_SET_GAIN_LIMIT_OEM,		// upper << 8 & lower
	C_VISCA_SET_SHUTTER_LIMIT_OEM,	// upper << 8 & lower
	C_VISCA_SET_IRIS_LIMIT_OEM,		// upper << 8 & lower
	C_VISCA_GET_ZOOM,

	C_VISCA_GET_EXPOSURE,
	C_VISCA_GET_GAIN,
	C_VISCA_GET_EXP_COMPMODE,
	C_VISCA_GET_EXP_COMPVAL,
	C_VISCA_GET_GAIN_LIM,
	C_VISCA_GET_SHUTTER,
	C_VISCA_GET_NOISE_REDUCT,
	C_VISCA_GET_WDRSTAT,
	C_VISCA_GET_GAMMA,
	C_VISCA_GET_AWB_MODE,
	C_VISCA_GET_DEFOG_MODE,
	C_VISCA_GET_DEFOG_MODE_OEM,
	C_VISCA_GET_DIGI_ZOOM_STAT,
	C_VISCA_GET_ZOOM_TYPE,
	C_VISCA_GET_FOCUS_MODE,
	C_VISCA_GET_ZOOM_TRIGGER,
	C_VISCA_GET_BLC_STATUS,
	C_VISCA_GET_IRCF_STATUS,
	C_VISCA_GET_AUTO_ICR,
	C_VISCA_GET_PROGRAM_AE_MODE,
	C_VISCA_GET_FLIP_MODE,
	C_VISCA_GET_MIRROR_MODE,

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

class SimpleStatistics
{
public:
	SimpleStatistics()
	{
		reset();
	}

	void reset()
	{
		total = 0;
		min = INT_MAX;
		max = 0;
		count = 0;
	}

	void add()
	{
		if (count == -1) {
			t.start();
			count = 0;
			return;
		}
		count++;
		int value = t.restart();
		total += value;
		if (value < min)
			min = value;
		if (value > max)
			max = value;
		if (count == 100) {
			reset();
			count = 100;
		}
		fDebug("samples=%d min=%d max=%d avg=%lf", count, min, max, double(total) / count);
	}

protected:
	qint64 total;
	int min;
	int max;
	int count;
	QElapsedTimer t;
};

class StationaryFilter
{
public:
	StationaryFilter()
	{

	}

	bool check(int value)
	{
		cache << value;
		if (cache.size() > 10)
			cache.removeFirst();
		bool allSame = true;
		for (int i = 1; i < cache.size(); i++) {
			if (cache[i] != cache[i - 1]) {
				allSame = false;
				break;
			}
		}
		if (!allSame) {
			cache.clear();
			return false;
		}
		return true;
	}
protected:
	QList<int> cache;
};

class ErrorRateChecker
{
public:
	ErrorRateChecker()
	{

	}

	float add(int value)
	{
		int target = updateCache(value);
		auto s = stats[target];
		s.count++;
		if (value != target)
			s.wrong++;
		stats[target] = s;
		if (value == target)
			return s.wrong * 1.0 / s.count;
		return 0;
	}

protected:
	struct Stats {
		int count;
		int wrong;
	};

	int updateCache(int value)
	{
		cache << value;
		if (cache.size() > 10)
			cache.removeFirst();
		/* find max entry */
		QHash<int, int> counts;
		foreach (int v, cache)
			counts[v]++;
		QHashIterator<int, int> hi(counts);
		int max = 0, maxv = 0;
		while (hi.hasNext()) {
			hi.next();
			if (hi.value() > max) {
				max = hi.value();
				maxv = hi.key();
			}
		}
		return maxv;
	}

	QList<int> cache;
	QHash<int, Stats> stats;
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
	/* [CR] [yca] Memory leak... */
	hist = new CommandHistory;
	nextSync = C_COUNT;
	syncEnabled = true;
	syncInterval = 40;
	syncTime.start();
	registersCache[R_IRCF_STATUS] = 0;
	registersCache[R_ZOOM_POS] = 0;
	zoomControls.zoomed = false;
	zoomControls.zoomFiltering = false;
	zoomControls.tiggerredRead = false;
	zoomControls.stationaryFiltering = false;
	zoomControls.errorRateCheck = false;
	zoomControls.readLatencyCheck = false;
	queryIndex = 0;
	settings = {
		{"exposure_value", {C_VISCA_SET_EXPOSURE, R_EXPOSURE_VALUE}},
		{"gain_value", {C_VISCA_SET_GAIN, R_GAIN_VALUE}},
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
		{"one_push_af", {C_VISCA_SET_ONE_PUSH, R_COUNT}},
		{"display_rotation", {-1, R_DISPLAY_ROT}},
		{"digi_zoom_pos", {-1, R_DIGI_ZOOM_POS}},
		{"optic_zoom_pos", {-1, R_OPTIC_ZOOM_POS}},
		{"focus", {C_VISCA_SET_FOCUS, R_COUNT}},
		{"cam_model", {-1, R_VISCA_MODUL_ID}}
	};
	setRegister(R_VISCA_MODUL_ID, 0);
	auto &m = SimpleMetrics::Metrics::instance();
	mp = m.addPoint("oemmodule");
	serialRecved = mp->addIntegerChannel("serial_recved");
	zoomRecved = mp->addIntegerChannel("zoom_recved");

	_mapCap = {
		{ptzp::PtzHead_Capability_EXPOSURE, {C_VISCA_SET_EXPOSURE, R_EXPOSURE_VALUE}},
		{ptzp::PtzHead_Capability_GAIN, {C_VISCA_SET_GAIN, R_GAIN_VALUE}},
		{ptzp::PtzHead_Capability_SHUTTER, {C_VISCA_SET_SHUTTER, R_SHUTTER}},
		{ptzp::PtzHead_Capability_NOISE_REDUCT, {C_VISCA_SET_NOISE_REDUCT, R_NOISE_REDUCT}},
		{ptzp::PtzHead_Capability_WDR, {C_VISCA_SET_WDRSTAT, R_WDRSTAT}},
		{ptzp::PtzHead_Capability_GAMMA, {C_VISCA_SET_GAMMA, R_GAMMA}},
//		{"awb_mode", {C_VISCA_SET_AWB_MODE, R_AWB_MODE}},
		{ptzp::PtzHead_Capability_DEFOG, {C_VISCA_SET_DEFOG_MODE, R_DEFOG_MODE}},
		{ptzp::PtzHead_Capability_DIGITAL_ZOOM, {C_VISCA_SET_DIGI_ZOOM_STAT, R_DIGI_ZOOM_STAT}},
//		{"zoom_type", {C_VISCA_SET_ZOOM_TYPE, R_ZOOM_TYPE}},
		{ptzp::PtzHead_Capability_FOCUS_MODE, {C_VISCA_SET_FOCUS_MODE, R_FOCUS_MODE}},
//		{"zoom_trigger", {C_VISCA_SET_ZOOM_TRIGGER, R_ZOOM_TRIGGER}},
		{ptzp::PtzHead_Capability_BACKLIGHT, {C_VISCA_SET_BLC_STATUS, R_BLC_STATUS}},
//		{"ircf_status", {C_VISCA_SET_IRCF_STATUS, R_IRCF_STATUS}},
//		{"auto_icr", {C_VISCA_SET_AUTO_ICR, R_AUTO_ICR}},
//		{"flip", {C_VISCA_SET_FLIP_MODE, R_FLIP}},
//		{"mirror", {C_VISCA_SET_MIRROR_MODE, R_MIRROR}},
		{ptzp::PtzHead_Capability_ONE_PUSH_FOCUS, {C_VISCA_SET_ONE_PUSH, R_COUNT}},
		{ptzp::PtzHead_Capability_VIDEO_ROTATE, {ptzp::PtzHead_Capability_VIDEO_ROTATE, R_DISPLAY_ROT}},
//		{"digi_zoom_pos", {-1, R_DIGI_ZOOM_POS}},
//		{"optic_zoom_pos", {-1, R_OPTIC_ZOOM_POS}},
//		{"cam_model", {-1, R_VISCA_MODUL_ID}}
	};
}

int OemModuleHead::addSpecialModulSettings()
{
	int module = getRegister(R_VISCA_MODUL_ID);
	if (module == SONY_FCB_CV7500) {
		settings.insert("exp_comp_mode", {C_VISCA_SET_EXP_COMPMODE, R_EXP_COMPMODE});
		settings.insert("exp_comp_val", {C_VISCA_SET_EXP_COMPVAL, R_EXP_COMPVAL});
		settings.insert("gain_limit", {C_VISCA_SET_GAIN_LIM, R_GAIN_LIM});
	} else if (module == OEM || module == POWERVIEW_36X) {
		settings.insert("exposure_target", {C_VISCA_SET_EXPOSURE_TARGET, R_EXPOSURE_TARGET});
		settings.insert("shutter_limit_bot", {C_VISCA_SET_SHUTTER_LIMIT_OEM, R_BOT_SHUTTER});
		settings.insert("shutter_limit_top", {C_VISCA_SET_SHUTTER_LIMIT_OEM, R_TOP_SHUTTER});
		settings.insert("iris_limit_bot", {C_VISCA_SET_IRIS_LIMIT_OEM, R_BOT_IRIS});
		settings.insert("iris_limit_top", {C_VISCA_SET_IRIS_LIMIT_OEM, R_TOP_IRIS});
		settings.insert("gain_limit_bot", {C_VISCA_SET_GAIN_LIMIT_OEM, R_BOT_GAIN});
		settings.insert("gain_limit_top", {C_VISCA_SET_GAIN_LIMIT_OEM, R_TOP_GAIN});
	}

	if (module == POWERVIEW_36X) {
		settings.remove("defog_mode");
	}
	return 0;
}

int OemModuleHead::addCustomSettings()
{
	settings.clear();
	settings = {
		{"exposure_value", {C_VISCA_SET_EXPOSURE, R_EXPOSURE_VALUE}},
		{"gain_value", {C_VISCA_SET_GAIN, R_GAIN_VALUE}},
		{"shutter", {C_VISCA_SET_SHUTTER, R_SHUTTER}},
		{"wdr_stat", {C_VISCA_SET_WDRSTAT, R_WDRSTAT}},
		{"awb_mode", {C_VISCA_SET_AWB_MODE, R_AWB_MODE}},
		{"auto_focus", {C_VISCA_SET_FOCUS_MODE, R_FOCUS_MODE}},
		{"blc_status", {C_VISCA_SET_BLC_STATUS, R_BLC_STATUS}},
		{"program_ae_mode", {C_VISCA_SET_PROGRAM_AE_MODE, R_PROGRAM_AE_MODE}},
		{"flip", {C_VISCA_SET_FLIP_MODE, R_FLIP}},
		{"mirror", {C_VISCA_SET_MIRROR_MODE, R_MIRROR}},
		{"one_push_af", {C_VISCA_SET_ONE_PUSH, R_COUNT}},
		{"display_rotation", {-1, R_DISPLAY_ROT}},
		{"optic_zoom_pos", {-1, R_OPTIC_ZOOM_POS}},
		{"focus", {C_VISCA_SET_FOCUS, R_COUNT}},
		{"cam_model", {-1, R_VISCA_MODUL_ID}}
	};
	return settings.size();
}

void OemModuleHead::fillCapabilities(ptzp::PtzHead *head)
{
	head->add_capabilities(ptzp::PtzHead_Capability_ZOOM);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_ZOOM);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_DAY_VIEW);
	head->add_capabilities(ptzp::PtzHead_Capability_FOCUS);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_FOCUS);
	head->add_capabilities(ptzp::PtzHead_Capability_BACKLIGHT);
	head->add_capabilities(ptzp::PtzHead_Capability_EXPOSURE);
	head->add_capabilities(ptzp::PtzHead_Capability_GAIN);
	head->add_capabilities(ptzp::PtzHead_Capability_SHUTTER);
	head->add_capabilities(ptzp::PtzHead_Capability_DEFOG);
	head->add_capabilities(ptzp::PtzHead_Capability_VIDEO_ROTATE);
	head->add_capabilities(ptzp::PtzHead_Capability_ONE_PUSH_FOCUS);
	head->add_capabilities(ptzp::PtzHead_Capability_NOISE_REDUCT);
	head->add_capabilities(ptzp::PtzHead_Capability_GAMMA);
	head->add_capabilities(ptzp::PtzHead_Capability_DIGITAL_ZOOM);
	head->add_capabilities(ptzp::PtzHead_Capability_FOCUS_MODE);
	head->add_capabilities(ptzp::PtzHead_Capability_WDR);

	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_DETECTION);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_TRACKING);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_AUTO_TRACK_DETECTION);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_CHANGE_DETECTION);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_MULTI_ROI);
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
	zoomControls.zoomed = true;
	unsigned char *p = protoBytes[C_VISCA_ZOOM_IN];
	p[4 + 2] = 0x20 + speed;
	return transport->send((const char *)p + 2, p[0]);
}

int OemModuleHead::startZoomOut(int speed)
{
	zoomSpeed = speed;
	zoomControls.zoomed = true;
	unsigned char *p = protoBytes[C_VISCA_ZOOM_OUT];
	p[4 + 2] = 0x30 + speed;
	return transport->send((const char *)p + 2, p[0]);
}

int OemModuleHead::stopZoom()
{
	zoomControls.zoomed = false;
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
		else if (getRegister(R_VISCA_MODUL_ID) == POWERVIEW_36X && modulFlag == 2) {
			if (nextSync != C_VISCA_GET_DEFOG_MODE) // Unsupported
				break;
		} else if (getRegister(R_VISCA_MODUL_ID) == OEM && modulFlag == 2)
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

	if (len < expected || len < 3)
		return -EAGAIN;

	if ((p[1] == 0x41 || p[1] == 0x51) && p[2] == 0xFF) {
		mLogv("acknowledge");
		return 3;
	} else if ((p[1] & 0x60) == 0x60) {
		// Visca error messages
		QByteArray ba = "Unknown";
		int errlen = 0;
		if (p[1] == 0x60 && p[2] == 0xff) {
			if (p[2] == 0x02)
				ba = "Syntax Error";
			if (p[2] == 0x03)
				ba = "Command buffer full";
			errlen = 3;
		} else if (p[1] == 0x61 && len < 4) {
			return -EAGAIN;
		} else if (p[1] == 0x61 && p[3] == 0xff) {
			if (p[2] == 0x04)
				ba = "Command Canceled";
			else if (p[2] == 0x05)
				ba = "No Socket";
			else if (p[2] == 0x41)
				ba = "Command not Excutable";
			errlen = 4;
		}

		if (errlen) {
			mDebug("Visca Error: %s [err: 0x%x]", qPrintable(ba), p[2]);
			hist->takeFirst();
			if (getHeadStatus() == ST_SYNCING) {
				// Last command should resend while syncing.
				syncNext();
			}
			return errlen;
		}
	} else if (expected == 0x00) {
		hist->takeFirst();
		return -EAGAIN;
	}

	hist->takeFirst();

	/* messages errors */
	IOErr err = IOE_NONE;
	int rtnlen = 0;
	if (sptr[2] != 0x81) {
		// Handle commands that not visca
		err = IOE_VISCA_INVALID_ADDR;
		rtnlen = expected;
	} else if (p[expected - 1] != 0xff) {
		if (getHeadStatus() == ST_SYNCING) {
			// Last command should resend while syncing.
			syncNext();
		}
		err = IOE_VISCA_LAST;
		rtnlen = -ENOENT;
	}

	if (err) {
		setIOError(err);
		return rtnlen;
	}

	pingTimer.restart();
	/* register sync support */
	if (getHeadStatus() == ST_SYNCING) {
		/* we are in sync mode, let's sync next */
		mInfo("Next sync property: %d", nextSync);
		if (++nextSync == C_COUNT) {
			fDebug("Visca register syncing completed, activating auto-sync");
		} else
			syncNext();
	}

	serialRecved->push(sendcmd);

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
		setRegister(R_ZOOM_TYPE, (p[2] == 0x0));
	} else if (sendcmd == C_VISCA_GET_FOCUS_MODE) {
		mInfo("Focus mode synced");
		setRegister(R_FOCUS_MODE, (p[2] == 0x03));
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
		if (p[2] != 0x02 && p[2] != 0x03) {
			mDebug("Ircf Status Error. %s", qPrintable(QByteArray((char*)bytes, len).toHex()));
			return expected;
		}
		if (registersCache[R_IRCF_STATUS] == p[2])
			setRegister(R_IRCF_STATUS, (p[2] == 0x02) ? true : false);
		registersCache[R_IRCF_STATUS] = p[2];
	} else if (sendcmd == C_VISCA_GET_AUTO_ICR) {
		mInfo("Auto Icr synced");
		setRegister(R_AUTO_ICR, (p[2] == 0x02) ? true : false);
	} else if (sendcmd == C_VISCA_GET_PROGRAM_AE_MODE) {
		mInfo("Program AE Mode synced");
		setRegister(R_PROGRAM_AE_MODE, p[2]);
	} else if (sendcmd == C_VISCA_GET_ZOOM) {
		if (p[1] != 0x50) {
			mInfo("Zoom response err: wrong mesg[%s]",
				  QByteArray((char *)p, len).toHex().data());
			return expected;
		}

		/* check first byte error on zoom response */
		if ((p[2] | p[3] | p[4] | p[5]) & 0xF0)
			return expected;

		uint regValue = getRegister(R_ZOOM_POS);
		uint value = ((p[2] & 0x0F) << 12) | ((p[3] & 0x0F) << 8) |
					 ((p[4] & 0x0F) << 4) | ((p[5] & 0x0F) << 0);

		/* check maximum limit value */
		if (value > 0x7AC0) {
			mInfo("Zoom response err: big value [%d]", value);
			return expected;
		}

		if (zoomControls.zoomFiltering) {
			int oldZoomValue = registersCache[R_ZOOM_POS];
			registersCache[R_ZOOM_POS] = value;
			if (oldZoomValue * 1.1 < value || value < oldZoomValue * 0.9) {
				if (regValue * 1.1 < value || value < regValue * 0.9) {
					oldZoomValue = value;
					mInfo("Zoom response err: value peak [%d]", value);
					return expected;
				}
			}
		}

		if (zoomControls.stationaryFiltering && !zoomControls.sfilter->check(value))
			return expected;

		zoomRecved->push((int)value);
		mLogv("Zoom Position synced");
		setRegister(R_ZOOM_POS, value);
		if (zoomControls.errorRateCheck) {
			float er = zoomControls.echeck->add(value);
			qDebug("Error rate for %d is %f", value, er);
		}
		if (zoomControls.readLatencyCheck)
			zoomControls.stats->add();

		/* check digital zoom */
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
			addSpecialModulSettings();
		} else if (p[1] == 0x50 && p[2] == 0x00 && p[3] == 0x10) {
			setRegister(R_VISCA_MODUL_ID, (uint(p[3]) << 16) + (uint(p[4]) << 8) + p[5]);
			addSpecialModulSettings();
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
		if (zoomControls.tiggerredRead && zoomControls.zoomed == false)
			return QByteArray();
		mLogv("Syncing zoom positon");
		syncTime.restart();
		Commands q = queryPatternList[queryIndex++];
		if (queryIndex >= sizeof(queryPatternList) / sizeof(queryPatternList[0]))
			queryIndex = 0;
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
	int value = 0;
	value = (root.value(key.arg(R_IRCF_STATUS)).toInt() > 0) ? 1:0;
	setProperty(C_VISCA_SET_IRCF_STATUS, value);
	usleep(sleepDur);

	value =	root.value(key.arg(R_NOISE_REDUCT)).toInt();
	setProperty(C_VISCA_SET_NOISE_REDUCT, value);
	usleep(sleepDur);

	value = (root.value(key.arg(R_WDRSTAT)).toInt() > 0) ? 1:0;
	setProperty(C_VISCA_SET_WDRSTAT, value);
	usleep(sleepDur);

	value = root.value(key.arg(R_AWB_MODE)).toInt();
	setProperty(C_VISCA_SET_AWB_MODE, value);
	usleep(sleepDur);

	value = root.value(key.arg(R_GAMMA)).toInt();
	setProperty(C_VISCA_SET_GAMMA, value);
	usleep(sleepDur);

	value = (root.value(key.arg(R_BLC_STATUS)).toInt() > 0) ? 1:0;
	setProperty(C_VISCA_SET_BLC_STATUS, value);
	usleep(sleepDur);

	value = (root.value(key.arg(R_DEFOG_MODE)).toInt() > 0) ? 1:0;
	setProperty(C_VISCA_SET_DEFOG_MODE, value);
	usleep(sleepDur);

	value = (root.value(key.arg(R_ZOOM_TYPE)).toInt() > 0) ? 1:0;
	setProperty(C_VISCA_SET_ZOOM_TYPE, value);
	usleep(sleepDur);

	value = (root.value(key.arg(R_DIGI_ZOOM_STAT)).toInt() > 0) ? 1:0;
	setProperty(C_VISCA_SET_DIGI_ZOOM_STAT, value);
	usleep(sleepDur);

	if (getRegister(R_VISCA_MODUL_ID) == SONY_FCB_CV7500) {
		// SONY Limits
		value = root.value(key.arg(R_GAIN_LIM)).toInt();
		setProperty(C_VISCA_SET_GAIN_LIM, value);
		usleep(sleepDur);

		value = (root.value(key.arg(R_EXP_COMPMODE)).toInt() > 0) ? 1:0;
		setProperty(C_VISCA_SET_EXP_COMPMODE, value);
		usleep(sleepDur);

		value = root.value(key.arg(R_EXP_COMPVAL)).toInt();
		setProperty(C_VISCA_SET_EXP_COMPVAL, value);
		usleep(sleepDur);
	} else if (getRegister(R_VISCA_MODUL_ID) == OEM) {
		// OEM Limits
		uint exposure_target = root.value(key.arg(R_EXPOSURE_TARGET)).toInt();
		if (exposure_target == 0)
			exposure_target = 110;
		setProperty(C_VISCA_SET_EXPOSURE_TARGET, exposure_target);
		usleep(sleepDur);

		uint gainLimit = (root.value(key.arg(R_TOP_GAIN)).toInt() << 8) +
						 root.value(key.arg(R_BOT_GAIN)).toInt();
		if (gainLimit == 0) {
			gainLimit = 0x7000; // 112,0
		}
		setProperty(C_VISCA_SET_GAIN_LIMIT_OEM, gainLimit);
		usleep(sleepDur);

		uint shutterLimit = (root.value(key.arg(R_TOP_SHUTTER)).toInt() << 12) +
							root.value(key.arg(R_BOT_SHUTTER)).toInt();
		if (shutterLimit == 0) {
			shutterLimit = 0x45f000; // 1119,0
		}
		setProperty(C_VISCA_SET_SHUTTER_LIMIT_OEM, shutterLimit);
		usleep(sleepDur);

		uint irisLimit = (root.value(key.arg(R_TOP_IRIS)).toInt() << 12) +
						 root.value(key.arg(R_BOT_IRIS)).toInt();
		if (irisLimit == 0) {
			irisLimit = 0x2BC000; // 700,0
		}
		setProperty(C_VISCA_SET_IRIS_LIMIT_OEM, irisLimit);
	}

	uint ae_mode = root.value(key.arg(R_PROGRAM_AE_MODE)).toInt();
	setProperty(C_VISCA_SET_PROGRAM_AE_MODE, ae_mode);
	sleep(1);

	if (ae_mode == 0x0a) {
		// Shutter priority
		value = root.value(key.arg(R_SHUTTER)).toInt();
		setProperty(C_VISCA_SET_SHUTTER, value);
		usleep(sleepDur);
	} else if (ae_mode == 0x0b) {
		// Iris priority
		value = root.value(key.arg(R_EXPOSURE_VALUE)).toInt();
		setProperty(C_VISCA_SET_EXPOSURE, value);
		usleep(sleepDur);
	} else if (ae_mode == 0x03) {
		// Manuel
		value = root.value(key.arg(R_SHUTTER)).toInt();
		setProperty(C_VISCA_SET_SHUTTER, value);
		usleep(sleepDur);

		value = root.value(key.arg(R_EXPOSURE_VALUE)).toInt();
		setProperty(C_VISCA_SET_EXPOSURE, value);
		usleep(sleepDur);

		value = root.value(key.arg(R_GAIN_VALUE)).toInt();
		setProperty(C_VISCA_SET_GAIN, value);
		usleep(sleepDur);
	}

	value = (root.value(key.arg(R_AUTO_ICR)).toInt() > 0) ? 1:0;
	setProperty(C_VISCA_SET_AUTO_ICR, value);
	usleep(sleepDur);

	value = (root.value(key.arg(R_FOCUS_MODE)).toInt() > 0) ? 1:0;
	setProperty(C_VISCA_SET_FOCUS_MODE, value);
	usleep(sleepDur);

	value = (root.value(key.arg(R_ZOOM_TRIGGER)).toInt() > 0) ? 1:0;
	setProperty(C_VISCA_SET_ZOOM_TRIGGER, value);
	usleep(sleepDur);
	setProperty(C_VISCA_SET_ZOOM_TRIGGER, value);
	usleep(sleepDur);

	value = (root.value(key.arg(R_FLIP)).toInt() > 0) ? 1:0;
	setProperty(C_VISCA_SET_FLIP_MODE, value);
	usleep(sleepDur);

	value = (root.value(key.arg(R_MIRROR)).toInt() > 0) ? 1:0;
	setProperty(C_VISCA_SET_MIRROR_MODE, value);
	usleep(sleepDur);

	setZoom(root.value(key.arg(R_ZOOM_POS)).toInt());
	usleep(sleepDur);
	deviceDefinition = root.value("deviceDefiniton").toString();
}

void OemModuleHead::setProperty(uint r, uint x)
{
	mInfo("Set Property %d , value: %d", r, x);
	if (r == C_VISCA_SET_EXPOSURE) {
		if (x > 13)
			return;
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
		if (x > 5)
			return;
		unsigned char *p = protoBytes[C_VISCA_SET_NOISE_REDUCT];
		p[4 + 2] = x & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_NOISE_REDUCT, (int)x);
	} else if (r == C_VISCA_SET_GAIN) {
		if (x > 0x0F)
			return;
		unsigned char *p = protoBytes[C_VISCA_SET_GAIN];
		p[6 + 2] = (x & 0xf0) >> 4;
		p[7 + 2] = x & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_GAIN_VALUE, (int)x);
	} else if (r == C_VISCA_SET_EXP_COMPMODE) {
		if (x > 1)
			return;
		unsigned char *p = protoBytes[C_VISCA_SET_EXP_COMPMODE];
		p[4 + 2] = x ? 0x02 : 0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_EXP_COMPMODE, (int)x);
	} else if (r == C_VISCA_SET_EXP_COMPVAL) {
		if (x > 0xE)
			return;
		unsigned char *p = protoBytes[C_VISCA_SET_EXP_COMPVAL];
		p[6 + 2] = (x & 0xf0) >> 4;
		p[7 + 2] = x & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_EXP_COMPVAL, (int)x);
	} else if (r == C_VISCA_SET_GAIN_LIM) {
		if (x > 0xB)
			return;
		unsigned char *p = protoBytes[C_VISCA_SET_GAIN_LIM];
		p[4 + 2] = (x + 4) & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_GAIN_LIM, (int)x);
	} else if (r == C_VISCA_SET_SHUTTER) {
		if (x > 0x15)
			return;
		unsigned char *p = protoBytes[C_VISCA_SET_SHUTTER];
		p[6 + 2] = x >> 4;
		p[7 + 2] = x & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_SHUTTER, (int)x);
	} else if (r == C_VISCA_SET_WDRSTAT) {
		if (x > 0x1)
			return;
		unsigned char *p = protoBytes[C_VISCA_SET_WDRSTAT];
		p[4 + 2] = x ? 0x02 : 0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_WDRSTAT, (int)x);
	} else if (r == C_VISCA_SET_GAMMA) {
		if (x > 0x5)
			return;
		unsigned char *p = protoBytes[C_VISCA_SET_GAMMA];
		p[4 + 2] = x & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_GAMMA, (int)x);
	} else if (r == C_VISCA_SET_AWB_MODE) {
		if (x > 0x09)
			return;
		unsigned char *p = protoBytes[C_VISCA_SET_AWB_MODE];
		if (x < 0x04)
			p[4 + 2] = x;
		else
			p[4 + 2] = x + 0x01;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_AWB_MODE, (int)x);
	} else if (r == C_VISCA_SET_DEFOG_MODE) {
		if (x > 1)
			return;
		unsigned char *p = protoBytes[C_VISCA_SET_DEFOG_MODE];
		p[4 + 2] = x ? 0x02 : 0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_DEFOG_MODE, (int)x);
	} else if (r == C_VISCA_SET_FOCUS_MODE) {
		if (x > 1)
			return;
		unsigned char *p = protoBytes[C_VISCA_SET_FOCUS_MODE];
		p[4 + 2] = x ? 0x03 : 0x02;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_FOCUS_MODE, (int)x);
	} else if (r == C_VISCA_SET_ZOOM_TRIGGER) {
		if (x > 1)
			return;
		unsigned char *p = protoBytes[C_VISCA_SET_ZOOM_TRIGGER];
		p[4 + 2] = x ? 0x02 : 0x01;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_ZOOM_TRIGGER, (int)x);
	} else if (r == C_VISCA_SET_BLC_STATUS) {
		if (x > 1)
			return;
		unsigned char *p = protoBytes[C_VISCA_SET_BLC_STATUS];
		p[4 + 2] = x ? 0x02 : 0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_BLC_STATUS, (int)x);
	} else if (r == C_VISCA_SET_IRCF_STATUS) {
		if (x > 1)
			return;
		unsigned char *p = protoBytes[C_VISCA_SET_IRCF_STATUS];
		p[4 + 2] = x ? 0x02 : 0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_IRCF_STATUS, (int)x);
	} else if (r == C_VISCA_SET_AUTO_ICR) {
		if (x > 1)
			return;
		unsigned char *p = protoBytes[C_VISCA_SET_AUTO_ICR];
		p[4 + 2] = x ? 0x02 : 0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_AUTO_ICR, (int)x);
	} else if (r == C_VISCA_SET_PROGRAM_AE_MODE) {
		if (x != 0 &&
				x != 3 &&
				x != 10 &&
				x != 11 &&
				x != 13)
			return;
		unsigned char *p = protoBytes[C_VISCA_SET_PROGRAM_AE_MODE];
		p[4 + 2] = x;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_PROGRAM_AE_MODE, (int)x);
	} else if (r == C_VISCA_SET_FLIP_MODE) {
		if (x > 1)
			return;
		unsigned char *p = protoBytes[C_VISCA_SET_FLIP_MODE];
		p[4 + 2] = x ? 0x02 : 0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_FLIP, (int)x);
	} else if (r == C_VISCA_SET_MIRROR_MODE) {
		if (x > 1)
			return;
		unsigned char *p = protoBytes[C_VISCA_SET_MIRROR_MODE];
		p[4 + 2] = x ? 0x02 : 0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_MIRROR, (int)x);
	} else if (r == C_VISCA_SET_DIGI_ZOOM_STAT) {
		if (x > 1)
			return;
		unsigned char *p = protoBytes[C_VISCA_SET_DIGI_ZOOM_STAT];
		p[4 + 2] = x ? 0x02 : 0x03;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_DIGI_ZOOM_STAT, (int)x);
	} else if (r == C_VISCA_SET_ZOOM_TYPE) {
		if (x > 1)
			return;
		unsigned char *p = protoBytes[C_VISCA_SET_ZOOM_TYPE];
		p[4 + 2] = x ? 0x00 : 0x01;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_ZOOM_TYPE, (int)x);
	} else if (r == C_VISCA_SET_ONE_PUSH) {
		unsigned char *p = protoBytes[C_VISCA_SET_ONE_PUSH];
		p[4 + 2] = 0x01;
		transport->send((const char *)p + 2, p[0]);
	} else if (r == C_VISCA_SET_EXPOSURE_TARGET) {
		if (x > 0xFF)
			return;
		unsigned char *p = protoBytes[C_VISCA_SET_EXPOSURE_TARGET];
		p[5 + 2] = (x & 0xf0) >> 4;
		p[6 + 2] = x & 0x0f;
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_EXPOSURE_TARGET, (int)x);
	} else if (r == C_VISCA_SET_GAIN_LIMIT_OEM) {
		unsigned char *p = protoBytes[r];
		p[5 + 2] = (x & 0xf000) >> 12;
		p[6 + 2] = (x & 0x0f00) >> 8;
		p[7 + 2] = (x & 0x00f0) >> 4;
		p[8 + 2] = (x & 0x000f);
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_TOP_GAIN, (int)((x & 0xff00) >> 8));
		setRegister(R_BOT_GAIN, (int)(x & 0x00ff));
	} else if (r == C_VISCA_SET_SHUTTER_LIMIT_OEM) {
		unsigned char *p = protoBytes[r];
		p[5 + 2] = (x & 0xf00000) >> 20;
		p[6 + 2] = (x & 0x0f0000) >> 16;
		p[7 + 2] = (x & 0x00f000) >> 12;
		p[8 + 2] = (x & 0x000f00) >> 8;
		p[9 + 2] = (x & 0x0000f0) >> 4;
		p[10 + 2] = (x & 0x00000f);
		transport->send((const char *)p + 2, p[0]);
		setRegister(R_TOP_SHUTTER, (int)((x & 0xfff000) >> 12));
		setRegister(R_BOT_SHUTTER, (int)(x & 0x000fff));
	} else if (r == C_VISCA_SET_IRIS_LIMIT_OEM) {
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
	}else if (r == ptzp::PtzHead_Capability_VIDEO_ROTATE){
		if (x > 3)
			return;
		if (x == 0){
			setProperty(C_VISCA_SET_FLIP_MODE, 1);
			setProperty(C_VISCA_SET_MIRROR_MODE, 0);
		} else if (x == 1){
			setProperty(C_VISCA_SET_FLIP_MODE, 0);
			setProperty(C_VISCA_SET_MIRROR_MODE, 1);
		} else if (x == 2){
			setProperty(C_VISCA_SET_FLIP_MODE, 1);
			setProperty(C_VISCA_SET_MIRROR_MODE, 1);
		} else if (x == 3){
			setProperty(C_VISCA_SET_FLIP_MODE, 0);
			setProperty(C_VISCA_SET_MIRROR_MODE, 0);
		}
		setRegister(R_DISPLAY_ROT, x);
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

float OemModuleHead::getAngle()
{
	return getZoom();
}

void OemModuleHead::enableZoomControlFiltering(bool en)
{
	mDebug("%s", en ? "enabled" : "disabled");
	zoomControls.zoomFiltering = en;
}

void OemModuleHead::enableZoomControlTriggerredRead(bool en)
{
	mDebug("%s", en ? "enabled" : "disabled");
	zoomControls.tiggerredRead = en;
}

void OemModuleHead::enableZoomControlStationaryFiltering(bool en)
{
	mDebug("%s", en ? "enabled" : "disabled");
	zoomControls.stationaryFiltering = en;
	if (en)
		zoomControls.sfilter = new StationaryFilter;
}

void OemModuleHead::enableZoomControlErrorRateChecking(bool en)
{
	mDebug("%s", en ? "enabled" : "disabled");
	zoomControls.errorRateCheck = en;
	if (en)
		zoomControls.echeck = new ErrorRateChecker;
}

void OemModuleHead::enableZoomControlReadLatencyChecking(bool en)
{
	mDebug("%s", en ? "enabled" : "disabled");
	zoomControls.readLatencyCheck = en;
	if (en)
		zoomControls.stats = new SimpleStatistics;
}

QVariant OemModuleHead::getCapabilityValues(ptzp::PtzHead_Capability c)
{
	return getRegister(_mapCap[c].second);
}

void OemModuleHead::setCapabilityValues(ptzp::PtzHead_Capability c, uint val)
{
	return setProperty(_mapCap[c].first, val);
}
