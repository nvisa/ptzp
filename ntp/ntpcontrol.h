#ifndef NTPCONTROL_H
#define NTPCONTROL_H

extern"C" {
	#include <ecl/net/ntp/ntpclient.h>
}

#include <QString>

class NtpControl
{
public:
    NtpControl();
	int start();
	void setLoop(bool mode);
	void setCycleTime(int secs);
	void setServer(QString server);
	void setCrossCheck(QString mode);
	void setClock(QString mode);
	void setManuel(QString isoTime);
protected:
	int setupNtp();
	void initializeNtpControl();
private:
	struct ntp_control ntpc;
};

#endif // NTPCONTROL_H
