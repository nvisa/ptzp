#include "pattern.h"
#include "QTimer"
#include "errno.h"

#define FILENAME "pattern_"
#define FILETAIL ".bin"

#define MAXCOMMANDLENGHT 9

Pattern::Pattern(int writeState, QObject *parent) :
	QObject(parent)
{
	writeAble = writeState;
}

Pattern::~Pattern()
{

}

int Pattern::start(int ind)
{
	if ((!writeAble) || (ind >= PATT_LIM))
		return -ENOSTR;
	mDebug("pattern start");
	if (patternStates[ind] == NO_RUN_RECORD) {
		QString filename = FILENAME + QString::number(ind) + FILETAIL;
		QFile patternFile;
		patternFile.setFileName(filename);
		if (patternFile.open(QFile::ReadWrite))
			if (patternFile.size() == 0) {
				patternStates[ind] = RECORD;
				patternFile.close();
				return 1;
			}
	}
	return -ENODATA;
}

int Pattern::stop(int ind)
{
	if ((!writeAble) || (ind >= PATT_LIM))
		return -ENOSTR;
	mDebug("pattern stop");
	patternStates[ind] = SAVED;
	patternTime[ind] = 0;
	return 1;
}

int Pattern::deletePat(int ind)
{
	if ((!writeAble) || (ind >= PATT_LIM))
		return -ENOSTR;
	QString filename = FILENAME + QString::number(ind) + FILETAIL;
	QFile patternFile;
	patternFile.setFileName(filename);
	if (patternFile.open(QFile::ReadWrite)){
		patternStates[ind] = NO_RUN_RECORD;
		patternFile.resize(0);
		patternFile.close();
	}
	return 1;
}

int Pattern::run(int ind)
{
	if ((!writeAble) || (ind >= PATT_LIM))
		return -ENOSTR;
	mDebug("pattern run") ;
	for (int i = 0; i < PATT_LIM; i++)
		if (patternStates[i] == RUN && (i != ind))
			return -ENODATA;

	QString filename = FILENAME + QString::number(ind) + FILETAIL;
	runPatternFile = new QFile(filename);
	if (runPatternFile->open(QFile::ReadWrite)) {
		if ((runPatternFile->size() % (MAXCOMMANDLENGHT + 7)) == 0 && runPatternFile->size()) {
			patternStates[ind] = RUN;
			QByteArray ba = runPatternFile->read(MAXCOMMANDLENGHT + 7);
			const char *baData = ba.constData();
			int time = baData[MAXCOMMANDLENGHT + 1] + (baData[MAXCOMMANDLENGHT + 2] << 8) +
					(baData[MAXCOMMANDLENGHT + 3] << 16)  + (baData[MAXCOMMANDLENGHT + 4] << 24);
			cmd.clear();
			cmd.append(baData, baData[MAXCOMMANDLENGHT]);
			mDebug("pattern next command time %d", time);
			QTimer::singleShot(time, this, SLOT(timeout()));
			return 1;
		} else {
			mDebug("pattern empty or wrong data");
			runPatternFile->resize(0);
			runPatternFile->close();
			return -ENODATA;
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
	for (int i = 0; i < PATT_LIM; i++) {
		QString filename = FILENAME + QString::number(i) + FILETAIL;
		QFile patternFile;
		patternFile.setFileName(filename);
		if (patternFile.open(QFile::ReadWrite)) {
			if ((patternFile.size() % (MAXCOMMANDLENGHT + 7)) == 0 && patternFile.size())
				patternStates[i] = SAVED;
			else {
				patternStates[i] = NO_RUN_RECORD;
				patternFile.resize(0);
			}
			patternFile.close();
			patternTime[i] = 0;
		}
	}
	patternT.start();
	return 1;
}

int Pattern::record(const char *command, int len)
{
	if ((!writeAble))
		return -ENOSTR;
	for (int i = 0; i < PATT_LIM; i++) {
		if (patternStates[i] == RECORD) {
			QString filename = FILENAME + QString::number(i) + FILETAIL;
			QFile patternFile;
			patternFile.setFileName(filename);
			if (patternFile.open(QFile::ReadWrite)) {
				mDebug("pattern rec");
				patternFile.seek(patternFile.size());
				QByteArray recCmd;
				recCmd.append(QByteArray(command, len));
				recCmd.resize(MAXCOMMANDLENGHT);
				recCmd.append((char)len);
				int elapseTime = patternT.elapsed() - patternTime[i];
				patternTime[i] = 0;
				if (patternFile.size() == 0) {
					elapseTime = 1;
					patternTime[i] = patternT.elapsed();
				}
				for (uint k = 0; k < sizeof(elapseTime); k++)
					recCmd.append((char) (((elapseTime & (0xFF << (k*8))) >> (k*8))));
				recCmd.append("\n\r");
				patternFile.write(recCmd, MAXCOMMANDLENGHT + 7);
				patternFile.close();
				patternT.restart();
				return 1;
			}
		}
	}
	patternT.restart();
	return -ENODATA;
}

int Pattern::cancelPat(int ind)
{
	if ((!writeAble) || (ind >= PATT_LIM))
		return -ENOSTR;
	mDebug("pattern cancel");
	patternStates[ind] = SAVED;
	return 1;
}

void Pattern::timeout()
{
	if ((!writeAble))
		return;
	for (int i = 0; i < PATT_LIM; i++) {
		if (patternStates[i] == RUN) {
			mDebug("pattern %d", i);
			emit nextCmd(cmd.constData(), cmd.size());
			if (!runPatternFile->atEnd()) {
				QByteArray ba = runPatternFile->read(MAXCOMMANDLENGHT + 7);
				const char *baData = ba.constData();
				int time = baData[MAXCOMMANDLENGHT + 1] + (baData[MAXCOMMANDLENGHT + 2] << 8) +
						(baData[MAXCOMMANDLENGHT + 3] << 16)  + (baData[MAXCOMMANDLENGHT + 4] << 24);
				mDebug("pattern next command time %d", time);
				cmd.clear();
				cmd.append(baData, baData[MAXCOMMANDLENGHT]);
				QTimer::singleShot(time, this, SLOT(timeout()));
				return;
			} else {
				run(i);
				return;
			}
		}
	}
	mDebug("pattern fin");
	runPatternFile->close();
}
