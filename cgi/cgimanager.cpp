#include "cgimanager.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>

#include "cgirequestmanager.h"
#include "debug.h"

using UniqueReqManager = std::unique_ptr<CgiRequestManager>;

enum class Ptz
{
	//Pan
	PAN_LEFT			= 2,
	PAN_RIGHT			= 3,
	PAN_SET				= 100,
	PAN_GET				= 101,
	//Tilt
	TILT_UP				= 4,
	TILT_DOWN			= 5,
	TILT_SET			= 102,
	TILT_GET			= 103,
	//Diagonal
	PT_UP_LEFT			= 6,
	PT_LOW_LEFT			= 7,
	PT_UP_RIGHT			= 8,
	PT_LOW_RIGHT		= 9,
	//Stop for all PT actions
	PT_STOP				= 39,
	//Zoom
	ZOOM_IN				= 13,
	ZOOM_OUT			= 14,
	ZOOM_STOP			= 40,
	ZOOM_SET			= 104,
	ZOOM_GET			= 105,
	//Focus
	FOCUS_PLUS			= 15,
	FOCUS_MINUS			= 16,
	FOCUS_STOP			= 41,
	FOCUS_SET			= 106,
	FOCUS_GET			= 107,
	//Aperture
	APERTURE_INCREASE	= 17,
	APERTURE_DECREASE	= 18,
	//Stop
	STOP = 0
};

enum class CgiGroup
{
	Cam,
	Ability
};

struct PTZQueryInfo
{
	QString groupName;
	QString queryName;
};

static int actionNumber(Ptz action)
{
	return static_cast<int>(action);
}

static PTZQueryInfo ptzQuery(Ptz action)
{
	switch (action) {
	case Ptz::PAN_SET:
		return PTZQueryInfo{"PTZPOS", "panpos"};
	case Ptz::PAN_GET:
		return PTZQueryInfo{"PTZPOS", "panpos"};
	case Ptz::TILT_SET:
		return PTZQueryInfo{"PTZPOS", "tiltpos"};
	case Ptz::TILT_GET:
		return PTZQueryInfo{"PTZPOS", "tiltpos"};
	case Ptz::ZOOM_SET:
		return PTZQueryInfo{"CAMPOS", "zoompos"};
	case Ptz::ZOOM_GET:
		return PTZQueryInfo{"CAMPOS", "zoompos"};
	case Ptz::FOCUS_SET:
		return PTZQueryInfo{"CAMPOS", "focuspos"};
	case Ptz::FOCUS_GET:
		return PTZQueryInfo{"CAMPOS", "focuspos"};
	default:
		return PTZQueryInfo{};
	}
}

static QString getGroupName(CgiGroup group)
{
	switch (group) {
	case CgiGroup::Cam:
		return "CAM";
	case CgiGroup::Ability:
		return "ABILITY";
	default:
		return "";
	}
}

static QString getCgiModule(CgiGroup group)
{
	switch (group) {
	case CgiGroup::Cam:
		return "param.cgi";
	case CgiGroup::Ability:
		return "devInfo.cgi";
	default:
		return "";
	}
}

namespace http
{
static int errorNo(QString const& data)
{
	return data.isEmpty() ? -1 : -1 * data.split("ERR.no=").last().split("\n").first().remove("\r").toInt();
}

static QString replyData(QNetworkReply* reply)
{
	QString data = "";
	if (reply->canReadLine()) {
		data = reply->readAll().data();
	}
	reply->deleteLater();
	return data;
}

static QString waitForReplyData(QNetworkReply* reply, int timeout)
{
	QEventLoop el;
	QTimer::singleShot(timeout, &el, &QEventLoop::quit);
	QObject::connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), &el, SLOT(quit()));
	QObject::connect(reply, &QNetworkReply::finished, &el, &QEventLoop::quit);
	el.exec();
	if (!reply->isFinished()) {
		reply->abort();
	}
	return replyData(reply);
}

static QString handleGetReply(QNetworkReply* reply, int timeout)
{
	QString data = waitForReplyData(reply, timeout);
	return data;
}

static int handlePostReply(QNetworkReply* reply, int timeout)
{
	QString data = waitForReplyData(reply, timeout);
	if (data.isEmpty())
		return -1;
	else
		return errorNo(data);
}

static QString parseReply(QString const& data, QString const& keyword)
{
	return data.split(keyword).last().split("\n").first().remove("\r");
}
}//namespace http

static int PTZSet(UniqueReqManager& manager, int timeout, CgiDeviceData const& devData, Ptz action ,int value)
{
	auto info = ptzQuery(action);
	QString cgiPath("/cgi-bin/param.cgi?"
					"action=update&group=%1"
					"&channel=0&%1.%2=%3");
	auto reply = manager->post(devData, cgiPath.arg(info.groupName).arg(info.queryName).arg(value));
	return http::handlePostReply(reply, timeout);
}

