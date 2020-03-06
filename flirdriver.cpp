#include "flirdriver.h"

#include "flirpttcphead.h"
#include "flirmodulehead.h"
#include <ptzptransport.h>

#include <QNetworkReply>
#include "debug.h"

FlirDriver::FlirDriver()
{
	headDome = new FlirPTTcpHead;
	headModule = new FlirModuleHead;
	defaultPTHead = headDome;
	defaultModuleHead = headModule;

	transportDome = new PtzpTcpTransport(PtzpTransport::PROTO_STRING_DELIM);
	httpTransportModule = new PtzpHttpTransport(PtzpTransport::PROTO_BUFFERED);
	reinitPT = false;
}

FlirDriver::~FlirDriver()
{
}

PtzpHead *FlirDriver::getHead(int index)
{
	if (index == 0)
		return headModule;
	else if (index == 1)
		return headDome;
	return NULL;
}

void FlirDriver::bumpStart()
{
	QNetworkAccessManager *man = new QNetworkAccessManager(this);
	req.setHeader(QNetworkRequest::ContentTypeHeader,
				  "application/x-www-form-urlencoded");
	QUrl url;
	url.setPort(80);
	url.setUrl(pturl);
	req.setUrl(url);
	connect(man, SIGNAL(finished(QNetworkReply*)), SLOT(finished(QNetworkReply*)));
	man->post(req, "C=V&PS=-100");
	qDebug() << "sending bump start command";
}

void FlirDriver::finished(QNetworkReply* reply)
{
	if (reply->error() != QNetworkReply::NoError) {
		mDebug("Bump start command failed %d %s ", reply->error(), qPrintable(reply->errorString()));
		reinitPT = true;
		return;
	}
	QString data = reply->readAll();
	if (data.isEmpty()) {
		reinitPT = true;
		return;
	}
	QNetworkAccessManager *man = static_cast<QNetworkAccessManager*>(sender());
	if (!data.contains("H"))
		man->post(req, "H");
	else {
		man->deleteLater();
		reply->deleteLater();
	}
}

int FlirDriver::setTarget(const QString &targetUri)
{
	int ret = 0;
	if (!targetUri.contains(";"))
		return -ENODATA;
	QStringList fields = targetUri.split(";");
	headDome->setTransport(transportDome);
	headModule->setTransport(httpTransportModule);

	pturl = "http://" + fields[1].split(":").first() + "/API/PTCmd";
	bumpStart();

	transportDome->connectTo(fields[1]);
	transportDome->enableQueueFreeCallbacks(true);
	transportDome->setTimerInterval(2000);
	transportDome->setDelimiter("*");

	httpTransportModule->setContentType(PtzpHttpTransport::AppJson);
	ret = httpTransportModule->connectTo(fields[0]);
	if (ret)
		return ret;
	httpTransportModule->enableQueueFreeCallbacks(true);
	httpTransportModule->setTimerInterval(2000);

	headDome->initialize();
	return 0;
}

QJsonObject FlirDriver::doExtraDeviceTests()
{
	QJsonObject obj;
	if (headModule->getLaserTimer() < 10000)
		return obj;
	obj.insert("type", QString("control_module"));
	obj.insert("index", 0);
	obj.insert("name", QString("laser"));
	obj.insert("elapsed_since_last_valid", headModule->getLaserTimer());
	return obj;
}

/*
 * [CR] [fo] flirmodulehead içerisinde modülün durumuna göre
 * işlem yapılan birkaç özellik var.Bu yüzden buraya sync state'i ve
 * modulehead içerisine sync işlemi eklenmeli.
 */
void FlirDriver::timeout()
{
	if (transportDome->getStatus() == PtzpTcpTransport::DEAD) {
		reinitPT = true;
		transportDome->reConnect();
		return;
	}
	if (reinitPT) {
		if (transportDome->getStatus() == PtzpTcpTransport::ALIVE) {
			bumpStart();
			reinitPT = false;
			headDome->initialize();
		}
	}
	PtzpDriver::timeout();
}
