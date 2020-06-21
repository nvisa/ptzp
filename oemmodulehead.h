#ifndef OEMMODULEHEAD_H
#define OEMMODULEHEAD_H

#include <ptzphead.h>

#include <QElapsedTimer>

class CommandHistory;

class ErrorRateChecker;
class SimpleStatistics;
class StationaryFilter;

namespace SimpleMetrics {
class Point;
class Channel;
}

class OemModuleHead : public PtzpHead
{
public:
	OemModuleHead();

	enum Registers {
		R_EXPOSURE_VALUE, // 14 //td:nd // exposure_value
		R_GAIN_VALUE,	  // 15 //td:nd // gainvalue
		R_ZOOM_POS,
		R_EXP_COMPMODE,	   // 16 //td:nd
		R_EXP_COMPVAL,	   // 17 //td:nd
		R_GAIN_LIM,		   // 18 //td:nd
		R_SHUTTER,		   // 19 //td:nd // getshutterspeed
		R_NOISE_REDUCT,	   // 22 //td:nd
		R_WDRSTAT,		   // 23 //td:nd
		R_GAMMA,		   // 25 //td.nd
		R_AWB_MODE,		   // 26 //td:nd
		R_DEFOG_MODE,	   // 27 //td:nd
		R_DIGI_ZOOM_STAT,  // 28 //td:nd
		R_ZOOM_TYPE,	   // 29 //td:nd
		R_FOCUS_MODE,	   // 30 //td:nd
		R_ZOOM_TRIGGER,	   // 31 //td:nd
		R_BLC_STATUS,	   // 32 //td:nd
		R_IRCF_STATUS,	   // 33 //td:nd
		R_AUTO_ICR,		   // 34 //td:nd
		R_PROGRAM_AE_MODE, // 35 //td:nd
		R_FLIP,
		R_MIRROR,
		R_DISPLAY_ROT,
		R_DIGI_ZOOM_POS,
		R_OPTIC_ZOOM_POS,
		R_PT_SPEED,
		R_ZOOM_SPEED,
		R_EXPOSURE_TARGET,
		R_BOT_SHUTTER,
		R_TOP_SHUTTER,
		R_BOT_IRIS,
		R_TOP_IRIS,
		R_TOP_GAIN,
		R_BOT_GAIN,

		R_VISCA_MODUL_ID,
		R_FOCUS_POS,

		R_COUNT
	};

	void fillCapabilities(ptzp::PtzHead *head);
	int syncRegisters();
	int getHeadStatus();

	int startZoomIn(int speed);
	int startZoomOut(int speed);
	int stopZoom();
	int getZoom();
	int setZoom(uint pos);
	int focusIn(int speed);
	int focusOut(int speed);
	int focusStop();
	uint getProperty(uint r);
	void setProperty(uint r, uint x);

	void enableSyncing(bool en);
	void setSyncInterval(int interval);
	void setDeviceDefinition(QString definition);
	QString getDeviceDefinition();
	int getZoomSpeed();
	void clockInvert(bool st);

	int setShutterLimit(uint topLim, uint botLim);
	QString getShutterLimit();
	int setIrisLimit(uint topLim, uint botLim);
	QString getIrisLimit();
	int setGainLimit(uchar topLim, uchar botLim);
	QString getGainLimit();

	int addCustomSettings();
	float getAngle();

	void enableZoomControlFiltering(bool en);
	void enableZoomControlTriggerredRead(bool en);
	void enableZoomControlStationaryFiltering(bool en);
	void enableZoomControlErrorRateChecking(bool en);
	void enableZoomControlReadLatencyChecking(bool en);
	QVariant getCapabilityValues(ptzp::PtzHead_Capability c);
	void setCapabilityValues(ptzp::PtzHead_Capability c, uint val);

protected:
	int syncNext();
	int dataReady(const unsigned char *bytes, int len);
	QByteArray transportReady();
	QJsonValue marshallAllRegisters();
	void unmarshallloadAllRegisters(const QJsonValue &node);
	int addSpecialModulSettings();

	struct zoomErrorChecks {
		bool zoomFiltering;
		bool tiggerredRead;
		bool stationaryFiltering;
		bool zoomed; //for triggerred reading
		bool errorRateCheck;
		bool readLatencyCheck;
		ErrorRateChecker *echeck;
		SimpleStatistics *stats;
		StationaryFilter *sfilter;
	} zoomControls;
	bool syncEnabled;
	int syncInterval;
	CommandHistory *hist;
	uint nextSync;
	uint queryIndex;
	QElapsedTimer syncTime;
	QString deviceDefinition;
	/* TODO: investigate zoomSpeed and move to PtzpHead if possible */
	int zoomSpeed;

	QHash<uint, uint> registersCache;
	SimpleMetrics::Point *mp;
	SimpleMetrics::Channel *serialRecved;
	SimpleMetrics::Channel *zoomRecved;
};

#endif // OEMMODULEHEAD_H
