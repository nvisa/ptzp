#ifndef SIMPLEMETRICS_H
#define SIMPLEMETRICS_H

#include <QList>
#include <QHash>
#include <QString>
#include <QDateTime>
#include <QMutexLocker>

class NetworkSource;

namespace SimpleMetrics {

class Point;

class Channel
{
public:
	enum ChannelDataType {
		INT,
		FLOAT,
		DOUBLE,
	};

	Channel(Point *parent, ChannelDataType type, int chno)
	{
		p = parent;
		dtype = type;
		no = chno;
	}

	int channelNo() { return no; }
	ChannelDataType dataType() { return dtype; }
	void push(int data);
	void push(float data);

	QList<QPair<qint64, int>> fetchIntSamples()
	{
		QMutexLocker ml(&m);
		QList<QPair<qint64, int>> s = samplesInt;
		samplesInt.clear();
		return s;
	}

	QList<QPair<qint64, float>> fetchFloatSamples()
	{
		QMutexLocker ml(&m);
		QList<QPair<qint64, float>> s = samplesFloat;
		samplesFloat.clear();
		return s;
	}

	QList<QPair<qint64, double>> fetchDoubleSamples()
	{
		QMutexLocker ml(&m);
		QList<QPair<qint64, double>> s = samplesDouble;
		samplesDouble.clear();
		return s;
	}

protected:
	int no;
	Point *p;
	QMutex m;
	ChannelDataType dtype;
	QList<QPair<qint64, int>> samplesInt;
	QList<QPair<qint64, float>> samplesFloat;
	QList<QPair<qint64, double>> samplesDouble;
};

class Point
{
public:
	Point(int channelOffset)
	{
		choff = channelOffset;
		chno = choff;
	}

	Channel * addIntegerChannel();
	Channel * addFloatChannel();
	Channel * addDoubleChannel();

protected:
	friend class Channel;
	void dataPushed(Channel *ch);

	int choff;
	int chno;
};

class Metrics
{
public:
	static Metrics & instance();

	void setupWebsocketTransport(const QString &url);

	QStringList getPointNames();
	Point * getPointByName(const QString &name);
	Point * addPoint(const QString &name);

protected:
	Metrics();

	friend class Point;
	void dataPushed(Point *point, Channel *ch);

private:
	QMutex mutex;
	NetworkSource *wssrc;
	QHash<QString, Point *> points;
};

}

#endif // SIMPLEMETRICS_H
