#include "cgirequestmanager.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QFileInfo>
#include <QTimer>

static void prepareHeaders(QNetworkRequest& request, CgiDeviceData const& devData, QUrl const& url)
{
	request.setUrl(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	request.setRawHeader("Authorization", "Basic "
						 + QString("%1:%2").arg(devData.userName).arg(devData.password).toLatin1().toBase64());
}

static void prepareHeaders(QNetworkRequest& request,  CgiDeviceData const& devData, QUrl const& url, QString const& contentTypeHeader)
{
	request.setUrl(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, contentTypeHeader);
	request.setRawHeader("Authorization", "Basic "
						 + QString("%1:%2").arg(devData.userName).arg(devData.password).toLatin1().toBase64());
}

static QNetworkRequest createGetRequest(CgiDeviceData const& devData, QString const& cgiRequest)
{
	QNetworkRequest request;
	QUrl url = devData.getUrl();
	QStringList pathAndQueries = cgiRequest.split("?");
	url.setPath(pathAndQueries.first(), QUrl::ParsingMode::StrictMode);
	url.setQuery(pathAndQueries.last());
	prepareHeaders(request, devData, url);
	return request;
}

static QPair<QNetworkRequest, QByteArray> createPostRequest(CgiDeviceData const& devData, QString const& cgiRequest)
{
	QNetworkRequest request;
	QUrl url = devData.getUrl();
	QStringList pathAndQueries = cgiRequest.split("?");
	url.setPath(pathAndQueries.first(), QUrl::ParsingMode::StrictMode);
	prepareHeaders(request, devData, url);
	return QPair<QNetworkRequest, QByteArray>(request, pathAndQueries.last().toLatin1());
}

static QNetworkRequest createUploadRequest(CgiDeviceData const& devData, QString const& boundary)
{
	QNetworkRequest request;
	QUrl url = devData.getUrl();
	url.setPath("/webs/uploadfile");
	url.setQuery("sessionId=164039502");
	prepareHeaders(request, devData, url, QString("multipart/form-data; boundary=").append(boundary));
	return request;
}

static QByteArray createUploadData(QString const& filePath, QString const& boundary)
{
	QByteArray data;
	QFile file(filePath);
	QFileInfo fileInfo(file);
	if (!file.open(QIODevice::ReadOnly))
	{
		return data;
	}
	data.append("--" + boundary + "--" + "\r\n");
	data.append("Content-Disposition:form-data; name\"file\"; filename=\"" + fileInfo.fileName() + "\"\r\n");
	data.append("Content-Type: application/octet-stream\r\n\r\n");
	data.append(file.readAll() + "\r\n");
	data.append("--" + boundary + "--" + "\r\n");
	file.close();
	return data;
}

CgiRequestManager::CgiRequestManager(QObject *parent)
	: QObject(parent),
	  networkManager(new QNetworkAccessManager) {}

QNetworkReply* CgiRequestManager::get(const CgiDeviceData &devData, const QString &cgiRequest)
{
	return networkManager->get(createGetRequest(devData, cgiRequest));
}

QNetworkReply* CgiRequestManager::post(const CgiDeviceData &devData, const QString &cgiRequest)
{
	auto request = createPostRequest(devData, cgiRequest);
	return networkManager->post(request.first, request.second);
}

QNetworkReply* CgiRequestManager::upload(const CgiDeviceData &devData, const QString &filePath)
{
	QString boundary("-----------------------7630590869859032859755285441");
	auto request = createUploadRequest(devData, boundary);
	auto data = createUploadData(filePath, boundary);
	return networkManager->post(request, data);
}

CgiRequestManager::~CgiRequestManager()
{

}
