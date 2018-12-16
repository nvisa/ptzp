#ifndef TBGTHDRIVER_H
#define TBGTHDRIVER_H

#include <ecl/ptzp/ptzpdriver.h>

class YamanoLensHead;
class EvpuPTHead;
class PtzpSerialTransport;
class PtzpTcpTransport;

class TbgthDriver : public PtzpDriver
{
	Q_OBJECT
public:
	explicit TbgthDriver(QObject *parent = 0);

	PtzpHead * getHead(int index);
	int setTarget(const QString &targetUri);
	QVariant get(const QString &key);
	int set(const QString &key, const QVariant &value);

protected slots:
	void timeout();

protected:
	enum DriverState {
		INIT,
		SYNC_HEAD_LENS,
		NORMAL,
	};

	YamanoLensHead *headLens;
	EvpuPTHead *headEvpuPt;
	PtzpTransport *tp;
	PtzpTransport *tp1;
	DriverState state;
};

#endif // TBGTHDRIVER_H
