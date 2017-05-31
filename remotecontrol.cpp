#include "remotecontrol.h"
#include "debug.h"
#include "settings/applicationsettings.h"

#include <QUdpSocket>
#include <QTcpSocket>
#include <QTcpServer>

#include <errno.h>

RemoteControl::RemoteControl(QObject *parent) :
	QObject(parent)
{
}

bool RemoteControl::listen(const QHostAddress &address, quint16 port)
{
	serv = new QTcpServer(this);
	connect(serv, SIGNAL(newConnection()), SLOT(newConnection()));

	sock = new QUdpSocket(this);
	sock->bind(QHostAddress::Any, port);
	connect(sock, SIGNAL(readyRead()), SLOT(readPendingDatagrams()));

	return serv->listen(address, port);
}

void RemoteControl::readPendingDatagrams()
{
	while (sock->hasPendingDatagrams()) {
		QByteArray datagram;
		datagram.resize(sock->pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;

		sock->readDatagram(datagram.data(), datagram.size(),
								&sender, &senderPort);
		QByteArray ba = processUdpMessage(datagram).toUtf8();
		if (ba.size())
			sock->writeDatagram(ba, sender, senderPort);
	}
}

void RemoteControl::newConnection()
{
	QTcpSocket *sock = serv->nextPendingConnection();
	connect(sock, SIGNAL(disconnected()), sock, SLOT(deleteLater()));
	connect(sock, SIGNAL(readyRead()), SLOT(dataReady()));
	mDebug("client %s connected", qPrintable(sock->peerAddress().toString()));
}

void RemoteControl::dataReady()
{
	QTcpSocket *sock = (QTcpSocket *)sender();

	while (sock->canReadLine()) {
		QString mes = QString::fromUtf8(sock->readLine()).trimmed();
		if (mes.isEmpty())
			continue;
		QStringList flds = mes.split(" ");
		mInfo("%s:%s < %s", qPrintable(sock->peerAddress().toString()), qPrintable(QString::number(sock->peerPort())), qPrintable(mes));
		if (flds.size() < 2) {
			sock->write(QString("error:no enough arguments\n").toUtf8());
			continue;
		}
		if (flds[0] == "get") {
			const QString set = getSetting(flds[1].trimmed()).toString();
			sock->write(QString("get:%1:%2\n").arg(flds[1].trimmed()).arg(set).toUtf8());
		} else if (flds[0] == "set") {
			if (flds.size() < 3) {
				sock->write(QString("error:no enough arguments\n").toUtf8());
				continue;
			}
			int err = setSetting(flds[1].trimmed(), flds[2].trimmed());
			sock->write(QString("set:%1:%2\n").arg(flds[1].trimmed()).arg(err).toUtf8());
		} else
			sock->write(QString("error:un-supported command\n").toUtf8());
	}
}

const QString RemoteControl::processUdpMessage(const QString &mes)
{
	QUdpSocket *udpSock = (QUdpSocket *)sender();
	mInfo("%s:%s < %s", qPrintable(udpSock->peerAddress().toString()), qPrintable(QString::number(udpSock->peerPort())), qPrintable(mes));
	if (mes.trimmed().isEmpty())
		return "";
	QString resp;
	QStringList fields = mes.trimmed().split(" ", QString::SkipEmptyParts);
	if (fields.size() > 1 && fields[0] == "get") {
		QString set = fields[1].trimmed();
		resp = QString("get:%1:%2").arg(set).arg(getSetting(set).toString());
	} else if (fields.size() > 2 && fields[0] == "set") {
		QString set = fields[1].trimmed();
		int err = setSetting(set, fields[2].trimmed());
		resp = QString("set:%1:%2").arg(set).arg(err);
	}
	return resp;
}

const QVariant RemoteControl::getSetting(const QString &setting)
{
	return ApplicationSettings::instance()->getm(setting);
}

int RemoteControl::setSetting(const QString &setting, const QVariant &value)
{
	QVariant var = getSetting(setting);
	QVariant next = value;
	if (next.canConvert(var.type()))
		next.convert(var.type());
	return ApplicationSettings::instance()->setm(setting, next);
}
