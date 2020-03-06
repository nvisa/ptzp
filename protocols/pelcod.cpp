#include "pelcod.h"

QByteArray PelcoD::get_pelcod(int addr, int cmd, int speed, int arg)
{
	char pelcod_buf[7];
	char *buf = pelcod_buf;
	buf[0] = 0xff;
	buf[1] = addr;
	buf[2] = 0x00;
	buf[3] = 0x00;
	buf[4] = speed;
	buf[5] = speed;
	buf[6] = 0;

	if (cmd == PELCOD_CMD_PANR)
		buf[3] = 0x02;
	else if (cmd == PELCOD_CMD_PANL)
		buf[3] = 0x04;
	else if (cmd == PELCOD_CMD_TILTU)
		buf[3] = 0x08;
	else if (cmd == PELCOD_CMD_TILTD)
		buf[3] = 0x10;
	else if (cmd == PELCOD_CMD_SET_PRESET) {
		buf[3] = 0x03;
		buf[4] = 0x00;
		buf[5] = arg;
	} else if (cmd == PELCOD_CMD_CLEAR_PRESET) {
		buf[3] = 0x05;
		buf[4] = 0x00;
		buf[5] = arg;
	} else if (cmd == PELCOD_CMD_GOTO_PRESET) {
		buf[3] = 0x07;
		buf[4] = 0x00;
		buf[5] = arg;
	} else if (cmd == PELCOD_CMD_FLIP180) {
		buf[3] = 0x07;
		buf[4] = 0x00;
		buf[5] = 21;
	} else if (cmd == PELCOD_CMD_ZERO_PAN) {
		buf[3] = 0x07;
		buf[4] = 0x00;
		buf[5] = 22;
	} else if (cmd == PELCOD_CMD_RESET) {
		buf[3] = 0x0F;
		buf[4] = 0x00;
		buf[5] = 0x00;
	} else
		buf[3] = 0;


	/* calculate checksum */
	int sum = 0;
	for (int i = 1; i < 6; i++)
		sum += buf[i];
	buf[6] = sum;

	return QByteArray(buf, 7);
}

PelcoD::PelcoD(QObject *parent) :
	QObject(parent)
{
}

QByteArray PelcoD::getCommand(int addr, pelco_d_commands cmd, int speed, int arg)
{
	return get_pelcod(addr, cmd, speed, arg);
}
