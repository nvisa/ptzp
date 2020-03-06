#ifndef PTZPTCPTRANSPORT_H
#define PTZPTCPTRANSPORT_H

#include <ptzptransport.h>

#include <QMutex>
#include <QObject>
#include <QTimer>

class QAbstractSocket;

class PtzpTcpTransport : public QObject, public PtzpTransport
{
	Q_OBJECT
public:
	explicit PtzpTcpTransport(LineProtocol proto, QObject *parent = 0);

	enum DevStatus {
		ALIVE,
		DEAD
	};

	class TransportFilterInteface
	{
	public:
		virtual QByteArray sendFilter(const char *bytes, int len) = 0;
		virtual int readFilter(QAbstractSocket *sock, QByteArray &ba) = 0;
	};

	int connectTo(const QString &targetUri);
	int disconnectFrom();
	int send(const char *bytes, int len);
	void setFilter(TransportFilterInteface *iface);
	PtzpTcpTransport::DevStatus getStatus();
	void reConnect();
	void setAutoReconnect(bool flag);
signals:
	void sendSocketMessage2Main(const QByteArray &ba);

protected slots:
	void connected();
	void dataReady();
	void clientDisconnected();
	void callback();
	void sendSocketMessage(const QByteArray &ba);

	void timeout();
protected:
	QAbstractSocket *sock;
	QTimer *timer;
	TransportFilterInteface *filterInterface;
	/* udp connection flag*/
	bool isUdp;
	int sendDstPort;
	DevStatus devStatus;
	QString serverUrl;
	int serverPort;
	bool autoReconnect;
};

#endif // PTZPTCPTRANSPORT_H
