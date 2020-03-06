#include "irdomedriver.h"
#include "debug.h"
#include "irdomepthead.h"
#include "oemmodulehead.h"
#include "ptzpserialtransport.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <errno.h>

IRDomeDriver::IRDomeDriver(QObject *parent) : PtzpDriver(parent)
{
	defaultPTHead = NULL;
	defaultModuleHead = NULL;
	state = UNINIT;
}

PtzpHead *IRDomeDriver::getHead(int index)
{
	if (index == 0)
		return defaultModuleHead;
	else if (index == 1)
		return defaultPTHead;
	return NULL;
}

/**
 * @brief IRDomeDriver::setTarget
 * @param targetUri: This API using for initialize Serial Devices.
 * First parameter must contains Camera module serial console info.
 * Because sometimes callback flows need to close the Module serial console
 * (first initialized console).
 * Second and last parameter must contains PT serial console info.
 * This API can do two different consoles initialize, But it can't do for three
 * or more consoles. TargetUri Example-1: ttyS0?baud=9600 // Ekinoks cams
 * TargetUri Example-2: ttyS0?baud=9600;ttyTHS2?baud=9600 // USB-Aselsan
 * @return
 */
int IRDomeDriver::setTarget(const QString &targetUri)
{
	ptSupport = false;
	headModule = new OemModuleHead;
	headDome = new IRDomePTHead;
	QStringList fields = targetUri.split(";");
	/* [CR] [yca] Bildigim kadari ile bu sinifa eger ikinci alan
	 * kullanilmayacaksa ikinci parametre olarak null gecmek gerekiyor.
	 * Yani fields.size() = 1 olamiyor. Ya bu kod bunu karsilayacak sekilde
	 * temizlenmeli ya da fields.size() = 1 oldugu durum calisir hale
	 * getirilmeli.
	 */
	if (fields.size() == 1) {
		/* [CR] [yca] memory leak, constructor'da release etmeli */
		tp = new PtzpSerialTransport();
		headModule->setTransport(tp);
		headModule->enableSyncing(true);
		headDome->setTransport(tp);
		headDome->enableSyncing(true);

		defaultPTHead = headDome;
		defaultModuleHead = headModule;

		tp->setMaxBufferLength(50);
		if (tp->connectTo(fields[0]))
			return -EPERM;
	} else {
		/* [CR] [yca] memory leak ... */
		tp = new PtzpSerialTransport();
		headModule->setTransport(tp);
		headModule->enableSyncing(true);

		defaultModuleHead = headModule;
		if (tp->connectTo(fields[0]))
			return -EPERM;

		if (fields[1] != "null") {
			ptSupport = true;
			/* [CR] [yca] memory leak ... */
			tp1 = new PtzpSerialTransport();
			headDome->setTransport(tp1);
			headDome->enableSyncing(true);

			defaultPTHead = headDome;
			if (tp1->connectTo(fields[1]))
				return -EPERM;

			SpeedRegulation sreg = getSpeedRegulation();
			sreg.enable = true;
			sreg.ipol = SpeedRegulation::LINEAR;
			sreg.minSpeed = 0.01;
			sreg.minZoom = 0;
			sreg.maxZoom = 16384;
			sreg.zoomHead = headModule;
			setSpeedRegulation(sreg);
		}
	}

	QStringList zoomRatios;
	QString zrfname = "/etc/smartstreamer/ZoomRatios_value.txt";
	if (ptSupport)
		zrfname = "/etc/smartstreamer/ZoomRatios_sony_value.txt";
	QFile fz(zrfname);
	if (fz.open(QIODevice::ReadOnly)) {
		zoomRatios = QString::fromUtf8(fz.readAll()).split("\n", QString::SkipEmptyParts);
		fz.close();
	}

	std::vector<int> cZRatios;
	foreach (const QString &var, zoomRatios) {
		cZRatios.push_back(var.toInt(0, 16));
	}
	headModule->setZoomRatios(cZRatios);

	QStringList zoomLines, hLines, vLines;
	QFile f("/etc/smartstreamer/Zoom_value.txt");
	if (f.open(QIODevice::ReadOnly)) {
		zoomLines = QString::fromUtf8(f.readAll()).split("\n");
		f.close();
	}
	f.setFileName("/etc/smartstreamer/Hfov.txt");
	if (f.open(QIODevice::ReadOnly)) {
		hLines = QString::fromUtf8(f.readAll()).split("\n");
		f.close();
	}
	f.setFileName("/etc/smartstreamer/Vfov.txt");
	if (f.open(QIODevice::ReadOnly)) {
		vLines = QString::fromUtf8(f.readAll()).split("\n");
		f.close();
	}
	if (zoomLines.size() && zoomLines.size() == vLines.size() &&
		zoomLines.size() == hLines.size()) {
		mDebug("Valid zoom mapping files found, parsing");
		std::vector<float> hmap, vmap;
		std::vector<int> zooms;
		for (int i = 0; i < zoomLines.size(); i++) {
			zooms.push_back(zoomLines[i].toInt());
			hmap.push_back(hLines[i].toFloat());
			vmap.push_back(vLines[i].toFloat());
		}
		headModule->getRangeMapper()->setLookUpValues(zooms);
		headModule->getRangeMapper()->addMap(hmap);
		headModule->getRangeMapper()->addMap(vmap);
	}

	state = INIT;
	headModule->syncRegisters();
	return 0;
}

