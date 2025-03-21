/***********************************************************************
*
* Copyright (c) 2012-2025 Barbara Geller
* Copyright (c) 2012-2025 Ansel Sermersheim
*
* Copyright (c) 2015 The Qt Company Ltd.
* Copyright (c) 2012-2016 Digia Plc and/or its subsidiary(-ies).
* Copyright (c) 2008-2012 Nokia Corporation and/or its subsidiary(-ies).
*
* This file is part of CopperSpice.
*
* CopperSpice is free software. You can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License
* version 2.1 as published by the Free Software Foundation.
*
* CopperSpice is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
* https://www.gnu.org/licenses/
*
***********************************************************************/

#include <qtimerinfo_unix_p.h>

#include <qcoreapplication.h>
#include <qdebug.h>
#include <qelapsedtimer.h>
#include <qthread.h>

#include <qabstracteventdispatcher_p.h>
#include <qcore_unix_p.h>

#include <sys/times.h>

Q_CORE_EXPORT bool qt_disable_lowpriority_timers = false;

QTimerInfoList::QTimerInfoList()
{
#if (_POSIX_MONOTONIC_CLOCK-0 <= 0) && ! defined(Q_OS_DARWIN) && ! defined(Q_OS_NACL)

   if (! QElapsedTimer::isMonotonic()) {
      // not using monotonic timers, initialize the timeChanged() machinery
      previousTime = qt_gettime();

      tms unused;
      previousTicks = times(&unused);

      ticksPerSecond = sysconf(_SC_CLK_TCK);
      msPerTick = 1000 / ticksPerSecond;

   } else {
      // detected monotonic timers
      previousTime.tv_sec = previousTime.tv_nsec = 0;
      previousTicks = 0;
      ticksPerSecond = 0;
      msPerTick = 0;
   }

#endif

   firstTimerInfo = nullptr;
}

timespec QTimerInfoList::updateCurrentTime()
{
   return (currentTime = qt_gettime());
}

#if (_POSIX_MONOTONIC_CLOCK-0 <= 0) && ! defined(Q_OS_DARWIN)

timespec qAbsTimespec(const timespec &t)
{
   timespec tmp = t;

   if (tmp.tv_sec < 0) {
      tmp.tv_sec   = -tmp.tv_sec - 1;
      tmp.tv_nsec -= 1000000000;
   }

   if (tmp.tv_sec == 0 && tmp.tv_nsec < 0) {
      tmp.tv_nsec = -tmp.tv_nsec;
   }

   return normalizedTimespec(tmp);
}

bool QTimerInfoList::timeChanged(timespec *delta)
{
#ifdef Q_OS_NACL
   (void) delta
   return false; // Calling "times" crashes.
#endif

   struct tms unused;
   clock_t currentTicks = times(&unused);

   clock_t elapsedTicks = currentTicks - previousTicks;
   timespec elapsedTime = currentTime - previousTime;

   timespec elapsedTimeTicks;
   elapsedTimeTicks.tv_sec = elapsedTicks / ticksPerSecond;
   elapsedTimeTicks.tv_nsec = (((elapsedTicks * 1000) / ticksPerSecond) % 1000) * 1000 * 1000;

   timespec dummy;

   if (! delta) {
      delta = &dummy;
   }

   *delta = elapsedTime - elapsedTimeTicks;

   previousTicks = currentTicks;
   previousTime = currentTime;

   // If tick drift is more than 10% off compared to realtime, we assume that the clock has
   // been set. Of course, we have to allow for the tick granularity as well.
   timespec tickGranularity;
   tickGranularity.tv_sec  = 0;
   tickGranularity.tv_nsec = msPerTick * 1000 * 1000;

   return elapsedTimeTicks < ((qAbsTimespec(*delta) - tickGranularity) * 10);
}

void QTimerInfoList::timerRepair(const timespec &diff)
{
   // repair all timers
   for (int i = 0; i < size(); ++i) {
      QTimerInfo_Unix *t = at(i);
      t->timeout = t->timeout + diff;
   }
}

void QTimerInfoList::repairTimersIfNeeded()
{
   if (QElapsedTimer::isMonotonic()) {
      return;
   }

   timespec delta;

   if (timeChanged(&delta)) {
      timerRepair(delta);
   }
}

#else

