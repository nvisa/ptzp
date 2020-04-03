#include "networksource.h"
#include "debug.h"

#include <QDateTime>
#include <QWebSocket>
#include <QDataStream>
#include <QWebSocketServer>

NetworkSource::NetworkSource(QObject *parent)
	: QObject(parent)
{
	clientSocket = nullptr;
	threaded = false;
	readyToSend = false;
}

void NetworkSource::setThreadSupport(bool en)
{
	threaded = true;
	connect(this, &NetworkSource::sendData2Main, this, &NetworkSource::sendThreadedData);
}

bool NetworkSource::listen()
{
	QWebSocketServer *server = new QWebSocketServer("ChartNetworkSourceServer",
													QWebSocketServer::NonSecureMode, this);
	connect(server, &QWebSocketServer::newConnection, this, &NetworkSource::onNewConnection);
	return server->listen(QHostAddress::Any, 45678);
	//connect(server, SIGNAL(closed()), this, SLOT(socketDisconnected()));
	return true;
}

bool NetworkSource::connectServer(const QString &url, bool autoConnect)
{
	autoReconnect = autoConnect;
	serverUrl = url;
	clientSocket = new QWebSocket();
	connect(clientSocket, &QWebSocket::connected, this, &NetworkSource::clientConnected);
	connect(clientSocket, &QWebSocket::disconnected, this, &NetworkSource::clientClosed);
	connect(clientSocket, &QWebSocket::binaryFrameReceived, this, &NetworkSource::clientProcessBinaryMessage);
	connect(clientSocket, SIGNAL(QWebSocket::error), this, SLOT(clientSocketError(QAbstractSocket::SocketError)));
	clientSocket->open(QUrl(url));
	return true;
}

int NetworkSource::sendMessage(const QByteArray &ba)
{
	if (readyToSend) {
		if (!threaded)
			return clientSocket->sendBinaryMessage(ba);
		emit sendData2Main(ba);
		return ba.size();
	}
	return -ENOENT;
}

static void writeHeader(QDataStream &out, qint32 type)
{
	out << qint32(0x12345678); //serialization version
	out << qint32(0); //reserved
	out << qint32(0); //reserved
	out << qint32(type); //message type for the following bytes
}

int NetworkSource::sendMessage(const ChartData &data)
{
	if (!readyToSend)
		return -ENOENT;
	QByteArray ba;
	QDataStream out(&ba, QIODevice::WriteOnly);
	out.setFloatingPointPrecision(QDataStream::DoublePrecision);
	out.setByteOrder(QDataStream::LittleEndian);
	writeHeader(out, 1);
	out << data.samples;
	out << data.ts;
	out << data.meta;
	out << data.channel;
	return sendMessage(ba);
}

int NetworkSource::sendMessage(const QList<ChartData> &data)
{
	if (!readyToSend)
		return -ENOENT;
	QByteArray ba;
	QDataStream out(&ba, QIODevice::WriteOnly);
	out.setFloatingPointPrecision(QDataStream::DoublePrecision);
	out.setByteOrder(QDataStream::LittleEndian);
	writeHeader(out, 2);
	out << data.size();
	for (int i = 0; i < data.size(); i++) {
		out << data[i].samples;
		out << data[i].ts;
		out << data[i].meta;
		out << data[i].channel;
	}
	return sendMessage(ba);
}

int NetworkSource::sendMessage(qreal sample)
{
	if (!readyToSend)
		return -ENOENT;
	ChartData d;
	d.samples << sample;
	d.ts << QDateTime::currentMSecsSinceEpoch();
	return sendMessage(d);
}

void NetworkSource::onNewConnection()
{
	QWebSocketServer *server = (QWebSocketServer *)sender();
	QWebSocket *pSocket = server->nextPendingConnection();
	connect(pSocket, &QWebSocket::textMessageReceived, this, &NetworkSource::processTextMessage);
	connect(pSocket, &QWebSocket::binaryMessageReceived, this, &NetworkSource::processBinaryMessage);
	connect(pSocket, &QWebSocket::disconnected, this, &NetworkSource::socketDisconnected);
	clients << pSocket;
	qDebug() << "client connected";
}

void NetworkSource::socketDisconnected()
{
	QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
	if (pClient) {
		clients.removeAll(pClient);
		pClient->deleteLater();
	}
}

void NetworkSource::processBinaryMessage(QByteArray message)
{
	/* we're server, and received a message */
	//qDebug() << __func__ << message.size();
	QDataStream s(&message, QIODevice::ReadOnly);
	s.setFloatingPointPrecision(QDataStream::DoublePrecision);
	s.setByteOrder(QDataStream::LittleEndian);
	qint32 sver; s >> sver;
	qint32 r0; s >> r0;
	qint32 r1; s >> r1;
	qint32 type; s >> type;
	if (type == 1) {
		ChartData d;
		s >> d.samples;
		s >> d.ts;
		s >> d.meta;
		s >> d.channel;
		emit newChartData(d);
	} else if (type == 2) {
		qint32 len; s >> len;
		for (int i = 0; i < len; i++) {
			ChartData d;
			s >> d.samples;
			s >> d.ts;
			s >> d.meta;
			s >> d.channel;
			emit newChartData(d);
		}
	}
	qDebug() << __func__ << message.size();
}

void NetworkSource::clientProcessBinaryMessage(QByteArray message)
{
	/* we're client, and received a message */
	qDebug() << __func__ << message.size();
}

void NetworkSource::processTextMessage(QString message)
{
	/* we're server, and received a message */
	qDebug() << __func__ << message;
}

void NetworkSource::clientConnected()
{
	/* we're client, and we connected to a remote server */
	qDebug() << "connected to server";
	readyToSend = true;
}

void NetworkSource::clientClosed()
{
	/* we're client, and we lost our connection to server */
	//qDebug() << "disconnected from server";
	readyToSend = false;
	if (autoReconnect)
		reconnectClient();
}

void NetworkSource::clientSocketError(QAbstractSocket::SocketError err)
{
	//fDebug("Recv'ed socket err '%d', %s", err, autoReconnect ? "reconnecting..." : "giving-up...");
	readyToSend = false;
	if (autoReconnect)
		reconnectClient();
}

void NetworkSource::sendThreadedData(const QByteArray &ba)
{
	if (clientSocket->isValid())
		clientSocket->sendBinaryMessage(ba);
}

void NetworkSource::reconnectClient()
{
	clientSocket->open(QUrl(serverUrl));
}
