#include "mgeodortgozhead.h"
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

/* Begin here
 * This code is licensed under the MIT License as stated below
 * Copyright (c) 1999-2016 Lammert Bies
 *
 * I just edited for this head.
 * Furkan ÖZOĞUL
 *
 */

static bool crc_tabccitt_init = false;
static uint16_t crc_tabccitt[256];

static void init_crcccitt_tab( void ) {

	uint16_t i;
	uint16_t j;
	uint16_t crc;
	uint16_t c;

	for (i=0; i<256; i++) {

		crc = 0;
		c   = i << 8;

		for (j=0; j<8; j++) {

			if ( (crc ^ c) & 0x8000 ) crc = ( crc << 1 ) ^ 0x1021;
			else                      crc =   crc << 1;

			c = c << 1;
		}

		crc_tabccitt[i] = crc;
	}

	crc_tabccitt_init = true;

}

static uint16_t crc_ccitt_generic( const unsigned char *input_str, size_t num_bytes) {

	uint16_t crc;
	uint16_t tmp;
	uint16_t short_c;
	const unsigned char *ptr;
	size_t a;

	if ( ! crc_tabccitt_init ) init_crcccitt_tab();

	crc = 0x0000;
	ptr = input_str;

	if ( ptr != NULL ) for (a=0; a<num_bytes; a++) {

		short_c = 0x00ff & (unsigned short) *ptr;
		tmp     = (crc >> 8) ^ short_c;
		crc     = (crc << 8) ^ crc_tabccitt[tmp];

		ptr++;
	}

	return crc;

}

/*
 * End
 */

#define MAX_CMD_LEN 43

enum Commands {
	C_SET_STATE,
//	C_SET_VIDEO_1, wip
	C_SET_VIDEO_STATE, // live/freeze (video2)
	C_SET_VIDEO_ROTATION, // video rotation (flip/mirror) (video2)
	C_SET_VIDEO_FRAME_RATE, // frame rate 12.5, 25, 50 (video3)
	C_SET_ZOOM,
	C_SET_E_ZOOM,
	C_SET_FOCUS,
	C_SET_BRIGHTNES,
	C_SET_CONTRAST,
	C_SET_RETICLE_MODE,
	C_SET_RETICLE_INTENSITY,
	C_SET_SYMBOLOGY,
	C_SET_POLARITY,
	C_SET_IMAGE_PROC,
	C_SET_FOCUS_MODE,
	C_SET_NUC,
	C_SET_RELAY_CONTROL,
	C_SET_FOV,
	C_SET_NUC_TABLE,
	C_SET_IBIT,
	C_SET_BUTTON,

	C_GET_IBIT,

	C_GET_ALL_SYSTEM_VALUES,
	C_COUNT
};