void QTimerInfoList::repairTimersIfNeeded()
{
}

#endif

// insert timer info into list
void QTimerInfoList::timerInsert(QTimerInfo_Unix *ti)
{
   int index = size();

   while (index--) {
      const QTimerInfo_Unix *const t = at(index);

      if (! (ti->timeout < t->timeout)) {
         break;
      }
   }

   insert(index + 1, ti);
}

inline timespec &operator+=(timespec &t1, int ms)
{
   t1.tv_sec  += ms / 1000;
   t1.tv_nsec += ms % 1000 * 1000 * 1000;

   return normalizedTimespec(t1);
}

inline timeval &operator+=(timeval &t1, int ms)
{
   t1.tv_sec  += ms / 1000;
   t1.tv_usec += ms % 1000 * 1000;

   return normalizedTimeval(t1);
}

inline timespec operator+(const timespec &t1, int ms)
{
   timespec t2 = t1;
   return t2 += ms;
}

inline timeval operator-(const timespec &t1, const timeval &t2)
{
   timeval retval;

   retval.tv_sec  = t1.tv_sec - t2.tv_sec;
   retval.tv_usec = (t1.tv_nsec / 1000) - t2.tv_usec;

   return normalizedTimeval(retval);
}

inline timeval operator-(const timeval &t1, const timespec &t2)
{
   timeval retval = t1;

   retval.tv_sec  -= t2.tv_sec;
   retval.tv_usec -= t2.tv_nsec / 1000;

   return normalizedTimeval(retval);
}

inline bool operator<(const timeval &t1, const timespec &t2)
{
   return t1.tv_sec < t2.tv_sec || (t1.tv_sec == t2.tv_sec && t1.tv_usec * 1000 < t2.tv_nsec);
}

inline bool operator<(const timespec &t1, const timeval &t2)
{
   return t1.tv_sec < t2.tv_sec || (t1.tv_sec == t2.tv_sec && t1.tv_nsec < t2.tv_usec * 1000);
}

static timespec roundToMillisecond(timespec val)
{
   // always round up
   // worst case scenario is that the first trigger of a 1-ms timer is 0.999 ms late

   int ns = val.tv_nsec % (1000 * 1000);
   val.tv_nsec += 1000 * 1000 - ns;

   return normalizedTimespec(val);
}

#if defined(CS_SHOW_DEBUG_CORE)
QDebug operator<<(QDebug s, timeval tv)
{
   QDebugStateSaver saver(s);
   s.nospace() << tv.tv_sec << "." << qSetFieldWidth(6) << qSetPadChar(QChar(48)) << tv.tv_usec << reset;

   return s;
}

QDebug operator<<(QDebug s, timespec ts)
{
   QDebugStateSaver saver(s);
   s.nospace() << ts.tv_sec << "." << qSetFieldWidth(9) << qSetPadChar(QChar(48)) << ts.tv_nsec << reset;

   return s;
}

QDebug operator<<(QDebug s, Qt::TimerType t)
{
   QDebugStateSaver saver(s);
   s << (t == Qt::PreciseTimer ? "P" : t == Qt::CoarseTimer ? "C" : "VC");

   return s;
}
#endif

