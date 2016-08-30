#ifndef MIMEINLINEFILE_H
#define MIMEINLINEFILE_H

#include "mimefile.h"

#include "smtpexports.h"

class SMTP_EXPORT MimeInlineFile : public MimeFile
{
public:
	MimeInlineFile(QFile *f);
	~MimeInlineFile();
protected:
	virtual void prepare();
};

#endif // MIMEINLINEFILE_H