static unsigned char protoBytes[C_COUNT][MAX_CMD_LEN] = {
//	 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41
	{0x2a,
	 0x00, 0x01 ,0x22, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
	 0x00, 0x00}, //set state
//	{0x2a,
//	 0x00, 0x01 ,0x22, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
//	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
//	 0x00, 0x00}, // video 1
	{0x2a,
	 0x00, 0x01 ,0x22, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
	 0x00, 0x00}, // video
	{0x2a,
	 0x00, 0x01 ,0x22, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
	 0x00, 0x00}, // video 3 rotation
	{0x2a,
	 0x00, 0x01 ,0x22, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
	 0x00, 0x00}, // video frame rate
	{0x2a,
	 0x00, 0x01 ,0x22, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
	 0x00, 0x00}, //zoom
	{0x2a,
	 0x00, 0x01 ,0x22, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
	 0x00, 0x00}, // e -zoom
	{0x2a,
	 0x00, 0x01 ,0x22, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
	 0x00, 0x00}, // focus
	{0x2a,
	 0x00, 0x01 ,0x22, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
	 0x00, 0x00}, // brightness
	{0x2a,
	 0x00, 0x01 ,0x22, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
	 0x00, 0x00}, // contrast
	{0x2a,
	 0x00, 0x01 ,0x22, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
	 0x00, 0x00}, // reticle mode
	{0x2a,
	 0x00, 0x01 ,0x22, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
	 0x00, 0x00}, // reticle intensity
	{0x2a,
	 0x00, 0x01 ,0x22, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
	 0x00, 0x00}, // symbology
	{0x2a,
	 0x00, 0x01 ,0x22, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
	 0x00, 0x00}, // polarity
	{0x2a,
	 0x00, 0x01 ,0x22, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
	 0x00, 0x00}, // image process mode
	{0x2a,
	 0x00, 0x01 ,0x22, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
	 0x00, 0x00}, // focus mode
	{0x2a,
	 0x00, 0x01 ,0x22, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
	 0x00, 0x00}, // nuc
	{}, // relay control
	{0x2a,
	 0x00, 0x01 ,0x22, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
	 0x00, 0x00}, // fov
	{0x2a,
	 0x00, 0x01 ,0x22, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
	 0x00, 0x00}, // nuc table
	{0x09,
	 0x03, 0x01 ,0x01, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00}, // ibit
	{0x0a,
	 0x09, 0x01, 0x02, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00}, // button

	{0x09,
	 0x03, 0x01 ,0x01, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00}, //get ibit

	{0x2a,
	 0x00, 0x01 ,0x22, 0x00, 0x01, 0x02, 0x04, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00,
	 0x00, 0x00}, // get all system value
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

MgeoDortgozHead::MgeoDortgozHead(QList<int> relayConfig)
{
	nextSync = C_GET_ALL_SYSTEM_VALUES;
	syncTimer.start();
	if (relayConfig.size() == 3) {
		dayCamRelay = relayConfig[0];
		thermalRelay = relayConfig[1];
		standbyRelay = relayConfig[2];
	}
	i2c = new PCA9538Driver;
	i2c->open();
	i2c->resetAllPorts();
	relth = new RelayControlThread;
	relth->i2c = i2c;
	relth->dayCamRelay = dayCamRelay;
	relth->thermalRelay = thermalRelay;
	relth->standbyRelay = standbyRelay;
	relth->start();
	settings = {
		{"choose_cam", {C_SET_VIDEO_STATE, R_STATE}},
		{"digital_zoom", {C_SET_E_ZOOM, R_E_ZOOM}},
		{"polarity", {C_SET_POLARITY, R_POLARITY}},
		{"reticle_mode", {C_SET_RETICLE_MODE, R_RETICLE_MODE}},
		{"reticle_type", {C_SET_RETICLE_MODE, R_RETICLE_MODE}},
		{"reticle_intensity", {C_SET_RETICLE_INTENSITY, R_RETICLE_INTENSITY}},
		{"symbology", {C_SET_SYMBOLOGY, R_SYMBOLOGY}},
		{"image_proc", {C_SET_IMAGE_PROC, R_IMG_PROC_MODE}},
		{"one_point_nuc", {C_SET_NUC, 0}},
		{"relay_control", {C_SET_RELAY_CONTROL, R_RELAY_STATUS}},
		{"brightness_change", {C_SET_BRIGHTNES, R_BRIGTNESS}},
		{"contrast_change", {C_SET_CONTRAST, R_CONTRAST}},
		{"button", {C_SET_BUTTON, 0}},
		{"focus", {}},
		{"fov", {C_SET_FOV, R_FOV}},
		{"thermal_chart", {C_SET_NUC_TABLE, R_THERMAL_TABLE}},
		{"start_ibit", {C_SET_IBIT, 0}},
		{"get_ibit", {C_GET_IBIT, R_IBIT_RESULT}},
		{"auto_focus", {C_SET_FOCUS_MODE, R_FOCUS_MODE}},

	};
}

void MgeoDortgozHead::fillCapabilities(ptzp::PtzHead *head)
{
	head->add_capabilities(ptzp::PtzHead_Capability_ZOOM);
	head->add_capabilities(ptzp::PtzHead_Capability_FOCUS);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_ZOOM);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_NIGHT_VIEW);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_FOCUS);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_DIGITAL_ZOOM);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_SHOW_RETICLE);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_SHOW_HIDE_SYMBOLOGY);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_POLARITY);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_NUC);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_BRIGHTNESS);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_CONTRAST);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_THERMAL_STANDBY_MODES);
	head->add_capabilities(ptzp::PtzHead_Capability_KARDELEN_MENU_OVER_VIDEO);
}

int MgeoDortgozHead::syncRegisters()
{
	if(!transport)
		return -ENOENT;
	nextSync = C_GET_ALL_SYSTEM_VALUES;
	return syncNext();
}

int MgeoDortgozHead::startZoomIn(int speed)
{
	uchar *cmd = protoBytes[C_SET_ZOOM];
	int len = cmd[0];
	cmd++;
	cmd[10] = (speed << 6) + 0x02;
	int chk = crc_ccitt_generic(cmd, len - 2);
	cmd[40] = chk & 0x00FF;
	cmd[41] = chk >> 8;
	return sendCommand(cmd, len);
}

