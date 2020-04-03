#ifndef SIMPLEMETRICS_H
#define SIMPLEMETRICS_H

#include <QList>
#include <QHash>
#include <QString>
#include <QDateTime>
#include <QMutexLocker>
#include <QElapsedTimer>

class DataPool;
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
		INT64,
	};

	Channel(Point *parent, ChannelDataType type, int chno)
	{
		p = parent;
		dtype = type;
		no = chno;
		ft.start();
	}

	int channelNo() { return no; }
	ChannelDataType dataType() { return dtype; }
	void push(int data);
	void push(float data);
	void push(double data);
	void push(qint64 data);
	QString getName() { return name; }
	void setName(const QString &n) { name = n; }

	qint64 lastFetchTime()
	{
		return ft.elapsed();
	}

	QList<QPair<qint64, int>> fetchIntSamples()
	{
		QMutexLocker ml(&m);
		QList<QPair<qint64, int>> s = samplesInt;
		samplesInt.clear();
		ft.restart();
		return s;
	}

	QList<QPair<qint64, float>> fetchFloatSamples()
	{
		QMutexLocker ml(&m);
		QList<QPair<qint64, float>> s = samplesFloat;
		samplesFloat.clear();
		ft.restart();
		return s;
	}

	QList<QPair<qint64, double>> fetchDoubleSamples()
	{
		QMutexLocker ml(&m);
		QList<QPair<qint64, double>> s = samplesDouble;
		samplesDouble.clear();
		ft.restart();
		return s;
	}

	QList<QPair<qint64, qint64>> fetchInt64Samples()
	{
		QMutexLocker ml(&m);
		QList<QPair<qint64, qint64>> s = samplesInt64;
		samplesInt64.clear();
		ft.restart();
		return s;
	}

protected:
	int no;
	Point *p;
	QMutex m;
	QString name;
	QElapsedTimer ft;
	ChannelDataType dtype;
	QList<QPair<qint64, int>> samplesInt;
	QList<QPair<qint64, float>> samplesFloat;
	QList<QPair<qint64, double>> samplesDouble;
	QList<QPair<qint64, qint64>> samplesInt64;
};

class Point
{
public:
	Point(int channelOffset)
	{
		choff = channelOffset;
		chno = choff;
	}

	Channel * addIntegerChannel(const QString &name);
	Channel * addFloatChannel(const QString &name);
	Channel * addDoubleChannel(const QString &name);
	Channel * addInt64Channel(const QString &name);

	QString getName() { return name; }
	void setName(const QString &n) { name = n; }

protected:
	friend class Channel;
	void dataPushed(Channel *ch);

	int choff;
	int chno;
	QString name;
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
	int fetchInterval;
	DataPool *pool;
};

}

#endif // SIMPLEMETRICS_H