static int PTZMove(UniqueReqManager& manager, int timeout, CgiDeviceData const& devData, Ptz action)
{
	QString cgiPath("/cgi-bin/control.cgi?"
					"action=update&group=PTZCTRL"
					"&channel=0&PTZCTRL.action=%1&nRanId=96325");
	auto reply = manager->post(devData, cgiPath.arg(actionNumber(action)));
	return http::handlePostReply(reply, timeout);
}

static int PTZMove(UniqueReqManager& manager, int timeout, CgiDeviceData const& devData, Ptz action, int speed)
{
	QString cgiPath("/cgi-bin/control.cgi?"
					"action=update&group=PTZCTRL"
					"&channel=0&PTZCTRL.action=%1&PTZCTRL.speed=%2&nRanId=341425");
	auto reply = manager->post(devData, cgiPath.arg(actionNumber(action)).arg(speed));
	return http::handlePostReply(reply, timeout);
}

static int PTZRead(UniqueReqManager& manager, int timeout, CgiDeviceData const& devData, Ptz action)
{
	auto info = ptzQuery(action);
	QString cgiPath("/cgi-bin/param.cgi?"
					"action=list&group=%1"
					"&channel=0");
	auto reply = manager->get(devData, cgiPath.arg(info.groupName));
	QString data = http::handleGetReply(reply, timeout);
	QString keyword = QString("%1.%2=").arg(info.groupName).arg(info.queryName);
	auto value = http::parseReply(data, keyword);
	int errNo = http::errorNo(data);
	if (value.isEmpty())
		return -1;
	else if (errNo != 0) {
		return errNo;
	} else {
		return value.toInt();
	}
}

static QHash<QString, QString> readSettings(UniqueReqManager& manager, int timeout, CgiDeviceData const& devData, CgiGroup group, int channel = -1)
{
	QString cgiPath("/cgi-bin/%1?action=list&group=%2");
	if (channel != -1) {
		cgiPath.append(QString("&channel=%1").arg(channel));
	}
	QString groupName = getGroupName(group);
	QString cgiModule = getCgiModule(group);
	auto reply = manager->get(devData, cgiPath.arg(cgiModule).arg(groupName));
	QString data = http::handleGetReply(reply, timeout);
	if (data.isEmpty() || http::errorNo(data) != 0)
		return QHash<QString, QString>{};
	QHash<QString, QString> queries;
	QStringList splittedByEOL = data.split("\n");
	foreach (auto const& splitted, splittedByEOL) {
		QStringList query = splitted.split("root." + groupName + ".").last().remove("\r").split("=");
		if (query.size() == 2 && !query.first().startsWith("root.ERR")) {
			queries.insert(query.first(), query.last());
		}
	}
	return queries;
}

static int setSettings(UniqueReqManager& manager, int timeout, CgiDeviceData const& devData, QHash<QString, QString> const& settings, CgiGroup group, int channel = -1)
{
	QString cgiPath("/cgi-bin/%1?action=list&group=%2");
	if (channel != -1) {
		cgiPath.append(QString("&channel=%1").arg(channel));
	}
	QString groupName = getGroupName(group);
	QString cgiModule = getCgiModule(group);
	QHashIterator<QString, QString> iter(settings);
	while (iter.hasNext()) {
		iter.next();
		cgiPath.append("&" + groupName + "." + iter.key() + "=" + iter.value());
	}
	auto reply = manager->post(devData, cgiPath.arg(cgiModule).arg(groupName));
	return http::handlePostReply(reply, timeout);
}

static int setSettings(UniqueReqManager& manager, int timeout, CgiDeviceData const& devData, QString key, QString value, CgiGroup group, int channel = -1)
{
	QString cgiPath("/cgi-bin/%1?action=list&group=%2");
	if (channel != -1) {
		cgiPath.append(QString("&channel=%1").arg(channel));
	}
	QString groupName = getGroupName(group);
	QString cgiModule = getCgiModule(group);
	cgiPath.append("&" + groupName + "." + key + "=" + value);
	auto reply = manager->post(devData, cgiPath.arg(cgiModule).arg(groupName));
	return http::handlePostReply(reply, timeout);
}

