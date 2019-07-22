#include "yamanolenshead.h"
#include "debug.h"
#include "ptzptransport.h"
#include "ioerrors.h"
#include "errno.h"

#define MAX_CMD_LEN	10

enum Commands {
	C_ZOOM_IN,
	C_ZOOM_OUT,
	C_ZOOM_STOP,
	C_ZOOM_SET,
	C_FOCUS_IN,
	C_FOCUS_OUT,
	C_FOCUS_STOP,
	C_FOCUS_SET,
	C_IRIS_OPEN,
	C_IRIS_CLOSE,
	C_IRIS_STOP,
	C_IRIS_SET,
	C_IR_FILTER_IN,
	C_IR_FILTER_OUT,
	C_IR_FILTER_STOP,
	C_FILTER_POS,
	C_AUTO_FOCUS,
	C_AUTO_IRIS,

	C_GET_ZOOM,
	C_GET_FOCUS,
	C_GET_IRIS,
	C_GET_VERSION,

	C_COUNT
};

static unsigned char protoBytes[C_COUNT][MAX_CMD_LEN] = {
	{0x07, 0xff, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00},//zoom in
	{0x07, 0xff, 0x01, 0x00, 0x40, 0x00, 0x00, 0x00},//zoom out
	{0x07, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00},//zoom stop
	{0x07, 0xff, 0x01, 0x00, 0x82, 0x00, 0x00, 0x00},//zoom set position
	{0x07, 0xff, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00},//focus in
	{0x07, 0xff, 0x01, 0x00, 0x80, 0x00, 0x00, 0x00},//focus out
	{0x07, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00},//focus stop
	{0x07, 0xff, 0x01, 0x00, 0x81, 0x00, 0x00, 0x00},//focus set
	{0x07, 0xff, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00},//iris open
	{0x07, 0xff, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00},//iris close
	{0x07, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00},//iris stop
	{0x07, 0xff, 0x01, 0x00, 0x83, 0x00, 0x00, 0x00},//iris set
	{0x07, 0xff, 0x01, 0x00, 0x09, 0x00, 0x01, 0x00},//ir filter in
	{0x07, 0xff, 0x01, 0x00, 0x0B, 0x00, 0x01, 0x00},//ir filter out
	{0x07, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00},//ir filter stop
	{0x07, 0xff, 0x01, 0x00, 0x0B, 0x00, 0x00, 0x00},//filter pos
	{0x07, 0xff, 0x01, 0x00, 0x2B, 0x00, 0x00, 0x00},//auto focus
	{0x07, 0xff, 0x01, 0x00, 0x2D, 0x00, 0x00, 0x00},//auto iris

	{0x07, 0xff, 0x01, 0x00, 0x85, 0x00, 0x00, 0x86},//zoom get position
	{0x07, 0xff, 0x01, 0x00, 0x84, 0x00, 0x00, 0x85},//focus get position
	{0x07, 0xff, 0x01, 0x00, 0x86, 0x00, 0x00, 0x87},//iris get position
	{0x07, 0xff, 0x01, 0x00, 0x73, 0x00, 0x00, 0x74},//version get
};

static uchar chksum(const uchar *buf, int len)
{
	uchar sum = 0;
	for (int i = 1; i < len; i++)
		sum += buf[i];
	return sum;
}

