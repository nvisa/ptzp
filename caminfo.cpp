#include "caminfo.h"

CamInfo::CamInfo(QextSerialPort *port, QObject *parent) : QObject(parent)
{
	camPort = port;
}

void CamInfo::getModuleSummary()
{
	BaudRateType tempBaund = camPort->baudRate();
	ParityType tempParity = camPort->parity();

	camPort->setBaudRate(BAUD9600);
	camPort->setParity(PAR_NONE);
	if(!getIrDomemodule()) {
		camPort->setBaudRate(tempBaund);
		camPort->setParity(tempParity);
		return;
	}

	camPort->setBaudRate(BAUD4800);
	camPort->setParity(PAR_EVEN);
	if (!getHitachi()) {
		camPort->setBaudRate(tempBaund);
		camPort->setParity(tempParity);
		return;
	} else {
		camPort->setBaudRate(tempBaund);
		camPort->setParity(tempParity);
		qDebug() << "Undefined camera modul or modul is not aviable.";
	}
}

int CamInfo::getIrDomemodule()
{
	IrDomeModule *dome = new IrDomeModule(camPort, 0);
	camPort->flush();
	int model = dome->getModel(camPort);
	if (model == IrDomeModule::MODULE_TYPE_PV6403_F12D ) {
		qDebug() << "Modul:\tOEM 3X";
	} else if (model == IrDomeModule::MODULE_TYPE_FCB_EV7500) {
		qDebug() << "Modul:\tSONY";
	} else if (model == IrDomeModule::MODULE_TYPE_PV8430_F2D ) {
		qDebug() << "Modul:\tOEM 30X";
	} else
		return -1;

	QString modeStr;
	int mode = dome->vQueReg(0x72);
	if (mode == 0x1) {
		modeStr = "0x01: 1080i/59.94";
	} else if (mode == 0x3) {
		modeStr = "0x03: NTSC";
	} else if (mode == 0x4) {
		modeStr = "0x04: 1080i/50";
	} else if (mode == 0x5) {
		modeStr = "0x05: PAL";
	} else if (mode == 0x6) {
		modeStr = "0x06: 1080p/29.97";
	} else if (mode == 0x8) {
		modeStr = "0x08: 1080p/25";
	} else if (mode == 0x9) {
		modeStr = "0x09: 720p/50";
	} else if (mode == 0x0C) {
		modeStr = "0x0C: 720p/50";
	} else if (mode == 0x0E) {
		modeStr = "0E: 720p/29.97";
	} else if (mode == 0x11) {
		modeStr = "0x11: 720p/25";
	} else if (mode == 0x13) {
		modeStr = "0x13: 1080p/59.94";
	} else if (mode == 0x14) {
		modeStr = "0x14: 1080p/50";
	} else {
		modeStr = "0x" + QString::number(mode, 16) + "RESERVED";
	}
	qDebug() << "Monitoring Mode:\t" << modeStr.toLatin1().data();

	if ( mode == 3 ) {
		modeStr = "1/100";
	} else if (mode == 5) {
		modeStr = "1/120";
	} else {
		int shut = dome->getShutterSpeed();
		if (shut == 0x0) {
			modeStr = "1/1";
		} else if (shut == 0x1) {
			modeStr = "1/2";
		} else if (shut == 0x2) {
			if (mode == 1 || mode == 6 || mode == 0x0e || mode == 0x13)
				modeStr = "1/4";
			else
				modeStr = "1/3";
		} else if (shut == 0x3) {
			if (mode == 1 || mode == 6 || mode == 0x0e || mode == 0x13) {
				modeStr = "1/8";
			} else {
				modeStr = "1/6";
			}
		} else if (shut == 0x4) {
			if (mode == 1 || mode == 6 || mode == 0x0e || mode == 0x13) {
				modeStr = "1/15";
			} else {
				modeStr = "1/12";
			}
		} else if (shut == 0x5) {
			if (mode == 1 || mode == 6 || mode == 0x0e || mode == 0x13) {
				modeStr = "1/30";
			} else {
				modeStr = "1/25";
			}
		} else if (shut == 0x6) {
			if (mode == 1 || mode == 6 || mode == 0x0e || mode == 0x13) {
				modeStr = "1/60";
			} else {
				modeStr = "1/50";
			}
		} else if (shut == 0x07) {
			if (mode == 1 || mode == 6 || mode == 0x0e || mode == 0x13) {
				modeStr = "1/90";
			} else {
				modeStr = "1/75";
			}
		} else if (shut == 0x08) {
			modeStr = "1/100";
		} else if (shut == 0x09) {
			if (mode == 1 || mode == 6 || mode == 0x0e || mode == 0x13) {
				modeStr = "1/125";
			} else {
				modeStr = "1/120";
			}
		} else if (shut == 0x0A) {
			if (mode == 1 || mode == 6 || mode == 0x0e || mode == 0x13) {
				modeStr = "1/180";
			} else {
				modeStr = "1/150";
			}
		} else if (shut == 0x0B) {
			if (mode == 1 || mode == 6 || mode == 0x0e || mode == 0x13) {
				modeStr = "1/250";
			} else {
				modeStr = "1/215";
			}
		} else if (shut == 0x0C) {
			if (mode == 1 || mode == 6 || mode == 0x0e || mode == 0x13) {
				modeStr = "1/350";
			} else {
				modeStr = "1/300";
			}
		} else if (shut == 0x0D) {
			if (mode == 1 || mode == 6 || mode == 0x0e || mode == 0x13) {
				modeStr = "1/500";
			} else {
				modeStr = "1/425";
			}
		} else if (shut == 0x0E) {
			if (mode == 1 || mode == 6 || mode == 0x0e || mode == 0x13) {
				modeStr = "1/725";
			} else {
				modeStr = "1/600";
			}
		} else if (shut == 0x0F) {
			modeStr = "1/1000";
		} else if (shut == 0x10) {
			if (mode == 1 || mode == 6 || mode == 0x0e || mode == 0x13) {
				modeStr = "1/1500";
			} else {
				modeStr = "1/1750";
			}
		} else if (shut == 0x11) {
			if (mode == 1 || mode == 6 || mode == 0x0e || mode == 0x13) {
				modeStr = "1/2000";
			} else {
				modeStr = "1/1750";
			}
		} else if (shut == 0x12) {
			if (mode == 1 || mode == 6 || mode == 0x0e || mode == 0x13) {
				modeStr = "1/3000";
			} else {
				modeStr = "1/2500";
			}
		} else if (shut == 0x13) {
			if (mode == 1 || mode == 6 || mode == 0x0e || mode == 0x13) {
				modeStr = "1/4000";
			} else {
				modeStr = "1/3500";
			}
		} else if (shut == 0x14) {
			modeStr = "1/6000";
		} else if (shut == 0x15) {
			modeStr = "1/10000";
		} else {
			modeStr = "Undefined";
		}
	}
	qDebug() << "Shutter Speed:\t" << modeStr.toLatin1().data();

	int focus = dome->getFocusMode();
	if  (focus == IrDomeModule::FOCUS_AUTO) {
		modeStr = "AUTO";
	} else if  (focus == IrDomeModule::FOCUS_MANUAL) {
		modeStr = "MANUAL";
	} else {
		modeStr = "UNDEFINED";
	}
	qDebug() << "Focus Mode:\t"<< modeStr.toLatin1().data();

	dome->vGetZoom();
	qDebug() << "Zoom:\t"<< dome->getZoomMem();
	return 0;
}

