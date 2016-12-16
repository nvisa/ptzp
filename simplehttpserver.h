#ifndef SIMPLEHTTPSERVER_H
#define SIMPLEHTTPSERVER_H

#include <QUrl>
#include <QHash>
#include <QObject>

class QTcpServer;
class QTcpSocket;

class SimpleHttpServer : public QObject
{
	Q_OBJECT
public:
	explicit SimpleHttpServer(int port, QObject *parent = 0);
protected:
	virtual const QByteArray getFile(const QString filename, QString &mime, QUrl &url);
	virtual int handlePostData(const QByteArray &);
	virtual void sendGetResponse(QTcpSocket *sock);

private slots:
	void newConnection();
	void readMessage();
protected:
	bool isAuthenticated();
	bool isAuthenticated(const QString &username);
	virtual QStringList addCustomGetHeaders(const QString &filename);

	QTcpServer *server;
	enum ServerState {
		IDLE,
		GET,
		POST_HEADER,
		POST,
	};
	ServerState state;
	QHash<QString, QString> mimeTypesByExtension;
	QHash<QString, QString> postHeaders;
	QHash<QString, QString> getHeaders;
	QByteArray postData;
	bool useAuthentication;
	QString authenticatedUserName;
	bool useCustomAuth;
	QUrl getUrl;
	QString authenticatedUserToken;
private:
	bool parsePostData(QTcpSocket *sock);
	bool shouldRead(QTcpSocket *sock);
	virtual int handlePostDataAuth(const QByteArray &ba);
	virtual const QByteArray getFileAuth(const QString filename, QString &mime, QUrl &url);
};

#endif // SIMPLEHTTPSERVER_H