YamanoLensHead::YamanoLensHead()
{
	nextSync = C_COUNT;
	syncEnabled = true;
	syncInterval = 40;
	syncTime.start();
#ifdef HAVE_PTZP_GRPC_API
	settings = {
		{"focus_in", {C_FOCUS_IN, R_FOCUS_POS}},
		{"focus_out", {C_FOCUS_OUT, R_FOCUS_POS}},
		{"focus_stop", {C_FOCUS_STOP, R_FOCUS_POS}},
		{"focus_set", {C_FOCUS_SET, R_FOCUS_POS}},
		{"iris_open", {C_IRIS_OPEN, R_IRIS_POS}},
		{"iris_close", {C_IRIS_CLOSE, R_IRIS_POS}},
		{"iris_stop", {C_IRIS_STOP, R_IRIS_POS}},
		{"iris_set", {C_IRIS_SET, R_IRIS_POS}},
		{"ir_filter_in", {C_IR_FILTER_IN, NULL}},
		{"ir_filter_out", {C_IR_FILTER_OUT, NULL}},
		{"ir_filter_stop", {C_IR_FILTER_STOP, NULL}},
		{"filter_pos", {C_FILTER_POS, R_FILTER_POS}},
		{"auto_focus", {C_AUTO_FOCUS, R_FOCUS_MODE}},
		{"auto_iris", {C_AUTO_IRIS, R_IRIS_MODE}},
		{"version", {NULL, R_VERSION}},
	};
#endif
}

int YamanoLensHead::getCapabilities()
{
	return PtzpHead::CAP_ZOOM | PtzpHead::CAP_ADVANCED;
}

int YamanoLensHead::syncRegisters()
{
	if (!transport)
		return -ENOENT;
	nextSync = C_GET_ZOOM;
	return syncNext();
}

int YamanoLensHead::getHeadStatus()
{
	if (nextSync == C_COUNT)
		return ST_NORMAL;
	return ST_SYNCING;
}

int YamanoLensHead::startZoomIn(int speed)
{
	Q_UNUSED(speed);
	unsigned char *p = protoBytes[C_ZOOM_IN];
	int len = p[0];
	p++;
	p[6] = chksum(p, len - 1);
	return transport->send((const char *)p, len);
}

int YamanoLensHead::startZoomOut(int speed)
{
	Q_UNUSED(speed);
	unsigned char *p = protoBytes[C_ZOOM_OUT];
	int len = p[0];
	p++;
	p[6] = chksum(p, len - 1);
	return transport->send((const char *)p, len);
}

int YamanoLensHead::stopZoom()
{
	unsigned char *p = protoBytes[C_ZOOM_STOP];
	int len = p[0];
	p++;
	p[6] = chksum(p, len - 1);
	return transport->send((const char *)p, len);
}

int YamanoLensHead::getZoom()
{
	return getRegister(R_ZOOM_POS);
}

int YamanoLensHead::setZoom(uint pos)
{
	unsigned char *p = protoBytes[C_ZOOM_SET];
	int len = p[0];
	p++;
	p[4] = (pos & 0xFF00) >> 8;
	p[5] = pos & 0x00FF;
	p[6] = chksum(p, len - 1);
	setRegister(R_ZOOM_POS, pos);
	return transport->send((const char *)p, len);
}

int YamanoLensHead::focusIn(int speed)
{
	Q_UNUSED(speed);
	unsigned char *p = protoBytes[C_FOCUS_IN];
	int len = p[0];
	p++;
	p[6] = chksum(p, len - 1);
	return transport->send((const char *)p, len);
}

int YamanoLensHead::focusOut(int speed)
{
	Q_UNUSED(speed);
	unsigned char *p = protoBytes[C_FOCUS_OUT];
	int len = p[0];
	p++;
	p[6] = chksum(p, len - 1);
	return transport->send((const char *)p, len);
}

int YamanoLensHead::focusStop()
{
	unsigned char *p = protoBytes[C_FOCUS_STOP];
	int len = p[0];
	p++;
	p[6] = chksum(p, len - 1);
	return transport->send((const char *)p, len);
}

uint YamanoLensHead::getProperty(uint r)
{
	return getRegister(r);
}

