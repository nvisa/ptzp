#ifndef HITACHIMODULE110_H
#define HITACHIMODULE110_H

#include "hitachimodule.h"

class HitachiModule110 : public HitachiModule
{
	Q_OBJECT
public:
	explicit HitachiModule110(QextSerialPort *hiPort, QObject *parent = 0);

	virtual ModuleType model() { return MODULE_TYPE_SC110; }
signals:
	
public slots:

private:
};

#endif // HITACHIMODULE110_H
