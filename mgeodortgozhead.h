#ifndef MGEODORTGOZHEAD_H
#define MGEODORTGOZHEAD_H

#include <i2cdevice.h>
#include <ptzphead.h>

#include <QElapsedTimer>
#include <QStringList>

class PCA9538Driver;
class RelayControlThread;

class MgeoDortgozHead : public PtzpHead
{
public:
	MgeoDortgozHead(QList<int> relayConfig);

	enum Registers {
		R_STATE,
		/*wip
//		R_VIDEO_1_COUNT,
//		R_VIDE0_1_1_MODE,
//		R_VIDEO_1_2_MODE,
		*/
		R_VIDEO_STATE,
		R_VIDEO_ROTATION,
		R_VIDEO_FRAME_RATE,
		R_ZOOM_SPEED,
		R_ZOOM_STEP_NUM,
		R_ZOOM_ANGLE,
		R_ZOOM_ENC_POS,
		R_E_ZOOM,
		R_FOCUS_SPEED,
		R_FOCUS_ENC_POS,
		R_BRIGTNESS,
		R_CONTRAST,
		R_RETICLE_MODE,
		R_RETICLE_INTENSITY,
		R_SYMBOLOGY,
		R_POLARITY,
		R_THERMAL_TABLE,
		R_IMG_PROC_MODE,
		R_FOCUS_MODE,
		R_RELAY_STATUS,

		R_COUNT
	};

	void fillCapabilities(ptzp::PtzHead *head); //ok
	virtual int syncRegisters(); //ok
	virtual int startZoomIn(int speed); //ok
	virtual int startZoomOut(int speed); //ok
	virtual int stopZoom(); //ok
	virtual int setZoom(uint pos); // ok
	virtual int getZoom(); //ok
	virtual int focusIn(int speed); //ok
	virtual int focusOut(int speed); //ok
	virtual int focusStop(); //ok
	virtual int getHeadStatus();//ok
	virtual void setProperty(uint r, uint x); // ok
	virtual uint getProperty(uint r); //ok

protected:
	int syncNext();//ok
	virtual int dataReady(const unsigned char *bytes, int len); //ok
	QByteArray transportReady();//OK
	uint nextSync;//ok

private:
	int sendCommand(const unsigned char *cmd, int len);//ok
	int readRelayConfig(QString filename);//
	QElapsedTimer syncTimer;
	PCA9538Driver *i2c;//OK
	RelayControlThread *relth; //OK
	bool fastSwitch;//OK
	int thermalRelay;//OK
	int dayCamRelay;//OK
	int standbyRelay;//ok
};

#endif // MGEODORTGOZHEAD_H
