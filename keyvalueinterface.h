#ifndef KEYVALUEINTERFACE_H
#define KEYVALUEINTERFACE_H

#include <QString>
#include <QVariant>

class KeyValueInterface
{
public:
	virtual QVariant get(const QString &key) = 0;
	virtual int set(const QString &key, const QVariant &value) = 0;
};

#endif // KEYVALUEINTERFACE_H

