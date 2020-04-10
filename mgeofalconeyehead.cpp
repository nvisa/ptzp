#include "mgeofalconeyehead.h"
#include "debug.h"
#include "ptzptransport.h"
#include "unistd.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSemaphore>
#include <QThread>

#include <errno.h>

#define dump(p, len)                                                           \
	for (int i = 0; i < len; i++)                                              \
		qDebug("%s: %d: 0x%x", __func__, i, p[i]);

static uchar chksum(const uchar *buf, int len)
{
	uchar sum = 0;
	for (int i = 0; i < len; i++)
		sum += buf[i];
	return (0x100 - (sum & 0xff));
}

#define MAX_CMD_LEN 10

enum Commands {
	C_SET_CONTINUOUS_ZOOM,
	C_SET_ZOOM,
	C_SET_CONTINUOUS_FOCUS,
	C_SET_FOCUS,
	C_SET_FOV,
	C_CHOOSE_CAM,
	C_SET_DIGITAL_ZOOM,
	C_SET_IR_POLARITY,
	C_SET_RETICLE_MODE,
	C_SET_RETICLE_TYPE,
	C_SET_RETICLE_INTENSITY,
	C_SET_SYMBOLOGY,
	C_SET_RAYLEIGH_SIGMA_COEFF,
	C_SET_RAYLEIGH_E_COEFF,
	C_SET_IMAGE_PROC,
	C_SET_HF_SIGMA_COEFF,
	C_SET_HF_FILTER_STD,
	C_SET_1_POINT_NUC,
	C_SET_2_POINT_NUC,
	C_SET_DMC_PARAM,
	C_SET_HEKOS_PARAM,
	C_SET_CURRENT_INTEGRATION_MODE,
	C_SET_TOGGLE_MODULE_POWER_STATUS,
	C_SET_OPTIC_BYPASS,
	C_SET_FLASH_PROTECTION,
	C_SET_OPTIC_STEP,
	C_SET_DMC_OFFSET_AZIMUTH,
	C_SET_DMC_OFFSET_ELEVATION,
	C_SET_DMC_OFFSET_BANK,
	C_SET_DMC_OFFSET_SAVE,
	C_SET_LASER_UP,
	C_SET_LASER_FIRE,
	C_SET_LASER_FIRE_MODE,
	C_SET_IBIT_START,
	C_SET_GMT,
	C_SET_BUTTON_PRESSED,
	C_SET_BUTTON_RELEASED,
	C_SET_RELAY_CONTROL,
	C_EXTRAS_SET_BRIGHTNESS_MODE,
	C_EXTRAS_SET_CONTRAST_MODE,
	C_EXTRAS_SET_BRIGHTNESS_CHANGE,
	C_EXTRAS_SET_CONTRAST_CHANGE,

	C_GET_TARGET_COORDINATES,
	C_GET_GPS_LOCATION,
	C_GET_ALL_SYSTEM_VALUES,
	C_GET_DMC_VALUES,
	C_GET_ZOOM_POS,
	C_GET_FOCUS_POS,
	C_GET_GPS_DATE_TIME,
	C_GET_OPTIMIZATION_PARAM,
	C_GET_FLASH_PROTECTION,
	//	C_GET_CURRENT_TIME,


	C_COUNT,
};

static unsigned char protoBytes[C_COUNT][MAX_CMD_LEN] = {
	{0x06, 0x1f, 0x02, 0xbc, 0x00, 0x00, 0x00},				// cont zoom
	{0x08, 0x1f, 0x04, 0xe9, 0x00, 0x00, 0x00, 0x00, 0x00}, // set zoom
	{0x06, 0x1f, 0x02, 0xbd, 0x00, 0x00, 0x00},				// cont focus
	{0x08, 0x1f, 0x04, 0xea, 0x00, 0x00, 0x00, 0x00, 0x00}, // set focus
	{0x05, 0x1f, 0x01, 0xbb, 0x00, 0x00},					// set fov
	{0x05, 0x1f, 0x01, 0xc4, 0x00, 0x00},					// choose camera
	{0x05, 0x1f, 0x01, 0xbe, 0x00, 0x00},					// digital zoom
	{0x05, 0x1f, 0x01, 0xc2, 0x00, 0x00},					// ir polarity
	{0x07, 0x1f, 0x03, 0xb4, 0x00, 0x00, 0x00, 0x00},		// reticle mode
	{0x07, 0x1f, 0x03, 0xb4, 0x00, 0x00, 0x00, 0x00},		// reticle type
	{0x07, 0x1f, 0x03, 0xb4, 0x00, 0x00, 0x00, 0x00},		// reticle intensity
	{0x05, 0x1f, 0x01, 0xc3, 0x00, 0x00},					// symbology
	{0x06, 0x1f, 0x02, 0xd8, 0x00, 0x00, 0x00}, // rayleigh sigma coeff
	{0x06, 0x1f, 0x02, 0xd9, 0x00, 0x00, 0x00}, // rayleigh epsilon coeff
	{0x05, 0x1f, 0x01, 0xda, 0x00, 0x00},		// image process
	{0x06, 0x1f, 0x02, 0xdb, 0x00, 0x00, 0x00}, // hf sigma coeff
	{0x06, 0x1f, 0x02, 0xdc, 0x00, 0x00, 0x00}, // hf filter std
	{0x05, 0x1f, 0x01, 0xb1, 0x00, 0x00},		// 1 point nuc
	{0x05, 0x1f, 0x01, 0xc6, 0x00, 0x00},		// 2 point nuc
	{0x05, 0x1f, 0x01, 0xba, 0x00, 0x00},		// dmc parameters (degree, mils)
	{0x06, 0x1f, 0x02, 0xbf, 0x00, 0x00, 0x00}, // hekos param
	{0x05, 0x1f, 0x01, 0xb3, 0x00, 0x00},		// current integration time mode
	{0x06, 0x1f, 0x02, 0xe0, 0x00, 0x00, 0x00}, // toggle module power status
	{0x04, 0x1f, 0x00, 0xe7, 0x00},				// optic bypass
	{0x05, 0x1f, 0x01, 0xa1, 0x00, 0x00},		// flash protection
	{0x05, 0x1f, 0x01, 0xa4, 0x00, 0x00},		// optic step
	{0x08, 0x1f, 0x04, 0xd7, 0x00, 0x00, 0x00, 0x00,
	 0x00}, // dmc offset azimuth
	{0x08, 0x1f, 0x04, 0xd7, 0x00, 0x00, 0x00, 0x00,
	 0x00}, // dmc offset elevation
	{0x08, 0x1f, 0x04, 0xd7, 0x00, 0x00, 0x00, 0x00, 0x00}, // dmc offset bank
	{0x04, 0x1f, 0x00, 0xdd, 0x00},							// dmc save
	{0x04, 0x1f, 0x00, 0xc5, 0x00},							// laser power up
	{0x04, 0x1f, 0x00, 0xb5, 0x00},							// laser fire
	{0x05, 0x1f, 0x01, 0xfa, 0x00, 0x00},					// laser fire mode
	{0x04, 0x1f, 0x00, 0xb7, 0x00},							// start ibit
	{0x05, 0x1f, 0x01, 0xcf, 0x00, 0x00},					// set gmt
	{0x06, 0x1f, 0x02, 0xb0, 0x00, 0x01, 0x00},				// button pressed
	{0x06, 0x1f, 0x02, 0xb0, 0x00, 0x00, 0x00},				// button released
	{},														// relay control
	{0x07, 0x1f, 0x03, 0x21, 0x00, 0x00, 0x00, 0x00},
	{0x07, 0x1f, 0x03, 0x21, 0x00, 0x00, 0x00, 0x00},
	{0x07, 0x1f, 0x03, 0x21, 0x00, 0x00, 0x00, 0x00},
	{0x07, 0x1f, 0x03, 0x21, 0x00, 0x00, 0x00, 0x00},

	{0x04, 0x1f, 0x00, 0xc1, 0x00}, // ask target coordinates
	{0x04, 0x1f, 0x00, 0xc0, 0x00}, // ask gps location -ok
	{0x04, 0x1f, 0x00, 0xb6, 0x00}, // get all system values - ok
	{0x04, 0x1f, 0x00, 0xb9, 0x00}, // ask dmc values - ok
	{0x04, 0x1f, 0x00, 0xeb, 0x00}, // zoom pos ok
	{0x04, 0x1f, 0x00, 0xec, 0x00}, // focus pos - ok
	{0x04, 0x1f, 0x00, 0xf1, 0x00}, // gps date time ok
	{0x04, 0x1f, 0x00, 0xe4, 0x00}, // optimization param -ok
	{0x04, 0x1f, 0x00, 0xa2, 0x00}, // flash protection ok
	//	{ 0x04, 0x1f, 0x00, 0xa3, 0x00}, // current time ok

};

