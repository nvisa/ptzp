#ifndef SIMPLEHTTPSERVER_H
#define SIMPLEHTTPSERVER_H

#include <QUrl>
#include <QHash>
#include <QFile>
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
	virtual int handlePostDataFile(const QString &);
	virtual void sendGetResponse(QTcpSocket *sock);

private slots:
	void newConnection();
	void readMessage();
	void deleteSocketVariables();
protected:
	bool isAuthenticated();
	bool isAuthenticated(const QString &username);
	QString getTemporaryDir() { return temporaryDir;}
	void setTemporaryDir(const QString &tempDir) { temporaryDir = tempDir;}
	virtual QStringList addCustomGetHeaders(const QString &filename);

	QTcpServer *server;
	enum ServerState {
		IDLE,
		GET,
		POST_HEADER,
		POST,
	};

	struct SocketVariables {
		ServerState state;
		QHash<QString, QString> postHeaders;
		QHash<QString, QString> getHeaders;
		QByteArray postData;
		QFile *postDataFile;
		QUrl getUrl;
	};
	QHash<QTcpSocket *, SocketVariables> socketVar;

	ServerState state;
	QHash<QString, QString> mimeTypesByExtension;
	QHash<QString, QString> postHeaders;
	QHash<QString, QString> getHeaders;
	bool useAuthentication;
	QString authenticatedUserName;
	bool useCustomAuth;
	QUrl getUrl;
	QString authenticatedUserToken;
	void getSocketVar(QTcpSocket *sock);
	void setSocketVar(QTcpSocket *sock);
private:
	QString temporaryDir;
	bool parsePostData(QTcpSocket *sock);
	bool shouldRead(QTcpSocket *sock);
	virtual int handlePostDataAuth(const QByteArray &ba);
	virtual int handlePostDataFileAuth(const QString &ba);
	virtual const QByteArray getFileAuth(const QString filename, QString &mime, QUrl &url);
};

#endif // SIMPLEHTTPSERVER_H
