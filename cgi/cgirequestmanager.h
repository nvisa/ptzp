#ifndef CGIREQUESTMANAGER_H
#define CGIREQUESTMANAGER_H

#include <QObject>
#include <memory>

#include "cgidevicedata.h"

class QNetworkAccessManager;
class QNetworkReply;

class CgiRequestManager final : public QObject
{
	Q_OBJECT
public:
	explicit CgiRequestManager(QObject *parent = 0);
	QNetworkReply* get(CgiDeviceData const& devData, QString const& cgiRequest);
	QNetworkReply* post(CgiDeviceData const& devData, QString const& cgiRequest);
	QNetworkReply* upload(CgiDeviceData const& devData, QString const& filePath);
	~CgiRequestManager();

private:
	std::unique_ptr<QNetworkAccessManager> networkManager;
	int timeout = 100;
};

#endif // CGIREQUESTMANAGER_H
