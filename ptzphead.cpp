#include "ptzphead.h"
#include "ptzptransport.h"
#include "debug.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

#include <errno.h>

static const char ioErrorStr[][256] = {
	"None",
	"Unsupported special command",
	"Special command checksum error",
	"Unsupported command",
	"Unsupported visca command",
	"Pelco-d checksum error",
	"Un-expected last byte for special command",
	"Un-expected last byte for visca command",
	"No last command written exist in history",
	"Visca invalid address"
};

PtzpHead::PtzpHead()
{
	systemChecker = -1;
	transport = NULL;
	pingTimer.start();
}

/**
 * @brief PtzpHead::setTransport
 * transport türünün belirtildiği metod.
 * @param tport = PtzpTcpTransport veya PtzpSerialTransport
 * sınıflarından türetilmiş nesnelerdir.
 * @return
 */

int PtzpHead::setTransport(PtzpTransport *tport)
{
	transport = tport;
	tport->addReadyCallback(PtzpHead::dataReady, this);
	tport->addQueueFreeCallback(PtzpHead::transportReady, this);
	return 0;
}

void PtzpHead::setTransportInterval(int interval)
{
	transport->setTimerInterval(interval);
}

int PtzpHead::syncRegisters()
{
	return 0;
}

/**
 * @brief PtzpHead::getHeadStatus
 * İlgili kafaların durumlarını döndüren metod.
 * @return 3 farklı bilgi içerebilir;
 *0 ST_SYNCING
 * 1 ST_NORMAL
 * 2 ST_ERROR
 * @note gövde içeriği her kafada tekrar yazılmalıdır.
 */

int PtzpHead::getHeadStatus()
{
	return ST_NORMAL;
}

/**
 * @brief PtzpHead::panLeft
 * @brief PtzpHead::panRight
 * @brief PtzpHead::tiltUp
 * @brief PtzpHead::tiltDown
 *
 * Pan ve tilt dönüşleri gerçekleştiren metodlar.
 * @param speed = Hareket hızı (Kafa tipleri ve pan-tilt
 * işlemlerine göre değişiklik göstermektedir.)
 * @return
 * @note Her bir bir metod ilgili PTHead içerisinde tekrar tanımlanmalı
 * ve metod gövdesi ilgili kafaya göre tekrar doldurulmalıdır.
 */

int PtzpHead::panLeft(float speed)
{
	Q_UNUSED(speed);
	return -ENOENT;
}

int PtzpHead::panRight(float speed)
{
	Q_UNUSED(speed);
	return -ENOENT;
}

int PtzpHead::tiltUp(float speed)
{
	Q_UNUSED(speed);
	return -ENOENT;
}

int PtzpHead::tiltDown(float speed)
{
	Q_UNUSED(speed);
	return -ENOENT;
}

int PtzpHead::panTiltAbs(float pan, float tilt)
{
	Q_UNUSED(pan);
	Q_UNUSED(tilt);
	return -ENOENT;
}

int PtzpHead::panTiltDegree(float pan, float tilt)
{
	Q_UNUSED(pan);
	Q_UNUSED(tilt);
	return -ENOENT;
}

int PtzpHead::panTiltStop()
{
	return -ENOENT;
}

/**
 * @brief PtzpHead::startZoomIn
 * @brief PtzpHead::startZoomOut
 *
 * Zoom işlemlerinin gerçekleştirildiği metod
 * @param speed = Zoom hızı
 * @return
 * @note Her bir bir metod ilgili Head içerisinde tekrar tanımlanmalı
 * ve metod gövdesi ilgili kafaya göre tekrar doldurulmalıdır.
 */

int PtzpHead::startZoomIn(int speed)
{
	Q_UNUSED(speed);
	return -ENOENT;
}

int PtzpHead::startZoomOut(int speed)
{
	Q_UNUSED(speed);
	return -ENOENT;
}

int PtzpHead::stopZoom()
{
	return -ENOENT;
}

