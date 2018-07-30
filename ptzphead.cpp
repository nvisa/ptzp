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

int PtzpHead::getHeadStatus()
{
	return ST_NORMAL;
}

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

float PtzpHead::getPanAngle()
{
	return 0;
}

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
