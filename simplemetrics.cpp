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
	return mp;
}

Metrics::Metrics()
{
	wssrc = nullptr;
}

void Metrics::dataPushed(Point *point, Channel *ch)
{
	if (!wssrc)
		return;
	if (ch->dataType() == Channel::INT) {
		const auto samples = ch->fetchIntSamples();
		ChartData data;
		data.channel = ch->channelNo();
		for (int i = 0; i < samples.length(); i++) {
			const auto p = samples[i];
			data.ts << p.first;
			data.samples << p.second;
		}
		mutex.lock();
		wssrc->sendMessage(data);
		mutex.unlock();
	}
}

Channel *Point::addIntegerChannel()
{
	return new Channel(this, Channel::INT, chno++);
}

Channel *Point::addFloatChannel()
{
	return new Channel(this, Channel::FLOAT, chno++);
}

Channel *Point::addDoubleChannel()
{
	return new Channel(this, Channel::DOUBLE, chno++);
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
