#include "ptzphead.h"
#include "ptzptransport.h"
#include "debug.h"

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
#ifdef HAVE_PTZP_GRPC_API
QVariantMap PtzpHead::getSettings()
{
	QVariantMap map;
	for (auto key : settings.keys())
		map.insert(key,getProperty(settings[key].second));
	return map;
}

void PtzpHead::setSettings(QVariantMap keyMap)
{
	for (auto key : keyMap.keys())
		setProperty(settings[key].first, keyMap[key].toUInt());
}
#endif
