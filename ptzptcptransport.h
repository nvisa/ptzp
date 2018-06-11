#ifndef PTZPTCPTRANSPORT_H
#define PTZPTCPTRANSPORT_H

#include <ecl/ptzp/ptzptransport.h>

#include <QObject>

class QTcpSocket;

class PtzpTcpTransport : public QObject, public PtzpTransport
{
	Q_OBJECT
public:
	explicit PtzpTcpTransport(QObject *parent = 0);

	int connectTo(const QString &targetUri);

protected slots:
	void connected();
	void dataReady();
	void clientDisconnected();
protected:
	QTcpSocket *sock;
};

#endif // PTZPTCPTRANSPORT_H
