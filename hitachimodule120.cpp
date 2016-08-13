#include "hitachimodule120.h"


HitachiModule120::HitachiModule120(QextSerialPort *port, QObject *parent) :
	HitachiModule(port, parent)
{
	/* zoom points */
	xPoints << 0x2372;
	xPoints << 0x3291;
	xPoints << 0x3b83;
	xPoints << 0x41b0;
	xPoints << 0x4668;
	xPoints << 0x49fb;
	xPoints << 0x4d3c;
	xPoints << 0x5000;
	xPoints << 0x5270;
	xPoints << 0x548d;
	xPoints << 0x56aa;
	xPoints << 0x589e;
	xPoints << 0x5a68;
	xPoints << 0x5bde;
	xPoints << 0x5d2b;
	xPoints << 0x5e4f;
	xPoints << 0x5f48;
	xPoints << 0x6018;
	xPoints << 0x60bf;
	xPoints << 0x6165;
	xPoints << 0x61e2;
	xPoints << 0x625f;
	xPoints << 0x62b2;
	xPoints << 0x6306;
	xPoints << 0x6359;
	xPoints << 0x6383;
	xPoints << 0x63ac;
	xPoints << 0x63d6;
	xPoints << 0x6400;
	xPoints << 0x7000;
	yPoints << 1.0;
	yPoints << 2.0;
	yPoints << 3.0;
	yPoints << 4.0;
	yPoints << 5.0;
	yPoints << 6.0;
	yPoints << 7.0;
	yPoints << 8.0;
	yPoints << 9.0;
	yPoints << 10.0;
	yPoints << 11.0;
	yPoints << 12.0;
	yPoints << 13.0;
	yPoints << 14.0;
	yPoints << 15.0;
	yPoints << 16.0;
	yPoints << 17.0;
	yPoints << 18.0;
	yPoints << 19.0;
	yPoints << 20.0;
	yPoints << 21.0;
	yPoints << 22.0;
	yPoints << 23.0;
	yPoints << 24.0;
	yPoints << 25.0;
	yPoints << 26.0;
	yPoints << 27.0;
	yPoints << 28.0;
	yPoints << 29.0;
	yPoints << 30.0;
	for (int i = 1; i < yPoints.size(); i++) {
		float mp = (yPoints[i] - yPoints[i - 1]) / (xPoints[i] - xPoints[i - 1]);
		float cp = yPoints[i] - mp * xPoints[i];
		m << mp;
		c << cp;
	}
}

int HitachiModule120::getZoomPosition()
{
	int val = readReg(hiPort, 0xfc91);
	while (val != 0xff)
		val = readReg(hiPort, 0xfc91);
	val = readRegW(hiPort, 0xf720);
	if (val <= 0x2372)
		return 1;
	if (val <= 0x3291)
		return 2;
	if (val <= 0x3b83)
		return 3;
	if (val <= 0x41b0)
		return 4;
	if (val <= 0x4668)
		return 5;
	if (val <= 0x49fb)
		return 6;
	if (val <= 0x4d3c)
		return 7;
	if (val <= 0x5000)
		return 8;
	if (val <= 0x5270)
		return 9;
	if (val <= 0x548d)
		return 10;
	if (val <= 0x56aa)
		return 11;
	if (val <= 0x589e)
		return 12;
	if (val <= 0x5a68)
		return 13;
	if (val <= 0x5bde)
		return 14;
	if (val <= 0x5d2b)
		return 15;
	if (val <= 0x5e4f)
		return 16;
	if (val <= 0x5f48)
		return 17;
	if (val < 0x6018)
		return 18;
	if (val < 0x60bf)
		return 19;
	if (val < 0x6165)
		return 20;
	if (val < 0x61e2)
		return 21;
	if (val < 0x625f)
		return 22;
	if (val < 0x62b2)
		return 23;
	if (val < 0x6306)
		return 24;
	if (val < 0x6359)
		return 25;
	if (val < 0x6383)
		return 26;
	if (val < 0x63ac)
		return 27;
	if (val < 0x63d6)
		return 28;
	if (val < 0x6400)
		return 29;
	return 30;
}

float HitachiModule120::getZoomPositionF()
{
	uint val = readReg(hiPort, 0xfc91);
	while (val != 0xff)
		val = readReg(hiPort, 0xfc91);
	val = readRegW(hiPort, 0xf720);
	int interval = 0;
	for (int i = xPoints.size() - 1; i > 0; i--) {
		if (val > xPoints[i]) {
			interval = i;
			break;
		}
	}
	return val * m[interval] + c[interval];
}
