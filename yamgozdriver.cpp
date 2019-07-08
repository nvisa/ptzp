#include "yamgozdriver.h"
#include "mgeoyamgozhead.h"
#include "ptzptcptransport.h"
#include "debug.h"

#include <QFile>

YamGozDriver::YamGozDriver(QObject *parent)
	: PtzpDriver(parent)
{
	head0 = new MgeoYamGozHead();
	head1 = new MgeoYamGozHead();
	head2 = new MgeoYamGozHead();
	tp0 = new PtzpTcpTransport(PtzpTransport::PROTO_BUFFERED);
	tp1 = new PtzpTcpTransport(PtzpTransport::PROTO_BUFFERED);
	tp2 = new PtzpTcpTransport(PtzpTransport::PROTO_BUFFERED);
	state = INIT;
}

PtzpHead *YamGozDriver::getHead(int index)
{
	if (index == 0)
		return head0;
	else if (index == 1)
		return head1;
	else if (index == 2)
		return head2;
	return NULL;
}

int YamGozDriver::setTarget(const QString &targetUri)
{
	QStringList fields = targetUri.split(";");
	head0->setTransport(tp0);
	if (tp0->connectTo(fields[0]))
		return -EPERM;

	head1->setTransport(tp1);
	if (tp1->connectTo(fields[1]))
		return -EPERM;
	defaultModuleHead = head1;

	head2->setTransport(tp2);
	if (tp2->connectTo(fields[2]))
		return -EPERM;
	return 0;
}

void YamGozDriver::timeout()
{
	switch (state) {
	case INIT:
		head0->setProperty(4, 0);
		head1->setProperty(4, 0);
		head2->setProperty(4, 0);
		state = NORMAL;
		break;
	case NORMAL:
		break;
	}

	PtzpDriver::timeout();
}

