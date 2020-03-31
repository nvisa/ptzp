#ifndef GPIOCONTROLLER_H
#define GPIOCONTROLLER_H

#include <QHash>
#include <QObject>

class QFile;

class GpioController : public QObject
{
	Q_OBJECT
public:
	explicit GpioController(QObject *parent = 0);

	enum Direction {
		INPUT,
		OUTPUT,
	};

	int setName(int gpio, const QString &name);
	QString getName(int gpio);
	int getGpioNo(const QString &name);
	int registerGpio(int gpio);
	int requestGpio(int gpio, Direction d, const QString &name = "");
	int releaseGpio(int gpio);
	int setGpio(int gpio);
	int setGpio(int gpio, int value);
	int toggleGpio(int gpio);
	int clearGpio(int gpio);
	int getGpioValue(int gpio);
	Direction getDirection(int gpio);
	int setActiveLow(int gpio);

protected:
	int exportGpio(int gpio);
	int setDirection(int gpio, Direction d);

	QList<int> gpios;

	struct Gpio {
		QFile *h;
		Direction d;
		QString name;
	};

	QHash<int, Gpio *> requested;
	QHash<QString, int> namemap;

};

#endif // GPIOCONTROLLER_H
