#ifndef MIMEHTML_H
#define MIMEHTML_H

#include "mimetext.h"
#include "smtpexports.h"

class SMTP_EXPORT MimeHtml : private MimeText
{
	Q_OBJECT
public:
	MimeHtml(const QString &html = "");
	~MimeHtml();
protected:
	void setHtml(const QString & html);
	const QString& getHtml() const;
	virtual void prepare();
};

#endif // MIMEHTML_H
