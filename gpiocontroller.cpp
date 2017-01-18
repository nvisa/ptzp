#include "gpiocontroller.h"

#include <QFile>

#include <errno.h>

GpioController::GpioController(QObject *parent) :
	QObject(parent)
{
}

int GpioController::registerGpio(int gpio)
{
	gpios << gpio;
	return 0;
}

int GpioController::requestGpio(int gpio, Direction d)
{
	QString dname = QString(QString("/sys/class/gpio/gpio%1").arg(gpio));
	if (!QFile::exists(dname)) {
		int err = exportGpio(gpio);
		if (err)
			return err;
	}

	int err = setDirection(gpio, d);
	if (err)
		return err;

	QFile *f = new QFile(QString("/sys/class/gpio/gpio%1/value").arg(gpio));
	if (!f->open(QIODevice::ReadWrite | QIODevice::Unbuffered)) {
		f->deleteLater();
		return -EPERM;
	}
	Gpio *g = new Gpio;
	g->h = f;
	g->d = d;
	requested.insert(gpio, g);

	return 0;
}

int GpioController::releaseGpio(int gpio)
{
	if (!requested.contains(gpio))
		return -ENOENT;
	Gpio *g = requested[gpio];
	g->h->close();
	g->h->deleteLater();
	delete g;
	return 0;
}

int GpioController::setGpio(int gpio)
{
	if (!requested.contains(gpio))
		return -ENOENT;
	Gpio *g = requested[gpio];
	if (g->d == INPUT)
		return -EINVAL;
	g->h->write("1\n");
	return 0;
}

int GpioController::setGpio(int gpio, int value)
{
	if (value)
		return setGpio(gpio);
	return clearGpio(gpio);
}

int GpioController::toggleGpio(int gpio)
{
	if (!requested.contains(gpio))
		return -ENOENT;
	Gpio *g = requested[gpio];
	if (g->d == INPUT)
		return -EINVAL;
	int state = QString::fromUtf8(g->h->readAll()).trimmed().toInt();
	if (state)
		g->h->write("0\n");
	else
		g->h->write("1\n");
	return 0;
}

int GpioController::clearGpio(int gpio)
{
	if (!requested.contains(gpio))
		return -ENOENT;
	Gpio *g = requested[gpio];
	if (g->d == INPUT)
		return -EINVAL;
	g->h->write("0\n");
	return 0;
}

int GpioController::getGpioValue(int gpio)
{
	if (!requested.contains(gpio))
		return -ENOENT;
	Gpio *g = requested[gpio];
	g->h->seek(0);
	g->h->write("0\n");
	return QString::fromUtf8(g->h->readAll()).trimmed().toInt();
}

int GpioController::exportGpio(int gpio)
{
	QFile f("/sys/class/gpio/export");
	if (!f.open(QIODevice::WriteOnly))
		return -EPERM;
	f.write(QString("%1\n").arg(gpio).toUtf8());
	f.close();
	return 0;
}

int GpioController::setDirection(int gpio, GpioController::Direction d)
{
	QFile f(QString("/sys/class/gpio/gpio%1/direction").arg(gpio));
	if (!f.open(QIODevice::WriteOnly))
		return -EPERM;
	if (d == INPUT)
		f.write("in\n");
	else
		f.write("out\n");
	f.close();
	return 0;
}