int PtzpHead::focusIn(int speed)
{
	Q_UNUSED(speed);
	return -ENOENT;
}

int PtzpHead::focusOut(int speed)
{
	Q_UNUSED(speed);
	return -ENOENT;
}

int PtzpHead::focusStop()
{
	return -ENOENT;
}

/**
 * @brief PtzpHead::getPanAngle
 * Pan açısının döndürüldüğü metod.
 * @return Açısal, kesirli sonuç verir.
 * @note Gövde her PTHead içinde tekrar yazılmalıdır.
 */

float PtzpHead::getPanAngle()
{
	return 0;
}

/**
 * @brief PtzpHead::getTiltAngle
 * Tilt açısının döndürüldüğü metod.
 * @return Açısal, kesirli sonuç verir.
 * @note Gövde her PTHead içinde tekrar yazılmalıdır.
 */

float PtzpHead::getTiltAngle()
{
	return 0;
}

int PtzpHead::getZoom()
{
	return 0;
}

int PtzpHead::setZoom(uint pos)
{
	Q_UNUSED(pos);
	return 0;
}

int PtzpHead::getFOV(float &hor, float &ver)
{
	if (!rmapper.isAvailable())
		return -ENOENT;
	auto v = rmapper.map(getZoom());
	hor = v[0];
	ver = v[1];
	return 0;
}

int PtzpHead::panTiltGoPos(float ppos, float tpos)
{
	Q_UNUSED(ppos);
	Q_UNUSED(tpos);
	return 0;
}

int PtzpHead::getErrorCount(uint err)
{
	if (err == IOE_NONE) {
		QHashIterator<uint, uint> hi(errorCount);
		uint total = 0;
		while (hi.hasNext()) {
			total += hi.value();
		}
		return total;
	}
	return errorCount[err];
}

void PtzpHead::enableSyncing(bool en)
{
	Q_UNUSED(en);
}

void PtzpHead::setSyncInterval(int interval)
{
	Q_UNUSED(interval);
}

int PtzpHead::dataReady(const unsigned char *bytes, int len, void *priv)
{
	return ((PtzpHead *)priv)->dataReady(bytes, len);
}

QByteArray PtzpHead::transportReady(void *priv)
{
	return ((PtzpHead *)priv)->transportReady();
}

/**
 * @brief PtzpHead::dataReady
 * @param bytes = cihaz tarafında tanımlanan get komutlarının
 * seri portvasıtasıyla okunan cevaplarını bir dizi şeklinde alır.
 * @param len = dizinin uzunluk bilgisidir.
 * @return cevapların uzunluğudur.
 */
int PtzpHead::dataReady(const unsigned char *bytes, int len)
{
	Q_UNUSED(bytes);
	return len;
}

QByteArray PtzpHead::transportReady()
{
	return QByteArray();
}

void PtzpHead::setIOError(int err)
{
	/* TODO: Implement IO error handling */
	errorCount[err]++;
}

/**
 * @brief PtzpHead::setRegister
 * @param reg = değiştirilmesi istenen registerin enum karşılık değerini alır.
 * @param value = registera atanmak istenen değeri alır.
 * @return 0 döndürür.
 * @note Program bazında register değeri değiştirmek için kullanılır.
 * Yani bu fonksiyonu kullanarak kamera üzerindeki değeri değiştiremezsiniz.
 */
int PtzpHead::setRegister(uint reg, uint value)
{
	QMutexLocker l(&rlock);
	registers[reg] = value;
	return 0;
}

uint PtzpHead::getRegister(uint reg)
{
	QMutexLocker l(&rlock);
	return registers[reg];
}

QJsonValue PtzpHead::marshallAllRegisters()
{
	return QJsonObject();
}

void PtzpHead::unmarshallloadAllRegisters(const QJsonValue &node)
{
	Q_UNUSED(node);
}

uint PtzpHead::getProperty(uint r)
{
	Q_UNUSED(r);
	return 0;
}

