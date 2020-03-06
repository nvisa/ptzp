#ifndef SMTPCLIENT_H
#define SMTPCLIENT_H

#include <QObject>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QTcpSocket>

#include "mimemessage.h"
#include "smtpexports.h"

class SMTP_EXPORT SmtpClient : public QObject
{
	Q_OBJECT
public:
	enum AuthMethod {
		AUTHPLAIN,
		AUTHLOGIN
	};

	enum SmtpError {
		CONNECTIONTIMEOUTERROR,
		RESPONSETIMEOUTERROR,
		SENDDATATIMEOUTERROR,
		AUTHENTICATIONFAILEDERROR,
		SERVERERROR,    // 4xx smtp error
		CLIENTERROR     // 5xx smtp error
	};

	enum ConnectionType {
		TCPCONNECTION,

#ifndef QT_NO_OPENSSL
		SSLCONNECTION,
		TSLCONNECTION       // STARTTLS
#endif
	};

	SmtpClient(const QString & host = "localhost", int port = 25, ConnectionType ct = TCPCONNECTION);

	~SmtpClient();
	const QString& getHost() const;
	void setHost(const QString &host);

	int getPort() const;
	void setPort(int port);

	const QString& getName() const;
	void setName(const QString &name);

	ConnectionType getConnectionType() const;
	void setConnectionType(ConnectionType ct);

	const QString & getUser() const;
	void setUser(const QString &user);

	const QString & getPassword() const;
	void setPassword(const QString &password);

	SmtpClient::AuthMethod getAuthMethod() const;
	void setAuthMethod(AuthMethod method);

	const QString & getResponseText() const;
	int getResponseCode() const;

	int getConnectionTimeout() const;
	void setConnectionTimeout(int msec);

	int getResponseTimeout() const;
	void setResponseTimeout(int msec);

	int getSendMessageTimeout() const;
	void setSendMessageTimeout(int msec);

	QTcpSocket* getSocket();

	bool connectToHost();

	bool login();
	bool login(const QString &user, const QString &password, AuthMethod method = AUTHLOGIN);

	bool sendMail(MimeMessage& email);

	void quit();

protected:
	QTcpSocket *socket;

	QString host;
	int port;
	ConnectionType connectionType;
	QString name;

	QString user;
	QString password;
	AuthMethod authMethod;

	int connectionTimeout;
	int responseTimeout;
	int sendMessageTimeout;

	QString responseText;
	int responseCode;

	class ResponseTimeoutException {};
	class SendMessageTimeoutException {};

	void waitForResponse();

	void sendMessage(const QString &text);

protected slots:
	void socketStateChanged(QAbstractSocket::SocketState state);
	void socketError(QAbstractSocket::SocketError error);
	void socketReadyRead();

signals:
	void smtpError(SmtpClient::SmtpError e);
};

#endif // SMTPCLIENT_H
