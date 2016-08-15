#include "remotecontrol.h"
#include "debug.h"
#include "settings/applicationsettings.h"

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
	return serv->listen(address, port);
}

void RemoteControl::addMapping(const QString &group, ApplicationSettings *s)
{
	mappings.insert(group, s);
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

const QVariant RemoteControl::getSetting(const QString &setting)
{
	return getMapping(setting)->get(setting);
}

int RemoteControl::setSetting(const QString &setting, const QVariant &value)
{
	QVariant var = getSetting(setting);
	QVariant next = value;
	if (!next.convert(var.type()))
		return -EIO;
	return getMapping(setting)->set(setting, next);
}

ApplicationSettings *RemoteControl::getMapping(const QString &setting)
{
	const QStringList &vals = setting.split(".");
	if (mappings.contains(vals.first()))
		return mappings[vals.first()];
	return ApplicationSettings::instance();
}