/**
 * @brief PtzpHead::setProperty
 * @param r = "enum Commands" içerisinde yer alan set işlemi yapan
 * komut.(Her kafanın kendine özgü set komutları bulunmaktadır.)
 * @param x = set işleminin yapılacağı komuta gönderilen değer.
 * @note Donanım üzerinde değişiklik yapmak için kullanılır.
 * Örneğin; shutter değerinin değiştirilmesi.
 */
void PtzpHead::setProperty(uint r, uint x)
{
	Q_UNUSED(r);
	Q_UNUSED(x);
}

QVariant PtzpHead::getProperty(const QString &key)
{
	Q_UNUSED(key)
	return QVariant();
}

void PtzpHead::setProperty(const QString &key, const QVariant &value)
{
	Q_UNUSED(key);
	Q_UNUSED(value);
}

int PtzpHead::headSystemChecker()
{
	return 0;
}

QString PtzpHead::whoAmI()
{
	return "";
}

int PtzpHead::getSystemStatus()
{
	// this function using just check system healt state
	// if returning value equal to `2`, system healty. But other values you must check Camera parts.
	return systemChecker;
}


int PtzpHead::saveRegisters(const QString &filename)
{
	QFile f(filename);
	if (!f.open(QIODevice::ReadWrite | QIODevice::Text))
		return -EPERM;
	QJsonValue json = marshallAllRegisters();

	QJsonDocument doc;
	QJsonObject obj;
	obj.insert("registers", json);
	doc.setObject(obj);
	f.write(doc.toJson());
	f.close();

	return 0;
}

int PtzpHead::loadRegisters(const QString &filename)
{
	QFile f(filename);
	if (!f.exists())
		return -ENOENT;
	if (!f.open(QIODevice::ReadWrite | QIODevice::Text))
		return -EPERM;

	const QByteArray &json = f.readAll();
	f.close();

	const QJsonDocument &doc = QJsonDocument::fromJson(json);
	QJsonObject root = doc.object();
	if (root.isEmpty())
		return -EINVAL;
	unmarshallloadAllRegisters(root["registers"]);

	return 0;
}

int PtzpHead::communicationElapsed()
{
	return pingTimer.elapsed();
}

std::vector<float> PtzpHead::RangeMapper::map(int value)
{
	std::vector<float> m;
	auto lower = std::lower_bound(lookup.begin(), lookup.end(), value);
	auto upper = std::lower_bound(lookup.begin(), lookup.end(), value);
	if (lower == lookup.begin()) {
		for (size_t i = 0; i < maps.size(); i++)
			m.push_back(maps[i].front());
	} else if (upper == lookup.end()) {
		for (size_t i = 0; i < maps.size(); i++)
			m.push_back(maps[i].back());
	} else {
		size_t offl = lower - lookup.begin();
		size_t offu = upper - lookup.begin();
#if 0
		if (offl >= lookup.size())
			offl = lookup.size() - 1;
		if (offu >= lookup.size())
			offu = lookup.size() - 1;
#endif
		int lval = lookup[offl];
		int uval = lookup[offu];
		float w1 = uval - value;
		float w0 = -value - lval;
		for (size_t i = 0; i < maps.size(); i++)
			m.push_back((w1 * maps[i][offu] + w0 * maps[i][offl]) / (w0 + w1));
	}
	return m;
}

#ifdef HAVE_PTZP_GRPC_API
QVariantMap PtzpHead::getSettings()
{
	QVariantMap map;
	for (auto key : settings.keys())
		map.insert(key,getProperty(settings[key].second));
	foreach (const QString &key, nonRegisterSettings)
		map.insert(key, getProperty(key));
	return map;
}

void PtzpHead::setSettings(QVariantMap keyMap)
{
	for (auto key : keyMap.keys()) {
		if (settings.contains(key))
			setProperty(settings[key].first, keyMap[key].toUInt());
		else if (nonRegisterSettings.contains(key))
			setProperty(key, keyMap[key]);
	}
}
#endif
