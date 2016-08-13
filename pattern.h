#ifndef PATTERNRECORDER_H
#define PATTERNRECORDER_H

#include <QObject>
#include <QFile>
#include "QElapsedTimer"
#include "debug.h"

#define PATT_LIM 8

class Pattern : public QObject
{
	Q_OBJECT
public:

	explicit Pattern(int writeState, QObject *parent = 0);
	~Pattern();

private slots:
	void timeout();

signals:
	void nextCmd(const char *,int);

protected:
	int start(int ind);
	int stop(int ind);
	int deletePat(int ind);
	int run(int ind);
	int cancelPat(int ind);

	int patternInit();

	int record(const char *command, int len);

	enum patternState {
		NO_RUN_RECORD,
		RECORD,
		RUN,
		SAVED
	};

	int writeAble;
	QElapsedTimer patternT;
	QFile *runPatternFile;
	int patternStates[PATT_LIM]; //0: NO_RUN_RECORD , 1: RECORD, 2: RUN, 3 :SAVED
	qint64 patternTime[PATT_LIM];
	QByteArray cmd;
};

#endif // PATTERNRECORDER_H
