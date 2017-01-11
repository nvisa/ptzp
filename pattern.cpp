#include "pattern.h"

#include <QTimer>
#include <QDataStream>

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

int Pattern::start(int ind)
{
	if ((!writeAble) || (ind >= patternLimit))
		return -ENOSTR;
	mDebug("pattern start");
	if (patternStates.at(ind) != NO_RUN_RECORD)
		return -ENODATA;
	patternStates.replace(ind, RECORD);
	patternCmds.clear();
	cmdAndTime key ((qint64)ind, QByteArray(KEYWORDS));
	patternCmds.append(key);
	patternT.restart();
	return 1;
}

int Pattern::stop(int ind)
{
	if ((!writeAble) || (ind >= patternLimit))
		return -ENOSTR;
	mDebug("pattern stop");
	QString filename = QString("%1%2%3").arg(patternFile).arg(ind).arg(FILETAIL);
	QFile runPatternFile(filename);
	if (!runPatternFile.open(QIODevice::ReadWrite)) {
		patternStates.replace(ind, NO_RUN_RECORD);
		return -ENOSTR;
	}
	QDataStream patternStream(&runPatternFile);
	patternStream.setByteOrder(QDataStream::LittleEndian);
	for (int i = 0; i < patternCmds.size(); i++) {
		patternStream << patternCmds.at(i).cmd;
		patternStream << patternCmds.at(i).time;
	}
	runPatternFile.close();
	patternCmds.clear();
	patternStates.replace(ind, SAVED);
	return 1;
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

int Pattern::readPatternFile(int ind)
{
	QString filename = QString("%1%2%3").arg(patternFile).arg(ind).arg(FILETAIL);
	QFile runPatternFile(filename);
	if (!runPatternFile.open(QIODevice::ReadWrite)) {
		mDebug("pattern not open");
		return -ENODATA;
	}
	if (!runPatternFile.size()) {
		mDebug("pattern empty or wrong data");
		runPatternFile.resize(0);
		runPatternFile.close();
		return -ENODATA;
	}

	QDataStream patternStream(&runPatternFile);
	patternStream.setByteOrder(QDataStream::LittleEndian);
	cmdAndTime cmdKey;
	patternStream >> cmdKey.cmd;
	patternStream >> cmdKey.time;

	if (cmdKey.cmd != KEYWORDS) {
		mDebug("pattern empty or wrong data");
		runPatternFile.resize(0);
		runPatternFile.close();
		return -ENODATA;
	}

	while (!patternStream.atEnd()) {
		patternStream >> cmdKey.cmd;
		patternStream >> cmdKey.time;
		patternCmds.append(cmdKey);
	}
	return 0;
}

int Pattern::run(int ind)
{
	if ((!writeAble) || (ind >= patternLimit))
		return -ENOSTR;
	for (int i = 0; i < patternLimit; i++)
		if (patternStates.at(i) == RUN)
			return -ENODATA;
	mDebug("pattern run %d", ind);

	readPatternFile(ind);
	if (!patternCmds.size())
		return -ENODATA;

	patternStates.replace(ind, RUN);
	cmdIndex = 0;
	timeout();
	return 1;
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
		if (!patternFile.open(QIODevice::ReadWrite))
			break;
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
	return 1;
}

int Pattern::record(const char *command, int len)
{
	if ((!writeAble))
		return -ENOSTR;
	for (int i = 0; i < patternLimit; i++) {
		if (patternStates.at(i) == RECORD) {
			qint64 time = patternT.elapsed();
			if (patternCmds.size() < 2)
				time = 1;
			patternCmds.append(cmdAndTime(time ,QByteArray(command, len)));
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
	int i = 0;
	for (; i < patternLimit; i++) {
		if (patternStates.at(i) == RUN)
			break;
	}
	if (i >= patternLimit)
		return;

	const cmdAndTime *t = &patternCmds.at(cmdIndex);
	emit nextCmd(t->cmd.constData(), t->cmd.size());

	if (cmdIndex < patternCmds.size() - 1)
		cmdIndex++;
	else
		cmdIndex = 0;

	t = &patternCmds.at(cmdIndex);

	mDebug("%d pattern next command time %lld",i , t->time);

	QTimer::singleShot(t->time, this, SLOT(timeout()));
}
