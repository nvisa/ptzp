#include "flirmodulehead.h"

#include "debug.h"
#include <ecl/net/cgi/cgimanager.h>
#include "ecl/net/cgi/cgidevicedata.h"

enum Commands{
	CGI_SET_FOCUS_INC,
	CGI_SET_FOCUS_DEC,
	CGI_SET_FOCUS_POS,
	CGI_SET_FOCUS_STOP,
	CGI_SET_FOCUS_MODE,

	CGI_GET_FOCUS_MODE,
	CGI_GET_FOCUS_POS,

	CGI_COUNT
};

FlirModuleHead::FlirModuleHead()
{
	settings = {
		{"focus_in", {CGI_SET_FOCUS_INC, CGI_GET_FOCUS_POS}},
		{"focus_out", {CGI_SET_FOCUS_DEC, CGI_GET_FOCUS_POS}},
		{"focus_stop", {CGI_SET_FOCUS_STOP, CGI_GET_FOCUS_POS}},
		{"focus_pos", {CGI_SET_FOCUS_POS, CGI_GET_FOCUS_POS}},
		{"focus_mode", {CGI_SET_FOCUS_MODE, CGI_GET_FOCUS_MODE}},
	};
}

int FlirModuleHead::connect(QString &targetUri)
{
	QStringList deviceData = targetUri.split("?");

	cgiConnData.ip = deviceData.at(0);
	cgiConnData.userName = deviceData.at(1);
	cgiConnData.password =  deviceData.at(2);
	cgiConnData.port = deviceData.at(3).toInt();

	cgiMan = new CgiManager(cgiConnData, this);
	return 0;
}

int FlirModuleHead::getCapabilities()
{
	return CAP_ZOOM;
}

int FlirModuleHead::startZoomIn(int speed)
{
	return cgiMan->startZoomIn(speed);
}

int FlirModuleHead::startZoomOut(int speed)
{
	return cgiMan->startZoomOut(speed);
}

int FlirModuleHead::stopZoom()
{
	return cgiMan->stopZoom();
}

int FlirModuleHead::getZoom()
{
	return cgiMan->getZoom();
}

int FlirModuleHead::focusIn(int speed)
{
	cgiMan->increaseFocus(speed);
	return 0;
}

int FlirModuleHead::focusOut(int speed)
{
	cgiMan->decreaseFocus(speed);
	return 0;
}

int FlirModuleHead::focusStop()
{
	cgiMan->stopFocus();
	return 0;
}

void FlirModuleHead::setProperty(uint r, uint x)
{
	if(r == CGI_SET_FOCUS_INC)
		cgiMan->increaseFocus(x);
	else if (r == CGI_SET_FOCUS_DEC)
		cgiMan->decreaseFocus(x);
	else if (r == CGI_SET_FOCUS_STOP)
		cgiMan->stopFocus();
	else if (r == CGI_SET_FOCUS_POS)
		cgiMan->setFocus(x);
	else if (r == CGI_SET_FOCUS_MODE)
		cgiMan->setCamSettings("focusMode",(QString)x);
}

uint FlirModuleHead::getProperty(uint r)
{
	if (r == CGI_GET_FOCUS_POS)
		return cgiMan->getFocus();
	else if (r == CGI_GET_FOCUS_MODE)
		return cgiMan->getCamSettings().value("focusMode").toInt();
	else
		return -1;
}
