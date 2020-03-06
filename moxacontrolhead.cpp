#include "moxacontrolhead.h"

#include <assert.h>
#include <debug.h>

#include <ptzptransport.h>

enum CommandList {
	C_ZOOM_SHOW,
	C_COUNT
};

static QStringList createMoxaCommandList()
{
	QStringList list;
	list << "/moxa-cgi/imageoverlay.cgi?ctoken=osdcfg01&action=setconfig&type=0&display=2&position=0&posx=0&posy=0&bgspan=0&textsize=24&datetimeformat=0&showdate=0&showtime=0&text=ZOOM:%1X";
	return list;
}

MoxaControlHead::MoxaControlHead()
	: PtzpHead()
{
	moxaCommandList = createMoxaCommandList();
	assert(moxaCommandList.size() == C_COUNT);
}

int MoxaControlHead::setZoomShow(int value)
{
	return sendCommand(moxaCommandList.at(C_ZOOM_SHOW).arg(value));
}

void MoxaControlHead::fillCapabilities(ptzp::PtzHead *head)
{
	Q_UNUSED(head);
}

int MoxaControlHead::getHeadStatus()
{
	return 0;
}

int MoxaControlHead::sendCommand(const QString &key)
{
	mInfo("Sending command %s", qPrintable(key));
	return transport->send(key.toUtf8());
}

int MoxaControlHead::dataReady(const unsigned char *bytes, int len)
{
	Q_UNUSED(bytes);
	return len;
}

QByteArray MoxaControlHead::transportReady()
{
	return QByteArray();
}