class PCA9538Driver : public I2CDevice
{
public:
	PCA9538Driver() {}

	int open()
	{
		fd = openDevice(0x70);
		if (fd < 0)
			return fd;
		return 0;
	}

	void resetAllPorts() { i2cWrite(0x03, 0x00); }

	uchar controlRelay(uchar reg, uint val) { return i2cWrite(reg, val); }
};

class RelayControlThread : public QThread
{
public:
	RelayControlThread()
	{
		switching = false;
		x = 0;
	}

	int switch2(int x)
	{
		if (switching)
			return -1;
		this->x = x;
		gosem.release();
		return 0;
	}

	void run()
	{
		while (1) {
			gosem.acquire();

			switching = true;
			i2c->controlRelay(0x01, 0x00);
			sleep(3);
			if (x == 0) // Thermal
				i2c->controlRelay(0x01, ((1 << (standbyRelay - 1)) +
										 (1 << (thermalRelay - 1))));
			else if (x == 1)
				i2c->controlRelay(0x01, 1 << (dayCamRelay - 1));
			else if (x == 2)
				i2c->controlRelay(0x01, ((1 << (standbyRelay - 1))));
			sleep(2);
			switching = false;
		}
	}

	QSemaphore gosem;
	bool switching;
	int x;
	int dayCamRelay;
	int thermalRelay;
	int standbyRelay;
	PCA9538Driver *i2c;
};

