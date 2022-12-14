#include "gpiocontroller.h"

#include <QFile>

#include <errno.h>

GpioController::GpioController(QObject *parent) :
	QObject(parent)
{
}

int GpioController::setName(int gpio, const QString &name)
{
	if (!requested.contains(gpio))
		return -ENOENT;
	requested[gpio]->name = name;
	namemap.insert(name, gpio);
	return 0;
}

QString GpioController::getName(int gpio)
{
	if (!requested.contains(gpio))
		return "";
	return requested[gpio]->name;
}

int GpioController::getGpioNo(const QString &name)
{
	if (!namemap.contains(name))
		return -1;
	return namemap[name];
}

int GpioController::registerGpio(int gpio)
{
	gpios << gpio;
	return 0;
}

int GpioController::requestGpio(int gpio, Direction d, const QString &name)
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
	setName(gpio, name);

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
	return QString::fromUtf8(g->h->readAll()).trimmed().toInt();
}

GpioController::Direction GpioController::getDirection(int gpio)
{
	if (!requested.contains(gpio))
		return INPUT;
	return requested[gpio]->d;
}

int GpioController::setActiveLow(int gpio)
{
	QFile f(QString("/sys/class/gpio/gpio%1/active_low").arg(gpio));
	if (!f.open(QIODevice::WriteOnly))
		return -EPERM;

	f.write("1\n");
	f.close();
	return 0;
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
