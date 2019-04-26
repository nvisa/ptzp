#include "mgeofalconeyehead.h"
#include "debug.h"
#include "ptzptransport.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include <errno.h>

#define dump(p, len) \
	for (int i = 0; i < len; i++) \
		qDebug("%s: %d: 0x%x", __func__, i, p[i]);

static uchar chksum(const uchar *buf, int len)
{
	uchar sum = 0;
	for (int i = 0 ; i < len; i++)
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

	C_GET_TARGET_COORDINATES,
	C_GET_ALL_SYSTEM_VALUES,
	C_GET_DMC_VALUES,
	C_GET_GPS_LOCATION,
	C_GET_OPTIMIZATION_PARAM,
	C_GET_ZOOM_POS,
	C_GET_FOCUS_POS,
	C_GET_GPS_DATE_TIME,
	C_GET_FLASH_PROTECTION,
	C_GET_CURRENT_TIME,

	C_COUNT
};

static unsigned char protoBytes[C_COUNT][MAX_CMD_LEN] = {
	{ 0x06, 0x1f, 0x02, 0xbc, 0x00, 0x00, 0x00}, // cont zoom
	{ 0x08, 0x1f, 0x04, 0xe9, 0x00, 0x00, 0x00, 0x00, 0x00}, // set zoom
	{ 0x06, 0x1f, 0x02, 0xbd, 0x00, 0x00, 0x00}, // cont focus
	{ 0x08, 0x1f, 0x04, 0xea, 0x00, 0x00, 0x00, 0x00, 0x00}, // set focus
	{ 0x05, 0x1f, 0x01, 0xbb, 0x00, 0x00}, // set fov
	{ 0x05, 0x1f, 0x01, 0xc4, 0x00, 0x00}, // choose camera
	{ 0x05, 0x1f, 0x01, 0xbe, 0x00, 0x00},// digital zoom
	{ 0x05, 0x1f, 0x01, 0xc2, 0x00, 0x00}, // ir polarity
	{ 0x07, 0x1f, 0x03, 0xb4, 0x00, 0x00, 0x00, 0x00}, // reticle mode
	{ 0x07, 0x1f, 0x03, 0xb4, 0x00, 0x00, 0x00, 0x00}, // reticle type
	{ 0x07, 0x1f, 0x03, 0xb4, 0x00, 0x00, 0x00, 0x00}, // reticle intensity
	{ 0x05, 0x1f, 0x01, 0xc3, 0x00, 0x00}, // symbology
	{ 0x06, 0x1f, 0x02, 0xd8, 0x00, 0x00, 0x00}, // rayleigh sigma coeff
	{ 0x06, 0x1f, 0x02, 0xd9, 0x00, 0x00, 0x00}, // rayleigh epsilon coeff
	{ 0x05, 0x1f, 0x01, 0xda, 0x00, 0x00}, // image process
	{ 0x06, 0x1f, 0x02, 0xdb, 0x00, 0x00, 0x00}, // hf sigma coeff
	{ 0x06, 0x1f, 0x02, 0xdc, 0x00, 0x00, 0x00}, // hf filter std
	{ 0x05, 0x1f, 0x01, 0xb1, 0x00, 0x00}, // 1 point nuc
	{ 0x05, 0x1f, 0x01, 0xc6, 0x00, 0x00}, // 2 point nuc
	{ 0x05, 0x1f, 0x01, 0xba, 0x00, 0x00}, // dmc parameters (degree, mils)
	{ 0x06, 0x1f, 0x02, 0xbf, 0x00, 0x00, 0x00}, // hekos param
	{ 0x05, 0x1f, 0x01, 0xb3, 0x00, 0x00}, // current integration time mode
	{ 0x06, 0x1f, 0x02, 0xe0, 0x00, 0x00, 0x00}, // toggle module power status
	{ 0x04, 0x1f, 0x00, 0xe7, 0x00}, // optic bypass
	{ 0x05, 0x1f, 0x01, 0xa1, 0x00, 0x00}, // flash protection
	{ 0x05, 0x1f, 0x01, 0xa4, 0x00, 0x00}, //optic step
	{ 0x08, 0x1f, 0x04, 0xd7, 0x00, 0x00, 0x00, 0x00, 0x00}, // dmc offset azimuth
	{ 0x08, 0x1f, 0x04, 0xd7, 0x00, 0x00, 0x00, 0x00, 0x00}, // dmc offset elevation
	{ 0x08, 0x1f, 0x04, 0xd7, 0x00, 0x00, 0x00, 0x00, 0x00}, // dmc offset bank
	{ 0x04, 0x1f, 0x00, 0xdd, 0x00}, // dmc save
	{ 0x04, 0x1f, 0x00, 0xc5, 0x00}, // laser power up
	{ 0x04, 0x1f, 0x00, 0xb5, 0x00}, // laser fire
	{ 0x05, 0x1f, 0x01, 0xfa, 0x00, 0x00}, // laser fire mode
	{ 0x04, 0x1f, 0x00, 0xb7, 0x00}, //start ibit
	{ 0x05, 0x1f, 0x01, 0xcf, 0x00, 0x00}, //set gmt

	{ 0x04, 0x1f, 0x00, 0xc1, 0x00}, // ask target coordinates
	{ 0x04, 0x1f, 0x00, 0xb6, 0x00}, // get all system values - ok
	{ 0x04, 0x1f, 0x00, 0xb9, 0x00}, // ask dmc values - ok
	{ 0x04, 0x1f, 0x00, 0xc0, 0x00}, // ask gps location -ok
	{ 0x04, 0x1f, 0x00, 0xe4, 0x00}, // optimization param -ok
	{ 0x04, 0x1f, 0x00, 0xeb, 0x00}, // zoom pos ok
	{ 0x04, 0x1f, 0x00, 0xec, 0x00}, // focus pos - ok
	{ 0x04, 0x1f, 0x00, 0xf1, 0x00}, // gps date time ok
	{ 0x04, 0x1f, 0x00, 0xa2, 0x00}, // flash protection ok
	{ 0x04, 0x1f, 0x00, 0xa3, 0x00}, // current time ok

};
MgeoFalconEyeHead::MgeoFalconEyeHead()
{
	nextSync = C_COUNT;
	settings = {
		{"focus_in", { C_SET_CONTINUOUS_FOCUS, R_FOCUS}},
		{"focus_pos_set", { C_SET_FOCUS, R_FOCUS}},
		{"fov_pos", { C_SET_FOV, R_FOV}},
		{"choose_cam", { C_CHOOSE_CAM, R_CAM}},
		{"digital_zoom", { C_SET_DIGITAL_ZOOM, R_DIGI_ZOOM_POS}},
		{"polarity", { C_SET_IR_POLARITY, R_IR_POLARITY}},
		{"reticle_mode", { C_SET_RETICLE_MODE, R_RETICLE_MODE}},
		{"reticle_type", { C_SET_RETICLE_TYPE, R_RETICLE_TYPE}},
		{"reticle_intensity", { C_SET_RETICLE_INTENSITY, R_RETICLE_INTENSITY}},
		{"symbology", { C_SET_SYMBOLOGY, R_SYMBOLOGY}},
		{"rayleigh_sigma_coeff", { C_SET_RAYLEIGH_SIGMA_COEFF, R_RAYLEIGH_SIGMA}},
		{"rayleigh_e_coeff", { C_SET_RAYLEIGH_E_COEFF, R_RAYLEIGH_E}},
		{"image_proc", { C_SET_IMAGE_PROC, R_IMAGE_PROC}},
		{"hf_sigma_coeff", { C_SET_HF_SIGMA_COEFF, R_HF_SIGMA}},
		{"hf_filter_std", { C_SET_HF_FILTER_STD, R_HF_FILTER}},
		{"one_point_nuc", { C_SET_1_POINT_NUC, R_NUC}},
		{"two_point_nuc", { C_SET_1_POINT_NUC, R_NUC}},
		{"dmc_param", { C_SET_DMC_PARAM, R_DMC_PARAM}},
		{"hekos_param", { C_SET_HEKOS_PARAM, R_HEKOS_PARAM}},
		{"current_integration_mode", { C_SET_CURRENT_INTEGRATION_MODE, R_CURRENT_INTEGRATION_MODE}},
		{"toggle_module_power", { C_SET_TOGGLE_MODULE_POWER_STATUS, NULL}},
		{"optic_bypass", { C_SET_OPTIC_BYPASS, NULL}},
		{"flas_protection", { C_SET_FLASH_PROTECTION, R_FLASH_PROTECTION}},
		{"optic_step", { C_SET_OPTIC_STEP, R_OPTIC_STEP}},
		{"dmc_azimuth", { C_SET_DMC_OFFSET_AZIMUTH, R_DMC_AZIMUTH}},
		{"dmc_elevation", { C_SET_DMC_OFFSET_ELEVATION, R_DMC_ELEVATION}},
		{"dmc_bank", { C_SET_DMC_OFFSET_BANK, R_DMC_BANK}},
		{"dmc_offset_save", { C_SET_DMC_OFFSET_SAVE, NULL}},
		{"laser_up", { C_SET_LASER_UP, R_LASER_STATUS}},
		{"laser_fire", { C_SET_LASER_FIRE, NULL}},
		{"laser_mode" , {C_SET_LASER_FIRE_MODE, R_LASER_MODE}},
		{"ibit_start", { C_SET_IBIT_START, NULL}},
		{"gmt", { C_SET_GMT, R_GMT}},

		{"software_version" , { NULL, R_SOFTWARE_VERSION}},
		{"cooler", { NULL, R_COOLER}},
		{"ibit_power", { NULL, R_IBIT_POWER}},
		{"ibit_system", { NULL, R_IBIT_SYSTEM}},
		{"ibit_optic", { NULL, R_IBIT_OPTIC}},
		{"ibit_lrf", { NULL, R_IBIT_LRF}},
	};
}