MgeoFalconEyeHead::MgeoFalconEyeHead(QList<int> relayConfig, bool gps, QString type)
{
	gpsStatus = gps;
	dayCamRelay = 4;
	thermalRelay = 5;
	standbyRelay = 6;
	lInfo = true;
	if (relayConfig.size() == 3) {
		dayCamRelay = relayConfig[0];
		thermalRelay = relayConfig[1];
		standbyRelay = relayConfig[2];
	}
	firmwareType = type;
	if(type == "absgs")
		syncEndPoint = C_GET_OPTIMIZATION_PARAM;
	else syncEndPoint = C_COUNT;

	i2c = new PCA9538Driver;
	i2c->open();
	i2c->resetAllPorts();
	relth = new RelayControlThread;
	relth->i2c = i2c;
	relth->dayCamRelay = dayCamRelay;
	relth->thermalRelay = thermalRelay;
	relth->standbyRelay = standbyRelay;
	relth->start();
	alive = false;

	syncTimer.start();

	nextSync = syncEndPoint;
	settings = {
		{"focus", {C_SET_CONTINUOUS_FOCUS, R_FOCUS}},
		{"focus_pos_set", {C_SET_FOCUS, R_FOCUS}},
		{"fov_pos", {C_SET_FOV, R_FOV}},
		{"choose_cam", {C_CHOOSE_CAM, R_CAM}},
		{"digital_zoom", {C_SET_DIGITAL_ZOOM, R_DIGI_ZOOM_POS}},
		{"polarity", {C_SET_IR_POLARITY, R_IR_POLARITY}},
		{"reticle_mode", {C_SET_RETICLE_MODE, R_RETICLE_MODE}},
		{"reticle_type", {C_SET_RETICLE_TYPE, R_RETICLE_TYPE}},
		{"reticle_intensity", {C_SET_RETICLE_INTENSITY, R_RETICLE_INTENSITY}},
		{"symbology", {C_SET_SYMBOLOGY, R_SYMBOLOGY}},
		{"rayleigh_sigma_coeff",
		 {C_SET_RAYLEIGH_SIGMA_COEFF, R_RAYLEIGH_SIGMA}},
		{"rayleigh_e_coeff", {C_SET_RAYLEIGH_E_COEFF, R_RAYLEIGH_E}},
		{"image_proc", {C_SET_IMAGE_PROC, R_IMAGE_PROC}},
		{"hf_sigma_coeff", {C_SET_HF_SIGMA_COEFF, R_HF_SIGMA}},
		{"hf_filter_std", {C_SET_HF_FILTER_STD, R_HF_FILTER}},
		{"one_point_nuc", {C_SET_1_POINT_NUC, R_NUC}},
		{"two_point_nuc", {C_SET_1_POINT_NUC, R_NUC}},
		{"dmc_param", {C_SET_DMC_PARAM, R_DMC_PARAM}},
		{"hekos_param", {C_SET_HEKOS_PARAM, R_HEKOS_PARAM}},
		{"current_integration_mode",
		 {C_SET_CURRENT_INTEGRATION_MODE, R_CURRENT_INTEGRATION_MODE}},
		{"toggle_module_power", {C_SET_TOGGLE_MODULE_POWER_STATUS, 0}},
		{"optic_bypass", {C_SET_OPTIC_BYPASS, 0}},
		{"flas_protection", {C_SET_FLASH_PROTECTION, R_FLASH_PROTECTION}},
		{"optic_step", {C_SET_OPTIC_STEP, R_OPTIC_STEP}},
		{"dmc_azimuth", {0, R_DMC_AZIMUTH}},
		{"dmc_elevation", {0, R_DMC_ELEVATION}},
		{"dmc_bank", {0, R_DMC_BANK}},
		{"dmc_offset_save", {C_SET_DMC_OFFSET_SAVE, 0}},
		{"laser_up", {C_SET_LASER_UP, R_LASER_STATUS}},
		{"laser_fire", {C_SET_LASER_FIRE, 0}},
		{"laser_mode", {C_SET_LASER_FIRE_MODE, R_LASER_MODE}},
		{"ibit_start", {C_SET_IBIT_START, 0}},
		{"gmt", {C_SET_GMT, R_GMT}},
		{"button_press", {C_SET_BUTTON_PRESSED, 0}},
		{"button_release", {C_SET_BUTTON_RELEASED, 0}},
		{"relay_control", {C_SET_RELAY_CONTROL, R_RELAY_STATUS}},
		{"dmc_azimuth_offset",
		 {C_SET_DMC_OFFSET_AZIMUTH, R_DMC_OFFSET_AZIMUTH}},
		{"dmc_elevation_offset",
		 {C_SET_DMC_OFFSET_ELEVATION, R_DMC_OFFSET_ELEVATION}},
		{"dmc_bank_offset", {C_SET_DMC_OFFSET_BANK, R_DMC_OFFSET_BANK}},

		{"software_version", {0, R_SOFTWARE_VERSION}},
		{"cooler", {0, R_COOLER}},
		{"ibit_power", {0, R_IBIT_POWER}},
		{"ibit_system", {0, R_IBIT_SYSTEM}},
		{"ibit_optic", {0, R_IBIT_OPTIC}},
		{"ibit_lrf", {0, R_IBIT_LRF}},
		{"brightness_mode", {C_EXTRAS_SET_BRIGHTNESS_MODE, R_EXTRAS_BRIGHTNESS_MODE}},
		{"contrast_mode", {C_EXTRAS_SET_CONTRAST_MODE, R_EXTRAS_CONTRAST_MODE}},
		{"brightness_change", {C_EXTRAS_SET_BRIGHTNESS_CHANGE, 0}},
		{"contrast_change", {C_EXTRAS_SET_CONTRAST_CHANGE, 0}},
	};
	nonRegisterSettings << "laser_reflections";
}

void MgeoFalconEyeHead::fillCapabilities(ptzp::PtzHead *head)
{
	if(getProperty(R_CAM) == 0){
		head->add_capabilities(ptzp::PtzHead_Capability_ZOOM);
		head->add_capabilities(ptzp::PtzHead_Capability_FOCUS);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_ZOOM);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_POLARITY);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_DAY_VIEW);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_NIGHT_VIEW);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_RANGE);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_MENU_OVER_VIDEO);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_LAZER_RANGE_FINDER);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_SHOW_HIDE_SYMBOLOGY);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_NUC);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_DIGITAL_ZOOM);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_FOCUS);

		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_THERMAL_STANDBY_MODES);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_SHOW_RETICLE);

		if(firmwareType == "absgs"){
			head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_BRIGHTNESS);
			head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_CONTRAST);
		}
	}
	else if (getProperty(R_CAM) == 1){
		head->add_capabilities(ptzp::PtzHead_Capability_ZOOM);
		head->add_capabilities(ptzp::PtzHead_Capability_FOCUS);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_ZOOM);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_DAY_VIEW);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_NIGHT_VIEW);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_RANGE);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_MENU_OVER_VIDEO);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_LAZER_RANGE_FINDER);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_SHOW_HIDE_SYMBOLOGY);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_DIGITAL_ZOOM);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_FOCUS);

		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_THERMAL_STANDBY_MODES);
		head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_SHOW_RETICLE);

		if(firmwareType == "absgs"){
			head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_BRIGHTNESS);
			head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_CONTRAST);
		}
	}

}

int MgeoFalconEyeHead::syncRegisters()
{
	if (!transport)
		return -ENOENT;
	if (gpsStatus)
		nextSync = C_GET_GPS_DATE_TIME;
	else
		nextSync = C_GET_ALL_SYSTEM_VALUES;
	return syncNext();
}

int MgeoFalconEyeHead::startZoomIn(int speed)
{
	Q_UNUSED(speed);
	uchar *cmd = protoBytes[C_SET_CONTINUOUS_ZOOM];
	int len = cmd[0];
	cmd++;
	cmd[3] = 0x00;
	cmd[4] = 0x01;
	cmd[5] = chksum(cmd, len - 1);
	sendCommand(cmd, len);
	return 0;
}

int MgeoFalconEyeHead::startZoomOut(int speed)
{
	Q_UNUSED(speed);
	uchar *cmd = protoBytes[C_SET_CONTINUOUS_ZOOM];
	int len = cmd[0];
	cmd++;
	cmd[3] = 0x01;
	cmd[4] = 0x01;
	cmd[5] = chksum(cmd, len - 1);
	sendCommand(cmd, len);
	return 0;
}

