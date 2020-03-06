#ifndef CGIDEVICEDATA_H
#define CGIDEVICEDATA_H

#include <QUrl>

struct CgiDeviceData
{
	CgiDeviceData() = default;
	QString ip;
	QString userName;
	QString password;
	int port;
	CgiDeviceData(QString ip, QString userName = "admin", QString password = "12345", int port = 80);
	QUrl getUrl() const;
};

#endif // CGIDEVICEDATA_H
