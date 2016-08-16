#include "pattern.h"

#include <QTimer>

#include <errno.h>

Pattern::Pattern(int writeState, QString pattFilename, int pattLimit, QObject *parent) :
	QObject(parent)
{
	writeAble = writeState;
	patternLimit = pattLimit;
	patternFile = pattFilename;
	for (int i = 0; i < patternLimit; i++)
		patternStates.append(NO_RUN_RECORD);
	if (writeAble)
		patternInit();
	patternT.start();
}

Pattern::~Pattern()
{

}

int Pattern::start(int ind)
{
	if ((!writeAble) || (ind >= patternLimit))
		return -ENOSTR;
	mDebug("pattern start");
	if (patternStates.at(ind) == NO_RUN_RECORD) {
		patternStates.replace(ind, RECORD);
		patternCmds.clear();
		const cmdAndTime key = {ind, KEYWORDS};
		patternCmds.append(key);
		patternT.restart();
		return 1;
	}
	return -ENODATA;
}

int Pattern::stop(int ind)
{
	if ((!writeAble) || (ind >= patternLimit))
		return -ENOSTR;
	mDebug("pattern stop");
	QString filename = QString("%1%2%3").arg(patternFile).arg(ind).arg(FILETAIL);
	QFile *runPatternFile = new QFile(filename);
	if (runPatternFile->open(QIODevice::ReadWrite)) {
		QDataStream patternStream(runPatternFile);
		patternStream.setByteOrder(QDataStream::LittleEndian);
		for (int i = 0; i < patternCmds.size(); i++) {
			patternStream << patternCmds.at(i).cmd;
			patternStream << patternCmds.at(i).time;
		}
		runPatternFile->close();
		patternCmds.clear();
		patternStates.replace(ind, SAVED);
		return 1;
	}
	return -ENOSTR;
}

int Pattern::deletePat(int ind)
{
	if ((!writeAble) || (ind >= patternLimit))
		return -ENOSTR;
	QString filename = QString("%1%2%3").arg(patternFile).arg(ind).arg(FILETAIL);
	QFile patternFile;
	patternFile.setFileName(filename);
	if (patternFile.open(QIODevice::ReadWrite)){
		patternStates.replace(ind, NO_RUN_RECORD);
		patternFile.resize(0);
		patternFile.close();
	}
	return 1;
}

int Pattern::run(int ind)
{
	if ((!writeAble) || (ind >= patternLimit))
		return -ENOSTR;
	mDebug("pattern run %d", ind) ;
	for (int i = 0; i < patternLimit; i++)
		if (patternStates.at(i) == RUN) {
			return -ENODATA;
		}
	QString filename = QString("%1%2%3").arg(patternFile).arg(ind).arg(FILETAIL);
	QFile *runPatternFile = new QFile(filename);
	if (runPatternFile->open(QIODevice::ReadWrite)) {
		if (runPatternFile->size()) {
			QDataStream patternStream(runPatternFile);
			patternStream.setByteOrder(QDataStream::LittleEndian);
			cmdAndTime cmdKey;
			patternStream >> cmdKey.cmd;
			patternStream >> cmdKey.time;
			if (cmdKey.cmd == KEYWORDS) {
				patternStates.replace(ind, RUN);
				while (!patternStream.atEnd()) {
					patternStream >> cmdKey.cmd;
					patternStream >> cmdKey.time;
					patternCmds.append(cmdKey);
				}
				if (patternCmds.size()) {
					emit nextCmd(patternCmds.at(0).cmd.constData(), patternCmds.at(0).cmd.size());
					patternCmds.removeFirst();
				}
				if (patternCmds.size()) {
					mDebug("pattern next command time %lld", patternCmds.at(0).time);
					QTimer::singleShot(patternCmds.at(0).time, this, SLOT(timeout()));
				}
				return 1;
			} else {
				mDebug("pattern empty or wrong data");
				runPatternFile->resize(0);
				runPatternFile->close();
				return -ENODATA;
			}
		}
	}
	mDebug("pattern not found");
	return -ENODATA;
}

int Pattern::patternInit()
{
	if ((!writeAble))
		return -ENOSTR;
	mDebug("pattern init");
	for (int i = 0; i < patternLimit; i++) {
		QString filename = QString("%1%2%3").arg(patternFile).arg(i).arg(FILETAIL);
		QFile patternFile;
		patternFile.setFileName(filename);
		if (patternFile.open(QIODevice::ReadWrite)) {
			QDataStream patternStream(&patternFile);
			patternStream.setByteOrder(QDataStream::LittleEndian);
			struct cmdAndTime key;
			patternStream >> key.cmd;
			if (patternFile.size() && (key.cmd == KEYWORDS)) {
				patternStates.replace(i, SAVED);
			} else {
				patternStates.replace(i, NO_RUN_RECORD);
				patternFile.resize(0);
			}
			patternFile.close();
		}
	}
	patternT.start();
	return 1;
}

int Pattern::record(const char *command, int len)
{
	if ((!writeAble))
		return -ENOSTR;
	for (int i = 0; i < patternLimit; i++) {
		if (patternStates.at(i) == RECORD) {
			struct cmdAndTime rec;
			rec.cmd = QByteArray(command, len);
			rec.time = patternT.elapsed();
			if (patternCmds.size() < 2)
				rec.time = 1;
			patternCmds.append(rec);
			patternT.restart();
			return 1;
		}
	}
	patternT.restart();
	return -ENODATA;
}

int Pattern::cancelPat(int ind)
{
	if ((!writeAble) || (ind >= patternLimit))
		return -ENOSTR;
	mDebug("pattern cancel");
	patternStates.replace(ind, SAVED);
	patternCmds.clear();
	return 1;
}

void Pattern::timeout()
{
	if ((!writeAble))
		return;
	for (int i = 0; i < patternLimit; i++) {
		if (patternStates.at(i) == RUN) {
			mDebug("pattern %d", i);
			if (patternCmds.size()) {
				emit nextCmd(patternCmds.at(0).cmd.constData(), patternCmds.at(0).cmd.size());
				patternCmds.removeFirst();
			}
			if (patternCmds.size()) {
				mDebug("pattern next command time %lld", patternCmds.at(0).time);
				QTimer::singleShot(patternCmds.at(0).time, this, SLOT(timeout()));
				return;
			} else {
				patternStates.replace(i, SAVED);
				patternCmds.clear();
				run(i);
				return;
			}
		}
	}
	mDebug("pattern fin");
	patternCmds.clear();
}
