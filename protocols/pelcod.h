#ifndef PELCOD_H
#define PELCOD_H

#include <QObject>

class PelcoD : public QObject
{
	Q_OBJECT
public:
	explicit PelcoD(QObject *parent = 0);

	enum pelco_d_commands {
		PELCOD_CMD_STOP,
		PELCOD_CMD_PANR,
		PELCOD_CMD_PANL,
		PELCOD_CMD_TILTU,
		PELCOD_CMD_TILTD,
		PELCOD_CMD_SET_PRESET,
		PELCOD_CMD_CLEAR_PRESET,
		PELCOD_CMD_GOTO_PRESET,
		PELCOD_CMD_FLIP180,
		PELCOD_CMD_ZERO_PAN,
		PELCOD_CMD_RESET,
	};

	static QByteArray getCommand(int addr, pelco_d_commands cmd, int speed = 0xff, int arg = 0);
signals:

public slots:
protected:
	static QByteArray get_pelcod(int addr, int cmd, int speed, int arg);
};

#endif // PELCOD_H
