#ifndef MIMECONTENTFORMATTER_H
#define MIMECONTENTFORMATTER_H

#include <QObject>
#include <QByteArray>
#include "smtpexports.h"

class SMTP_EXPORT MimeContentFormatter : public QObject
{
	Q_OBJECT
public:
	MimeContentFormatter (int max_length = 76);
	QString format(const QString &content, bool quotedPrintable = false) const;

	void setMaxLength(int l);
	int getMaxLength() const;
protected:
	int max_length;
};

#endif // MIMECONTENTFORMATTER_H
