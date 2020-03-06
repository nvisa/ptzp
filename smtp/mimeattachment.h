#ifndef MIMEATTACHMENT_H
#define MIMEATTACHMENT_H

#include <QFile>
#include "mimepart.h"
#include "mimefile.h"
#include "smtpexports.h"

class SMTP_EXPORT MimeAttachment : public MimeFile
{
	Q_OBJECT
public:
	MimeAttachment(QFile* file);
	MimeAttachment(const QByteArray& stream, const QString& fileName);
	~MimeAttachment();
protected:
	virtual void prepare();
};

#endif // MIMEATTACHMENT_H