int MgeoFalconEyeHead::stopZoom()
{
	uchar *cmd = protoBytes[C_SET_CONTINUOUS_ZOOM];
	int len = cmd[0];
	cmd++;
	cmd[3] = 0x00;
	cmd[4] = 0x00;
	cmd[5] = chksum(cmd, len - 1);
	sendCommand(cmd, len);
	unsigned char *p = protoBytes[C_GET_ZOOM_POS];
	len = p[0];
	p++;
	p[3] = chksum(p, len - 1);
	sendCommand(p, len);
	return 0;
}

int MgeoFalconEyeHead::setZoom(uint pos)
{
	uchar *cmd = protoBytes[C_SET_ZOOM];
	int len = cmd[0];
	cmd++;
	cmd[3] = (pos & 0x000000ff);
	cmd[4] = (pos & 0x0000ff00) >> 8;
	cmd[5] = (pos & 0x00ff0000) >> 16;
	cmd[6] = (pos & 0xff000000) >> 24;
	cmd[7] = chksum(cmd, len - 1);
	return sendCommand(cmd, len);
}

int MgeoFalconEyeHead::getZoom()
{
	return getRegister(R_ZOOM);
}

int MgeoFalconEyeHead::focusIn(int speed)
{
	Q_UNUSED(speed);
	unsigned char *p = protoBytes[C_SET_CONTINUOUS_FOCUS];
	int len = p[0];
	p++;
	p[3] = 0x01;
	p[4] = 0x01;
	p[5] = chksum(p, len - 1);
	return sendCommand(p, len);
}

int MgeoFalconEyeHead::focusOut(int speed)
{
	Q_UNUSED(speed);
	unsigned char *p = protoBytes[C_SET_CONTINUOUS_FOCUS];
	int len = p[0];
	p++;
	p[3] = 0x00;
	p[4] = 0x01;
	p[5] = chksum(p, len - 1);
	return sendCommand(p, len);
}

int MgeoFalconEyeHead::focusStop()
{
	unsigned char *p = protoBytes[C_SET_CONTINUOUS_FOCUS];
	int len = p[0];
	p++;
	p[4] = 0x00;
	p[5] = chksum(p, len - 1);
	return sendCommand(p, len);
}

int MgeoFalconEyeHead::getHeadStatus()
{
	if (nextSync == syncEndPoint)
		return ST_NORMAL;
	return ST_SYNCING;
}

void MgeoFalconEyeHead::setProperty(uint r, uint x)
{
	setPropertyInt(r, (int)x);
}

uint MgeoFalconEyeHead::getProperty(uint r)
{
	return getRegister(r);
}

QVariant MgeoFalconEyeHead::getProperty(const QString &key)
{
	if (key == "laser_reflections") {
		QStringList lines;
		foreach (const LaserReflection &r, reflections) {
			lines << QString("%1,%2,%3,%4,%5,%6,%7,%8")
						 .arg(r.range)
						 .arg(r.height)
						 .arg(r.latdegree)
						 .arg(r.latminute)
						 .arg(r.latsecond)
						 .arg(r.londegree)
						 .arg(r.lonminute)
						 .arg(r.lonsecond);
		}
		return lines.join(";");
	}

	return QVariant();
}

void MgeoFalconEyeHead::setProperty(const QString &key, const QVariant &value)
{
	Q_UNUSED(key);
	Q_UNUSED(value);
}

int MgeoFalconEyeHead::getFOV(float &hor, float &ver)
{
	bool day = true;
	if (getRegister(R_RELAY_STATUS) == 0 && getRegister(R_CAM) == 0)
		day = false;
	int fov_type = getRegister(R_FOV); // wide: 0, narrow: 2, middle: 1
	if (day) {
		if (fov_type == 0) {
			hor = 11.0;
			ver = 8.25;
		} else if (fov_type == 1) {
			hor = 6.0;
			ver = 4.5;
		} else {
			hor = 2.0;
			ver = 1.5;
		}
		return 0;
	}

	/* thermal */
	if (fov_type == 0) { // thermal
		hor = 25.0;
		ver = 20.0;
	} else if (fov_type == 1) {
		hor = 6.0;
		ver = 4.8;
	} else {
		hor = 2.0;
		ver = 1.6;
	}
	return 0;
}

float *MgeoFalconEyeHead::getFOVAbsgs()
{
	/***
	 * q[0] = day hor
	 * q[1] = day ver
	 * q[2] = therm hor
	 * q[3] = therm ver
	 */
	float *q = new float[4];
	int fov_type = getRegister(R_FOV); // wide: 0, narrow: 2, middle: 1
	if (fov_type == 0) {//wide
		q[0] = 11.0;
		q[1] = 8.25;
		q[2] = 25.0;
		q[3] = 20.0;
	} else if (fov_type == 1) {//midd
		q[0] = 6.0;
		q[1] = 4.5;
		q[2] = 6;
		q[3] = 4.8;
	} else if(fov_type == 2){//narrow
		q[0] = 2.0;
		q[1] = 1.5;
		q[2] = 2.0;
		q[3] = 1.6;
	} else if(fov_type == 3){//widemidd
		q[0] = 10.0;
		q[1] = 7.5;
		q[2] = 10.0;
		q[3] = 8.0;
	}else if(fov_type == 4){//narrowmidd
		q[0] = 3.5;
		q[1] = 2.63;
		q[2] = 3.5;
		q[3] = 2.8;
	}
	return q;
}

QString MgeoFalconEyeHead::whoAmI()
{
	if (getProperty(3))
		return "tv";
	return "thermal";
}

bool MgeoFalconEyeHead::isAlive()
{
	return alive;
}

void MgeoFalconEyeHead::buttonClick(int b, int action){
	if (action == 1)
		setPropertyInt(35, b);
	else if (action == -1)
		setPropertyInt(36, b);
}