int MgeoDortgozHead::startZoomOut(int speed)
{
	uchar *cmd = protoBytes[C_SET_ZOOM];
	int len = cmd[0];
	cmd++;
	cmd[10] = (speed << 6) + 0x03;
	int chk = crc_ccitt_generic(cmd, len - 2);
	cmd[40] = chk & 0x00FF;
	cmd[41] = chk >> 8;
	return sendCommand(cmd, len);
}

int MgeoDortgozHead::stopZoom()
{
	uchar *cmd = protoBytes[C_SET_ZOOM];
	int len = cmd[0];
	cmd++;
	cmd[10] =(2 << 6) + 0x01;
	int chk = crc_ccitt_generic(cmd, len - 2);
	cmd[40] = chk & 0x00FF;
	cmd[41] = chk >> 8;
	return sendCommand(cmd, len);
}

int MgeoDortgozHead::setZoom(uint pos)
{
	uchar *cmd = protoBytes[C_SET_ZOOM];
	int len = cmd[0];
	cmd++;
	cmd[10] =(2 << 6) + 0x06;
	cmd[14] = pos & 0x00FF;
	cmd[15] = pos >> 8;
	int chk = crc_ccitt_generic(cmd, len - 2);
	cmd[40] = chk & 0x00FF;
	cmd[41] = chk >> 8;
	return sendCommand(cmd, len);
}

int MgeoDortgozHead::getZoom()
{
	return getRegister(R_ZOOM_ENC_POS);
}

int MgeoDortgozHead::focusIn(int speed)
{
	uchar *cmd = protoBytes[C_SET_FOCUS];
	int len = cmd[0];
	cmd++;
	cmd[17] = (2 << 6) + 0x02;
	int chk = crc_ccitt_generic(cmd, len - 2);
	cmd[40] = chk & 0x00FF;
	cmd[41] = chk >> 8;
	return sendCommand(cmd, len);
}

int MgeoDortgozHead::focusOut(int speed)
{
	uchar *cmd = protoBytes[C_SET_ZOOM];
	int len = cmd[0];
	cmd++;
	cmd[17] = (2 << 6) + 0x03;
	int chk = crc_ccitt_generic(cmd, len - 2);
	cmd[40] = chk & 0x00FF;
	cmd[41] = chk >> 8;
	return sendCommand(cmd, len);
}

int MgeoDortgozHead::focusStop()
{
	uchar *cmd = protoBytes[C_SET_ZOOM];
	int len = cmd[0];
	cmd++;
	cmd[17] = 0x01;
	int chk = crc_ccitt_generic(cmd, len - 2);
	cmd[40] = chk & 0x00FF;
	cmd[41] = chk >> 8;
	return sendCommand(cmd, len);
}

int MgeoDortgozHead::getHeadStatus()
{
	if (nextSync == C_COUNT)
		return ST_NORMAL;
	return ST_SYNCING;
}