static void calculateCoarseTimerTimeout(QTimerInfo_Unix *t, timespec currentTime)
{
   // The coarse timer works like this:
   //  - interval under 40 ms: round to even
   //  - between 40 and 99 ms: round to multiple of 4
   //  - otherwise: try to wake up at a multiple of 25 ms, with a maximum error of 5%
   //
   // We try to wake up at the following second-fraction, in order of preference:
   //    0 ms
   //  500 ms
   //  250 ms or 750 ms
   //  200, 400, 600, 800 ms
   //  other multiples of 100
   //  other multiples of 50
   //  other multiples of 25
   //
   // The objective is to make most timers wake up at the same time, thereby reducing CPU wakeups.

   uint interval = uint(t->interval);
   uint msec     = uint(t->timeout.tv_nsec) / 1000 / 1000;
   Q_ASSERT(interval >= 20);

   // Calculate how much we can round and still keep within 5% error
   uint absMaxRounding = interval / 20;

   if (interval < 100 && interval != 25 && interval != 50 && interval != 75) {
      // special mode for timers of less than 100 ms

      if (interval < 50) {
         // round to even
         // round towards multiples of 50 ms
         bool roundUp = (msec % 50) >= 25;
         msec >>= 1;
         msec |= uint(roundUp);
         msec <<= 1;

      } else {
         // round to multiple of 4
         // round towards multiples of 100 ms
         bool roundUp = (msec % 100) >= 50;
         msec >>= 2;
         msec |= uint(roundUp);
         msec <<= 2;
      }

   } else {
      uint min = qMax(0, static_cast<int>(msec) - static_cast<int>(absMaxRounding));
      uint max = qMin(1000u, msec + absMaxRounding);

      // find the boundary that we want, according to the rules above extra rules:
      // 1) whatever the interval, we'll take any round-to-the-second timeout
      if (min == 0) {
         msec = 0;
         goto recalculate;

      } else if (max == 1000) {
         msec = 1000;
         goto recalculate;
      }

      uint wantedBoundaryMultiple;

      // 2) if the interval is a multiple of 500 ms and > 5000 ms, we'll always round
      //    towards a round-to-the-second
      // 3) if the interval is a multiple of 500 ms, we'll round towards the nearest
      //    multiple of 500 ms
      if ((interval % 500) == 0) {
         if (interval >= 5000) {
            msec = msec >= 500 ? max : min;
            goto recalculate;

         } else {
            wantedBoundaryMultiple = 500;
         }

      } else if ((interval % 50) == 0) {
         // 4) same for multiples of 250, 200, 100, 50
         uint mult50 = interval / 50;

         if ((mult50 % 4) == 0) {
            // multiple of 200
            wantedBoundaryMultiple = 200;
         } else if ((mult50 % 2) == 0) {
            // multiple of 100
            wantedBoundaryMultiple = 100;
         } else if ((mult50 % 5) == 0) {
            // multiple of 250
            wantedBoundaryMultiple = 250;
         } else {
            // multiple of 50
            wantedBoundaryMultiple = 50;
         }

      } else {
         wantedBoundaryMultiple = 25;
      }

      uint base = msec / wantedBoundaryMultiple * wantedBoundaryMultiple;
      uint middlepoint = base + wantedBoundaryMultiple / 2;

      if (msec < middlepoint) {
         msec = qMax(base, min);
      } else {
         msec = qMin(base + wantedBoundaryMultiple, max);
      }
   }

recalculate:

   if (msec == 1000u) {
      ++t->timeout.tv_sec;
      t->timeout.tv_nsec = 0;
   } else {
      t->timeout.tv_nsec = msec * 1000 * 1000;
   }

   if (t->timeout < currentTime) {
      t->timeout += interval;
   }
}

static void calculateNextTimeout(QTimerInfo_Unix *t, timespec currentTime)
{
   switch (t->timerType) {
      case Qt::PreciseTimer:
      case Qt::CoarseTimer:
         t->timeout += t->interval;

         if (t->timeout < currentTime) {
            t->timeout = currentTime;
            t->timeout += t->interval;
         }

#if defined(CS_SHOW_DEBUG_CORE)
         t->expected += t->interval;

         if (t->expected < currentTime) {
            t->expected.tv_sec  = currentTime.tv_sec;
            t->expected.tv_usec = currentTime.tv_nsec / 1000;
            t->expected += t->interval;
         }
#endif

         if (t->timerType == Qt::CoarseTimer) {
            calculateCoarseTimerTimeout(t, currentTime);
         }

         return;

      case Qt::VeryCoarseTimer:
         // we do not need to take care of the microsecond component of t->interval
         t->timeout.tv_sec += t->interval;

         if (t->timeout.tv_sec <= currentTime.tv_sec) {
            t->timeout.tv_sec = currentTime.tv_sec + t->interval;
         }

#if defined(CS_SHOW_DEBUG_CORE)
         t->expected.tv_sec += t->interval;

         if (t->expected.tv_sec <= currentTime.tv_sec) {
            t->expected.tv_sec = currentTime.tv_sec + t->interval;
         }
#endif

         return;
   }

#if defined(CS_SHOW_DEBUG_CORE)
   if (t->timerType != Qt::PreciseTimer) {
      qDebug() << "calculateNextTimeout() timer =" << t->timerType << hex << t->id << dec
            << "interval =" << t->interval << "originally expected at =" << t->expected
            << "time out =" << t->timeout << "late by =" << (t->timeout - t->expected);
   }
#endif
}

