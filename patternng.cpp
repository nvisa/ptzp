#include "patternng.h"
#include "debug.h"

#include <QTimer>
#include <QDataStream>

#include <errno.h>

/**
	\class PatternNg

	\brief This class performs PTZ pattern recording, saving, loading
	and replaying functionalities.

	PatternNg is not a PTZ controller on its own, instead it relies on
	on other classes for doing PTZ operations. PatternNg implements
	PtzPatternInterface interface and requires a class implementing
	PtzControlInterface (if replaying functionaliry is neeeded).

	PatternNg records PTZ commands along with related position and
	timing information. In replay mode, class re-plays recorded commands
	again using related position and timing information. Since it uses
	both timing and position for implemeting patterns, it is more resilient
	against delays and shifts.

	PatternNg has 2 working modes, recording and replaying. It start recording
	commands with issuing of first start() command. To stop recording, one should
	issue stop(). Then save() function can be used to save recorded pattern to disk.
	Recorded pattern can be loaded using load() function later. Replaying mode
	is entered using replay() function, which can be stopped using stop() command
	when desired.

	PatternNg doesn't understand underlying PTZ commands. It just records commands
	and position updates during the recording phase, and issues back same commands
	during replay phase.

	\section Replay Details

	Before starting playback, PatternNg tries to move to the initial point of the
	given pattern. During this time, timer is not started and will be inactive till
	dome reaches initial start point. 'rpars' member can be adjusted for changing
	default behaviours.

	When initial position is reached, replay starts and elapsed time and current position
	is compared with every positionUpdate() callback. Upper layers should call positionUpdate()
	as frequent as possible for best pattern playback resolution. positionUpdate() is
	also called during recording phase so positionUpdate() differentiates what to do
	in different modes.

	TODO:
		- Implement position-based replaying
		- Zoom commands

*/

PatternNg::PatternNg(PtzControlInterface *ctrl)
{
	recording = false;
	replaying = false;
	sm = SYNC_TIME;
	current = 0;
	ptzctrl = ctrl;

	rpars.panResolution = 100;
	rpars.tiltResolution = 100;
	rpars.approachSpeedHigh = 63;
	rpars.approachSpeedMedium = 40;
	rpars.approachSpeedLow = 20;
	rpars.approachSpeedUltraLow = 10;
	rpars.approachPointHigh = 10000;
	rpars.approachPointMedium = 5000;
	rpars.approachPointLow = 1500;
}

void PatternNg::positionUpdate(int pan, int tilt, int zoom)
{
	if (isRecording()) {
		//pattern record
		SpaceTime st;
		st.pan = pan;
		st.tilt = tilt;
		st.zoom = zoom;
		st.cmd = -1;
		st.time = ptime.elapsed();
		QMutexLocker ml(&mutex);
		geometry << st;
	} else if (isReplaying()) {
		//pattern replay
		replayCurrent(pan, tilt, zoom);
	} else if (patrolMap[currentPatrol].replay == true) {
		//patrol replay
		const patrol p = patrolMap[currentPatrol];
		int elapsed = rtime.elapsed();
		int currentTime = p.timeOrder[currentPreset].toInt()*1000;

		if (elapsed > currentTime) {
			currentPreset++;
			if (currentPreset >= p.timeOrder.length())
				currentPreset = 0;
			rtime.start();
			goToPreset(p.pSetOrder[currentPreset]);
		}
	}
}

void PatternNg::commandUpdate(int pan, int tilt, int zoom, int c, int par1, int par2)
{
	if (!isRecording()) {
		return;
	}
	SpaceTime st;
	st.pan = pan;
	st.tilt = tilt;
	st.zoom = zoom;
	st.cmd = c;
	st.par1 = par1;
	st.par2 = par2;
	st.time = ptime.elapsed();
	fDebug("command=%d time=%lld", st.cmd, st.time);
	QMutexLocker ml(&mutex);
	geometry << st;
}

int PatternNg::start(int pan, int tilt, int zoom)
{
	if (isReplaying())
		return -EINVAL;
	recording = true;
	ptime.start();
	positionUpdate(pan, tilt, zoom);
	return 0;
}

void PatternNg::stop(int pan, int tilt, int zoom)
{
	recording = false;
	replaying = false;
	positionUpdate(pan, tilt, zoom);
}

int PatternNg::replay()
{
	if (!ptzctrl)
		return -ENOENT;
	if (isRecording())
		return -EINVAL;
	replaying = true;
	ptime.restart();
	current = 0;
	rs = RS_PAN_INIT;
	return 0;
}

bool PatternNg::isRecording()
{
	return recording;
}

bool PatternNg::isReplaying()
{
	return replaying;
}

int PatternNg::save(const QString &filename)
{
	if (isRecording())
		return -EINVAL;

	QMutexLocker ml(&mutex);

	QByteArray ba;
	QDataStream out(&ba, QIODevice::WriteOnly);
	out.setByteOrder(QDataStream::LittleEndian);
	out.setFloatingPointPrecision(QDataStream::SinglePrecision);
	out << (qint32)0x77177177; //key
	out << (qint32)1; //version
	out << geometry;

	QFile f(filename);
	if (!f.open(QIODevice::WriteOnly))
		return -EPERM;
	f.write(ba);
	f.close();

	return 0;
}

