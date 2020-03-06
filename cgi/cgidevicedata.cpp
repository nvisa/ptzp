#include "cgidevicedata.h"

CgiDeviceData::CgiDeviceData(QString ip, QString userName, QString password, int port)
	: ip(ip), userName(userName), password(password), port(port)
{}

QUrl CgiDeviceData::getUrl() const
{
	QUrl url;
	url.setUrl("http://" + ip);
	url.setPort(port);
	return url;
}
