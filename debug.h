/*
 * Copyright (C) 2010 Yusuf Caglar Akyuz
 *
 * Her hakki Bilkon Ltd. Sti.'ye  aittir.
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <QTime>
#include <QDebug>
#include <QDateTime>
#include <QStringList>
#include <QElapsedTimer>

namespace ecl {

extern QStringList __dbg_classes;
extern QStringList __dbg_classes_info;
extern QStringList __dbg_classes_log;
extern QStringList __dbg_classes_logv;
extern int dbgtemp;
extern int __dbg_debugging_mode;
extern QString __dbg__network_addr;
extern QString __dbg_file_log_dir;
QString getLibraryVersion();
void initDebug();
void changeDebug(QString debug, int defaultLevel = 0);
void setDebuggingMode(int mode, QString networkAddr);
void useAbsoluteTimeForMessages(int on);
int isUsingAbsoluteTimeForMessages();
#ifdef DEBUG_TIMING
extern QElapsedTimer __debugTimer;
extern int __useAbsTime;
extern unsigned int __lastTime;
extern unsigned int __totalTime;
static inline unsigned int __totalTimePassed()
{
	__lastTime = __debugTimer.restart();
	__totalTime += __lastTime;
	return __totalTime;
}

#define __debug(__mes, __list, __class, __place, arg...) ({ \
	if (__list.size() == 0 || \
		__list.contains(__class->metaObject()->className())) { \
			ecl::__totalTimePassed(); \
			if (ecl::__useAbsTime) \
				qDebug("[%s] [%u] " __mes, qPrintable(QDateTime::currentDateTime().toString()), ecl::__lastTime, __place, ##arg); \
			else \
				qDebug("[%d] [%u] " __mes, ecl::__totalTime, ecl::__lastTime, __place, ##arg); \
		} \
	})
#define __debug_fast qDebug

#else
#define __debug(__mes, __list, __class, __place, arg...) ({ \
	if (__list.size() == 0 || \
		__list.contains(__class->metaObject()->className())) { \
			qDebug(__mes, __place, ##arg); } \
	})
#define __debug_fast qDebug
#endif

} //namespace ecl

#ifdef DEBUG_INFO
#define DEBUG
#define INFO
#endif

#ifdef DEBUG_FORCE
#ifndef DEBUG
#define DEBUG
#endif
#ifndef INFO
#define INFO
#endif
#endif

#define mMessage(mes, arg...) __debug("%s: " mes, this, __PRETTY_FUNCTION__, ##arg)

#ifdef DEBUG
#define mDebug(mes, arg...) __debug("%s: " mes, ecl::__dbg_classes, this, __PRETTY_FUNCTION__, ##arg)
#define fDebug(mes, arg...) __debug_fast("%s: " mes, __PRETTY_FUNCTION__, ##arg)
#define ffDebug() qDebug() << __PRETTY_FUNCTION__
#define debugMessagesAvailable() 1
#else
#define mDebug(mes, arg...) do { if (0) qDebug(mes, ##arg); } while (0)
#define fDebug(mes, arg...) do { if (0) qDebug(mes, ##arg); } while (0)
#define ffDebug() qDebug() << __PRETTY_FUNCTION__
#define debugMessagesAvailable() 0
#endif

#ifdef INFO
#define mInfo(mes, arg...) __debug("%s: " mes, ecl::__dbg_classes_info, this, __PRETTY_FUNCTION__, ##arg)
#define fInfo(mes, arg...) __debug_fast("%s: " mes, __PRETTY_FUNCTION__, ##arg)
#define oInfo(_other, mes, arg...) __debug("%s: " mes, ecl::__dbg_classes_info, _other, __PRETTY_FUNCTION__, ##arg)
#define infoMessagesAvailable() 1
#else
#define mInfo(mes, arg...) do { if (0) qDebug(mes, ##arg); } while (0)
#define fInfo(mes, arg...) do { if (0) qDebug(mes, ##arg); } while (0)
#define infoMessagesAvailable() 0
#endif

#ifdef LOG
#define mLog(mes, arg...) __debug("%s: " mes, ecl::__dbg_classes_log, this, __PRETTY_FUNCTION__, ##arg)
#define logMessagesAvailable() 1
#else
#define mLog(mes, arg...) do { if (0) qDebug(mes, ##arg); } while (0)
#define logMessagesAvailable() 0
#endif

#ifdef LOGV
#define mLogv(mes, arg...) __debug("%s: " mes, ecl::__dbg_classes_logv, this, __PRETTY_FUNCTION__, ##arg)
#define logvMessagesAvailable() 1
#else
#define mLogv(mes, arg...) do { if (0) qDebug(mes, ##arg); } while (0)
#define logvMessagesAvailable() 0
#endif

#ifdef DEBUG_TIMING
#define mTime(mes, arg...) __debug("%s: " mes, __PRETTY_FUNCTION__, ##arg)
#else
#define mTime(mes, arg...) do { if (0) qDebug(mes, ##arg); } while (0)
#endif

#endif // DEBUG_H
