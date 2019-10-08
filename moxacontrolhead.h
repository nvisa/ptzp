#ifndef MOXACONTROLHEAD_H
#define MOXACONTROLHEAD_H

#include <ecl/ptzp/ptzphead.h>

class MoxaControlHead : public PtzpHead
{
public:
	MoxaControlHead();
	int setZoomShow(int value);

	int getCapabilities();
	int getHeadStatus();
protected:
	int sendCommand(const QString &key);
	int dataReady(const unsigned char *bytes, int len);
	QByteArray transportReady();
private:
	QStringList moxaCommandList;
};

#endif // MOXACONTROLHEAD_H
