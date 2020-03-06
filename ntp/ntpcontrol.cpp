#include "ntpcontrol.h"

#include <unistd.h>
#include <debug.h>
#include <QProcess>

NtpControl::NtpControl()
{
	memset(&ntpc, 0, sizeof(ntpc));
	initializeNtpControl();
}

int NtpControl::start()
{
	int usd = setupNtp();
	if (usd != -1)
		primary_loop(usd, &ntpc);
	return usd;
}

int NtpControl::setupNtp()
{
	int usd = setup_socket(&ntpc);
	if (usd == -1)
		ffDebug() << "Socket setup failed";
	setup_signals();
	return usd;
}

void NtpControl::setLoop(bool mode)
{
	if (mode)
		ntpc.probe_count = 0;
	else
		ntpc.probe_count = 1;
}

void NtpControl::setManuel(QString isoTime)
{
	QDateTime dt = QDateTime::fromString(isoTime, "yyyy-MM-ddThh:mm:ss");
	if (!dt.isValid())
		dt = QDateTime::fromString(isoTime, "yyyy-MM-ddThh:mm:ss.zzzZ");
	if (!dt.isValid()) {
		ffDebug() << "Time format is invalid.. You can check `--help` argument..";
		return;
	}
	QProcess::execute(QString("date -u -s %1").arg(dt.toString("MMddhhmmyyyy.ss")));
}

/**
 * @brief NtpControl::setCycleTime
 * Default value is 600, if you use it on loop mode every X secs
 * do check NTP time.
 * @param secs
 */
void NtpControl::setCycleTime(int secs)
{
	if (secs > 0)
		ntpc.cycle_time = secs;
}

void NtpControl::setServer(QString server)
{
	if (!server.isEmpty()) {
		QByteArray ba = server.toLocal8Bit();
		char *data = new char[ba.size() + 1];
		strcpy(data, ba.data());
		ntpc.server = data;
	} else ntpc.server = (char*)"pool.ntp.org";
}

/**
 * @brief NtpControl::setCrossCheck
 * Trust cross server, it must be true, or cross_check value is 1
 * @param mode
 */
void NtpControl::setCrossCheck(QString mode)
{
    if (mode == "true")
		ntpc.cross_check = 0;
	else ntpc.cross_check = 1;
}

/**
 * @brief NtpControl::setClock
 * @param mode: it must be true.
 */
void NtpControl::setClock(QString mode)
{
    if (mode == "true")
		ntpc.set_clock++;
	else ntpc.set_clock = 0;
}

/**
 * @brief NtpControl::initializeNtpControl
 * Standart initializing, one shot procedure checking to server.
 */
void NtpControl::initializeNtpControl()
{
	ntpc.probe_count = 1; // for loop if value is 0, means loop forever
	ntpc.cycle_time = 600; // seconds
	ntpc.goodness = 0;
	ntpc.set_clock = 0;

	if (geteuid() == 0) {
		log_enable++;
		ntpc.usermode    = 0;
		ntpc.live        = 1;
		ntpc.cross_check = 0;
	} else {
		ntpc.usermode    = 1;
		ntpc.live        = 0;
		ntpc.cross_check = 1;
	}
	ntpc.set_clock++; // -s
	ntpc.cross_check = 0; // -t

	if (ntpc.server == NULL)
		ntpc.server = "pool.ntp.org";
	if (ntpc.probe_count > 0)
		ntpc.live = 0;
}