void MgeoFalconEyeHead::screenClick(int x, int y, int action)
{
	ffDebug() << "Screen Click x: " << x << " y: " << y;
	double BUTTON_HEIGHT 	= 0.108; //0.112
	double BUTTON_WIDTH 	= 0.158; //0.158
	double FIRST_BUTTON_UP_LEFT_X = 0.026; //0.026
	double FIRST_BUTTON_UP_LEFT_Y = 0.246; //0.246
	double SECOND_BUTTON_UP_LEFT_X = 0.026; //0.026
	double SECOND_BUTTON_UP_LEFT_Y = 0.424; //0.555
	double THIRD_BUTTON_UP_LEFT_X = 0.026; //0.026
	double THIRD_BUTTON_UP_LEFT_Y = 0.602; //0.710
	double FOURTH_BUTTON_UP_LEFT_X = 0.793; //0.793
	double FOURTH_BUTTON_UP_LEFT_Y = 0.246; //0.246
	double FIFTH_BUTTON_UP_LEFT_X = 0.793; //0.793
	double FIFTH_BUTTON_UP_LEFT_Y = 0.424;//0.555
	double SIXTH_BUTTON_UP_LEFT_X = 0.793; //0.793
	double SIXTH_BUTTON_UP_LEFT_Y = 0.602; //0.710

	double ROUTING_LEFT_X1 = 0.794; //0.794
	double ROUTING_LEFT_X2 = 0.825; //0.825
	double ROUTING_RIGHT_X1 = 0.845; //0.845
	double ROUTING_RIGHT_X2 = 0.872; //0.872
	double ROUTING_RIGHT_LEFT_Y1 = 0.850;  //0.850
	double ROUTING_RIGHT_LEFT_Y2 = 0.875;  //0.875
	double ROUTING_UP_DOWN_X1 = 0.82; //0.82
	double ROUTING_UP_DOWN_X2 = 0.857; //0.857
	double ROUTING_UP_Y1 = 0.84; //0.84
	double ROUTING_UP_Y2 = 0.82; //0.82
	double ROUTING_DOWN_Y1 = 0.90; //0.90
	double ROUTING_DOWN_Y2 = 0.88; //0.88

	double ROUTING_AREA_TOP 	= 0.82; //0.82
	double ROUTING_AREA_BOTTOM = 0.90; //0.90
	int L0 = 0;
	int L1 = 1;
	int L2 = 2;
	int R0 = 3;
	int R1 = 4;
	int R2 = 5;
	int UP = 6;
	int DOWN = 7;
	int LEFT = 8;
	int RIGHT = 9;

	double ratioX = x / 720.0;
	double ratioY = y / 576.0;

	if(ratioY >= FIRST_BUTTON_UP_LEFT_Y && ratioY <= (FIRST_BUTTON_UP_LEFT_Y + BUTTON_HEIGHT)) {
		if(ratioX >= FIRST_BUTTON_UP_LEFT_X && ratioX <= (FIRST_BUTTON_UP_LEFT_X + BUTTON_WIDTH)){
			buttonClick(L0, action);
		} else if(ratioX >= FOURTH_BUTTON_UP_LEFT_X && ratioX <= (FOURTH_BUTTON_UP_LEFT_X + BUTTON_WIDTH)) {
			buttonClick(R0, action);
		}
	} else if(ratioY >= SECOND_BUTTON_UP_LEFT_Y && ratioY <= (SECOND_BUTTON_UP_LEFT_Y + BUTTON_HEIGHT)) {
		if(ratioX >= SECOND_BUTTON_UP_LEFT_X && ratioX <= (SECOND_BUTTON_UP_LEFT_X + BUTTON_WIDTH)){
			buttonClick(L1, action);
		} else if(ratioX >= FIFTH_BUTTON_UP_LEFT_X && ratioX <= (FIFTH_BUTTON_UP_LEFT_X + BUTTON_WIDTH)) {
			buttonClick(R1, action);
		}
	} else if(ratioY >= THIRD_BUTTON_UP_LEFT_Y && ratioY <= (THIRD_BUTTON_UP_LEFT_Y + BUTTON_HEIGHT)) {
		if(ratioX >= THIRD_BUTTON_UP_LEFT_X && ratioX <= (THIRD_BUTTON_UP_LEFT_X + BUTTON_WIDTH)){
			buttonClick(L2, action);
		} else if(ratioX >= SIXTH_BUTTON_UP_LEFT_X && ratioX <= (SIXTH_BUTTON_UP_LEFT_X + BUTTON_WIDTH)) {
			buttonClick(R2, action);
		}
	} else if(ratioY >= ROUTING_AREA_TOP && ratioY <= ROUTING_AREA_BOTTOM){
		if(ratioX >= ROUTING_LEFT_X1 && ratioX <= ROUTING_LEFT_X2 && ratioY >= ROUTING_RIGHT_LEFT_Y1 && ratioY<= ROUTING_RIGHT_LEFT_Y2) {
			buttonClick(LEFT, action);
		} else if(ratioX >= ROUTING_RIGHT_X1 && ratioX <= ROUTING_RIGHT_X2 && ratioY >= ROUTING_RIGHT_LEFT_Y1 && ratioY<= ROUTING_RIGHT_LEFT_Y2) {
			buttonClick(RIGHT, action);
		} else if(ratioY >= ROUTING_DOWN_Y2 && ratioY <= ROUTING_DOWN_Y1 && ratioX >= ROUTING_UP_DOWN_X1 && ratioX <= ROUTING_UP_DOWN_X2) {
			buttonClick(DOWN, action);
		} else if(ratioY >= ROUTING_UP_Y2 && ratioY <= ROUTING_UP_Y1 && ratioX >= ROUTING_UP_DOWN_X1 && ratioX <= ROUTING_UP_DOWN_X2) {
			buttonClick(UP, action);
		}
	}
}

int MgeoFalconEyeHead::syncNext()
{
	unsigned char *p = protoBytes[nextSync];
	int len = p[0];
	p++;
	p[3] = chksum(p, len - 1);
	return sendCommand(p, len);
}