void IRDomeDriver::timeout()
{
	//	mLog("Driver state: %d", state);
	switch (state) {
	case INIT:
		if (headModule->getHeadStatus() == PtzpHead::ST_SYNCING)
			break;
		if (headModule->getHeadStatus() == PtzpHead::ST_ERROR)
			headModule->syncRegisters();
		state = SYNC_HEAD_MODULE;
		break;
	case SYNC_HEAD_MODULE:
		if (headModule->getHeadStatus() == PtzpHead::ST_SYNCING)
			break;

		if (headModule->getHeadStatus() == PtzpHead::ST_NORMAL) {
			if (registerSavingEnabled)
				headModule->loadRegisters("head0.json");
			tp->enableQueueFreeCallbacks(true);
		}

		if (ptSupport == true) {
			state = SYNC_HEAD_DOME;
			headDome->syncRegisters();
		} else {
			state = NORMAL;
		}
		break;
	case SYNC_HEAD_DOME:
		if (headDome->getHeadStatus() != PtzpHead::ST_NORMAL)
			break;

		if (registerSavingEnabled)
			headDome->loadRegisters("head1.json");
		state = NORMAL;
		tp1->enableQueueFreeCallbacks(true);
		break;
	case NORMAL: {
		if (ptSupport) {
			autoIRcontrol();
		}
		manageRegisterSaving();
		/*
		 * [CR] [yca] Bu ozellige arya'da rastladim, tam olarak
		 * doStartupProcess nedir anlayamadim.
		 *
		 * 1. Bunun gercek yeri burasi mi yoksa bu bir hack() mi
		 *	  aciklayabilir misiniz?
		 * 2. Eger dogru bir is yapiyorsak, en azindan bununla ilgili bir
		 *	  dokumantastasyon yazabilir miyiz? Nedir, neler yapabilir vs.
		 * 3. Eger bu faydali bir ozellik ise diger suruculurde de olmasi
		 *	  gerekmez mi? ilk soru ile birlikte dusunulebilir...
		 */
		if (doStartupProcess) {
			doStartupProcess = false;
			getStartupProcess();
		}

		/*
		 * [CR] [yca] [fo] 2243fe3110 commit'ini de review edebilir miyiz?
		 * O committe dogru olmayan bazi isler yapilimis gibi hissediyorum?
		 * Ornegin bu timeout() burada hic olmamis gibi...
		 */
		PtzpDriver::timeout();
		break;
	}
	case UNINIT:
		break;
	}
}

/* [CR] [yca] IR kontrolunun burada olmasi dogru mu? */
void IRDomeDriver::autoIRcontrol()
{
	int ledLevel = 1;
	if (headModule->getProperty(OemModuleHead::R_IRCF_STATUS)) {
		ledLevel = headModule->getZoomRatio() / 5 + 2;
		if (ledLevel > 7)
			ledLevel = 7;
	}
	if (ledLevel != headDome->getIRLed())
		headDome->setIRLed(ledLevel);
}