void MgeoDortgozHead::setProperty(uint r, uint x)
{
	if(r == C_SET_STATE){
//		termal: 1 standby:2
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[6] = x;
		int chk = crc_ccitt_generic(p, len - 2);
		p[40] = chk & 0x00FF;
		p[41] = chk >> 8;
		setRegister(R_STATE, x);
		sendCommand(p, len);
	} else if (r == C_SET_VIDEO_STATE) {
//		live: 1 freeze: 2
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[8] = x;
		int chk = crc_ccitt_generic(p, len - 2);
		p[40] = chk & 0x00FF;
		p[41] = chk >> 8;
		setRegister(R_VIDEO_STATE, x);
		sendCommand(p, len);
	} else if (r == C_SET_VIDEO_ROTATION) {
//		off: 0 mirror: 1 flip: 2 flip+mirror: 3
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		if(x == 0)
			p[8] == 0x00;
		else if (x == 1)
			p[8] = 1 << 2;
		else if (x == 2)
			p[8] = 1 << 4;
		else if (x ==3)
			p[8] = (1 << 4) + (1 << 2);
		int chk = crc_ccitt_generic(p, len - 2);
		p[40] = chk & 0x00FF;
		p[41] = chk >> 8;
		setRegister(R_VIDEO_ROTATION, x);
		sendCommand(p, len);
	} else if (r == C_SET_VIDEO_FRAME_RATE) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[9] = x;
		int chk = crc_ccitt_generic(p, len - 2);
		p[40] = chk & 0x00FF;
		p[41] = chk >> 8;
		setRegister(R_VIDEO_FRAME_RATE, x);
		sendCommand(p, len);
	} else if (r == C_SET_E_ZOOM) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[16] = x;
		int chk = crc_ccitt_generic(p, len - 2);
		p[40] = chk & 0x00FF;
		p[41] = chk >> 8;
		setRegister(R_E_ZOOM, x);
		sendCommand(p, len);
	} else if (r == C_SET_BRIGHTNES) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[20] = (x << 1) + 0x01;
		int chk = crc_ccitt_generic(p, len - 2);
		p[40] = chk & 0x00FF;
		p[41] = chk >> 8;
		setRegister(R_BRIGTNESS, x);
		sendCommand(p, len);
	} else if (r == C_SET_CONTRAST) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[21] = (x << 1) + 0x01;
		int chk = crc_ccitt_generic(p, len - 2);
		p[40] = chk & 0x00FF;
		p[41] = chk >> 8;
		setRegister(R_CONTRAST, x);
		sendCommand(p, len);
	} else if (r == C_SET_RETICLE_MODE) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[28] = x;
		int chk = crc_ccitt_generic(p, len - 2);
		p[40] = chk & 0x00FF;
		p[41] = chk >> 8;
		setRegister(R_RETICLE_MODE, x);
		sendCommand(p, len);
	} else if (r == C_SET_RETICLE_INTENSITY) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[28] = (x << 3) + (1 << 2);
		int chk = crc_ccitt_generic(p, len - 2);
		p[40] = chk & 0x00FF;
		p[41] = chk >> 8;
		setRegister(R_RETICLE_INTENSITY, x);
		sendCommand(p, len);
	} else if (r == C_SET_SYMBOLOGY) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[29] = x << 6;
		int chk = crc_ccitt_generic(p, len - 2);
		p[40] = chk & 0x00FF;
		p[41] = chk >> 8;
		setRegister(R_SYMBOLOGY, x);
		sendCommand(p, len);
	} else if (r == C_SET_POLARITY) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[34] = x;
		int chk = crc_ccitt_generic(p, len - 2);
		p[40] = chk & 0x00FF;
		p[41] = chk >> 8;
		setRegister(R_POLARITY, x);
		sendCommand(p, len);
	} else if (r == C_SET_IMAGE_PROC) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[35] = x ;
		int chk = crc_ccitt_generic(p, len - 2);
		p[40] = chk & 0x00FF;
		p[41] = chk >> 8;
		setRegister(R_IMG_PROC_MODE, x);
		sendCommand(p, len);
	} else if (r == C_SET_FOCUS_MODE) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[36] = x;
		int chk = crc_ccitt_generic(p, len - 2);
		p[40] = chk & 0x00FF;
		p[41] = chk >> 8;
		setRegister(R_FOCUS_MODE, x);
		sendCommand(p, len);
	} else if (r == C_SET_NUC) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[34] = 1 << 6;
		int chk = crc_ccitt_generic(p, len - 2);
		p[40] = chk & 0x00FF;
		p[41] = chk >> 8;
		sendCommand(p, len);
	} else if (r == C_SET_RELAY_CONTROL) { // switch mode selection
		if (!relth->switch2(x))
			setRegister(R_RELAY_STATUS, x);
	}
	else if (r == C_SET_FOV) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		if(x == 0)
			p[10] = (2 << 6) + 0x07;
		else if (x == 1)
			p[10] = (2 << 6) + 0x08;
		else if (x == 2)
			p[10] = (2 << 6) + 0x09;
		int chk = crc_ccitt_generic(p, len - 2);
		p[40] = chk & 0x00FF;
		p[41] = chk >> 8;
		setRegister(R_FOV, x);
		sendCommand(p, len);
	}
	else if (r == C_SET_NUC_TABLE){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[34] = x << 4;
		int chk = crc_ccitt_generic(p, len - 2);
		p[40] = chk & 0x00FF;
		p[41] = chk >> 8;
		setRegister(R_THERMAL_TABLE, x);
		sendCommand(p, len);
	}
	else if (r == C_SET_IBIT){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[6] = 0x01;
		int chk = crc_ccitt_generic(p, len - 2);
		p[7] = chk & 0x00FF;
		p[8] = chk >> 8;
		sendCommand(p, len);
	}
	else if (r == C_GET_IBIT){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[6] = 0x03;
		int chk = crc_ccitt_generic(p, len - 2);
		p[7] = chk & 0x00FF;
		p[8] = chk >> 8;
		sendCommand(p, len);
	}
	else if (r == C_SET_BUTTON){
		int i = (int)x;
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		if(i < 0){
			p[6] = -1*i;
			p[7] = 0x01;
		} else {
			p[6] = i;
			p[7] = 0x02;
		}
		int chk = crc_ccitt_generic(p, len - 2);
		p[8] = chk & 0x00FF;
		p[9] = chk >> 8;
		sendCommand(p, len);
	}
}