bool QTimerInfoList::timerWait(timespec &tm)
{
   timespec newCurrentTime = updateCurrentTime();
   repairTimersIfNeeded();

   // find first waiting timer not already active
   QTimerInfo_Unix *t = nullptr;

   for (QTimerInfoList::const_iterator it = constBegin(); it != constEnd(); ++it) {
      if (! (*it)->activateRef) {
         t = *it;
         break;
      }
   }

   if (! t) {
      return false;
   }

   if (newCurrentTime < t->timeout) {
      // time to wait
      tm = roundToMillisecond(t->timeout - newCurrentTime);

   } else {
      // no time to wait
      tm.tv_sec  = 0;
      tm.tv_nsec = 0;
   }

   return true;
}

int QTimerInfoList::timerRemainingTime(int timerId)
{
   timespec newCurrentTime = updateCurrentTime();
   repairTimersIfNeeded();
   timespec tm = {0, 0};

   for (int i = 0; i < count(); ++i) {
      QTimerInfo_Unix *t = at(i);

      if (t->id == timerId) {
         if (newCurrentTime < t->timeout) {
            // time to wait
            tm = roundToMillisecond(t->timeout - newCurrentTime);
            return tm.tv_sec * 1000 + tm.tv_nsec / 1000 / 1000;
         } else {
            return 0;
         }
      }
   }

#if defined(CS_SHOW_DEBUG_CORE)
   qDebug("QTimerInfoList::timerRemainingTime() Timer id %d was not found", timerId);
#endif

   return -1;
}

void QTimerInfoList::registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object)
{
   QTimerInfo_Unix *t = new QTimerInfo_Unix;
   t->id          = timerId;
   t->interval    = interval;
   t->timerType   = timerType;
   t->obj         = object;
   t->activateRef = nullptr;

   timespec expected = updateCurrentTime() + interval;

   switch (timerType) {
      case Qt::PreciseTimer:
         // high precision timer is based on millisecond precision
         // so no adjustment is necessary
         t->timeout = expected;
         break;

      case Qt::CoarseTimer:

         // this timer has up to 5% coarseness so the boundaries are 20 ms and 20 s
         // below 20 ms, 5% inaccuracy is below 1 ms, so we convert to high precision
         // above 20 s, 5% inaccuracy is above 1 s, so we convert to VeryCoarseTimer

         if (interval >= 20000) {
            t->timerType = Qt::VeryCoarseTimer;
            // do nothing

         } else {
            t->timeout = expected;

            if (interval <= 20) {
               t->timerType = Qt::PreciseTimer;
               // no adjustment is necessary

            } else if (interval <= 20000) {
               calculateCoarseTimerTimeout(t, currentTime);
            }

            break;
         }

         [[fallthrough]];

      case Qt::VeryCoarseTimer:
         // the very coarse timer is based on full second precision,
         // so we keep the interval in seconds (round to closest second)
         t->interval /= 500;
         t->interval += 1;
         t->interval >>= 1;
         t->timeout.tv_sec = currentTime.tv_sec + t->interval;
         t->timeout.tv_nsec = 0;

         // if we're past the half-second mark, increase the timeout again
         if (currentTime.tv_nsec > 500 * 1000 * 1000) {
            ++t->timeout.tv_sec;
         }
   }

   timerInsert(t);

#if defined(CS_SHOW_DEBUG_CORE)
   t->expected.tv_sec  = expected.tv_sec;
   t->expected.tv_usec = expected.tv_nsec / 1000;
   t->cumulativeError = 0;
   t->count = 0;

   if (t->timerType != Qt::PreciseTimer) {
      qDebug() << "QTimerInfoList::registerTimer() timer =" << t->timerType << hex << t->id << dec
            << "interval =" << t->interval << "expected at =" << t->expected << "timeout =" << t->timeout;
   }
#endif
}

bool QTimerInfoList::unregisterTimer(int timerId)
{
   // set timer inactive
   for (int i = 0; i < count(); ++i) {
      QTimerInfo_Unix *t = at(i);

      if (t->id == timerId) {
         // found it
         removeAt(i);

         if (t == firstTimerInfo) {
            firstTimerInfo = nullptr;
         }

         if (t->activateRef) {
            *(t->activateRef) = nullptr;
         }

         delete t;
         return true;
      }
   }

   // id not found
   return false;
}