int MgeoFalconEyeHead::dataReady(const unsigned char *bytes, int len)
{
	if (bytes[0] != 0x1f)
		return -ENOENT;
	if (len < bytes[1] + 4)
		return -EAGAIN;
	if (bytes[2] == 0x90)
		return 4; // ack
	if (bytes[2] == 0x95) {
		alive = true;
		pingTimer.restart();
		return 4; // alive
	}
	uchar chk = chksum(bytes, len - 1);
	if (chk != bytes[len - 1]) {
		fDebug("Checksum error");
		return -ENOENT;
	}

	pingTimer.restart();
	if (nextSync != syncEndPoint) {
		mInfo("Next sync property: %d", nextSync);
		if (++nextSync == syncEndPoint) {
			fDebug(
				"FalconEye register syncing completed, activating auto-sync");
		} else
			syncNext();
	}

	if (bytes[2] == 0x92) {
		setRegister(R_SOFTWARE_VERSION, (bytes[3] + (bytes[4] / 100)));
		setRegister(R_COOLER, bytes[5]);
		setRegister(R_IR_POLARITY, bytes[6]);
		setRegister(R_FOV, bytes[7]);
		setRegister(R_NUC, bytes[13]);
		setRegister(R_RETICLE_MODE, bytes[14]);
		setRegister(R_RETICLE_TYPE, bytes[15]);
		setRegister(R_RETICLE_INTENSITY, bytes[16]);
		setRegister(R_IBIT_POWER, bytes[17]);
		setRegister(R_IBIT_SYSTEM, bytes[18]);
		setRegister(R_IBIT_OPTIC, bytes[19]);
		setRegister(R_IBIT_LRF, bytes[20]);
		setRegister(R_SYMBOLOGY, bytes[22]);
		setRegister(R_DMC_PARAM, bytes[24]);
		setRegister(R_HEKOS_PARAM, ((bytes[25] * 10) + bytes[26]));
		setRegister(R_DIGI_ZOOM_POS, bytes[27]);
		setRegister(R_CAM, bytes[28]);
		setRegister(R_LASER_STATUS, bytes[29]);
		setRegister(R_GMT, (bytes[39] * 10));
	} else if (bytes[2] == 0x96) {
		if (getRegister(R_DMC_PARAM) == 0) {
			setRegister(R_DMC_AZIMUTH,
						((bytes[4] * 0x0100) + bytes[3] + (bytes[5] / 100)));
			setRegister(R_DMC_ELEVATION,
						((bytes[7] * 0x0100) + bytes[6] + (bytes[8] / 100)));
			setRegister(R_DMC_BANK,
						((bytes[10] * 0x0100) + bytes[9] + (bytes[11] / 100)));
		} else if (getRegister(R_DMC_PARAM) == 1) {
			setRegister(R_DMC_AZIMUTH, ((bytes[4] * 0x0100) + bytes[3]));
			setRegister(R_DMC_ELEVATION, ((bytes[6] * 0x0100) + bytes[5]));
			setRegister(R_DMC_BANK, ((bytes[8] * 0x0100) + bytes[7]));
		}
	} else if (bytes[2] == 0x97) {
		if (bytes[4] == 1) { // geo
			setRegister(R_GPS_LAT_DEGREE, bytes[5]);
			setRegister(R_GPS_LAT_MINUTE, bytes[6]);
			setRegister(R_GPS_LAT_SECOND, (bytes[7] + (bytes[8] / 100.0)));
			setRegister(R_GPS_LONG_DEGREE, bytes[9]);
			setRegister(R_GPS_LONG_MINUTE, bytes[10]);
			setRegister(R_GPS_LONG_SECOND, (bytes[11] + (bytes[12] / 100.0)));
			setRegister(R_GPS_ALTITUDE, (bytes[13] + (bytes[14] * 0x0100)));
		} else if (bytes[4] == 0) { // utm
			setRegister(R_GPS_UTM_NORTH,
						(bytes[5] + bytes[6] * 0x0100 + bytes[7] * 0x010000));
			setRegister(R_GPS_UTM_EAST,
						(bytes[8] + bytes[9] * 0x0100 + bytes[10] * 0x010000));
			setRegister(R_GPS_UTM_ZONE, (bytes[11] + bytes[12] * 0x0100));
			setRegister(R_GPS_ALTITUDE, (bytes[13] + (bytes[14] * 0x0100)));
		}
	} else if (bytes[2] == 0xa5) {
		setRegister(R_FLASH_PROTECTION, bytes[3]);
	} else if (bytes[2] == 0xa6) {
		setRegister(R_CURRENT_INTEGRATION_MODE, bytes[6]);
	} else if (bytes[2] == 0x9a) {
		setRegister(R_RAYLEIGH_SIGMA, bytes[3]);
		setRegister(R_RAYLEIGH_E, bytes[4]);
		setRegister(R_IMAGE_PROC, bytes[5]);
		setRegister(R_HF_SIGMA, bytes[6]);
		setRegister(R_HF_FILTER, bytes[7]);
	} else if (bytes[2] == 0x9d) {
		setRegister(R_ZOOM, (bytes[3] + bytes[4] * 0x0100 +
							 bytes[5] * 0x010000 + bytes[6] * 0x01000000));
	} else if (bytes[2] == 0x9e) {
		setRegister(R_FOCUS, (bytes[3] + bytes[4] * 0x0100 +
							  bytes[5] * 0x010000 + bytes[6] * 0x01000000));
	} else if (bytes[2] == 0xa1) {
		setRegister(R_GPS_DATE_AND_TIME_DAY, bytes[3]);
		setRegister(R_GPS_DATE_AND_TIME_MOUNTH, bytes[4]);
		setRegister(R_GPS_DATE_AND_TIME_YEAR, bytes[5] + (bytes[6] * 0x0100));
		setRegister(R_GPS_DATE_AND_TIME_HOURS, bytes[7]);
		setRegister(R_GPS_DATE_AND_TIME_MIN, bytes[8]);
	} else if (bytes[2] == 0x98) {
		int refcnt = bytes[2 + 1];

		mDebug("%d reflections found", refcnt);
		reflections.clear();
		for (int i = 0; i < refcnt; i++) {
			LaserReflection r;
			int off = 2 + 4 + i * 12;
			if(lInfo){
				r.range = bytes[off] + bytes[off + 1] * 256;
				lInfo =false;
			} else {
				r.range = -1 * (bytes[off] + bytes[off + 1] * 256);
				lInfo = true;
			}
			r.latdegree = bytes[off + 2];
			r.latminute = bytes[off + 3];
			r.latsecond = bytes[off + 4];
			r.latsecond += bytes[off + 5] / 100.0;
			r.londegree = bytes[off + 6];
			r.lonminute = bytes[off + 7];
			r.lonsecond = bytes[off + 8];
			r.lonsecond += bytes[off + 9] / 100.0;
			r.height = bytes[off + 10] + bytes[off + 11] * 256;
			reflections << r;
		}
	}
	return len;
}

