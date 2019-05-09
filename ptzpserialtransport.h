#ifndef PTZPSERIALTRANSPORT_H
#define PTZPSERIALTRANSPORT_H

#include <ecl/ptzp/ptzptransport.h>

class ExarConfig;
class ReadThread;
class WriteThread;
class QextSerialPort;

class PtzpSerialTransport : public PtzpTransport
{
public:
	PtzpSerialTransport();
	PtzpSerialTransport(LineProtocol proto);

	int connectTo(const QString &targetUri);
	int send(const char *bytes, int len);

protected:
	ExarConfig *exar;
	QextSerialPort *port;
	ReadThread *readThread;
	WriteThread *writeThread;
};

#endif // PTZPSERIALTRANSPORT_H
