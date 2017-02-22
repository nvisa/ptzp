#ifndef FTP_H
#define FTP_H

#include <QFtp>

class Ftp : public QFtp
{
	Q_OBJECT
public:
	Ftp(QObject * parent);

	void ftpConnect(QString address, QString username, QString pass);
	QString ftpAddress(const QString address);
	QString ftpUsername(const QString username);
	QString ftpPass(const QString pass);
	void direction(QString command, const QString path);
	void direction(QString rename, QString oldname, QString newname);
	void direction(QString command);
	void getFile(QString path);
	int putFile(QString filePath, QString location);
	void ftpClose(void);

public slots:
	void stateChanged(void);
	void stateCommand(void);
};

#endif // FTP_H
