#include "simplehttpserver.h"

#include <debug.h>

#include <QUrl>
#include <QFile>
#include <QTcpSocket>
#include <QTcpServer>
#include <QStringList>

SimpleHttpServer::SimpleHttpServer(int port, QObject *parent) :
	QObject(parent)
{
	server = new QTcpServer(this);
	server->listen(QHostAddress::Any, port);
	connect(server, SIGNAL(newConnection()), SLOT(newConnection()));
	mDebug("listening on port %d", port);
	state = IDLE;

	mimeTypesByExtension.insert("html", "text/html");
	mimeTypesByExtension.insert("php", "text/html");
	mimeTypesByExtension.insert("js", "application/javascript");
	mimeTypesByExtension.insert("jpeg", "image/jpeg");
	mimeTypesByExtension.insert("jpg", "image/jpeg");
	mimeTypesByExtension.insert("png", "image/png");
	mimeTypesByExtension.insert("css", "text/css");
	mimeTypesByExtension.insert("mpd", "text/xml");
	mimeTypesByExtension.insert("xml", "text/xml");

	useAuthentication = false;
	useCustomAuth = false;
	temporaryDir = "/tmp";
}

const QByteArray SimpleHttpServer::getFileAuth(const QString filename, QString &mime, QUrl &url)
{
	if (isAuthenticated())
		return getFile(filename, mime, url);
	mime = "text/plain";
	return QByteArray();
}

const QByteArray SimpleHttpServer::getFile(const QString filename, QString &mime, QUrl &url)
{
	Q_UNUSED(filename);
	Q_UNUSED(url);
	QString fname = filename;
	if (fname.isEmpty())
		fname = "index.html";
	mime = mimeTypesByExtension[fname.split(".").last()];
	if (mime.isEmpty())
		mime = "text/html";
	QFile f("/usr/local/bin/www/" + fname);
	if (!f.open(QIODevice::ReadOnly))
		return QByteArray();
	return f.readAll();
}

int SimpleHttpServer::handlePostData(const QByteArray &)
{
	return 404;
}

int SimpleHttpServer::handlePostDataFile(const QString &)
{
	return 404;
}

void SimpleHttpServer::sendGetResponse(QTcpSocket *sock)
{
	QString mime;
	const QByteArray ba = getFileAuth(getHeaders["filename"], mime, getUrl);
	if (ba.size()) {
		mInfo("sending file '%s'", qPrintable(getHeaders["filename"]));
		QStringList resp;
		resp << "HTTP/1.1 200 OK";
		resp << "Accept-Ranges: bytes";
		resp << QString("Content-Length: %1").arg(ba.size());
		resp << "Keep-Alive: timeout=3,max=100";
		resp << "Connection: Keep-Alive";
		resp << QString("Content-Type: %1").arg(mime);
		/* following 3 headers are for preventing browsers(or proxies) caching the result */
		resp << "Cache-Control: no-cache, no-store, must-revalidate";
		resp << "Pragma: no-cache";
		resp << "Expires: 0";
		resp << addCustomGetHeaders(getHeaders["filename"]);
		resp << "\r\n";
		sock->write(resp.join("\r\n").toUtf8());
		sock->write(ba);
	} else {
		QStringList resp;
		resp << "HTTP/1.1 404 Not Found";
		resp << "Accept-Ranges: bytes";
		QString nf = "<html>"
					 "<head><title>404 Not Found</title></head>"
					 "<body>"
					 "<center><h1>404 Not Found</h1></center>"
					 "</body>"
					 "</html>";
		resp << QString("Content-Length: %1").arg(nf.size());
		resp << "Keep-Alive: timeout=3,max=100";
		resp << "Connection: Keep-Alive";
		resp << QString("Content-Type: %1").arg(mime);
		/* following 3 headers are for preventing browsers(or proxies) caching the result */
		resp << "Cache-Control: no-cache, no-store, must-revalidate";
		resp << "Pragma: no-cache";
		resp << "Expires: 0";
		resp << addCustomGetHeaders(getHeaders["filename"]);
		resp << "\r\n";
		sock->write(resp.join("\r\n").toUtf8().append(nf));
	}
}

int SimpleHttpServer::handlePostDataAuth(const QByteArray &ba)
{
	if (isAuthenticated())
		return handlePostData(ba);
	return 404;
}

int SimpleHttpServer::handlePostDataFileAuth(const QString &ba)
{
	if (isAuthenticated())
		return handlePostDataFile(ba);
	return 404;
}

QStringList SimpleHttpServer::addCustomGetHeaders(const QString &filename)
{
	Q_UNUSED(filename);
	return QStringList();
}

