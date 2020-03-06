#ifndef MIMEPART_H
#define MIMEPART_H

#include <QObject>
#include "mimecontentformatter.h"
#include "smtpexports.h"

class SMTP_EXPORT MimePart : public QObject
{
	Q_OBJECT
public:
	enum Encoding {
		_7BIT,
		_8BIT,
		BASE64,
		QUOTEDPRINTABLE
	};

	MimePart();
	~MimePart();
	const QString& getHeader() const;
	const QByteArray& getContent() const;

	void setContent(const QByteArray & content);
	void setHeader(const QString & header);

	void addHeaderLine(const QString & line);

	void setContentId(const QString & cId);
	const QString & getContentId() const;

	void setContentName(const QString & cName);
	const QString & getContentName() const;

	void setContentType(const QString & cType);
	const QString & getContentType() const;

	void setCharset(const QString & charset);
	const QString & getCharset() const;

	void setEncoding(Encoding enc);
	Encoding getEncoding() const;

	MimeContentFormatter& getContentFormatter();
	virtual QString toString();

	virtual void prepare();

protected:

	QString header;
	QByteArray content;

	QString cId;
	QString cName;
	QString cType;
	QString cCharset;
	QString cBoundary;
	Encoding cEncoding;

	QString mimeString;
	bool prepared;

	MimeContentFormatter formatter;
};

#endif // MIMEPART_H
