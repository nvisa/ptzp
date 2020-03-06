#include "hitachimodule110.h"

HitachiModule110::HitachiModule110(QextSerialPort *port, QObject *parent) :
	HitachiModule(port, parent)
{
	/* zoom points */
	xPoints << 0x17b3;
	xPoints << 0x2f0a;
	xPoints << 0x3b56;
	xPoints << 0x435e;
	xPoints << 0x4934;
	xPoints << 0x4dba;
	xPoints << 0x516b;
	xPoints << 0x5484;
	xPoints << 0x5737;
	xPoints << 0x5999;
	xPoints << 0x5bb3;
	xPoints << 0x5d9b;
	xPoints << 0x5f46;
	xPoints << 0x60c8;
	xPoints << 0x6218;
	xPoints << 0x632a;
	xPoints << 0x63f5;
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
	for (int i = 1; i < yPoints.size(); i++) {
		float mp = (yPoints[i] - yPoints[i - 1]) / (xPoints[i] - xPoints[i - 1]);
		float cp = yPoints[i] - mp * xPoints[i];
		m << mp;
		c << cp;
	}
}