static void uploadFile(UniqueReqManager& manager, CgiDeviceData const& devData, QString const& filePath)
{
	auto reply = manager->upload(devData, filePath);
	QObject::connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

static int readUpgradePercentage(UniqueReqManager& manager, int timeout, CgiDeviceData const& devData)
{
	auto reply = manager->get(devData, "/cgi-bin/upgrade.cgi?action=list&group=UPGRADE");
	auto data = http::handleGetReply(reply, timeout);
	return http::parseReply(data, "UPGRADE.state=").toInt();
}

CgiManager::CgiManager(const CgiDeviceData &devData, QObject *parent)
	: QObject(parent),
	  requestManager(new CgiRequestManager),
	  devData(devData)
{}

int CgiManager::getPan()
{
	return PTZRead(requestManager,timeout, devData, Ptz::PAN_GET);
}

int CgiManager::setPan(int panPos)
{
	return PTZSet(requestManager, timeout, devData, Ptz::PAN_SET, panPos);
}

int CgiManager::startPanLeft(int speed)
{
	return PTZMove(requestManager, timeout, devData, Ptz::PAN_LEFT, speed);
}

int CgiManager::startPanRight(int speed)
{
	return PTZMove(requestManager, timeout, devData, Ptz::PAN_RIGHT, speed);
}

int CgiManager::getTilt()
{
	return PTZRead(requestManager,timeout, devData, Ptz::TILT_GET);
}

int CgiManager::setTilt(int tiltPos)
{
	return PTZSet(requestManager, timeout, devData, Ptz::TILT_SET, tiltPos);
}

int CgiManager::startTiltUp(int speed)
{
	return PTZMove(requestManager, timeout, devData, Ptz::TILT_UP, speed);
}

int CgiManager::startTiltDown(int speed)
{
	return PTZMove(requestManager, timeout, devData, Ptz::TILT_DOWN, speed);
}

int CgiManager::startPTUpLeft(int speed)
{
	return PTZMove(requestManager, timeout, devData, Ptz::PT_UP_LEFT, speed);
}

int CgiManager::startPTUpRight(int speed)
{
	return PTZMove(requestManager, timeout, devData, Ptz::PT_UP_RIGHT, speed);
}

int CgiManager::startPTLowLeft(int speed)
{
	return PTZMove(requestManager, timeout, devData, Ptz::PT_LOW_LEFT, speed);
}

int CgiManager::startPTLowRight(int speed)
{
	return PTZMove(requestManager, timeout, devData, Ptz::PT_LOW_RIGHT, speed);
}

int CgiManager::stopPt()
{
	return PTZMove(requestManager, timeout, devData, Ptz::PT_STOP);
}

int CgiManager::getZoom()
{
	return PTZRead(requestManager,timeout, devData, Ptz::ZOOM_GET);
}

int CgiManager::setZoom(int zoomPos)
{
	return PTZSet(requestManager, timeout, devData, Ptz::ZOOM_SET, zoomPos);
}

int CgiManager::startZoomIn(int speed)
{
	return PTZMove(requestManager, timeout, devData, Ptz::ZOOM_IN, speed);
}

int CgiManager::startZoomOut(int speed)
{
	return PTZMove(requestManager, timeout, devData, Ptz::ZOOM_OUT, speed);
}

int CgiManager::stopZoom()
{
	return PTZMove(requestManager, timeout, devData, Ptz::ZOOM_STOP);
}

int CgiManager::getFocus()
{
	return PTZRead(requestManager,timeout, devData, Ptz::FOCUS_GET);
}

int CgiManager::setFocus(int focus)
{
	int errorNo = setSettings(requestManager, timeout, devData, "autoFocusMode", "0", CgiGroup::Cam, 0);
	if (errorNo == 0)
		return PTZSet(requestManager, timeout, devData, Ptz::FOCUS_SET, focus);
	else
		return errorNo;
}

int CgiManager::increaseFocus(int speed)
{
	int errorNo = setSettings(requestManager, timeout, devData, "autoFocusMode", "0", CgiGroup::Cam, 0);
	if (errorNo == 0)
		return PTZMove(requestManager, timeout, devData, Ptz::FOCUS_PLUS, speed);
	else
		return errorNo;
}

int CgiManager::decreaseFocus(int speed)
{
	int errorNo = setSettings(requestManager, timeout, devData, "autoFocusMode", "0", CgiGroup::Cam, 0);
	if (errorNo == 0)
		return PTZMove(requestManager, timeout, devData, Ptz::FOCUS_MINUS, speed);
	else
		return errorNo;
}

int CgiManager::stopFocus()
{
	return PTZMove(requestManager, timeout, devData, Ptz::FOCUS_STOP);
}

int CgiManager::increaseAperture(int speed)
{
	return PTZMove(requestManager, timeout, devData, Ptz::APERTURE_INCREASE, speed);
}

int CgiManager::decreaseAperture(int speed)
{
	return PTZMove(requestManager, timeout, devData, Ptz::APERTURE_DECREASE, speed);
}

int CgiManager::stopAperture()
{
	return PTZMove(requestManager, timeout, devData, Ptz::STOP);
}

QHash<QString, QString> CgiManager::getCamSettings()
{
	return readSettings(requestManager, timeout, devData, CgiGroup::Cam, 0);
}

int CgiManager::setCamSettings(const QHash<QString, QString> &settings)
{
	return setSettings(requestManager, timeout, devData, settings, CgiGroup::Cam, 0);
}

QHash<QString, QString> CgiManager::getDeviceAbilities()
{
	return readSettings(requestManager, timeout, devData, CgiGroup::Ability);
}

void CgiManager::startUpgrade(const QString &filePath)
{
	uploadFile(requestManager, devData, filePath);
}

int CgiManager::getUpgradePercentage()
{
	return readUpgradePercentage(requestManager, timeout, devData);
}

void CgiManager::setTimeout(int msec)
{
	timeout = msec;
}

CgiManager::~CgiManager()
{

}