bool SimpleHttpServer::parsePostData(QTcpSocket *sock)
{
	int postSize = postHeaders["Content-Length:"].toInt();
	QFile *postDataFile = socketVar[sock].postDataFile;
	QByteArray *postData = &socketVar[sock].postData;
	int err = -1;
	if (postSize > 3 * 1024 * 1024) {
		if (!postDataFile->isOpen()) {
			if (!postDataFile->open(QIODevice::WriteOnly)) {
				sock->readAll();
				postData->clear();
				err = 400;
			}
		}
		if (postDataFile->isOpen()) {
			postDataFile->write(sock->readAll());
			if (postDataFile->size() >= postSize) {
				postDataFile->close();
				err = handlePostDataFileAuth(postDataFile->fileName());
			}
		}
	} else {
		postData->append(sock->readAll());
		if (postData->size() >= postSize) {
			err = handlePostDataAuth(*postData);
			postData->clear();
		}
	}
	if (err >= 0) {
		state = IDLE;
		QStringList resp;
		if (err == 0)
			resp << "HTTP/1.1 200 OK";
		else
			resp << QString("HTTP/1.1 %1").arg(err);
		resp << QString("Content-Length: 0");
		resp << "\r\n";
		sock->write(resp.join("\r\n").toUtf8());
	} else
		return true;
	return false;
}

void SimpleHttpServer::newConnection()
{
	while (server->hasPendingConnections()) {
		QTcpSocket *sock = server->nextPendingConnection();
		socketVar.insert(sock, SocketVariables());
		socketVar[sock].postDataFile = new QFile (QString("%1/simpleHttp%2.tmp").arg(temporaryDir).arg(rand()));
		mInfo("new connection from '%s'", qPrintable(sock->peerAddress().toString()));
		connect(sock, SIGNAL(readyRead()), SLOT(readMessage()));
		connect(sock, SIGNAL(disconnected()), socketVar[sock].postDataFile, SLOT(deleteLater()));
		connect(sock, SIGNAL(disconnected()), SLOT(deleteSocketVariables()));
		connect(sock, SIGNAL(disconnected()), sock, SLOT(deleteLater()));
	}
}

void SimpleHttpServer::readMessage()
{
	QTcpSocket *sock = (QTcpSocket *)sender();
	getSocketVar(sock);
	while (shouldRead(sock)) {
		if (state == IDLE) {
			QStringList tokens = QString(sock->readLine()).split(QRegExp("[ \r\n][ \r\n]*"));
			if (tokens[0] == "GET" && tokens.size() > 1) {
				state = GET;
				QUrl url = QUrl(tokens[1]);
				QString filename = url.path();
				if (filename.isEmpty() || filename == "/")
					filename = "/index.html";
				getHeaders.clear();
				getHeaders.insert("filename", filename);
				getUrl = url;
			} else if (tokens[0] == "POST" && tokens.size() > 1) {
				postHeaders.clear();
				state = POST_HEADER;
				QString filename = QUrl(tokens[1]).path();
				postHeaders.insert("filename", filename);
				postHeaders.insert("__url__", tokens[1]);
				getUrl = QUrl(tokens[1]);
			} else {
				qDebug() << "wrong data:" << tokens;
				QStringList resp;
				resp << "HTTP/1.1 405 Wrong Ha Ha wrong wrong wrong !!!";
				resp << "Connection: Keep-Alive";
				resp << "\r\n";
				sock->write(resp.join("\r\n").toUtf8());
			}
		} else if (state == GET) {
			QStringList tokens = QString(sock->readLine()).split(QRegExp("[ \r\n][ \r\n]*"));
			tokens.removeAll("");
			if (!tokens.size()) {
				sendGetResponse(sock);
				state = IDLE;
			} else
				getHeaders.insert(tokens[0], tokens[1]);
		} else if (state == POST_HEADER) {
			QStringList tokens = QString(sock->readLine()).split(QRegExp("[ \r\n][ \r\n]*"));
			tokens.removeAll("");
			if (!tokens.size()) {
				if (parsePostData(sock))
					state = POST;
			} else if (tokens.size() == 2)
				postHeaders.insert(tokens[0], tokens[1]);
		} else if (state == POST) {
			parsePostData(sock);
		}
		//sock->close();
		//sock->abort();
		//sock->deleteLater();
	}
	setSocketVar(sock);
}

void SimpleHttpServer::deleteSocketVariables()
{
	QTcpSocket *sock = (QTcpSocket *)sender();
	if (socketVar.contains(sock)) {
		socketVar.remove(sock);
	}
}

bool SimpleHttpServer::isAuthenticated()
{
	if (!useAuthentication || useCustomAuth)
		return true;
	return false;
}

bool SimpleHttpServer::isAuthenticated(const QString &username)
{
	if (authenticatedUserToken != getHeaders["Auth-Token:"])
		return false;
	if (authenticatedUserName == username)
		return true;
	/* admin have the rights to be every user */
	if (authenticatedUserName == "admin")
		return true;
	if (username == "*" && !authenticatedUserName.isEmpty())
		return true;
	return false;
}

bool SimpleHttpServer::shouldRead(QTcpSocket *sock)
{
	if (state == POST)
		return sock->bytesAvailable();
	return sock->canReadLine();
}

void SimpleHttpServer::getSocketVar(QTcpSocket *sock)
{
	postHeaders = socketVar[sock].postHeaders;
	getHeaders = socketVar[sock].getHeaders;
	getUrl = socketVar[sock].getUrl;
	state = socketVar[sock].state;
}

void SimpleHttpServer::setSocketVar(QTcpSocket *sock)
{
	socketVar[sock].postHeaders = postHeaders;
	getHeaders = socketVar[sock].getHeaders = getHeaders;
	socketVar[sock].getUrl = getUrl;
	socketVar[sock].state = state;
}
