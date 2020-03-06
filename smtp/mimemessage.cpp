#include "mimemessage.h"
#include <QDateTime>
#include "quotedprintable.h"
#include <typeinfo>

MimeMessage::MimeMessage(bool createAutoMimeContent) :
	hEncoding(MimePart::_8BIT)
{
	if (createAutoMimeContent)
		this->content = new MimeMultiPart();
	autoMimeContentCreated = createAutoMimeContent;
}

MimeMessage::~MimeMessage()
{
	if (this->autoMimeContentCreated) {
		this->autoMimeContentCreated = false;
		delete (this->content);
	}
}

MimePart& MimeMessage::getContent()
{
	return *content;
}

void MimeMessage::setContent(MimePart *content)
{
	if (this->autoMimeContentCreated) {
		this->autoMimeContentCreated = false;
		delete (this->content);
	}
	this->content = content;
}

void MimeMessage::setSender(EmailAddress* e)
{
	this->sender = e;
}

void MimeMessage::addRecipient(EmailAddress* rcpt, RecipientType type)
{
	switch (type) {
	case TO:
		recipientsTo << rcpt;
		break;
	case CC:
		recipientsCc << rcpt;
		break;
	case BCC:
		recipientsBcc << rcpt;
		break;
	}
}

void MimeMessage::addTo(EmailAddress* rcpt)
{
	this->recipientsTo << rcpt;
}

void MimeMessage::addCc(EmailAddress* rcpt)
{
	this->recipientsCc << rcpt;
}

void MimeMessage::addBcc(EmailAddress* rcpt)
{
	this->recipientsBcc << rcpt;
}

void MimeMessage::setSubject(const QString & subject)
{
	this->subject = subject;
}

void MimeMessage::addPart(MimePart *part)
{
	if (typeid(*content) == typeid(MimeMultiPart)) {
		((MimeMultiPart*) content)->addPart(part);
	};
}

void MimeMessage::setHeaderEncoding(MimePart::Encoding hEnc)
{
	this->hEncoding = hEnc;
}

const EmailAddress & MimeMessage::getSender() const
{
	return *sender;
}

const QList<EmailAddress*> & MimeMessage::getRecipients(RecipientType type) const
{
	switch (type) {
	default:
	case TO:
		return recipientsTo;
	case CC:
		return recipientsCc;
	case BCC:
		return recipientsBcc;
	}
}

const QString & MimeMessage::getSubject() const
{
	return subject;
}

const QList<MimePart*> & MimeMessage::getParts() const
{
	if (typeid(*content) == typeid(MimeMultiPart)) {
		return ((MimeMultiPart*) content)->getParts();
	} else {
		QList<MimePart*> *res = new QList<MimePart*>();
		res->append(content);
		return *res;
	}
}

QString MimeMessage::toString()
{
	QString mime;

	/* =========== MIME HEADER ============ */

	/* ---------- Sender / From ----------- */
	mime = "From:";
	if (sender->getName() != "") {
		switch (hEncoding) {
		case MimePart::BASE64:
			mime += " =?utf-8?B?" + QByteArray().append(sender->getName()).toBase64() + "?=";
			break;
		case MimePart::QUOTEDPRINTABLE:
			mime += " =?utf-8?Q?" + QuotedPrintable::encode(QByteArray().append(sender->getName())).replace(' ', "_").replace(':',"=3A") + "?=";
			break;
		default:
			mime += " " + sender->getName();
		}
	}
	mime += " <" + sender->getAddress() + ">\r\n";
	/* ---------------------------------- */


	/* ------- Recipients / To ---------- */
	mime += "To:";
	QList<EmailAddress*>::iterator it;  int i;
	for (i = 0, it = recipientsTo.begin(); it != recipientsTo.end(); ++it, ++i) {
		if (i != 0) {
			mime += ",";
		}

		if ((*it)->getName() != "") {
			switch (hEncoding) {
			case MimePart::BASE64:
				mime += " =?utf-8?B?" + QByteArray().append((*it)->getName()).toBase64() + "?=";
				break;
			case MimePart::QUOTEDPRINTABLE:
				mime += " =?utf-8?Q?" + QuotedPrintable::encode(QByteArray().append((*it)->getName())).replace(' ', "_").replace(':',"=3A") + "?=";
				break;
			default:
				mime += " " + (*it)->getName();
			}
		}
		mime += " <" + (*it)->getAddress() + ">";
	}
	mime += "\r\n";
	/* ---------------------------------- */

	/* ------- Recipients / Cc ---------- */
	if (recipientsCc.size() != 0) {
		mime += "Cc:";
	}
	for (i = 0, it = recipientsCc.begin(); it != recipientsCc.end(); ++it, ++i) {
		if (i != 0) {
			mime += ",";
		}

		if ((*it)->getName() != "") {
			switch (hEncoding) {
			case MimePart::BASE64:
				mime += " =?utf-8?B?" + QByteArray().append((*it)->getName()).toBase64() + "?=";
				break;
			case MimePart::QUOTEDPRINTABLE:
				mime += " =?utf-8?Q?" + QuotedPrintable::encode(QByteArray().append((*it)->getName())).replace(' ', "_").replace(':',"=3A") + "?=";
				break;
			default:
				mime += " " + (*it)->getName();
			}
		}
		mime += " <" + (*it)->getAddress() + ">";
	}
	if (recipientsCc.size() != 0) {
		mime += "\r\n";
	}
	/* ---------------------------------- */

	/* ------------ Subject ------------- */
	mime += "Subject: ";


	switch (hEncoding) {
	case MimePart::BASE64:
		mime += "=?utf-8?B?" + QByteArray().append(subject).toBase64() + "?=";
		break;
	case MimePart::QUOTEDPRINTABLE:
		mime += "=?utf-8?Q?" + QuotedPrintable::encode(QByteArray().append(subject)).replace(' ', "_").replace(':',"=3A") + "?=";
		break;
	default:
		mime += subject;
	}
	/* ---------------------------------- */

	mime += "\r\n";
	mime += "MIME-Version: 1.0\r\n";
	mime += QString("Date: %1\r\n").arg(QDateTime::currentDateTime().toString(Qt::ISODate));

	mime += content->toString();
	return mime;
}
