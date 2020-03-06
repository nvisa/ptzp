#ifndef MOXACONTROLHEAD_H
#define MOXACONTROLHEAD_H

#include <ptzphead.h>

class MoxaControlHead : public PtzpHead
{
public:
	MoxaControlHead();
	int setZoomShow(int value);

	void fillCapabilities(ptzp::PtzHead *head);
	int getHeadStatus();
protected:
	int sendCommand(const QString &key);
	int dataReady(const unsigned char *bytes, int len);
	QByteArray transportReady();
private:
	QStringList moxaCommandList;
};

#endif // MOXACONTROLHEAD_H