QByteArray MgeoFalconEyeHead::transportReady()
{
	/* we have nothing to sync atm, zoom reading is buggy */
//	return QByteArray();
	if (syncTimer.elapsed() > 1000) {
		syncTimer.restart();
		unsigned char *p = protoBytes[C_GET_ALL_SYSTEM_VALUES];
		int len = p[0];
		p++;
		p[3] = chksum(p, len - 1);
		return QByteArray((const char *)p, len);
	}
	return QByteArray();
}

void MgeoFalconEyeHead::setPropertyInt(uint r, int x)
{
	if (r == C_SET_FOCUS) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = (char)(x & 0x000000ff);
		p[4] = (char)(x & 0x0000ff00) >> 8;
		p[5] = (char)(x & 0x00ff0000) >> 16;
		p[6] = (char)(x & 0xff000000) >> 24;
		p[7] = chksum(p, len - 1);
		sendCommand(p, len);
	} else if (r == C_SET_FOV) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_FOV, x);
		sendCommand(p, len);
	} else if (r == C_CHOOSE_CAM) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_CAM, x);
		sendCommand(p, len);
	} else if (r == C_SET_DIGITAL_ZOOM) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_DIGI_ZOOM_POS, x);
		sendCommand(p, len);
	} else if (r == C_SET_IR_POLARITY) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_IR_POLARITY, x);
		sendCommand(p, len);
	} else if (r == C_SET_RETICLE_MODE) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = getRegister(R_RETICLE_TYPE);
		p[5] = getRegister(R_RETICLE_INTENSITY);
		p[6] = chksum(p, len - 1);
		setRegister(R_RETICLE_MODE, x);
		sendCommand(p, len);
	} else if (r == C_SET_RETICLE_TYPE) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = getRegister(R_RETICLE_MODE);
		p[4] = x;
		p[5] = getRegister(R_RETICLE_INTENSITY);
		p[6] = chksum(p, len - 1);
		setRegister(R_RETICLE_TYPE, x);
		sendCommand(p, len);
	} else if (r == C_SET_RETICLE_INTENSITY) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = getRegister(R_RETICLE_MODE);
		p[4] = getRegister(R_RETICLE_TYPE);
		p[5] = x;
		p[6] = chksum(p, len - 1);
		setRegister(R_RETICLE_INTENSITY, x);
		sendCommand(p, len);
	} else if (r == C_SET_SYMBOLOGY) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_SYMBOLOGY, x);
		sendCommand(p, len);
	} else if (r == C_SET_RAYLEIGH_SIGMA_COEFF) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = (char)(x & 0x00ff);
		p[4] = (char)(x & 0xff00) >> 8;
		p[5] = chksum(p, len - 1);
		setRegister(R_RAYLEIGH_SIGMA, x);
		sendCommand(p, len);
	} else if (r == C_SET_RAYLEIGH_E_COEFF) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = (char)(x & 0x00ff);
		p[4] = (char)(x & 0xff00) >> 8;
		p[5] = chksum(p, len - 1);
		setRegister(R_RAYLEIGH_E, x);
		sendCommand(p, len);
	} else if (r == C_SET_IMAGE_PROC) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_IMAGE_PROC, x);
		sendCommand(p, len);
	} else if (r == C_SET_HF_SIGMA_COEFF) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = (char)(x & 0x00ff);
		p[4] = (char)(x & 0xff00) >> 8;
		p[5] = chksum(p, len - 1);
		setRegister(R_HF_SIGMA, x);
		sendCommand(p, len);
	} else if (r == C_SET_HF_FILTER_STD) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = (char)(x & 0x00ff);
		p[4] = (char)(x & 0xff00) >> 8;
		p[5] = chksum(p, len - 1);
		setRegister(R_HF_FILTER, x);
		sendCommand(p, len);
	} else if (r == C_SET_1_POINT_NUC) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		sendCommand(p, len);
	} else if (r == C_SET_2_POINT_NUC) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		sendCommand(p, len);
	} else if (r == C_SET_DMC_PARAM) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_DMC_PARAM, x);
		sendCommand(p, len);
	} else if (r == C_SET_HEKOS_PARAM) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = (x / 10);
		p[4] = (x % 10);
		p[5] = chksum(p, len - 1);
		setRegister(R_HEKOS_PARAM, x);
		sendCommand(p, len);
	} else if (r == C_SET_CURRENT_INTEGRATION_MODE) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_CURRENT_INTEGRATION_MODE, x);
		sendCommand(p, len);
	} else if (r == C_SET_TOGGLE_MODULE_POWER_STATUS) {
		// x < 0 ise off
		// x >= 0 ise on
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		if (x < 0) {
			p[3] = (-1 * x);
			p[4] = 0x00;
		} else {
			p[3] = x;
			p[4] = 0x01;
		}
		p[5] = chksum(p, len - 1);
		sendCommand(p, len);
	} else if (r == C_SET_OPTIC_BYPASS) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = chksum(p, len - 1);
		sendCommand(p, len);
	} else if (r == C_SET_FLASH_PROTECTION) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_FLASH_PROTECTION, x);
		sendCommand(p, len);
	} else if (r == C_SET_OPTIC_STEP) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_OPTIC_STEP, x);
		sendCommand(p, len);
	} else if (r == C_SET_DMC_OFFSET_AZIMUTH) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = qAbs(x);
		p[4] = getRegister(R_DMC_OFFSET_ELEVATION);
		p[5] = getRegister(R_DMC_OFFSET_BANK);
		if (x < 0)
			p[6] = 0x04 + getRegister(R_DMC_OFFSET_SIGN);
		else
			p[6] = getRegister(R_DMC_OFFSET_SIGN);
		p[7] = chksum(p, len - 1);
		setRegister(R_DMC_OFFSET_AZIMUTH, x);
		setRegister(R_DMC_OFFSET_SIGN, p[6]);
		sendCommand(p, len);
	} else if (r == C_SET_DMC_OFFSET_ELEVATION) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = getRegister(R_DMC_OFFSET_AZIMUTH);
		p[4] = qAbs(x);
		p[5] = getRegister(R_DMC_OFFSET_BANK);
		if (x < 0)
			p[6] = 0x02 + getRegister(R_DMC_OFFSET_SIGN);
		else
			p[6] = getRegister(R_DMC_OFFSET_SIGN);
		p[7] = chksum(p, len - 1);
		setRegister(R_DMC_OFFSET_ELEVATION, x);
		setRegister(R_DMC_OFFSET_SIGN, p[6]);
		sendCommand(p, len);
	} else if (r == C_SET_DMC_OFFSET_BANK) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = getRegister(R_DMC_OFFSET_AZIMUTH);
		p[4] = getRegister(R_DMC_OFFSET_ELEVATION);
		p[5] = qAbs(x);
		if (x < 0)
			p[6] = 0x01 + getRegister(R_DMC_OFFSET_SIGN);
		else
			p[6] = getRegister(R_DMC_OFFSET_SIGN);
		p[7] = chksum(p, len - 1);
		setRegister(R_DMC_OFFSET_BANK, x);
		setRegister(R_DMC_OFFSET_SIGN, p[6]);
		sendCommand(p, len);
	} else if (r == C_SET_DMC_OFFSET_SAVE) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = chksum(p, len - 1);
		sendCommand(p, len);
	} else if (r == C_SET_LASER_UP) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = chksum(p, len - 1);
		setRegister(R_LASER_STATUS, 1);
		sendCommand(p, len);
	} else if (r == C_SET_LASER_FIRE) {
		if(x < 0){
			setPropertyInt(C_SET_LASER_UP, x);
			return;
		}
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = chksum(p, len - 1);
		sendCommand(p, len);
	} else if (r == C_SET_LASER_FIRE_MODE) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_LASER_MODE, x);
		sendCommand(p, len);
	} else if (r == C_SET_IBIT_START) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = chksum(p, len - 1);
		sendCommand(p, len);
	} else if (r == C_SET_GMT) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		if (x >= 0)
			p[3] = x * 10;
		else if (x < 0)
			p[3] = (x * -10) | (1 << 7);
		p[4] = chksum(p, len - 1);
		setRegister(R_GMT, x);
		sendCommand(p, len);
	} else if (r == C_SET_BUTTON_PRESSED) {
		if(x < 0){
			setPropertyInt(C_SET_BUTTON_RELEASED, (-1 * x));
			return;
		}
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[5] = chksum(p, len - 1);
		sendCommand(p, len);
	} else if (r == C_SET_BUTTON_RELEASED) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[5] = chksum(p, len - 1);
		sendCommand(p, len);
	} else if (r == C_SET_RELAY_CONTROL) { // switch mode selection
#if 0
		/* TODO: implement relay control */
		ffDebug() << "relay con";
		i2c->controlRelay(0x01, 0x00);
		sleep(3);
		ffDebug() << "relay con after 3 sec";
		if (x == 0) {// Thermal
			if(standbyRelay != 0 && thermalRelay != 0)
			{
				int ret = i2c->controlRelay(0x01, ((1 << (standbyRelay-1)) + (1 << (thermalRelay-1))));
				mDebug("I2C return Thermal cam: 0x%x", ret);
			}
		} else if (x == 1){
			if(dayCamRelay != 0)
			{
				int ret = i2c->controlRelay(0x01, 1 << (dayCamRelay-1));
				mDebug("I2C return Day cam: 0x%x", ret);
			}
		} else if (x == 2){
			i2c->controlRelay(0x01, ((1 << (standbyRelay-1))));
		}
#else
		if (!relth->switch2(x))
			setRegister(R_RELAY_STATUS, x);
#endif
	} else if (r == C_EXTRAS_SET_BRIGHTNESS_MODE){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = 0x01;
		p[5] = 0x00;
		p[6] = chksum(p, len - 1);
		setRegister(R_EXTRAS_BRIGHTNESS_MODE, x);
		sendCommand(p, len);
	} else if (r == C_EXTRAS_SET_CONTRAST_MODE){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = 0x02;
		p[5] = 0x00;
		p[6] = chksum(p, len - 1);
		setRegister(R_EXTRAS_BRIGHTNESS_MODE, x);
		sendCommand(p, len);
	} else if (r == C_EXTRAS_SET_BRIGHTNESS_CHANGE){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = getRegister(R_EXTRAS_BRIGHTNESS_MODE);
		p[4] = 0x01;
		if(x > 0)
			p[5] = 0x01;
		else if (x < 0)
			p[5] = 0x02;
		p[6] = chksum(p, len - 1);
		sendCommand(p, len);
	} else if (r == C_EXTRAS_SET_CONTRAST_CHANGE){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = getRegister(R_EXTRAS_BRIGHTNESS_MODE);
		p[4] = 0x02;
		if(x > 0)
			p[5] = 0x01;
		else if (x < 0) p[5] = 0x02;
		p[6] = chksum(p, len - 1);
		sendCommand(p, len);
	}
}

