#ifndef PATTERNRECORDER_H
#define PATTERNRECORDER_H

#include <QFile>
#include <QObject>
#include <QElapsedTimer>

#include <ecl/debug.h>

#define DEF_PATT_LIM 8
#define DEF_PATT_FILENAME "pattern_"
#define FILETAIL ".bin"
#define KEYWORDS "Pattern File\n"

class Pattern : public QObject
{
	Q_OBJECT
public:

	explicit Pattern(int writeState, QString pattFilename = DEF_PATT_FILENAME, int pattLimit = DEF_PATT_LIM, QObject *parent = 0);
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
	struct cmdAndTime {
		qint64 time;
		QByteArray cmd;
	};

	QString patternFile;
	int patternLimit;
	int writeAble;
	QElapsedTimer patternT;
	QList<patternState> patternStates; //0: NO_RUN_RECORD , 1: RECORD, 2: RUN, 3 :SAVED
	QList<struct cmdAndTime> patternCmds;
};

#endif // PATTERNRECORDER_H
