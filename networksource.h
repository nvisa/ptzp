#ifndef NETWORKSOURCE_H
#define NETWORKSOURCE_H

#include <QHash>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QAbstractSocket>

class QWebSocket;

struct ChartData
{
	QList<qreal> samples;
	QList<qint64> ts;
	int channel;
	QHash<QString, QVariant> meta;
};

class NetworkSource : public QObject
{
	Q_OBJECT
public:
	explicit NetworkSource(QObject *parent = nullptr);

	void setThreadSupport(bool en);
	bool listen();
	bool connectServer(const QString &url, bool autoConnect = true);

	int sendMessage(const QByteArray &ba);
	/* chart related messages */
	int sendMessage(const ChartData &data);
	int sendMessage(qreal sample);

signals:
	void sendData2Main(const QByteArray &ba);
	void newChartData(const ChartData &data);

public slots:
	void onNewConnection();
	void socketDisconnected();
	void processBinaryMessage(QByteArray message);
	void clientProcessBinaryMessage(QByteArray message);
	void processTextMessage(QString message);
	void clientConnected();
	void clientClosed();
	void clientSocketError(QAbstractSocket::SocketError err);

protected slots:
	void sendThreadedData(const QByteArray &ba);

protected:
	void reconnectClient();

	bool threaded;
	bool autoReconnect;
	bool readyToSend;
	QString serverUrl;
	QWebSocket *clientSocket;
	QList<QWebSocket *> clients;
};

#endif // NETWORKSOURCE_H
