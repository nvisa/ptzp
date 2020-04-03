#include "simplemetrics.h"

#include <QVariant>

#ifndef QT_WEBSOCKETS_LIB
struct ChartData
{
	QList<qreal> samples;
	QList<qint64> ts;
	int channel;
	QHash<QString, QVariant> meta;
};
class NetworkSource
{
public:
	void setThreadSupport(bool)
	{
	}
	void connectServer(const QString &, bool reconnect = true)
	{
		Q_UNUSED(reconnect);
	}
	void sendMessage(const ChartData &)
	{
	}
};
#define NO_METRICS
#else
#include "networksource.h"
#endif

using namespace SimpleMetrics;

template <typename T>
static void convertToChartData(QList<QPair<qint64, T>> samples, ChartData &data)
{
	for (int i = 0; i < samples.length(); i++) {
		const auto p = samples[i];
		data.ts << p.first;
		data.samples << p.second;
	}
}

class DataPool
{
public:
	DataPool()
	{
		ft.start();
		pushInterval = 500;
	}
	QList<ChartData> add(ChartData data)
	{
		QList<ChartData> l;
		m.lock();
		list << data;
		if (ft.elapsed() >= pushInterval) {
			l = list;
			list = QList<ChartData>();
			ft.restart();
		}
		m.unlock();
		return l;
	}
protected:
	QMutex m;
	QElapsedTimer ft;
	QList<ChartData> list;
	int pushInterval;
};

Metrics & Metrics::instance()
{
	static Metrics m;
	return m;
}

void Metrics::setupWebsocketTransport(const QString &url)
{
	wssrc = new NetworkSource();
	wssrc->setThreadSupport(true);
	wssrc->connectServer(url);
}

QStringList Metrics::getPointNames()
{
	return points.keys();
}

Point *Metrics::getPointByName(const QString &name)
{
	if (points.contains(name))
		return points[name];
	return nullptr;
}

Point *Metrics::addPoint(const QString &name)
{
	auto mp = new Point(points.size() * 1000 + 1000);
	points[name] = mp;
	mp->setName(name);
	return mp;
}

Metrics::Metrics()
{
	wssrc = nullptr;
	fetchInterval = 100;
	pool = new DataPool;
}

void Metrics::dataPushed(Point *point, Channel *ch)
{
	Q_UNUSED(point);
	if (!wssrc)
		return;
	if (ch->lastFetchTime() < fetchInterval)
		return;
	ChartData data;
	data.channel = ch->channelNo();
	if (ch->dataType() == Channel::INT) {
		const auto samples = ch->fetchIntSamples();
		convertToChartData<int>(samples, data);
	} else if (ch->dataType() == Channel::DOUBLE) {
		const auto samples = ch->fetchDoubleSamples();
		convertToChartData<double>(samples, data);
	} else if (ch->dataType() == Channel::FLOAT) {
		const auto samples = ch->fetchFloatSamples();
		convertToChartData<float>(samples, data);
	} else if (ch->dataType() == Channel::INT64) {
		const auto samples = ch->fetchInt64Samples();
		convertToChartData<qint64>(samples, data);
	}
	data.meta["point"] = point->getName();
	data.meta["channel"] = ch->getName();
	auto list = pool->add(data);
	if (list.size()) {
		mutex.lock();
		wssrc->sendMessage(list);
		mutex.unlock();
	}
}

Channel *Point::addIntegerChannel(const QString &name)
{
	auto ch = new Channel(this, Channel::INT, chno++);
	ch->setName(name);
	return ch;
}

Channel *Point::addFloatChannel(const QString &name)
{
	auto ch = new Channel(this, Channel::FLOAT, chno++);
	ch->setName(name);
	return ch;
}

Channel *Point::addDoubleChannel(const QString &name)
{
	auto ch = new Channel(this, Channel::DOUBLE, chno++);
	ch->setName(name);
	return ch;
}

Channel *Point::addInt64Channel(const QString &name)
{
	auto ch = new Channel(this, Channel::INT64, chno++);
	ch->setName(name);
	return ch;
}

void Point::dataPushed(Channel *ch)
{
	Metrics::instance().dataPushed(this, ch);
}

void Channel::push(int data)
{
#ifndef NO_METRICS
	qint64 now = QDateTime::currentMSecsSinceEpoch();
	m.lock();
	samplesInt << QPair<qint64, int>(now, data);
	m.unlock();
	p->dataPushed(this);
#endif
}

void Channel::push(float data)
{
#ifndef NO_METRICS
	qint64 now = QDateTime::currentMSecsSinceEpoch();
	m.lock();
	samplesFloat << QPair<qint64, float>(now, data);
	m.unlock();
	p->dataPushed(this);
#endif
}

void Channel::push(double data)
{
#ifndef NO_METRICS
	qint64 now = QDateTime::currentMSecsSinceEpoch();
	m.lock();
	samplesDouble << QPair<qint64, double>(now, data);
	m.unlock();
	p->dataPushed(this);
#endif
}

void Channel::push(qint64 data)
{
#ifndef NO_METRICS
	qint64 now = QDateTime::currentMSecsSinceEpoch();
	m.lock();
	samplesInt64 << QPair<qint64, qint64>(now, data);
	m.unlock();
	p->dataPushed(this);
#endif
}
