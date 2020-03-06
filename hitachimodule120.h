#ifndef HITACHIMODULE120_H
#define HITACHIMODULE120_H

#include "hitachimodule.h"

class HitachiModule120 : public HitachiModule
{
	Q_OBJECT
public:
	explicit HitachiModule120(QextSerialPort *port, QObject *parent = 0);

	virtual ModuleType model() { return MODULE_TYPE_SC120; }
	virtual int getZoomPosition();
	virtual float getZoomPositionF();

signals:
	
public slots:
	
};

#endif // HITACHIMODULE120_H