int PatternNg::load(const QString &filename)
{
	if (isRecording())
		return -EINVAL;

	QFile f(filename);
	if (!f.open(QIODevice::ReadOnly))
		return -EPERM;
	QByteArray ba = f.readAll();
	f.close();

	QDataStream in(ba);
	in.setByteOrder(QDataStream::LittleEndian);
	in.setFloatingPointPrecision(QDataStream::SinglePrecision);
	qint32 key; in >> key;
	if (key != 0x77177177) {
		fDebug("Wrong key '0x%x'", key);
		return -1;
	}
	qint32 ver; in >> ver;
	if (ver != 1) {
		fDebug("Unsupported version '0x%x'", ver);
		return -2;
	}
	geometry.clear();
	in >> geometry;

	return 0;
}

void PatternNg::addPreset(QString name, float panPos, float tiltPos, int zoomPos)
{
	presetMap[name].panPos = panPos;
	presetMap[name].tiltPos = tiltPos;
	presetMap[name].zoomPos = zoomPos;
	presetMap[name].stat = true;
}

void PatternNg::goToPreset(QString name)
{
	if(!presetMap[name].stat) {
		return;
	}
	ptzctrl->goToPosition( presetMap[name].panPos, presetMap[name].tiltPos, presetMap[name].zoomPos);
}

void PatternNg::deletePreset(QString name)
{
	presetMap[name].stat = false;
}

void PatternNg::addPatrol(QString name, QString pSet, QString time)
{
	patrolMap[name].stat =true;
	QStringList prst = pSet.split(",");
	for(int j = 0; j < prst.length(); j++)
		patrolMap[name].pSetOrder << prst[j];

	QStringList tme = time.split(",");
	for(int j = 0; j < prst.length(); j++)
		patrolMap[name].timeOrder << tme[j];
}

void PatternNg::runPatrol(QString name)
{
	if(!patrolMap[name].stat) {
		return;
	}
	patrolMap[name].replay = true;
	currentPatrol = name;
	goToPreset(patrolMap[name].pSetOrder[0]);
	rtime.start();
}

void PatternNg::deletePatrol(QString name)
{
	patrolMap[name].stat = false;
}

void PatternNg::stopPatrol(QString name)
{
	patrolMap[name].replay = false;
}

void PatternNg::replayCurrent(int pan, int tilt, int zoom)
{
	const SpaceTime &st = geometry[current];
	switch (rs) {
	case RS_PAN_INIT: {
		int pdiff = qAbs(st.pan - pan);
		if (pdiff < rpars.panResolution) {
			ptzctrl->sendCommand(ptzctrl->C_PAN_TILT_STOP, 0, 0); //todo: pan stop
			rs = RS_TILT_INIT;
		} else {
			int speed = rpars.approachSpeedUltraLow;
			if (pdiff > rpars.approachPointHigh)
				speed = rpars.approachSpeedHigh;
			else if (pdiff > rpars.approachPointMedium)
				speed = rpars.approachSpeedMedium;
			else if (pdiff > rpars.approachPointLow)
				speed = rpars.approachSpeedLow;
			else
				speed = rpars.approachSpeedUltraLow;
			ptzctrl->sendCommand(ptzctrl->C_PAN_LEFT, 0, 0); //pan_left, goto start position
		}
		break;
	}
	case RS_TILT_INIT: {
		int tdiff = qAbs(st.tilt - tilt);
		if (tdiff < rpars.tiltResolution) {
			ptzctrl->sendCommand(ptzctrl->C_PAN_TILT_STOP, 0, 0); //pan stop
			rs = RS_ZOOM_INIT;
		} else {
			int speed = rpars.approachSpeedUltraLow;
			if (tdiff > rpars.approachPointHigh)
				speed = rpars.approachSpeedHigh;
			else if (tdiff > rpars.approachPointMedium)
				speed = rpars.approachSpeedMedium;
			else if (tdiff > rpars.approachPointLow)
				speed = rpars.approachSpeedLow;
			else
				speed = rpars.approachSpeedUltraLow;
			ptzctrl->sendCommand(ptzctrl->C_TILT_UP, 0, speed); //tilt_up, goto start position
		}
		break;
	}
	case RS_ZOOM_INIT:
		rs = RS_RUN;
		ptime.restart();
		break;
	case RS_RUN:
		if (ptime.elapsed() > st.time) {
			if (st.cmd != -1)
				ptzctrl->sendCommand(st.cmd, st.par1, st.par2);
			current++;
			if (current == geometry.size()) {
				ptime.restart();
				rs = RS_PAN_INIT;
				current = 0;
			}
			replayCurrent(pan, tilt, zoom);
		}
		break;
	case RS_FINALIZE:
		break;
	}
}

QDataStream &operator<<(QDataStream &s, const PatternNg::SpaceTime &st)
{
	s << st.cmd;
	s << st.pan;
	s << st.par1;
	s << st.par2;
	s << st.tilt;
	s << st.time;
	s << st.zoom;
	return s;
}

QDataStream &operator>>(QDataStream &s, PatternNg::SpaceTime &st)
{
	s >> st.cmd;
	s >> st.pan;
	s >> st.par1;
	s >> st.par2;
	s >> st.tilt;
	s >> st.time;
	s >> st.zoom;
	return s;
}
