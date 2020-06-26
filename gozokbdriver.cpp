#include "gozokbdriver.h"

#include "debug.h"
#include "errno.h"
#include "okbsrppthead.h"
#include "ptzptcptransport.h"

GozOKBDriver::GozOKBDriver()
{
	head = new OkbSrpPTHead;
	defaultPTHead = head;
}

PtzpHead *GozOKBDriver::getHead(int index)
{
	return head;
}

int GozOKBDriver::setTarget(const QString &targetUri)
{
	tp = new PtzpTcpTransport(PtzpTransport::PROTO_BUFFERED);
	head->setTransport(tp);
	if (tp->connectTo(targetUri))
		return -EPERM;
	return 0;
}

void GozOKBDriver::timeout()
{
	PtzpDriver::timeout();
}
