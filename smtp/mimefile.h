#ifndef MIMEFILE_H
#define MIMEFILE_H

#include "mimepart.h"
#include <QFile>
#include "smtpexports.h"

class SMTP_EXPORT MimeFile : public MimePart
{
	Q_OBJECT
public:
	MimeFile(const QByteArray& stream, const QString& fileName);
	MimeFile(QFile *f);
	~MimeFile();
protected:
	QFile* file;
	virtual void prepare();
};

#endif // MIMEFILE_H
