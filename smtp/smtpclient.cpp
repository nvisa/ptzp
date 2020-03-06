#include "smtpclient.h"
#include <QFileInfo>
#include <QByteArray>

SmtpClient::SmtpClient(const QString & host, int port, ConnectionType connectionType) :
	socket(NULL),
	name("localhost"),
	authMethod(AUTHPLAIN),
	connectionTimeout(5000),
	responseTimeout(5000),
	sendMessageTimeout(60000)
{
	setConnectionType(connectionType);

	this->host = host;
	this->port = port;

	connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
			this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));
	connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
			this, SLOT(socketError(QAbstractSocket::SocketError)));
	connect(socket, SIGNAL(readyRead()),
			this, SLOT(socketReadyRead()));
}

SmtpClient::~SmtpClient()
{
	if (socket)
		delete socket;
}

void SmtpClient::setUser(const QString &user)
{
	this->user = user;
}

void SmtpClient::setPassword(const QString &password)
{
	this->password = password;
}

void SmtpClient::setAuthMethod(AuthMethod method)
{
	this->authMethod = method;
}

void SmtpClient::setHost(const QString &host)
{
	this->host = host;
}

void SmtpClient::setPort(int port)
{
	this->port = port;
}

void SmtpClient::setConnectionType(ConnectionType ct)
{
	this->connectionType = ct;

	if (socket)
		delete socket;

	switch (connectionType)
	{
	case TCPCONNECTION:
		socket = new QTcpSocket(this);
		break;

#ifndef QT_NO_OPENSSL
	case SSLCONNECTION:
	case TSLCONNECTION:
		socket = new QSslSocket(this);
#endif
	}
}

const QString& SmtpClient::getHost() const
{
	return this->host;
}

const QString& SmtpClient::getUser() const
{
	return this->user;
}

const QString& SmtpClient::getPassword() const
{
	return this->password;
}

SmtpClient::AuthMethod SmtpClient::getAuthMethod() const
{
	return this->authMethod;
}

int SmtpClient::getPort() const
{
	return this->port;
}

SmtpClient::ConnectionType SmtpClient::getConnectionType() const
{
	return connectionType;
}

const QString& SmtpClient::getName() const
{
	return this->name;
}

void SmtpClient::setName(const QString &name)
{
	this->name = name;
}

const QString & SmtpClient::getResponseText() const
{
	return responseText;
}

int SmtpClient::getResponseCode() const
{
	return responseCode;
}

QTcpSocket* SmtpClient::getSocket() {
	return socket;
}

int SmtpClient::getConnectionTimeout() const
{
	return connectionTimeout;
}

void SmtpClient::setConnectionTimeout(int msec)
{
	connectionTimeout = msec;
}

int SmtpClient::getResponseTimeout() const
{
	return responseTimeout;
}

void SmtpClient::setResponseTimeout(int msec)
{
	responseTimeout = msec;
}
int SmtpClient::getSendMessageTimeout() const
{
	return sendMessageTimeout;
}
void SmtpClient::setSendMessageTimeout(int msec)
{
	sendMessageTimeout = msec;
}


bool SmtpClient::connectToHost()
{
	switch (connectionType)
	{
#ifndef QT_NO_OPENSSL
	case TSLCONNECTION:
#endif

	case TCPCONNECTION:
		socket->connectToHost(host, port);
		break;

#ifndef QT_NO_OPENSSL
	case SSLCONNECTION:
		((QSslSocket*) socket)->connectToHostEncrypted(host, port);
		break;
#endif

	}

	// Tries to connect to server
	if (!socket->waitForConnected(connectionTimeout)) {
		emit smtpError(CONNECTIONTIMEOUTERROR);
		return false;
	}

	try {
		// Wait for the server's response
		waitForResponse();

		// If the response code is not 220 (Service ready)
		// means that is something wrong with the server
		if (responseCode != 220) {
			emit smtpError(SERVERERROR);
			return false;
		}

		// Send a EHLO/HELO message to the server
		// The client's first command must be EHLO/HELO
		sendMessage("EHLO " + name);

		// Wait for the server's response
		waitForResponse();

		// The response code needs to be 250.
		if (responseCode != 250) {
			emit smtpError(SERVERERROR);
			return false;
		}


#ifndef QT_NO_OPENSSL
		if (connectionType == TSLCONNECTION) {
			// send a request to start TLS handshake
			sendMessage("STARTTLS");

			// Wait for the server's response
			waitForResponse();

			// The response code needs to be 220.
			if (responseCode != 220) {
				emit smtpError(SERVERERROR);
				return false;
			};

			((QSslSocket*) socket)->startClientEncryption();

			if (!((QSslSocket*) socket)->waitForEncrypted(connectionTimeout)) {
				qDebug() << ((QSslSocket*) socket)->errorString();
				emit smtpError(CONNECTIONTIMEOUTERROR);
				return false;
			}

			// Send ELHO one more time
			sendMessage("EHLO " + name);

			// Wait for the server's response
			waitForResponse();

			// The response code needs to be 250.
			if (responseCode != 250) {
				emit smtpError(SERVERERROR);
				return false;
			}
		}

#endif
	}
	catch (ResponseTimeoutException) {
		return false;
	}
	catch (SendMessageTimeoutException) {
		return false;
	}

	// If no errors occured the function returns true.
	return true;
}