int MgeoFalconEyeHead::sendCommand(const unsigned char *cmd, int len)
{
	return transport->send((const char *)cmd, len);
}

int MgeoFalconEyeHead::readRelayConfig(QString filename)
{
	QFile f(filename);
	if (!f.open(QIODevice::ReadOnly)) {
		mDebug("File opening error %s", qPrintable(filename));
		return -1;
	}
	QByteArray ba = f.readAll();
	f.close();
	QJsonDocument doc = QJsonDocument::fromJson(ba);
	QJsonObject obj = doc.object();
	QJsonArray arr = obj["ptzp"].toArray();
	foreach (QJsonValue v, arr) {
		QJsonObject src = v.toObject();
		if (src["type"].isString())
			if (src["type"] == QString("kayi"))
				obj = src["relay"].toObject();
	}

	if (obj["thermal"].isDouble() && obj["standby"].isDouble() &&
		obj["daycam"].isDouble()) {
		thermalRelay = obj["thermal"].toInt();
		standbyRelay = obj["standby"].toInt();
		dayCamRelay = obj["daycam"].toInt();
	} else {
		dayCamRelay = 4;
		thermalRelay = 5;
		standbyRelay = 6;
	}
	mDebug("Read relay config :\n DAYCAM = %d \n STANDBY = %d\n THERMAL = %d",
		   dayCamRelay, standbyRelay, thermalRelay);
	return 0;
}
