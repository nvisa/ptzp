#include "ftp.h"

#include <QString>
#include <QFile>

#include <ecl/debug.h>

Ftp::Ftp(QObject *parent) : QFtp(parent)
{
	connect(this,SIGNAL(commandStarted(int)),this,SLOT(stateCommand()));
	connect(this,SIGNAL(stateChanged(int)),this,SLOT(stateChanged()));
}

QString Ftp::ftpAddress(const QString address )
{
	return address;
}

QString Ftp::ftpUsername(const QString username)
{
	return username;
}

QString Ftp::ftpPass(const QString pass)
{
	return pass;
}

void Ftp::ftpConnect(QString address,QString username,QString pass)
{
	this->connectToHost(address,21);
	this->login(username,pass);
}

void Ftp::direction(QString command,const QString path)
{
	if (command == "cd")
		this->cd(path);
	else if (command == "remove")
		this->remove(path);
	else if (command == "mkdir")
		this->mkdir(path);
	else if (command == "rmdir")
		this->rmdir(path);
	else if (command == "list")
		this->list(path);

}

void Ftp::direction(QString command, QString oldname, QString newname)
{
	if (command == "rename")
		rename(oldname,newname);
}

void Ftp::direction(QString command)
{
	this->rawCommand(command);
}

void Ftp::getFile(QString path)
{
	this->get(path);
}

int Ftp::putFile(QString filePath,QString location)
{
	QFile *file = new QFile(filePath, this);
	if (file->open(QIODevice::ReadWrite))
		mDebug ("file is open");
	else
		mDebug ("file not open");
	return this->put(file,location);
}

void Ftp::ftpClose()
{
	close();
}

void Ftp::stateChanged()
{
	QFtp::State status = state();
	if( status == QFtp::HostLookup)
		mDebug("HostLookup");
	else if (status == QFtp::Connecting)
		mDebug ( "Connecting");
	else if (status == QFtp::Connected)
		mDebug ( "Connected");
	else if (status == QFtp::LoggedIn)
		mDebug ( "LoggedIn");
	else if (status == QFtp::Closing)
		mDebug ( "Closing");
	else
		mDebug ( "Unconnected");
}

void Ftp::stateCommand()
{
	QFtp::Command state = currentCommand();
	if( state == QFtp::None)
		mDebug ( "None");
	else if (state == QFtp::SetTransferMode)
		mDebug ( "SetTransferMode");
	else if (state == QFtp::SetProxy)
		mDebug ( "SetProxy");
	else if (state == QFtp::ConnectToHost)
		mDebug ( "ConnectToHost");
	else if (state == QFtp::Login)
		mDebug ( "Login");
	else if (state == QFtp::Close)
		mDebug ( "Close");
	else if( state == QFtp::List)
		mDebug ( "List");
	else if (state == QFtp::Cd)
		mDebug ( "Cd");
	else if (state == QFtp::Get)
		mDebug ( "Get");
	else if (state == QFtp::Put)
		mDebug ( "Put");
	else if (state == QFtp::Remove)
		mDebug ( "Remove");
	else if (state == QFtp::Mkdir)
		mDebug ( "Mkdir");
	else if (state == QFtp::Rmdir)
		mDebug ( "Rmdir");
	else if (state == QFtp::Rename)
		mDebug ( "Rename");
	else if (state == QFtp::RawCommand)
		mDebug ( "RawCommand");
}
