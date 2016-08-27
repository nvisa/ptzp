#ifndef MIMEMULTIPART_H
#define MIMEMULTIPART_H

#include "mimepart.h"
#include "smtpexports.h"

class SMTP_EXPORT MimeMultiPart : public MimePart
{
	Q_OBJECT
public:
	enum MultiPartType {
		MIXED			= 0,			// RFC 2046, section 5.1.3
		DIGEST			= 1,			// RFC 2046, section 5.1.5
		ALTERNATIVE		= 2,			// RFC 2046, section 5.1.4
		RELATED			= 3,			// RFC 2387
		REPORT			= 4,			// RFC 6522
		SIGNED			= 5,			// RFC 1847, section 2.1
		ENCRYTED		= 6				// RFC 1847, section 2.2
	};
	MimeMultiPart(const MultiPartType type = RELATED);

	~MimeMultiPart();

	const QList<MimePart*> & getParts() const;
	void addPart(MimePart *part);
	void setMimeType(const MultiPartType type);
	MultiPartType getMimeType() const;
	virtual void prepare();

protected:
	QList< MimePart* > parts;
	MultiPartType type;
};

#endif // MIMEMULTIPART_H