bool QTimerInfoList::unregisterTimers(QObject *object)
{
   if (isEmpty()) {
      return false;
   }

   for (int i = 0; i < count(); ++i) {
      QTimerInfo_Unix *t = at(i);

      if (t->obj == object) {
         // object found
         removeAt(i);

         if (t == firstTimerInfo) {
            firstTimerInfo = nullptr;
         }

         if (t->activateRef) {
            *(t->activateRef) = nullptr;
         }

         delete t;
         // move back one so that we do not skip the new current item
         --i;
      }
   }

   return true;
}

QList<QTimerInfo> QTimerInfoList::registeredTimers(QObject *object) const
{
   QList<QTimerInfo> list;

   for (int i = 0; i < count(); ++i) {
      const QTimerInfo_Unix *const t = at(i);

      if (t->obj == object) {
         list << QTimerInfo(t->id,
               (t->timerType == Qt::VeryCoarseTimer ? t->interval * 1000 : t->interval), t->timerType);
      }
   }

   return list;
}

int QTimerInfoList::activateTimers()
{
   if (qt_disable_lowpriority_timers || isEmpty()) {
      return 0;   // nothing to do
   }

   int n_act      = 0;
   int maxCount   = 0;
   firstTimerInfo = nullptr;

   timespec newTime = updateCurrentTime();
   repairTimersIfNeeded();

   // Find out how many timer have expired
   for (QTimerInfoList::const_iterator it = constBegin(); it != constEnd(); ++it) {
      if (newTime < (*it)->timeout) {
         break;
      }

      ++maxCount;
   }

   // fire the timers
   while (maxCount--) {
      if (isEmpty()) {
         break;
      }

      QTimerInfo_Unix *currentTimerInfo = first();

      if (newTime < currentTimerInfo->timeout) {
         // no timer has expired
         break;
      }

      if (! firstTimerInfo) {
         firstTimerInfo = currentTimerInfo;

      } else if (firstTimerInfo == currentTimerInfo) {
         // avoid sending the same timer multiple times
         break;

      } else if (currentTimerInfo->interval <  firstTimerInfo->interval
            || currentTimerInfo->interval == firstTimerInfo->interval) {
         firstTimerInfo = currentTimerInfo;
      }

      // remove from list
      removeFirst();

#if defined(CS_SHOW_DEBUG_CORE)
      float diff;

      if (newTime < currentTimerInfo->expected) {
         // early
         timeval early = currentTimerInfo->expected - newTime;
         diff = -(early.tv_sec + early.tv_usec / 1000000.0);

      } else {
         timeval late = newTime - currentTimerInfo->expected;
         diff = late.tv_sec + late.tv_usec / 1000000.0;
      }

      currentTimerInfo->cumulativeError += diff;
      ++currentTimerInfo->count;

      if (currentTimerInfo->timerType != Qt::PreciseTimer) {
         qDebug() << "QTimerInfoList::activateTimers() timer =" << currentTimerInfo->timerType << hex << currentTimerInfo->id << dec
               << "interval =" << currentTimerInfo->interval << "new time =" << newTime
               << "expected at =" << currentTimerInfo->expected << "scheduled at =" << currentTimerInfo->timeout
               << "late by =" << diff << "activation" << currentTimerInfo->count
               << "error =" << (currentTimerInfo->cumulativeError / currentTimerInfo->count);
      }
#endif

      // determine next timeout time
      calculateNextTimeout(currentTimerInfo, newTime);

      // reinsert timer
      timerInsert(currentTimerInfo);

      if (currentTimerInfo->interval > 0) {
         n_act++;
      }

      if (! currentTimerInfo->activateRef) {
         // send event, but don't allow it to recurse
         currentTimerInfo->activateRef = &currentTimerInfo;

         QTimerEvent e(currentTimerInfo->id);
         QCoreApplication::sendEvent(currentTimerInfo->obj, &e);

         if (currentTimerInfo) {
            currentTimerInfo->activateRef = nullptr;
         }
      }
   }

   firstTimerInfo = nullptr;

   return n_act;
}