bool SmtpClient::login()
{
	return login(user, password, authMethod);
}

bool SmtpClient::login(const QString &user, const QString &password, AuthMethod method)
{
	try {
		if (method == AUTHPLAIN) {
			// Sending command: AUTH PLAIN base64('\0' + username + '\0' + password)
			sendMessage("AUTH PLAIN " + QByteArray().append((char) 0).append(user).append((char) 0).append(password).toBase64());

			// Wait for the server's response
			waitForResponse();

			// If the response is not 235 then the authentication was faild
			if (responseCode != 235) {
				emit smtpError(AUTHENTICATIONFAILEDERROR);
				return false;
			}
		} else if (method == AUTHLOGIN) {
			// Sending command: AUTH LOGIN
			sendMessage("AUTH LOGIN");

			// Wait for 334 response code
			waitForResponse();
			if (responseCode != 334) {
				emit smtpError(AUTHENTICATIONFAILEDERROR);
				return false;
			}

			// Send the username in base64
			sendMessage(QByteArray().append(user).toBase64());

			// Wait for 334
			waitForResponse();
			if (responseCode != 334) {
				emit smtpError(AUTHENTICATIONFAILEDERROR);
				return false;
			}

			// Send the password in base64
			sendMessage(QByteArray().append(password).toBase64());

			// Wait for the server's responce
			waitForResponse();

			// If the response is not 235 then the authentication was faild
			if (responseCode != 235) {
				emit smtpError(AUTHENTICATIONFAILEDERROR);
				return false;
			}
		}
	}
	catch (ResponseTimeoutException) {
		// Responce Timeout exceeded
		emit smtpError(AUTHENTICATIONFAILEDERROR);
		return false;
	}
	catch (SendMessageTimeoutException) {
		// Send Timeout exceeded
		emit smtpError(AUTHENTICATIONFAILEDERROR);
		return false;
	}

	return true;
}

bool SmtpClient::sendMail(MimeMessage& email)
{
	try	{
		// Send the MAIL command with the sender
		sendMessage("MAIL FROM: <" + email.getSender().getAddress() + ">");

		waitForResponse();

		if (responseCode != 250)
			return false;

		// Send RCPT command for each recipient
		QList<EmailAddress*>::const_iterator it, itEnd;
		// To (primary recipients)
		for (it = email.getRecipients().begin(), itEnd = email.getRecipients().end();
			 it != itEnd; ++it) {
			sendMessage("RCPT TO: <" + (*it)->getAddress() + ">");
			waitForResponse();

			if (responseCode != 250)
				return false;
		}

		// Cc (carbon copy)
		for (it = email.getRecipients(MimeMessage::CC).begin(), itEnd = email.getRecipients(MimeMessage::CC).end();
			 it != itEnd; ++it) {
			sendMessage("RCPT TO: <" + (*it)->getAddress() + ">");
			waitForResponse();

			if (responseCode != 250)
				return false;
		}

		// Bcc (blind carbon copy)
		for (it = email.getRecipients(MimeMessage::BCC).begin(), itEnd = email.getRecipients(MimeMessage::BCC).end();
			 it != itEnd; ++it) {
			sendMessage("RCPT TO: <" + (*it)->getAddress() + ">");
			waitForResponse();

			if (responseCode != 250)
				return false;
		}

		// Send DATA command
		sendMessage("DATA");
		waitForResponse();

		if (responseCode != 354)
			return false;

		sendMessage(email.toString());

		// Send \r\n.\r\n to end the mail data
		sendMessage(".");

		waitForResponse();

		if (responseCode != 250)
			return false;
	}
	catch (ResponseTimeoutException){
		return false;
	}
	catch (SendMessageTimeoutException)	{
		return false;
	}

	return true;
}

void SmtpClient::quit()
{
	sendMessage("QUIT");
}

void SmtpClient::waitForResponse()
{
	do {
		if (!socket->waitForReadyRead(responseTimeout))	{
			emit smtpError(RESPONSETIMEOUTERROR);
			throw ResponseTimeoutException();
		}

		while (socket->canReadLine()) {
			// Save the server's response
			responseText = socket->readLine();

			// Extract the respose code from the server's responce (first 3 digits)
			responseCode = responseText.left(3).toInt();

			if (responseCode / 100 == 4)
				emit smtpError(SERVERERROR);

			if (responseCode / 100 == 5)
				emit smtpError(CLIENTERROR);

			if (responseText[3] == ' ') {
				return;
			}
		}
	} while (true);
}

void SmtpClient::sendMessage(const QString &text)
{
	socket->write(text.toUtf8() + "\r\n");
	if (! socket->waitForBytesWritten(sendMessageTimeout)) {
		emit smtpError(SENDDATATIMEOUTERROR);
		throw SendMessageTimeoutException();
	}
}


void SmtpClient::socketStateChanged(QAbstractSocket::SocketState )
{
}

void SmtpClient::socketError(QAbstractSocket::SocketError)
{
}

void SmtpClient::socketReadyRead()
{
}
