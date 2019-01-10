#ifndef PTZPTCPTRANSPORT_H
#define PTZPTCPTRANSPORT_H

#include <ecl/ptzp/ptzptransport.h>

#include <QTimer>
#include <QMutex>
#include <QObject>

class QTcpSocket;
class PtzpTcpTransport : public QObject, public PtzpTransport
{
	Q_OBJECT
public:
	explicit PtzpTcpTransport(LineProtocol proto, QObject *parent = 0);

	class TransportFilterInteface
	{
	public:
		virtual QByteArray sendFilter(const char *bytes, int len) = 0;
		virtual int readFilter(QTcpSocket *sock, QByteArray &ba) = 0;
	};

	int connectTo(const QString &targetUri);
	int send(const char *bytes, int len);

	void setFilter(TransportFilterInteface *iface);

protected slots:
	void connected();
	void dataReady();
	void clientDisconnected();
	void callback();

protected:
	QTcpSocket *sock;
	QTimer *timer;
	TransportFilterInteface *filterInterface;
};

#endif // PTZPTCPTRANSPORT_H