int CamInfo::getHitachi()
{
	HitachiModule *hitachi = new HitachiModule(camPort);
	camPort->flush();
	int modul = hitachi->getModel(camPort);
	if (modul == HitachiModule::MODULE_TYPE_SC110) {
		qDebug() << "Modul:\tHitachi SC110";
	} else if (modul == HitachiModule::MODULE_TYPE_SC120) {
		qDebug() << "Modul:\tHitachi SC120";
		int mode = hitachi->getCameraMode();
		if (mode == HitachiModule::MODE_720P_25)
			qDebug() << "Monitoring Mode:\t7201p/25fps";
		else if (mode == HitachiModule::MODE_720P_30)
			qDebug() << "Monitoring Mode:\t7201p/30fps";
	} else {
		hitachi->deleteLater();
		return -1;
	}

	HitachiModule::ProgramAEmode ae = hitachi->getProgramAEmode();
	QString modeStr;
	if (ae == HitachiModule::MODE_PRG_AE) {
		modeStr = "Program AE";
	} else if (ae == HitachiModule::MODE_PRG_AER_IR_RMV1) {
		modeStr = "Program AER1 [IR Remove - 1]";
	} else if (ae == HitachiModule::MODE_PRG_AER_IR_RMV2) {
		modeStr = "Program AER2 [IR Remove - 2]";
	} else if (ae == HitachiModule::MODE_PRG_AE_DSS) {
		modeStr = "Program AE+ (DSS)";
	} else if (ae == HitachiModule::MODE_PRG_AE_DSS_IR_RMV1) {
		modeStr = "Program AE+1 (DSS) [IR Remove - 1]";
	} else if (ae == HitachiModule::MODE_PRG_AE_DSS_IR_RMV2) {
		modeStr = "Program AE+2 (DSS) [IR Remove - 2]";
	} else if (ae == HitachiModule::MODE_PRG_AE_DSS_IR_RMV3) {
		modeStr = "Program AE+3 (DSS) [IR Remove - 3]";
	} else if (ae == HitachiModule::MODE_SHUTTER_PRIORITY) {
		modeStr = "Shutter Priority";
	} else if (ae == HitachiModule::MODE_EXPOSURE_PRIORITY) {
		modeStr = "Exposure Priority";
	} else if (ae == HitachiModule::MODE_FULL_MANUAL_CTRL) {
		modeStr = "Full Manual";
	} else {
		modeStr = "INVALID";
	}
	qDebug() << "Program AE mode:\t" << modeStr.toLatin1().data();

	HitachiModule::ShutterSpeed shut = hitachi->getShutterSpeed(ae);
	if (shut == HitachiModule::SH_SPD_1_OV_4) {
		modeStr = "1/4";
	} else if (shut == HitachiModule::SH_SPD_1_OV_8) {
		modeStr = "1/8";
	} else if (shut == HitachiModule::SH_SPD_1_OV_15) {
		modeStr = "1/15";
	} else if (shut == HitachiModule::SH_SPD_1_OV_30) {
		modeStr = "1/30";
	} else if (shut == HitachiModule::SH_SPD_1_OV_50) {
		modeStr = "1/50";
	} else if (shut == HitachiModule::SH_SPD_1_OV_60) {
		modeStr = "1/60";
	} else if (shut == HitachiModule::SH_SPD_1_OV_100) {
		modeStr = "1/100";
	} else if (shut == HitachiModule::SH_SPD_1_OV_120) {
		modeStr = "1/120";
	} else if (shut == HitachiModule::SH_SPD_1_OV_180) {
		modeStr = "1/180";
	} else if (shut == HitachiModule::SH_SPD_1_OV_250) {
		modeStr = "1/250";
	} else if (shut == HitachiModule::SH_SPD_1_OV_500) {
		modeStr = "1/500";
	} else if (shut == HitachiModule::SH_SPD_1_OV_1000) {
		modeStr = "1/1000";
	} else if (shut == HitachiModule::SH_SPD_1_OV_2000) {
		modeStr = "1/2000";
	} else if (shut == HitachiModule::SH_SPD_1_OV_4000) {
		modeStr = "1/4000";
	} else if (shut == HitachiModule::SH_SPD_1_OV_10000) {
		modeStr = "1/10000";
	} else if (shut == HitachiModule::SH_SPD_UNDEFINED) {
		modeStr = "UNDEFINED, Please check ae mode.";
	} else {
		modeStr = "Undefined";
	}
	qDebug() << "Shutter Speed:\t" << modeStr.toLatin1().data();

	HitachiModule::FocusMode focus = hitachi->getFocusMode();
	if (focus == HitachiModule::FOCUS_AUTO) {
		modeStr = "AUTO";
	} else if  (focus == HitachiModule::FOCUS_MANUAL) {
		modeStr = "MANUAL";
	} else {
		modeStr = "UNDEFINED";
	}
	qDebug() << "Focus Mode:\t" << modeStr.toLatin1().data();

	qDebug() << "Zoom:\t"<< hitachi->readReg(camPort, 0xfc91);
	hitachi->deleteLater();

	return 0;
}