void YamanoLensHead::setProperty(uint r, uint x)
{
	if (r == C_FOCUS_IN) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[6] = chksum(p, len - 1);
		transport->send((const char *)p, len);
	} else if (r == C_FOCUS_OUT) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[6] = chksum(p, len - 1);
		transport->send((const char *)p, len);
	} else if (r == C_FOCUS_STOP) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[6] = chksum(p, len - 1);
		transport->send((const char *)p, len);
	} else if (r == C_FOCUS_SET) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[4] = (x & 0xFF00) >> 8;
		p[5] = (x & 0x00FF);
		p[6] = chksum(p, len - 1);
		setRegister(R_FOCUS_POS, x);
		transport->send((const char *)p, len);
	} else if (r == C_IRIS_OPEN){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[6] = chksum(p, len - 1);
		transport->send((const char *)p, len);
	} else if (r == C_IRIS_CLOSE){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[6] = chksum(p, len - 1);
		transport->send((const char *)p, len);
	} else if (r == C_IRIS_STOP){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[6] = chksum(p, len - 1);
		transport->send((const char *)p, len);
	} else if (r == C_IRIS_SET){
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[4] = (x & 0xFF00) >> 8;
		p[5] = (x & 0x00FF);
		p[6] = chksum(p, len - 1);
		setRegister(R_IRIS_POS, x);
		transport->send((const char *)p, len);
	} else if (r == C_IR_FILTER_IN) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[6] = chksum(p, len - 1);
		transport->send((const char *)p, len);
	} else if (r == C_IR_FILTER_OUT) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[6] = chksum(p, len - 1);
		transport->send((const char *)p, len);
	} else if (r == C_IR_FILTER_STOP) {
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[6] = chksum(p, len - 1);
		transport->send((const char *)p, len);
	} else if (r == C_FILTER_POS){
		if (!(x > 1 && x <= 6))
			return;
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[5] = x;
		p[6] = chksum(p, len - 1);
		setRegister(R_FILTER_POS, x);
		transport->send((const char *)p, len);
	} else if (r == C_AUTO_FOCUS) {
		if (!(x >0 && x < 4))
			return;
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[5] = x;
		p[6] = chksum(p, len - 1);
		setRegister(R_FOCUS_MODE, x);
		transport->send((const char *)p, len);
	} else if (r == C_AUTO_IRIS) {
		if (!(x >0 && x < 3))
			return;
		unsigned char *p = protoBytes[r];
		int len = p[0];
		p++;
		p[5] = x;
		p[6] = chksum(p, len - 1);
		setRegister(R_IRIS_MODE, x);
		transport->send((const char *)p, len);
	}
}

void YamanoLensHead::enableSyncing(bool en)
{
	syncEnabled = en;
}

void YamanoLensHead::setSyncInterval(int interval)
{
	syncInterval = interval;
}

int YamanoLensHead::syncNext()
{
	const unsigned char *p = protoBytes[nextSync];
	return transport->send((const char *)p + 1, p[0]);
}

int YamanoLensHead::dataReady(const unsigned char *bytes, int len)
{
	if (bytes[0] != 0xFF)
		return len;

	if (bytes[1] != 0x01)
		return len;

	if (len < 5)
		return len;
	pingTimer.restart();

	if (nextSync != C_COUNT) {
		/* we are in sync mode, let's sync next */
		mInfo("Next sync property: %d",nextSync);
		if (++nextSync == C_COUNT) {
			fDebug("Yamano register syncing completed, activating auto-sync");
		} else
			syncNext();
	}

	uchar chk = chksum(bytes, len - 1);
	if (chk != bytes[len - 1]) {
		fDebug("Checksum error");
		return len;
	}

	if (bytes[3] == 0x85) {
		setRegister(R_ZOOM_POS, ((bytes[4] << 8) + bytes[5]));
	} else if (bytes[3] == 0x84) {
		setRegister(R_FOCUS_POS, ((bytes[4] << 8) + bytes[5]));
	}else if (bytes[3] == 0x86) {
		setRegister(R_IRIS_POS, ((bytes[4] << 8) + bytes[5]));
	}else if (bytes[3] == 0x73) {
		setRegister(R_VERSION, ((bytes[4] << 8) + bytes[5]));
	}

	return len;
}

QByteArray YamanoLensHead::transportReady()
{
	unsigned char *p = protoBytes[C_GET_ZOOM];
	return QByteArray((const char *)p + 1, p[0]);
}