int MgeoFalconEyeHead::getCapabilities()
{
	return CAP_ZOOM;
}

int MgeoFalconEyeHead::syncRegisters()
{
	if (!transport)
		return -ENOENT;
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
	cmd[5] = chksum(cmd, len -1);
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

int MgeoFalconEyeHead::getHeadStatus()
{
	if (nextSync == C_COUNT)
		return ST_NORMAL;
	return ST_SYNCING;
}

void MgeoFalconEyeHead::setProperty(uint r, uint x)
{
	if (r == C_SET_CONTINUOUS_FOCUS){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[4] = 0x01;
		if ( x == 0)
			p[4] = 0x00;
		else if (x == 1)
			p[3] = 0x01;
		else if (x == 2)
			p[3] = 0x00;
		p[5] = chksum(p, len - 1);
		sendCommand(p, len);
	}
	else if (r == C_SET_FOCUS){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = (char) (x & 0x000000ff);
		p[4] = (char) (x & 0x0000ff00) >> 8;
		p[5] = (char) (x & 0x00ff0000) >> 16;
		p[6] = (char) (x & 0xff000000) >> 24;
		p[7] = chksum(p, len - 1);
		sendCommand(p, len);
	}
	else if (r == C_SET_FOV){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_FOV, x);
		sendCommand(p, len);
	}
	else if (r == C_CHOOSE_CAM){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_CAM, x);
		sendCommand(p, len);
	}
	else if (r == C_SET_DIGITAL_ZOOM){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_DIGI_ZOOM_POS, x);
		sendCommand(p, len);
	}
	else if (r == C_SET_IR_POLARITY){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_IR_POLARITY, x);
		sendCommand(p, len);
	}
	else if (r == C_SET_RETICLE_MODE){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p[3] = x;
		p[4] = getRegister(R_RETICLE_TYPE);
		p[5] = getRegister(R_RETICLE_INTENSITY);
		p[6] = chksum(p, len - 1);
		setRegister(R_RETICLE_MODE, x);
		sendCommand(p, len);
	}
	else if (r == C_SET_RETICLE_TYPE){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p[3] = getRegister(R_RETICLE_MODE);
		p[4] = x;
		p[5] = getRegister(R_RETICLE_INTENSITY);
		p[6] = chksum(p, len - 1);
		setRegister(R_RETICLE_TYPE, x);
		sendCommand(p, len);
	}
	else if (r == C_SET_RETICLE_INTENSITY){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p[3] = getRegister(R_RETICLE_MODE);
		p[4] = getRegister(R_RETICLE_TYPE);
		p[5] = x;
		p[6] = chksum(p, len - 1);
		setRegister(R_RETICLE_INTENSITY, x);
		sendCommand(p, len);
	}
	else if (r == C_SET_SYMBOLOGY){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_SYMBOLOGY, x);
		sendCommand(p, len);
	}
	else if (r == C_SET_RAYLEIGH_SIGMA_COEFF){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = (char) (x & 0x00ff);
		p[4] = (char) (x & 0xff00) >> 8;
		p[5] = chksum(p, len - 1);
		setRegister(R_RAYLEIGH_SIGMA, x);
		sendCommand(p, len);
	}
	else if (r == C_SET_RAYLEIGH_E_COEFF){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = (char) (x & 0x00ff);
		p[4] = (char) (x & 0xff00) >> 8;
		p[5] = chksum(p, len - 1);
		setRegister(R_RAYLEIGH_E, x);
		sendCommand(p, len);
	}
	else if (r == C_SET_IMAGE_PROC){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_IMAGE_PROC, x);
		sendCommand(p, len);
	}
	else if (r == C_SET_HF_SIGMA_COEFF){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = (char) (x & 0x00ff);
		p[4] = (char) (x & 0xff00) >> 8;
		p[5] = chksum(p, len - 1);
		setRegister(R_HF_SIGMA, x);
		sendCommand(p, len);
	}
	else if (r == C_SET_HF_FILTER_STD){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = (char) (x & 0x00ff);
		p[4] = (char) (x & 0xff00) >> 8;
		p[5] = chksum(p, len - 1);
		setRegister(R_HF_FILTER, x);
		sendCommand(p, len);
	}
	else if (r == C_SET_1_POINT_NUC){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		sendCommand(p, len);
	}
	else if (r == C_SET_2_POINT_NUC){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		sendCommand(p, len);
	}
	else if (r == C_SET_DMC_PARAM){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_DMC_PARAM, x);
		sendCommand(p, len);
	}
	else if (r == C_SET_HEKOS_PARAM){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = (x / 10);
		p[4] = (x % 10) ;
		p[5] = chksum(p, len - 1);
		setRegister(R_HEKOS_PARAM, x);
		sendCommand(p, len);
	}
	else if (r == C_SET_CURRENT_INTEGRATION_MODE){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_CURRENT_INTEGRATION_MODE, x);
		sendCommand(p, len);
	}
	else if (r == C_SET_TOGGLE_MODULE_POWER_STATUS){
		// x < 0 ise off
		// x >= 0 ise on
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		if (x < 0){
			p[3] = (-1 * x);
			p[4] = 0x00;
		}
		else {
			p[3] = x;
			p[4] = 0x01;
		}
		p[5] = chksum(p, len - 1);
		sendCommand(p, len);
	}
	else if (r == C_SET_OPTIC_BYPASS){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = chksum(p, len - 1);
		sendCommand(p, len);
	}
	else if (r == C_SET_FLASH_PROTECTION){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_FLASH_PROTECTION, x);
		sendCommand(p, len);
	}
	else if (r == C_SET_OPTIC_STEP){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_OPTIC_STEP, x);
		sendCommand(p, len);
	}
	else if (r == C_SET_DMC_OFFSET_AZIMUTH) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p[3] = qAbs(x);
		p[4] = R_DMC_ELEVATION;
		p[5] = R_DMC_BANK;
		if (x < 0)
			p[6] = 0x04 + R_DMC_OFFSET_SIGN;
		else p[6] = R_DMC_OFFSET_SIGN;
		p[7] = chksum(p, len - 1);
		setRegister(R_DMC_AZIMUTH, x);
		setRegister(R_DMC_OFFSET_SIGN, p[6]);
		sendCommand(p, len);
	}
	else if (r == C_SET_DMC_OFFSET_ELEVATION) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p[3] = R_DMC_AZIMUTH;
		p[4] = qAbs(x);
		p[5] = R_DMC_BANK;
		if (x < 0)
			p[6] = 0x02 + R_DMC_OFFSET_SIGN;
		else p[6] = R_DMC_OFFSET_SIGN;
		p[7] = chksum(p, len - 1);
		setRegister(R_DMC_ELEVATION, x);
		setRegister(R_DMC_OFFSET_SIGN, p[6]);
		sendCommand(p, len);
	}
	else if (r == C_SET_DMC_OFFSET_BANK) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p[3] = R_DMC_AZIMUTH;
		p[4] = R_DMC_ELEVATION;
		p[5] = qAbs(x);
		if (x < 0)
			p[6] = 0x01 + R_DMC_OFFSET_SIGN;
		else p[6] = R_DMC_OFFSET_SIGN;
		p[7] = chksum(p, len - 1);
		setRegister(R_DMC_BANK, x);
		setRegister(R_DMC_OFFSET_SIGN, p[6]);
		sendCommand(p, len);
	}
	else if (r == C_SET_DMC_OFFSET_SAVE){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = chksum(p, len - 1);
		sendCommand(p, len);
	}
	else if (r == C_SET_LASER_UP){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = chksum(p, len - 1);
		setRegister(R_LASER_STATUS, 1);
		sendCommand(p, len);
	}
	else if (r == C_SET_LASER_FIRE){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = chksum(p, len - 1);
		sendCommand(p, len);
	}
	else if (r == C_SET_LASER_FIRE_MODE){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[4] = chksum(p, len - 1);
		setRegister(R_LASER_MODE, x);
		sendCommand(p, len);
	}
	else if (r == C_SET_IBIT_START){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = chksum(p, len - 1);
		sendCommand(p, len);
	}
	else if (r == C_SET_GMT){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		if (x >= 0)
			p[3] = x*10;
		else if (x < 0)
			p[3] = (x * -10) | (1 << 7);
		p[4] = chksum(p, len - 1);
		setRegister(R_GMT, x);
		sendCommand(p, len);
	}
	else if (r == C_SET_BUTTON_PRESSED){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[5] = chksum(p, len - 1);
		sendCommand(p, len);
	}
	else if (r == C_SET_BUTTON_RELEASED){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[3] = x;
		p[5] = chksum(p, len - 1);
		sendCommand(p, len);
	}
}

QByteArray MgeoFalconEyeHead::transportReady()
{
	unsigned char *p = protoBytes[C_GET_ZOOM_POS];
	int len = p[0];
	p++;
	p[3] = chksum(p ,len);
	sendCommand(p, len);
}

int MgeoFalconEyeHead::sendCommand(const unsigned char *cmd, int len)
{
	return transport->send((const char *)cmd, len);
}