uint MgeoDortgozHead::getProperty(uint r)
{
	return getRegister(r);
}

int MgeoDortgozHead::syncNext()
{
	unsigned char *p = protoBytes[nextSync];
	int len = p[0];
	p++;
	int chk = crc_ccitt_generic(p, len - 2);
	p[40] = chk & 0x00FF;
	p[41] = chk >> 8;
	return sendCommand(p, len);
}

int MgeoDortgozHead::dataReady(const unsigned char *bytes, int len)
{
	if(len < bytes[2])
		return -EAGAIN;

	if(bytes[4] != 0x01)
		return -ENOENT;
	if(bytes[5] != 0x02)
		return -ENOENT;

	int chk = crc_ccitt_generic(bytes, len - 2);
	if(((chk >> 8) != bytes[41]) || ((chk &0x00FF) != bytes[40])){
		fDebug("Cheksum error");
		return len;
	}

	pingTimer.restart();
	if (nextSync != C_COUNT) {
		mInfo("Next sync property: %d", nextSync);
		if (++nextSync == C_COUNT) {
			fDebug(
				"Dortgoz register syncing completed, activating auto-sync");
		} else
			syncNext();
	}
	if (bytes[0] == 0x00 && bytes[1] == 0x01){
		uint fov = bytes[10] & 0x0F;
		if(fov >= 0x07)
			setRegister(R_FOV, fov - 0x07);
		setRegister(R_STATE, bytes[6] & 0x03);
//		setRegister(, bytes[7])

		setRegister(R_VIDEO_STATE, bytes[8] & 0x03);
		setRegister(R_VIDEO_ROTATION, (bytes[8] >> 2) & 0x0F);
		setRegister(R_VIDEO_FRAME_RATE, bytes[9] & 0x03);
		setRegister(R_ZOOM_SPEED, bytes[10] >> 6);
		setRegister(R_ZOOM_STEP_NUM, bytes[11]);
		setRegister(R_ZOOM_ANGLE, (int)bytes[13] + (((int)bytes[12]) / 100) );
		setRegister(R_ZOOM_ENC_POS, (bytes[15] << 8 )+ bytes[14]);
		setRegister(R_E_ZOOM, bytes[16]);
		setRegister(R_FOCUS_SPEED, bytes[17] >> 6);
		setRegister(R_FOCUS_ENC_POS, (bytes[19] >> 8) + bytes[18]);
		setRegister(R_BRIGTNESS, bytes[20] >> 1);
		setRegister(R_CONTRAST, bytes[21] >> 1);
		setRegister(R_RETICLE_MODE, bytes[28] & 0x03);
		setRegister(R_RETICLE_INTENSITY, bytes[28] >> 0x0F);
		setRegister(R_SYMBOLOGY, bytes[29] >> 6);
		setRegister(R_POLARITY, bytes[34] & 0x07 );
		setRegister(R_THERMAL_TABLE, (bytes[34] >> 4) & 0x03);
		setRegister(R_IMG_PROC_MODE, bytes[35] & 0x0F);
	}

	if(bytes[0] == 0x03 && bytes[1] == 0x01){
		setRegister(R_IBIT_RESULT,(bytes[7] + bytes[8]+ bytes[9]+ bytes[10]));
	}

	return len;
}

QByteArray MgeoDortgozHead::transportReady()
{
	if(syncTimer.elapsed() > 500){
		syncTimer.restart();
		unsigned char *p = protoBytes[C_GET_ALL_SYSTEM_VALUES];
		int len = p[0];
		p++;
		int chk = crc_ccitt_generic(p, len - 2);
		p[40] = chk & 0x00FF;
		p[41] = chk >> 8;
		return QByteArray((const char *)p, len);
	}
	return QByteArray();
}

int MgeoDortgozHead::sendCommand(const unsigned char *cmd, int len)
{
	return transport->send((const char *)cmd, len);
}

int MgeoDortgozHead::readRelayConfig(QString filename)
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

