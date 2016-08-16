#ifndef CAMINFO_H
#define CAMINFO_H

#include <QObject>

#include <drivers/irdomemodule.h>
#include <drivers/hitachimodule.h>
#include <drivers/qextserialport/qextserialport.h>

class CamInfo : public QObject
{
	Q_OBJECT
public:
	explicit CamInfo(QextSerialPort *port, QObject *parent = 0);
	void getModuleSummary();

protected:
	int getIrDomemodule();
	int getHitachi();

private:
	QextSerialPort *camPort;
};

#endif // CAMINFO_H
