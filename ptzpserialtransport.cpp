#include "ptzpserialtransport.h"
#include "debug.h"
#include "drivers/qextserialport/qextserialport.h"

#include <QMutex>
#include <QThread>

#include <errno.h>
#include <unistd.h>

#define WRITE_PERIOD	20000
#define READ_PERIOD		5000

static QMutex serlock;

class ReadThread : public QThread
{
public:
	ReadThread(QextSerialPort *p, PtzpSerialTransport::dataReady cb, void *cbp)
	{
		port = p;
		this->cb = cb;
		this->cbp = cbp;
		debug = 0;
	}

	void setDebugLevel(int v)
	{
		debug = v;
	}

	void run()
	{
		QByteArray buf;
		while (1) {
			usleep(READ_PERIOD);
			serlock.lock();
			if (!port->bytesAvailable()) {
				serlock.unlock();
				continue;
			}
			buf.append(port->readAll());
			if (debug & 0x1)
				qDebug("read thread: read %d bytes", buf.size());
			serlock.unlock();
			int readlen = 0;
			do {
				readlen = process(buf);
				if (readlen > 0)
					buf = buf.mid(readlen);
			} while (readlen > 0 && buf.size());
		}
	}

protected:
	int process(const QByteArray &buf)
	{
		/* if we have no callback, assume all is read */
		if (cb == NULL)
			return buf.size();
		const uchar *p = (const uchar *)buf.constData();
		return cb(p, buf.size(), cbp);
	}

	QextSerialPort *port;
	int debug;
	PtzpTransport::dataReady cb;
	void *cbp;
};

class WriteThread : public QThread
{
public:
	WriteThread(QextSerialPort *p, PtzpTransport::queueFree cb, void *cbp)
	{
		port = p;
		debug = 0;
		this->cb = cb;
		this->cbp = cbp;
	}

	void setDebugLevel(int v) { debug = v; }
	void add(const QByteArray &mes, int prio)
	{
		l.lock();
		if (prio == 1)
			pq1 << mes;
		if (prio == 2)
			pq2 << mes;
		if (prio == 3)
			pq3 << mes;
		l.unlock();
	}

	void run()
	{
		while (1) {
			usleep(WRITE_PERIOD);
			QByteArray m;
			l.lock();
			if (pq1.size())
				m = pq1.takeFirst();
			else if (pq2.size())
				m = pq2.takeFirst();
			else if (pq3.size()) {
				/* pq3 holds periodic sent commands */
				m = pq3.takeFirst();
				pq3 << m;
			} else if (cb) {
				m = cb(cbp);
			}
			l.unlock();
			if (!m.size())
				continue;
			send(m);
		}
	}


protected:
	void send(const QByteArray &ba)
	{
		serlock.lock();
		if (debug) {
			for (int i = 0; i < ba.size(); i++)
				qDebug("%s: %d: 0x%x", __func__, i, (uchar)ba.at(i));
		}
		port->write(ba);
		serlock.unlock();
	}

	int debug;
	QextSerialPort *port;
	QList<QByteArray> pq1;
	QList<QByteArray> pq2;
	QList<QByteArray> pq3;
	QMutex l;
	PtzpTransport::queueFree cb;
	void *cbp;
};

PtzpSerialTransport::PtzpSerialTransport()
{
	port = NULL;
	readThread = NULL;
	writeThread = NULL;
}

int PtzpSerialTransport::connectTo(const QString &targetUri)
{
	QStringList fields = targetUri.split(";");
	QString filename = fields.first();
	QHash<QString, QString> values;
	for (int i = 0; i < fields.size(); i++) {
		if (fields[i].contains("="))
			values.insert(fields[i].split("=")[0], fields[i].split("=")[1]);
		else
			values.insert(fields[i], "");
	}
	port = new QextSerialPort(filename, QextSerialPort::Polling);
	/* some sane defaults */
	port->setBaudRate(BAUD9600);
	port->setFlowControl(FLOW_OFF);
	port->setParity(PAR_NONE);
	port->setDataBits(DATA_8);
	port->setStopBits(STOP_1);
	if (values.contains("baud"))
		port->setBaudRate((BaudRateType)values["baud"].toInt());
	if (!port->open(QIODevice::ReadWrite)) {
		fDebug("error opening serial port '%s': %s", qPrintable(port->portName()), strerror(errno));
		return -EPERM;
	}
	port->readAll();

	readThread = new ReadThread(port, PtzpTransport::dataReadyCallback, this);
	readThread->start();
	writeThread = new WriteThread(port, PtzpTransport::queueFreeCallback, this);
	writeThread->start();

	return 0;
}

int PtzpSerialTransport::send(const char *bytes, int len)
{
	if (!writeThread)
		return -ENOENT;
	writeThread->add(QByteArray(bytes, len), 1);
	return 0;
}

