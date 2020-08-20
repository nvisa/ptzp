#ifndef GOZOKBDRIVER_H
#define GOZOKBDRIVER_H

#include "ptzpdriver.h"

class PtzpTcpTransport;
class OkbSrpPTHead;

class GozOKBDriver : public PtzpDriver
{
	Q_OBJECT

public:
	GozOKBDriver();

	PtzpHead *getHead(int index);
	int setTarget(const QString &targetUri);

protected slots:
	void timeout();

protected:

	OkbSrpPTHead *head;

	PtzpTransport *tp;
};

#endif // GOZOKBDRIVER_H
