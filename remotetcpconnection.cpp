#include "remotetcpconnection.h"

#include "debug.h"

#include <QTimer>

static bool wait(QEventLoop *el, int timeout)
{
	QTimer t;
	QObject::connect(&t, SIGNAL(timeout()), el, SLOT(quit()));
	if (timeout)
		t.start(timeout);
	el->exec();
	if (!timeout || t.isActive())
		return true;
	return false;
}

RemoteTcpConnection::RemoteTcpConnection(QObject *parent) :
	QTcpSocket(parent)
{
}

QString RemoteTcpConnection::get(const QString &key)
{
	resp.clear();
	if (state() != QTcpSocket::ConnectedState)
		return "__no_connection__";
	QString str = QString("get %1\n").arg(key);
	write(str.toUtf8());
	waitForBytesWritten();
	if (!wait(&el, 100))
		return "__timeout__";
	foreach (QString line, resp) {
		QString pre = QString("get:%1:").arg(key);
		if (!line.startsWith(pre))
			continue;
		resp.removeOne(line);
		return line.remove(pre);
	}
	return "";
}

int RemoteTcpConnection::set(const QString &key, const QString &value)
{
	QString str = QString("set %1 %2\n").arg(key).arg(value);
	write(str.toUtf8());
	return 0;
}

void RemoteTcpConnection::setTarget(const QString &targetIp, int port)
{
	target = QHostAddress(targetIp);
	connect(this, SIGNAL(connected()), SLOT(connectedTo()));
	connect(this, SIGNAL(disconnected()), SLOT(disconnectedFrom()));
	connect(this, SIGNAL(readyRead()), this, SLOT(dataReady()));
	connect(&timer, SIGNAL(timeout()), &el, SLOT(quit()));
	timer.setSingleShot(true);
	dstPort = port;
	connectToHost(target, dstPort);
}

void RemoteTcpConnection::dataReady()
{
	QTcpSocket *sock = this;

	while (sock->canReadLine()) {
		QString mes = QString::fromUtf8(sock->readLine()).trimmed();
		if (mes.isEmpty())
			continue;
		if (mes.startsWith("get:") || mes.startsWith("set:"))
			resp << mes;
	}
	el.quit();
}

void RemoteTcpConnection::connectedTo()
{

}

void RemoteTcpConnection::disconnectedFrom()
{

}
