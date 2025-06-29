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

#include <qdatetimeedit_p.h>

#include <qabstractspinbox.h>
#include <qapplication.h>
#include <qdatetimeedit.h>
#include <qdebug.h>
#include <qdesktopwidget.h>
#include <qevent.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlocale.h>
#include <qpainter.h>
#include <qset.h>
#include <qstyle.h>
#include <qtimezone.h>

#include <qlineedit_p.h>

#ifndef QT_NO_DATETIMEEDIT

QDateTimeEdit::QDateTimeEdit(QWidget *parent)
   : QAbstractSpinBox(*new QDateTimeEditPrivate, parent)
{
   Q_D(QDateTimeEdit);
   d->init(QDateTime(QDATETIME_DATE_DEFAULT, QDATETIME_TIME_MIN));
}

QDateTimeEdit::QDateTimeEdit(const QDateTime &datetime, QWidget *parent)
   : QAbstractSpinBox(*new QDateTimeEditPrivate, parent)
{
   Q_D(QDateTimeEdit);
   d->init(datetime.isValid() ? datetime : QDateTime(QDATETIME_DATE_DEFAULT, QDATETIME_TIME_MIN));
}

QDateTimeEdit::QDateTimeEdit(const QDate &date, QWidget *parent)
   : QAbstractSpinBox(*new QDateTimeEditPrivate, parent)
{
   Q_D(QDateTimeEdit);
   d->init(date.isValid() ? date : QDATETIME_DATE_DEFAULT);
}

QDateTimeEdit::QDateTimeEdit(const QTime &time, QWidget *parent)
   : QAbstractSpinBox(*new QDateTimeEditPrivate, parent)
{
   Q_D(QDateTimeEdit);
   d->init(time.isValid() ? time : QDATETIME_TIME_MIN);
}

QDateTimeEdit::QDateTimeEdit(const QVariant &var, QVariant::Type parserType, QWidget *parent)
   : QAbstractSpinBox(*new QDateTimeEditPrivate, parent)
{
   Q_D(QDateTimeEdit);
   d->parserType = parserType;
   d->init(var);
}

QDateTimeEdit::~QDateTimeEdit()
{
}

QDateTime QDateTimeEdit::dateTime() const
{
   Q_D(const QDateTimeEdit);
   return d->m_spinBoxValue.toDateTime();
}

void QDateTimeEdit::setDateTime(const QDateTime &datetime)
{
   Q_D(QDateTimeEdit);

   if (datetime.isValid()) {
      d->clearCache();

      if (! (d->sections & DateSections_Mask)) {
         setDateRange(datetime.date(), datetime.date());
      }

      d->setValue(QDateTime(datetime.date(), datetime.time(), d->m_timeZone), EmitIfChanged);
   }
}

QDate QDateTimeEdit::date() const
{
   Q_D(const QDateTimeEdit);
   return d->m_spinBoxValue.toDate();
}

void QDateTimeEdit::setDate(const QDate &date)
{
   Q_D(QDateTimeEdit);

   if (date.isValid()) {
      if (! (d->sections & DateSections_Mask)) {
         setDateRange(date, date);
      }

      d->clearCache();
      d->setValue(QDateTime(date, d->m_spinBoxValue.toTime(), d->m_timeZone), EmitIfChanged);
      d->updateTimeZone();
   }
}

QTime QDateTimeEdit::time() const
{
   Q_D(const QDateTimeEdit);
   return d->m_spinBoxValue.toTime();
}

void QDateTimeEdit::setTime(const QTime &time)
{
   Q_D(QDateTimeEdit);

   if (time.isValid()) {
      d->clearCache();
      d->setValue(QDateTime(d->m_spinBoxValue.toDate(), time, d->m_timeZone), EmitIfChanged);
   }
}

QDateTime QDateTimeEdit::minimumDateTime() const
{
   Q_D(const QDateTimeEdit);
   return d->minimum.toDateTime();
}

void QDateTimeEdit::clearMinimumDateTime()
{
   setMinimumDateTime(QDateTime(QDATETIME_GREGORIAN_DATE, QDATETIME_TIME_MIN));
}

void QDateTimeEdit::setMinimumDateTime(const QDateTime &dt)
{
   Q_D(QDateTimeEdit);

   if (dt.isValid() && dt.date() >= QDATETIME_DATE_MIN) {
      const QDateTime m   = dt.toTimeZone(d->m_timeZone);
      const QDateTime max = d->maximum.toDateTime();

      d->setRange(m, (max > m ? max : m));
   }
}

QDateTime QDateTimeEdit::maximumDateTime() const
{
   Q_D(const QDateTimeEdit);
   return d->maximum.toDateTime();
}

void QDateTimeEdit::clearMaximumDateTime()
{
   setMaximumDateTime(QDATETIME_DATETIME_MAX);
}

void QDateTimeEdit::setMaximumDateTime(const QDateTime &dt)
{
   Q_D(QDateTimeEdit);

   if (dt.isValid() && dt.date() <= QDATETIME_DATE_MAX) {
      const QDateTime localTime = dt.toTimeZone(d->m_timeZone);
      const QDateTime minimum   = d->minimum.toDateTime();

      if (minimum < localTime) {
         d->setRange(minimum, localTime);

      } else {
         d->setRange(localTime, localTime);
      }
   }
}

void QDateTimeEdit::setDateTimeRange(const QDateTime &min, const QDateTime &max)
{
   Q_D(QDateTimeEdit);

   const QDateTime minimum = min.toTimeZone(d->m_timeZone);
   const QDateTime maximum = max.toTimeZone(d->m_timeZone);

   if (min > max) {
      d->setRange(minimum, minimum);

   } else {
       d->setRange(minimum, maximum);
   }
}

QDate QDateTimeEdit::minimumDate() const
{
   Q_D(const QDateTimeEdit);
   return d->minimum.toDate();
}

void QDateTimeEdit::setMinimumDate(const QDate &min)
{
   Q_D(QDateTimeEdit);

   if (min.isValid() && min >= QDATETIME_DATE_MIN) {
      setMinimumDateTime(QDateTime(min, d->minimum.toTime(), d->m_timeZone));
   }
}

void QDateTimeEdit::clearMinimumDate()
{
   setMinimumDate(QDATETIME_GREGORIAN_DATE);
}

QDate QDateTimeEdit::maximumDate() const
{
   Q_D(const QDateTimeEdit);
   return d->maximum.toDate();
}

void QDateTimeEdit::setMaximumDate(const QDate &max)
{
   Q_D(QDateTimeEdit);

   if (max.isValid()) {
      setMaximumDateTime(QDateTime(max, d->maximum.toTime(), d->m_timeZone));
   }
}

void QDateTimeEdit::clearMaximumDate()
{
   setMaximumDate(QDATETIME_DATE_MAX);
}

QTime QDateTimeEdit::minimumTime() const
{
   Q_D(const QDateTimeEdit);
   return d->minimum.toTime();
}

void QDateTimeEdit::setMinimumTime(const QTime &min)
{
   Q_D(QDateTimeEdit);

   if (min.isValid()) {
      const QDateTime m(d->minimum.toDate(), min, d->m_timeZone);
      setMinimumDateTime(m);
   }
}

void QDateTimeEdit::clearMinimumTime()
{
   setMinimumTime(QDATETIME_TIME_MIN);
}

QTime QDateTimeEdit::maximumTime() const
{
   Q_D(const QDateTimeEdit);
   return d->maximum.toTime();
}

void QDateTimeEdit::setMaximumTime(const QTime &max)
{
   Q_D(QDateTimeEdit);

   if (max.isValid()) {
      const QDateTime m(d->maximum.toDate(), max);
      setMaximumDateTime(m);
   }
}

void QDateTimeEdit::clearMaximumTime()
{
   setMaximumTime(QDATETIME_TIME_MAX);
}

void QDateTimeEdit::setDateRange(const QDate &min, const QDate &max)
{
   Q_D(QDateTimeEdit);

   if (min.isValid() && max.isValid()) {
      setDateTimeRange(QDateTime(min, d->minimum.toTime(), d->m_timeZone),
         QDateTime(max, d->maximum.toTime(), d->m_timeZone));
   }
}

void QDateTimeEdit::setTimeRange(const QTime &min, const QTime &max)
{
   Q_D(QDateTimeEdit);

   if (min.isValid() && max.isValid()) {
      setDateTimeRange(QDateTime(d->minimum.toDate(), min, d->m_timeZone),
         QDateTime(d->maximum.toDate(), max, d->m_timeZone));
   }
}

QDateTimeEdit::Sections QDateTimeEdit::displayedSections() const
{
   Q_D(const QDateTimeEdit);
   return d->sections;
}

QDateTimeEdit::Section QDateTimeEdit::currentSection() const
{
   Q_D(const QDateTimeEdit);

#ifdef QT_KEYPAD_NAVIGATION
   if (QApplication::keypadNavigationEnabled() && d->focusOnButton) {
      return NoSection;
   }
#endif

   return d->convertToPublic(d->sectionType(d->currentSectionIndex));
}

void QDateTimeEdit::setCurrentSection(Section section)
{
   Q_D(QDateTimeEdit);

   if (section == NoSection || !(section & d->sections)) {
      return;
   }

   d->updateCache(d->m_spinBoxValue, d->displayText());
   const int size = d->sectionNodes.size();
   int index      = d->currentSectionIndex + 1;

   for (int i = 0; i < 2; ++i) {
      while (index < size) {
         if (d->convertToPublic(d->sectionType(index)) == section) {
            d->edit->setCursorPosition(d->sectionPos(index));

            return;
         }

         ++index;
      }

      index = 0;
   }
}

QDateTimeEdit::Section QDateTimeEdit::sectionAt(int index) const
{
   Q_D(const QDateTimeEdit);
   if (index < 0 || index >= d->sectionNodes.size()) {
      return NoSection;
   }
   return d->convertToPublic(d->sectionType(index));
}

int QDateTimeEdit::sectionCount() const
{
   Q_D(const QDateTimeEdit);
   return d->sectionNodes.size();
}

int QDateTimeEdit::currentSectionIndex() const
{
   Q_D(const QDateTimeEdit);
   return d->currentSectionIndex;
}

void QDateTimeEdit::setCurrentSectionIndex(int index)
{
   Q_D(QDateTimeEdit);
   if (index < 0 || index >= d->sectionNodes.size()) {
      return;
   }
   d->edit->setCursorPosition(d->sectionPos(index));
}

QCalendarWidget *QDateTimeEdit::calendarWidget() const
{
   Q_D(const QDateTimeEdit);

   if (!d->calendarPopup || !(d->sections & QDateTimeParser::DateSectionMask)) {
      return nullptr;
   }

   if (!d->monthCalendar) {
      const_cast<QDateTimeEditPrivate *>(d)->initCalendarPopup();
   }

   return d->monthCalendar->calendarWidget();
}

void QDateTimeEdit::setCalendarWidget(QCalendarWidget *calendarWidget)
{
   Q_D(QDateTimeEdit);

   if (! calendarWidget) {
      qWarning("QDateTimeEdit::setCalendarWidget() Unable to set the calendar widget to an invalid value (nullptr)");
      return;
   }

   if (! d->calendarPopup) {
      qWarning("QDateTimeEdit::setCalendarWidget() Calendar Popup is disabled");
      return;
   }

   if (!(d->display & QDateTimeParser::DateSectionMask)) {
      qWarning("QDateTimeEdit::setCalendarWidget() No calendar date section was specified");
      return;
   }

   d->initCalendarPopup(calendarWidget);
}

void QDateTimeEdit::setSelectedSection(Section section)
{
   Q_D(QDateTimeEdit);

   if (section == NoSection) {
      d->edit->setSelection(d->edit->cursorPosition(), 0);
   } else if (section & d->sections) {
      if (currentSection() != section) {
         setCurrentSection(section);
      }
      d->setSelected(d->currentSectionIndex);
   }
}

QString QDateTimeEdit::sectionText(Section section) const
{
   Q_D(const QDateTimeEdit);

   if (section == QDateTimeEdit::NoSection || !(section & d->sections)) {
      return QString();
   }

   d->updateCache(d->m_spinBoxValue, d->displayText());
   const int sectionIndex = d->absoluteIndex(section, 0);
   return d->sectionText(sectionIndex);
}

QString QDateTimeEdit::displayFormat() const
{
   Q_D(const QDateTimeEdit);
   return isRightToLeft() ? d->unreversedFormat : d->displayFormat;
}

template <typename R>
static inline R reverse(const R &data)
{
   R retval;

   for (int i = data.size() - 1; i >= 0; --i) {
      retval.append(data.at(i));
   }

   return retval;
}

void QDateTimeEdit::setDisplayFormat(const QString &format)
{
   Q_D(QDateTimeEdit);

   if (d->parseFormat(format)) {
      d->unreversedFormat.clear();

      if (isRightToLeft()) {
         d->unreversedFormat = format;
         d->displayFormat.clear();

         for (int i = d->sectionNodes.size() - 1; i >= 0; --i) {
            d->displayFormat += d->separators.at(i + 1);
            d->displayFormat += d->sectionNode(i).format();
         }

         d->displayFormat += d->separators.at(0);
         d->separators   = reverse(d->separators);
         d->sectionNodes = reverse(d->sectionNodes);
      }

      d->formatExplicitlySet = true;
      d->sections = d->convertSections(d->display);
      d->clearCache();

      d->currentSectionIndex = qMin(d->currentSectionIndex, d->sectionNodes.size() - 1);
      const bool timeShown = (d->sections & TimeSections_Mask);
      const bool dateShown = (d->sections & DateSections_Mask);

      Q_ASSERT(dateShown || timeShown);

      if (timeShown && ! dateShown) {
         QTime time = d->m_spinBoxValue.toTime();
         setDateRange(d->m_spinBoxValue.toDate(), d->m_spinBoxValue.toDate());

         if (d->minimum.toTime() >= d->maximum.toTime()) {
            setTimeRange(QDATETIME_TIME_MIN, QDATETIME_TIME_MAX);

            // if the time range became invalid during the adjustment the time would have been reset
            setTime(time);
         }

      } else if (dateShown && ! timeShown) {
         setTimeRange(QDATETIME_TIME_MIN, QDATETIME_TIME_MAX);
         d->m_spinBoxValue= QDateTime(d->m_spinBoxValue.toDate(), QTime(), d->m_timeZone);
      }

      d->updateEdit();
      d->_q_editorCursorPositionChanged(-1, 0);
   }
}

bool QDateTimeEdit::calendarPopup() const
{
   Q_D(const QDateTimeEdit);
   return d->calendarPopup;
}

void QDateTimeEdit::setCalendarPopup(bool enable)
{
   Q_D(QDateTimeEdit);

   if (enable == d->calendarPopup) {
      return;
   }

   setAttribute(Qt::WA_MacShowFocusRect, !enable);
   d->calendarPopup = enable;

#ifdef QT_KEYPAD_NAVIGATION
   if (! enable) {
      d->focusOnButton = false;
   }
#endif

   d->updateEditFieldGeometry();
   update();
}

QTimeZone QDateTimeEdit::timeZone() const
{
   Q_D(const QDateTimeEdit);

   return d->m_timeZone;
}

void QDateTimeEdit::setTimeZone(QTimeZone tz)
{
   Q_D(QDateTimeEdit);

   if (tz != d->m_timeZone) {
      d->m_timeZone = tz;
      d->updateTimeZone();
   }
}

QSize QDateTimeEdit::sizeHint() const
{
   Q_D(const QDateTimeEdit);

   if (d->cachedSizeHint.isEmpty()) {
      ensurePolished();

      const QFontMetrics fm(fontMetrics());
      int h = d->edit->sizeHint().height();
      int w = 0;
      QString s;

      s = d->textFromValue(d->minimum) + "   ";
      w = qMax(w, fm.width(s));
      s = d->textFromValue(d->maximum) + "   ";
      w = qMax(w, fm.width(s));

      if (d->specialValueText.size()) {
         s = d->specialValueText;
         w = qMax(w, fm.width(s));
      }

      w += 2; // cursor blinking space

      QSize hint(w, h);

#ifdef Q_OS_DARWIN
      if (d->calendarPopupEnabled()) {
         QStyleOptionComboBox opt;
         d->cachedSizeHint = style()->sizeFromContents(QStyle::CT_ComboBox, &opt, hint, this);
      } else
#endif
      {
         QStyleOptionSpinBox opt;
         initStyleOption(&opt);

         d->cachedSizeHint = style()->sizeFromContents(QStyle::CT_SpinBox, &opt, hint, this)
            .expandedTo(QApplication::globalStrut());
      }

      d->cachedMinimumSizeHint = d->cachedSizeHint;
      // essentially make minimumSizeHint return the same as sizeHint for datetimeedits
   }
   return d->cachedSizeHint;
}

bool QDateTimeEdit::event(QEvent *event)
{
   Q_D(QDateTimeEdit);

   switch (event->type()) {
      case QEvent::ApplicationLayoutDirectionChange: {
         const bool was = d->formatExplicitlySet;
         const QString oldFormat = d->displayFormat;
         d->displayFormat.clear();
         setDisplayFormat(oldFormat);
         d->formatExplicitlySet = was;
         break;
      }

      case QEvent::LocaleChange:
         d->updateEdit();
         break;

      case QEvent::StyleChange:

#ifdef Q_OS_DARWIN
      case QEvent::MacSizeChange:
#endif
         d->setLayoutItemMargins(QStyle::SE_DateTimeEditLayoutItem);
         break;

      default:
         break;
   }
   return QAbstractSpinBox::event(event);
}

void QDateTimeEdit::clear()
{
   Q_D(QDateTimeEdit);
   d->clearSection(d->currentSectionIndex);
}

void QDateTimeEdit::keyPressEvent(QKeyEvent *event)
{
   Q_D(QDateTimeEdit);
   int oldCurrent = d->currentSectionIndex;
   bool select = true;
   bool inserted = false;

   switch (event->key()) {
#ifdef QT_KEYPAD_NAVIGATION
      case Qt::Key_NumberSign:    //shortcut to popup calendar
         if (QApplication::keypadNavigationEnabled() && d->calendarPopupEnabled()) {
            d->initCalendarPopup();
            d->positionCalendarPopup();
            d->monthCalendar->show();
            return;
         }
         break;

      case Qt::Key_Select:
         if (QApplication::keypadNavigationEnabled()) {
            if (hasEditFocus()) {
               if (d->focusOnButton) {
                  d->initCalendarPopup();
                  d->positionCalendarPopup();
                  d->monthCalendar->show();
                  d->focusOnButton = false;
                  return;
               }
               setEditFocus(false);
               selectAll();

            } else {
               setEditFocus(true);

               //hide cursor
               d->edit->d_func()->setCursorVisible(false);
               d->edit->d_func()->control->setCursorBlinkPeriod(0);
               d->setSelected(0);
            }
         }
         return;
#endif

      case Qt::Key_Enter:
      case Qt::Key_Return:
         d->interpret(AlwaysEmit);
         d->setSelected(d->currentSectionIndex, true);
         event->ignore();
         emit editingFinished();
         return;

      default:
#ifdef QT_KEYPAD_NAVIGATION
         if (QApplication::keypadNavigationEnabled() && !hasEditFocus()
            && !event->text().isEmpty() && event->text().at(0).isLetterOrNumber()) {
            setEditFocus(true);

            // hide cursor
            d->edit->d_func()->setCursorVisible(false);
            d->edit->d_func()->control->setCursorBlinkPeriod(0);
            d->setSelected(0);
            oldCurrent = 0;
         }
#endif
         if (! d->isSeparatorKey(event)) {
            inserted = select = ! event->text().isEmpty() && event->text().at(0).isPrint()
                  && ! (event->modifiers() & ~(Qt::ShiftModifier | Qt::KeypadModifier));
            break;
         }
         [[fallthrough]];

      case Qt::Key_Left:
      case Qt::Key_Right:
         if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) {

#ifdef QT_KEYPAD_NAVIGATION
            if (QApplication::keypadNavigationEnabled() && ! hasEditFocus()
                  || ! QApplication::keypadNavigationEnabled() && ! (event->modifiers() & Qt::ControlModifier)) {
#else
            if (! (event->modifiers() & Qt::ControlModifier)) {
#endif
               select = false;
               break;
            }
         }
         [[fallthrough]];

      case Qt::Key_Backtab:
      case Qt::Key_Tab: {
         event->accept();

         if (d->specialValue()) {
            d->edit->setSelection(d->edit->cursorPosition(), 0);
            return;
         }

         const bool forward = event->key() != Qt::Key_Left && event->key() != Qt::Key_Backtab
            && (event->key() != Qt::Key_Tab || !(event->modifiers() & Qt::ShiftModifier));

#ifdef QT_KEYPAD_NAVIGATION
         int newSection = d->nextPrevSection(d->currentSectionIndex, forward);

         if (QApplication::keypadNavigationEnabled()) {
            if (d->focusOnButton) {
               newSection = forward ? 0 : d->sectionNodes.size() - 1;
               d->focusOnButton = false;
               update();
            } else if (newSection < 0 && select && d->calendarPopupEnabled()) {
               setSelectedSection(NoSection);
               d->focusOnButton = true;
               update();
               return;
            }
         }

         // only allow date/time sections to be selected.
         if (newSection & ~(QDateTimeParser::TimeSectionMask | QDateTimeParser::DateSectionMask)) {
            return;
         }
#endif
         // key tab and backtab will be managed by QWidget::event
         if (event->key() != Qt::Key_Backtab && event->key() != Qt::Key_Tab) {
            focusNextPrevChild(forward);
         }

         return;
      }
   }

   QAbstractSpinBox::keyPressEvent(event);

   if (select && !d->edit->hasSelectedText()) {
      if (inserted && d->sectionAt(d->edit->cursorPosition()) == QDateTimeParser::NoSectionIndex) {
         QString str = d->displayText();
         int pos     = d->edit->cursorPosition();

         if (validate(str, pos) == QValidator::Acceptable && (d->sectionNodes.at(oldCurrent).count != 1
               || d->sectionMaxSize(oldCurrent) == d->sectionSize(oldCurrent)
               || d->skipToNextSection(oldCurrent, d->m_spinBoxValue.toDateTime(), d->sectionText(oldCurrent)))) {

            const int tmp = d->closestSection(d->edit->cursorPosition(), true);

            if (tmp >= 0) {
               d->currentSectionIndex = tmp;
            }
         }
      }

      if (d->currentSectionIndex != oldCurrent) {
         d->setSelected(d->currentSectionIndex);
      }
   }

   if (d->specialValue()) {
      d->edit->setSelection(d->edit->cursorPosition(), 0);
   }
}

#ifndef QT_NO_WHEELEVENT
void QDateTimeEdit::wheelEvent(QWheelEvent *event)
{
   QAbstractSpinBox::wheelEvent(event);
}
#endif

void QDateTimeEdit::focusInEvent(QFocusEvent *event)
{
   Q_D(QDateTimeEdit);
   QAbstractSpinBox::focusInEvent(event);
   QString *frm = nullptr;
   const int oldPos = d->edit->cursorPosition();
   if (!d->formatExplicitlySet) {
      if (d->displayFormat == d->defaultTimeFormat) {
         frm = &d->defaultTimeFormat;
      } else if (d->displayFormat == d->defaultDateFormat) {
         frm = &d->defaultDateFormat;
      } else if (d->displayFormat == d->defaultDateTimeFormat) {
         frm = &d->defaultDateTimeFormat;
      }

      if (frm) {
         d->readLocaleSettings();
         if (d->displayFormat != *frm) {
            setDisplayFormat(*frm);
            d->formatExplicitlySet = false;
            d->edit->setCursorPosition(oldPos);
         }
      }
   }

   const bool oldHasHadFocus = d->hasHadFocus;
   d->hasHadFocus = true;
   bool first = true;

   switch (event->reason()) {
      case Qt::BacktabFocusReason:
         first = false;
         break;

      case Qt::MouseFocusReason:
      case Qt::PopupFocusReason:
         return;

      case Qt::ActiveWindowFocusReason:
         if (oldHasHadFocus) {
            return;
         }

      case Qt::ShortcutFocusReason:
      case Qt::TabFocusReason:
      default:
         break;
   }

   if (isRightToLeft()) {
      first = !first;
   }

   d->updateEdit();          // needed to make it update specialValueText

   d->setSelected(first ? 0 : d->sectionNodes.size() - 1);
}

bool QDateTimeEdit::focusNextPrevChild(bool next)
{
   Q_D(QDateTimeEdit);
   const int newSection = d->nextPrevSection(d->currentSectionIndex, next);

   switch (d->sectionType(newSection)) {
      case QDateTimeParser::NoSection:
      case QDateTimeParser::FirstSection:
      case QDateTimeParser::LastSection:
         return QAbstractSpinBox::focusNextPrevChild(next);

      default:
         d->edit->deselect();
         d->edit->setCursorPosition(d->sectionPos(newSection));
         d->setSelected(newSection, true);

         return false;
   }
}

void QDateTimeEdit::stepBy(int steps)
{
   Q_D(QDateTimeEdit);

#ifdef QT_KEYPAD_NAVIGATION
   // with keypad navigation and not editFocus, left right change the date/time by a fixed amount.

   if (QApplication::keypadNavigationEnabled() && !hasEditFocus()) {
      // if date based, shift by day.  else shift by 15min
      if (d->sections & DateSections_Mask) {
         setDateTime(dateTime().addDays(steps));

      } else {
         int minutes = time().hour() * 60 + time().minute();
         int blocks = minutes / 15;
         blocks += steps;

         /* rounding involved */
         if (minutes % 15) {
            if (steps < 0) {
               blocks += 1; // do one less step;
            }
         }

         minutes = blocks * 15;

         /* need to take wrapping into account */
         if (!d->wrapping) {
            int max_minutes = d->maximum.toTime().hour() * 60 + d->maximum.toTime().minute();
            int min_minutes = d->minimum.toTime().hour() * 60 + d->minimum.toTime().minute();

            if (minutes >= max_minutes) {
               setTime(maximumTime());
               return;
            } else if (minutes <= min_minutes) {
               setTime(minimumTime());
               return;
            }
         }
         setTime(QTime(minutes / 60, minutes % 60));
      }
      return;
   }
#endif

   // do not optimize away steps == 0, this is the only way to select the currentSection

   if (d->specialValue() && displayedSections() != AmPmSection) {
      for (int i = 0; i < d->sectionNodes.size(); ++i) {
         if (d->sectionType(i) != QDateTimeParser::AmPmSection) {
            d->currentSectionIndex = i;
            break;
         }
      }
   }

   d->setValue(d->stepBy(d->currentSectionIndex, steps, false), EmitIfChanged);
   d->updateCache(d->m_spinBoxValue, d->displayText());

   d->setSelected(d->currentSectionIndex);
   d->updateTimeZone();
}

QString QDateTimeEdit::textFromDateTime(const QDateTime &dateTime) const
{
   Q_D(const QDateTimeEdit);
   return locale().toString(dateTime, d->displayFormat);
}

QDateTime QDateTimeEdit::dateTimeFromText(const QString &text) const
{
   Q_D(const QDateTimeEdit);

   QString copy = text;
   int pos = d->edit->cursorPosition();
   QValidator::State state = QValidator::Acceptable;

   return d->validateAndInterpret(copy, pos, state);
}

QValidator::State QDateTimeEdit::validate(QString &text, int &pos) const
{
   Q_D(const QDateTimeEdit);

   QValidator::State state;
   d->validateAndInterpret(text, pos, state);

   return state;
}

void QDateTimeEdit::fixup(QString &input) const
{
   Q_D(const QDateTimeEdit);

   QValidator::State state;
   int copy = d->edit->cursorPosition();

   d->validateAndInterpret(input, copy, state, true);
}

QDateTimeEdit::StepEnabled QDateTimeEdit::stepEnabled() const
{
   Q_D(const QDateTimeEdit);

   if (d->readOnly) {
      return Qt::EmptyFlag;
   }

   if (d->specialValue()) {
      return (d->minimum == d->maximum ? Qt::EmptyFlag : StepEnabled(StepUpEnabled));
   }

   QAbstractSpinBox::StepEnabled ret = Qt::EmptyFlag;

#ifdef QT_KEYPAD_NAVIGATION
   if (QApplication::keypadNavigationEnabled() && !hasEditFocus()) {
      if (d->wrapping) {
         return StepEnabled(StepUpEnabled | StepDownEnabled);
      }

      // 3 cases.  date, time, datetime. each case looks at just the relavant component
      QVariant max, min, val;

      if (!(d->sections & DateSections_Mask)) {
         // time only, no date
         max = d->maximum.toTime();
         min = d->minimum.toTime();
         val = d->value.toTime();

      } else if (!(d->sections & TimeSections_Mask)) {
         // date only, no time
         max = d->maximum.toDate();
         min = d->minimum.toDate();
         val = d->value.toDate();

      } else {
         // both
         max = d->maximum;
         min = d->minimum;
         val = d->value;
      }

      if (val != min) {
         ret |= QAbstractSpinBox::StepDownEnabled;
      }

      if (val != max) {
         ret |= QAbstractSpinBox::StepUpEnabled;
      }

      return ret;
   }
#endif

   switch (d->sectionType(d->currentSectionIndex)) {
      case QDateTimeParser::NoSection:
      case QDateTimeParser::FirstSection:
      case QDateTimeParser::LastSection:
         return Qt::EmptyFlag;

      default:
         break;
   }

   if (d->wrapping) {
      return StepEnabled(StepDownEnabled | StepUpEnabled);
   }

   QVariant v = d->stepBy(d->currentSectionIndex, 1, true);
   if (v != d->m_spinBoxValue) {
      ret |= QAbstractSpinBox::StepUpEnabled;
   }

   v = d->stepBy(d->currentSectionIndex, -1, true);

   if (v != d->m_spinBoxValue) {
      ret |= QAbstractSpinBox::StepDownEnabled;
   }

   return ret;
}

void QDateTimeEdit::mousePressEvent(QMouseEvent *event)
{
   Q_D(QDateTimeEdit);
   if (!d->calendarPopupEnabled()) {
      QAbstractSpinBox::mousePressEvent(event);
      return;
   }
   d->updateHoverControl(event->pos());
   if (d->hoverControl == QStyle::SC_ComboBoxArrow) {
      event->accept();

      if (d->readOnly) {
         return;
      }

      d->updateArrow(QStyle::State_Sunken);
      d->initCalendarPopup();
      d->positionCalendarPopup();
      //Show the calendar
      d->monthCalendar->show();

   } else {
      QAbstractSpinBox::mousePressEvent(event);
   }
}

QTimeEdit::QTimeEdit(QWidget *parent)
   : QDateTimeEdit(QDATETIME_TIME_MIN, QVariant::Time, parent)
{
   connect(this, &QDateEdit::timeChanged, this, &QTimeEdit::userTimeChanged);
}

QTimeEdit::QTimeEdit(const QTime &time, QWidget *parent)
   : QDateTimeEdit(time, QVariant::Time, parent)
{
}

QTimeEdit::~QTimeEdit()
{
}

QDateEdit::QDateEdit(QWidget *parent)
   : QDateTimeEdit(QDATETIME_DATE_DEFAULT, QVariant::Date, parent)
{
   connect(this, &QDateEdit::dateChanged, this, &QDateEdit::userDateChanged);
}

QDateEdit::QDateEdit(const QDate &date, QWidget *parent)
   : QDateTimeEdit(date, QVariant::Date, parent)
{
}

QDateEdit::~QDateEdit()
{
}

QDateTimeEditPrivate::QDateTimeEditPrivate()
   : QDateTimeParser(QVariant::DateTime, QDateTimeParser::DateTimeEdit)
{
   hasHadFocus         = false;
   formatExplicitlySet = false;
   cacheGuard          = false;
   fixday              = true;
   calendarPopup       = false;

   type       = QVariant::DateTime;
   sections   = Qt::EmptyFlag;
   cachedDay  = -1;
   first.pos  = 0;

   currentSectionIndex = FirstSectionIndex;

   minimum = QDATETIME_GREGORIAN_DATETIME;
   maximum = QDATETIME_DATETIME_MAX;

   arrowState    = QStyle::State_None;
   monthCalendar = nullptr;

   readLocaleSettings();

#ifdef QT_KEYPAD_NAVIGATION
   focusOnButton = false;
#endif
}

QDateTimeEditPrivate::~QDateTimeEditPrivate()
{
}

void QDateTimeEditPrivate::updateTimeZone()
{
   minimum = minimum.toDateTime().toTimeZone(m_timeZone);
   maximum = maximum.toDateTime().toTimeZone(m_timeZone);

   m_spinBoxValue = m_spinBoxValue.toDateTime().toTimeZone(m_timeZone);

   // time zone changes can lead to 00:00:00 becomes 01:00:00 and 23:59:59 becomes 00:59:59 (invalid range)
   const bool dateShown = (sections & QDateTimeEdit::DateSections_Mask);

   if (! dateShown) {
      if (minimum.toTime() >= maximum.toTime()) {
         minimum = QDateTime(m_spinBoxValue.toDate(), QDATETIME_TIME_MIN, m_timeZone);
         maximum = QDateTime(m_spinBoxValue.toDate(), QDATETIME_TIME_MAX, m_timeZone);
      }
   }
}

void QDateTimeEditPrivate::updateEdit()
{
   const QString newText = (specialValue() ? specialValueText : textFromValue(m_spinBoxValue));

   if (newText == displayText()) {
      return;
   }

   int selsize = edit->selectedText().size();
   const bool sb = edit->blockSignals(true);

   edit->setText(newText);

#ifdef QT_KEYPAD_NAVIGATION
   if (! specialValue() && ! (QApplication::keypadNavigationEnabled() && !edit->hasEditFocus())) {

#else
   if (! specialValue()) {

#endif

      int cursor = sectionPos(currentSectionIndex);

      cursor = qBound(0, cursor, displayText().size());

      if (selsize > 0) {
         edit->setSelection(cursor, selsize);

      } else {
         edit->setCursorPosition(cursor);
      }
   }

   edit->blockSignals(sb);
}

void QDateTimeEditPrivate::setSelected(int sectionIndex, bool forward)
{
#ifdef QT_KEYPAD_NAVIGATION
   if (specialValue() || (QApplication::keypadNavigationEnabled() && !edit->hasEditFocus())) {
#else
   if (specialValue()) {
#endif
      edit->selectAll();

   } else {
      const SectionNode &node = sectionNode(sectionIndex);
      if (node.type == NoSection || node.type == LastSection || node.type == FirstSection) {
         return;
      }

      updateCache(m_spinBoxValue, displayText());
      const int size = sectionSize(sectionIndex);

      if (forward) {
         edit->setSelection(sectionPos(node), size);
      } else {
         edit->setSelection(sectionPos(node) + size, -size);
      }
   }
}

int QDateTimeEditPrivate::sectionAt(int pos) const
{
   if (pos < separators.first().size()) {
      return (pos == 0 ? FirstSectionIndex : NoSectionIndex);

   } else if (displayText().size() - pos < separators.last().size() + 1) {
      if (separators.last().size() == 0) {
         return sectionNodes.count() - 1;
      }

      return (pos == displayText().size() ? LastSectionIndex : NoSectionIndex);
   }

   updateCache(m_spinBoxValue, displayText());

   for (int i = 0; i < sectionNodes.size(); ++i) {
      const int tmp = sectionPos(i);
      if (pos < tmp + sectionSize(i)) {
         return (pos < tmp ? -1 : i);
      }
   }
   return -1;
}

int QDateTimeEditPrivate::closestSection(int pos, bool forward) const
{
   Q_ASSERT(pos >= 0);

   if (pos < separators.first().size()) {
      if (forward) {
         return 0;
      } else {
         return FirstSectionIndex;
      }

   } else if (displayText().size() - pos < separators.last().size() + 1) {

      if (forward) {
         return LastSectionIndex;
      } else {
         return sectionNodes.size() - 1;
      }
   }

   updateCache(m_spinBoxValue, displayText());

   for (int i = 0; i < sectionNodes.size(); ++i) {
      const int tmp = sectionPos(sectionNodes.at(i));

      if (pos < tmp + sectionSize(i)) {
         if (pos < tmp && ! forward) {
            return i - 1;
         }

         return i;

      } else if (i == sectionNodes.size() - 1 && pos > tmp) {
         return i;
      }
   }

   qWarning("QDateTimeEdit::closestSection() No calendar section found");

   return NoSectionIndex;
}

int QDateTimeEditPrivate::nextPrevSection(int current, bool forward) const
{
   Q_Q(const QDateTimeEdit);

   if (q->isRightToLeft()) {
      forward = !forward;
   }

   switch (current) {
      case FirstSectionIndex:
         if (forward) {
            return 0;
         } else {
            return FirstSectionIndex;
         }

      case LastSectionIndex:
         if (forward) {
            return LastSectionIndex;

         } else {
            return sectionNodes.size() - 1;
         }

      case NoSectionIndex:
         return FirstSectionIndex;

      default:
         break;
   }
   Q_ASSERT(current >= 0 && current < sectionNodes.size());

   current += (forward ? 1 : -1);

   if (current >= sectionNodes.size()) {
      return LastSectionIndex;
   } else if (current < 0) {
      return FirstSectionIndex;
   }

   return current;
}

void QDateTimeEditPrivate::clearSection(int index)
{
   const QChar space(' ');
   int cursorPos = edit->cursorPosition();

   bool blocked  = edit->blockSignals(true);

   QString t = edit->text();
   const int pos = sectionPos(index);

   if (pos == -1) {
      qWarning("QDateTimeEdit::clearSection() No section found");
      return;
   }

   const int size = sectionSize(index);
   t.replace(pos, size, QString().fill(space, size));

   edit->setText(t);
   edit->setCursorPosition(cursorPos);
   edit->blockSignals(blocked);
}

void QDateTimeEditPrivate::updateCache(const QVariant &val, const QString &str) const
{
   if (val != cachedValue || str != cachedText || cacheGuard) {
      cacheGuard   = true;
      QString copy = str;
      int unused   = edit->cursorPosition();

      QValidator::State unusedState;
      validateAndInterpret(copy, unused, unusedState);
      cacheGuard = false;
   }
}

QDateTime QDateTimeEditPrivate::validateAndInterpret(QString &input, int &position,
   QValidator::State &state, bool fixup) const
{
   if (input.isEmpty()) {
      if (sectionNodes.size() == 1 || ! specialValueText.isEmpty()) {
         state = QValidator::Intermediate;
      } else {
         state = QValidator::Invalid;
      }

      return getZeroVariant().toDateTime();

   } else if (cachedText == input && ! fixup) {
      state = cachedState;
      return cachedValue.toDateTime();

   } else if (! specialValueText.isEmpty()) {
      bool changeCase = false;
      const int max = qMin(specialValueText.size(), input.size());
      int i;

      for (i = 0; i < max; ++i) {
         const QChar ic = input.at(i);
         const QChar sc = specialValueText.at(i);

         if (ic != sc) {
            if (sc.toLower() == ic.toLower()) {
               changeCase = true;
            } else {
               break;
            }
         }
      }

      if (i == max) {
         state = specialValueText.size() == input.size() ? QValidator::Acceptable : QValidator::Intermediate;
         if (changeCase) {
            input = specialValueText.left(max);
         }

         return minimum.toDateTime();
      }
   }

   StateNode tmp = parse(input, position, m_spinBoxValue.toDateTime(), fixup);
   input = tmp.input;
   state = QValidator::State(int(tmp.state));

   if (state == QValidator::Acceptable) {
      if (tmp.conflicts && conflictGuard != tmp.value) {
         conflictGuard = tmp.value;
         clearCache();
         input = textFromValue(tmp.value);
         updateCache(tmp.value, input);
         conflictGuard.clear();
      } else {
         cachedText = input;
         cachedState = state;
         cachedValue = tmp.value;
      }

   } else {
      clearCache();
   }

   return (tmp.value.isNull() ? getZeroVariant().toDateTime() : tmp.value);
}

QString QDateTimeEditPrivate::textFromValue(const QVariant &f) const
{
   Q_Q(const QDateTimeEdit);
   return q->textFromDateTime(f.toDateTime());
}

QVariant QDateTimeEditPrivate::valueFromText(const QString &f) const
{
   Q_Q(const QDateTimeEdit);
   return q->dateTimeFromText(f).toTimeZone(m_timeZone);
}

QDateTime QDateTimeEditPrivate::stepBy(int sectionIndex, int steps, bool test) const
{
   Q_Q(const QDateTimeEdit);

   QDateTime v = m_spinBoxValue.toDateTime();
   QString str = displayText();
   int pos = edit->cursorPosition();
   const SectionNode sn = sectionNode(sectionIndex);

   int val;

   // to make sure it behaves reasonably when typing something and then stepping in non-tracking mode
   if (!test && pendingEmit) {
      if (q->validate(str, pos) != QValidator::Acceptable) {
         v = m_spinBoxValue.toDateTime();
      } else {
         v = q->dateTimeFromText(str);
      }
      val = getDigit(v, sectionIndex);

   } else {
      val = getDigit(v, sectionIndex);
   }

   val += steps;

   const int min = absoluteMin(sectionIndex);
   const int max = absoluteMax(sectionIndex, m_spinBoxValue.toDateTime());

   if (val < min) {
      val = (wrapping ? max - (min - val) + 1 : min);
   } else if (val > max) {
      val = (wrapping ? min + val - max - 1 : max);
   }


   const int oldDay = v.date().day();

   setDigit(v, sectionIndex, val);
   // if this sets year or month it will make
   // sure that days are lowered if needed.

   const QDateTime minimumDateTime = minimum.toDateTime();
   const QDateTime maximumDateTime = maximum.toDateTime();
   // changing one section should only modify that section, if possible
   if (sn.type != AmPmSection && (v < minimumDateTime || v > maximumDateTime)) {
      const int localmin = getDigit(minimumDateTime, sectionIndex);
      const int localmax = getDigit(maximumDateTime, sectionIndex);

      if (wrapping) {
         // just because we hit the roof in one direction, it
         // doesn't mean that we hit the floor in the other
         if (steps > 0) {
            setDigit(v, sectionIndex, min);
            if (!(sn.type & DaySectionMask) && sections & DateSectionMask) {
               const int daysInMonth = v.date().daysInMonth();
               if (v.date().day() < oldDay && v.date().day() < daysInMonth) {
                  const int adds = qMin(oldDay, daysInMonth);
                  v = v.addDays(adds - v.date().day());
               }
            }

            if (v < minimumDateTime) {
               setDigit(v, sectionIndex, localmin);
               if (v < minimumDateTime) {
                  setDigit(v, sectionIndex, localmin + 1);
               }
            }
         } else {
            setDigit(v, sectionIndex, max);
            if (!(sn.type & DaySectionMask) && sections & DateSectionMask) {
               const int daysInMonth = v.date().daysInMonth();
               if (v.date().day() < oldDay && v.date().day() < daysInMonth) {
                  const int adds = qMin(oldDay, daysInMonth);
                  v = v.addDays(adds - v.date().day());
               }
            }

            if (v > maximumDateTime) {
               setDigit(v, sectionIndex, localmax);
               if (v > maximumDateTime) {
                  setDigit(v, sectionIndex, localmax - 1);
               }
            }
         }
      } else {
         setDigit(v, sectionIndex, (steps > 0 ? localmax : localmin));
      }
   }

   if (!test && oldDay != v.date().day() && !(sn.type & DaySectionMask)) {
      // this should not happen when called from stepEnabled
      cachedDay = qMax(oldDay, cachedDay);
   }

   if (v < minimumDateTime) {
      if (wrapping) {
         QDateTime t = v;
         setDigit(t, sectionIndex, steps < 0 ? max : min);
         bool mincmp = (t >= minimumDateTime);
         bool maxcmp = (t <= maximumDateTime);
         if (!mincmp || !maxcmp) {
            setDigit(t, sectionIndex, getDigit(steps < 0
                  ? maximumDateTime
                  : minimumDateTime, sectionIndex));
            mincmp = (t >= minimumDateTime);
            maxcmp = (t <= maximumDateTime);
         }
         if (mincmp && maxcmp) {
            v = t;
         }
      } else {
         v = m_spinBoxValue.toDateTime();
      }

   } else if (v > maximumDateTime) {
      if (wrapping) {
         QDateTime t = v;
         setDigit(t, sectionIndex, steps > 0 ? min : max);
         bool mincmp = (t >= minimumDateTime);
         bool maxcmp = (t <= maximumDateTime);

         if (!mincmp || !maxcmp) {
            setDigit(t, sectionIndex, getDigit(steps > 0 ?
                  minimumDateTime :
                  maximumDateTime, sectionIndex));
            mincmp = (t >= minimumDateTime);
            maxcmp = (t <= maximumDateTime);
         }

         if (mincmp && maxcmp) {
            v = t;
         }

      } else {
         v = m_spinBoxValue.toDateTime();
      }
   }

   const QDateTime retval = bound(v, m_spinBoxValue, steps).toDateTime().toTimeZone(m_timeZone);

   return retval;
}

void QDateTimeEditPrivate::emitSignals(EmitPolicy ep, const QVariant &old)
{
   Q_Q(QDateTimeEdit);

   if (ep == NeverEmit) {
      return;
   }

   pendingEmit = false;

   const bool dodate      = m_spinBoxValue.toDate().isValid() && (sections & DateSectionMask);
   const bool datechanged = (ep == AlwaysEmit || old.toDate() != m_spinBoxValue.toDate());
   const bool dotime      = m_spinBoxValue.toTime().isValid() && (sections & TimeSectionMask);
   const bool timechanged = (ep == AlwaysEmit || old.toTime() != m_spinBoxValue.toTime());

   updateCache(m_spinBoxValue, displayText());

   syncCalendarWidget();
   if (datechanged || timechanged) {
      emit q->dateTimeChanged(m_spinBoxValue.toDateTime());
   }

   if (dodate && datechanged) {
      emit q->dateChanged(m_spinBoxValue.toDate());
   }

   if (dotime && timechanged) {
      emit q->timeChanged(m_spinBoxValue.toTime());
   }
}

void QDateTimeEditPrivate::_q_editorCursorPositionChanged(int oldpos, int newpos)
{
   if (ignoreCursorPositionChanged || specialValue()) {
      return;
   }

   const QString oldText = displayText();
   updateCache(m_spinBoxValue, oldText);

   const bool allowChange = !edit->hasSelectedText();
   const bool forward = oldpos <= newpos;
   ignoreCursorPositionChanged = true;

   int s = sectionAt(newpos);
   if (s == NoSectionIndex && forward && newpos > 0) {
      s = sectionAt(newpos - 1);
   }

   int c = newpos;

   const int selstart = edit->selectionStart();
   const int selSection = sectionAt(selstart);
   const int l = selSection != -1 ? sectionSize(selSection) : 0;

   if (s == NoSectionIndex) {
      if (l > 0 && selstart == sectionPos(selSection) && edit->selectedText().size() == l) {
         s = selSection;
         if (allowChange) {
            setSelected(selSection, true);
         }
         c = -1;

      } else {
         int closest = closestSection(newpos, forward);
         c = sectionPos(closest) + (forward ? 0 : qMax(0, sectionSize(closest)));

         if (allowChange) {
            edit->setCursorPosition(c);
         }

         s = closest;
      }
   }

   if (allowChange && currentSectionIndex != s) {
      interpret(EmitIfChanged);
   }

   if (c == -1) {
      setSelected(s, true);
   } else if (!edit->hasSelectedText()) {
      if (oldpos < newpos) {
         edit->setCursorPosition(displayText().size() - (oldText.size() - c));
      } else {
         edit->setCursorPosition(c);
      }
   }

   currentSectionIndex = s;

   Q_ASSERT_X(currentSectionIndex < sectionNodes.size(), "QDateTimeEditPrivate::_q_editorCursorPositionChanged()",
      csPrintable(QString("Internal error (%1 %2)").formatArg(currentSectionIndex).formatArg(sectionNodes.size())) );

   ignoreCursorPositionChanged = false;
}

void QDateTimeEditPrivate::readLocaleSettings()
{
   const QLocale loc;
   defaultTimeFormat = loc.timeFormat(QLocale::ShortFormat);
   defaultDateFormat = loc.dateFormat(QLocale::ShortFormat);
   defaultDateTimeFormat = loc.dateTimeFormat(QLocale::ShortFormat);
}

QDateTimeEdit::Section QDateTimeEditPrivate::convertToPublic(QDateTimeParser::Section s)
{
   switch (s & ~Internal) {
      case AmPmSection:
         return QDateTimeEdit::AmPmSection;

      case MSecSection:
         return QDateTimeEdit::MSecSection;

      case SecondSection:
         return QDateTimeEdit::SecondSection;

      case MinuteSection:
         return QDateTimeEdit::MinuteSection;

      case DayOfWeekSectionShort:
      case DayOfWeekSectionLong:
      case DaySection:
         return QDateTimeEdit::DaySection;

      case MonthSection:
         return QDateTimeEdit::MonthSection;

      case YearSection2Digits:
      case YearSection:
         return QDateTimeEdit::YearSection;

      case Hour12Section:
      case Hour24Section:
         return QDateTimeEdit::HourSection;

      case FirstSection:
      case NoSection:
      case LastSection:
         break;
   }

   return QDateTimeEdit::NoSection;
}

QDateTimeEdit::Sections QDateTimeEditPrivate::convertSections(QDateTimeParser::Sections s)
{
   QDateTimeEdit::Sections ret = Qt::EmptyFlag;

   if (s & QDateTimeParser::MSecSection) {
      ret |= QDateTimeEdit::MSecSection;
   }

   if (s & QDateTimeParser::SecondSection) {
      ret |= QDateTimeEdit::SecondSection;
   }

   if (s & QDateTimeParser::MinuteSection) {
      ret |= QDateTimeEdit::MinuteSection;
   }

   if (s & (QDateTimeParser::HourSectionMask)) {
      ret |= QDateTimeEdit::HourSection;
   }

   if (s & QDateTimeParser::AmPmSection) {
      ret |= QDateTimeEdit::AmPmSection;
   }

   if (s & (QDateTimeParser::DaySectionMask)) {
      ret |= QDateTimeEdit::DaySection;
   }

   if (s & QDateTimeParser::MonthSection) {
      ret |= QDateTimeEdit::MonthSection;
   }

   if (s & (QDateTimeParser::YearSectionMask)) {
      ret |= QDateTimeEdit::YearSection;
   }

   return ret;
}

void QDateTimeEdit::paintEvent(QPaintEvent *event)
{
   Q_D(QDateTimeEdit);

   if (! d->calendarPopupEnabled()) {
      QAbstractSpinBox::paintEvent(event);
      return;
   }

   QStyleOptionSpinBox opt;
   initStyleOption(&opt);

   QStyleOptionComboBox optCombo;
   optCombo.initFrom(this);
   optCombo.editable = true;
   optCombo.frame = opt.frame;
   optCombo.subControls = opt.subControls;
   optCombo.activeSubControls = opt.activeSubControls;
   optCombo.state = opt.state;
   if (d->readOnly) {
      optCombo.state &= ~QStyle::State_Enabled;
   }

   QPainter p(this);
   style()->drawComplexControl(QStyle::CC_ComboBox, &optCombo, &p, this);
}

QString QDateTimeEditPrivate::getAmPmText(AmPm ap, Case cs) const
{
   if (ap == AmText) {
      return (cs == UpperCase ? QDateTimeParser::tr("AM") : QDateTimeParser::tr("am"));
   } else {
      return (cs == UpperCase ? QDateTimeParser::tr("PM") : QDateTimeParser::tr("pm"));
   }
}

int QDateTimeEditPrivate::absoluteIndex(QDateTimeEdit::Section s, int index) const
{
   for (int i = 0; i < sectionNodes.size(); ++i) {
      if (convertToPublic(sectionNodes.at(i).type) == s && index-- == 0) {
         return i;
      }
   }

   return NoSectionIndex;
}

int QDateTimeEditPrivate::absoluteIndex(const SectionNode &s) const
{
   return sectionNodes.indexOf(s);
}

void QDateTimeEditPrivate::interpret(EmitPolicy ep)
{
   Q_Q(QDateTimeEdit);

   QString tmp = displayText();
   int pos = edit->cursorPosition();
   const QValidator::State state = q->validate(tmp, pos);

   if (state != QValidator::Acceptable && correctionMode == QAbstractSpinBox::CorrectToPreviousValue
         && (state == QValidator::Invalid || currentSectionIndex < 0  || ! (fieldInfo(currentSectionIndex) & AllowPartial))) {

      setValue(m_spinBoxValue, ep);
      updateTimeZone();

   } else {
      QAbstractSpinBoxPrivate::interpret(ep);
   }
}

void QDateTimeEditPrivate::clearCache() const
{
   QAbstractSpinBoxPrivate::clearCache();
   cachedDay = -1;
}

void QDateTimeEdit::initStyleOption(QStyleOptionSpinBox *option) const
{
   if (! option) {
      return;
   }

   Q_D(const QDateTimeEdit);
   QAbstractSpinBox::initStyleOption(option);

   if (d->calendarPopupEnabled()) {
      option->subControls = QStyle::SC_ComboBoxFrame | QStyle::SC_ComboBoxEditField | QStyle::SC_ComboBoxArrow;

      if (d->arrowState == QStyle::State_Sunken) {
         option->state |= QStyle::State_Sunken;
      } else {
         option->state &= ~QStyle::State_Sunken;
      }
   }
}

void QDateTimeEditPrivate::init(const QVariant &var)
{
   Q_Q(QDateTimeEdit);

   switch (var.type()) {
      case QVariant::Date:
         m_spinBoxValue = QDateTime(var.toDate(), QDATETIME_TIME_MIN);
         updateTimeZone();

         q->setDisplayFormat(defaultDateFormat);

         if (sectionNodes.isEmpty()) {
            // safeguard for broken locale
            q->setDisplayFormat("dd/MM/yyyy");
         }
         break;

      case QVariant::DateTime:
         m_spinBoxValue = var;
         updateTimeZone();

         q->setDisplayFormat(defaultDateTimeFormat);

         if (sectionNodes.isEmpty()) {
            // safeguard for broken locale
            q->setDisplayFormat("dd/MM/yyyy hh:mm:ss");
         }
         break;

      case QVariant::Time:
         m_spinBoxValue = QDateTime(QDATETIME_DATE_DEFAULT, var.toTime());
         updateTimeZone();

         q->setDisplayFormat(defaultTimeFormat);

         if (sectionNodes.isEmpty()) {
            // safeguard for broken locale
            q->setDisplayFormat("hh:mm:ss");
         }
         break;

      default:
         Q_ASSERT_X(0, "QDateTimeEditPrivate::init", "Internal error");
         break;
   }

#ifdef QT_KEYPAD_NAVIGATION
   if (QApplication::keypadNavigationEnabled()) {
      q->setCalendarPopup(true);
   }
#endif

   q->setInputMethodHints(Qt::ImhPreferNumbers);
   setLayoutItemMargins(QStyle::SE_DateTimeEditLayoutItem);
}

void QDateTimeEditPrivate::_q_resetButton()
{
   updateArrow(QStyle::State_None);
}

void QDateTimeEditPrivate::updateArrow(QStyle::StateFlag state)
{
   Q_Q(QDateTimeEdit);

   if (arrowState == state) {
      return;
   }
   arrowState = state;
   if (arrowState != QStyle::State_None) {
      buttonState |= Mouse;
   } else {
      buttonState = 0;
      hoverControl = QStyle::SC_ComboBoxFrame;
   }
   q->update();
}

QStyle::SubControl QDateTimeEditPrivate::newHoverControl(const QPoint &pos)
{
   if (!calendarPopupEnabled()) {
      return QAbstractSpinBoxPrivate::newHoverControl(pos);
   }

   Q_Q(QDateTimeEdit);

   QStyleOptionComboBox optCombo;
   optCombo.initFrom(q);
   optCombo.editable = true;
   optCombo.subControls = QStyle::SC_All;
   hoverControl = q->style()->hitTestComplexControl(QStyle::CC_ComboBox, &optCombo, pos, q);

   return hoverControl;
}

void QDateTimeEditPrivate::updateEditFieldGeometry()
{
   if (! calendarPopupEnabled()) {
      QAbstractSpinBoxPrivate::updateEditFieldGeometry();
      return;
   }

   Q_Q(QDateTimeEdit);

   QStyleOptionComboBox optCombo;
   optCombo.initFrom(q);
   optCombo.editable = true;
   optCombo.subControls = QStyle::SC_ComboBoxEditField;
   edit->setGeometry(q->style()->subControlRect(QStyle::CC_ComboBox, &optCombo,
         QStyle::SC_ComboBoxEditField, q));
}

QVariant QDateTimeEditPrivate::getZeroVariant() const
{
   Q_ASSERT(type == QVariant::DateTime);
   return QDateTime(QDATETIME_DATE_DEFAULT, QTime(), m_timeZone);
}

void QDateTimeEditPrivate::setRange(const QVariant &min, const QVariant &max)
{
   QAbstractSpinBoxPrivate::setRange(min, max);
   syncCalendarWidget();
}

bool QDateTimeEditPrivate::isSeparatorKey(const QKeyEvent *ke) const
{
   if (! ke->text().isEmpty() && currentSectionIndex + 1 < sectionNodes.size() && currentSectionIndex >= 0) {
      if (fieldInfo(currentSectionIndex) & Numeric) {
         if (ke->text().at(0).isNumber()) {
            return false;
         }

      } else if (ke->text().at(0).isLetterOrNumber()) {
         return false;
      }

      return separators.at(currentSectionIndex + 1).contains(ke->text());
   }

   return false;
}

void QDateTimeEditPrivate::initCalendarPopup(QCalendarWidget *cw)
{
   Q_Q(QDateTimeEdit);

   if (! monthCalendar) {
      monthCalendar = new QCalendarPopup(q, cw);
      monthCalendar->setObjectName("qt_datetimedit_calendar");

      QObject::connect(monthCalendar, &QCalendarPopup::newDateSelected, q, &QDateTimeEdit::setDate);
      QObject::connect(monthCalendar, &QCalendarPopup::hidingCalendar,  q, &QDateTimeEdit::setDate);
      QObject::connect(monthCalendar, &QCalendarPopup::activated,       q, &QDateTimeEdit::setDate);
      QObject::connect(monthCalendar, &QCalendarPopup::activated,       monthCalendar, &QCalendarPopup::close);
      QObject::connect(monthCalendar, &QCalendarPopup::resetButton,     q, &QDateTimeEdit::_q_resetButton);

   } else if (cw) {
      monthCalendar->setCalendarWidget(cw);
   }

   syncCalendarWidget();
}

void QDateTimeEditPrivate::positionCalendarPopup()
{
   Q_Q(QDateTimeEdit);

   QPoint pos  = (q->layoutDirection() == Qt::RightToLeft) ? q->rect().bottomRight() : q->rect().bottomLeft();
   QPoint pos2 = (q->layoutDirection() == Qt::RightToLeft) ? q->rect().topRight() : q->rect().topLeft();
   pos  = q->mapToGlobal(pos);
   pos2 = q->mapToGlobal(pos2);

   QSize size = monthCalendar->sizeHint();
   QRect screen = QApplication::desktop()->availableGeometry(pos);

   // handle popup falling "off screen"

   if (q->layoutDirection() == Qt::RightToLeft) {
      pos.setX(pos.x() - size.width());
      pos2.setX(pos2.x() - size.width());

      if (pos.x() < screen.left()) {
         pos.setX(qMax(pos.x(), screen.left()));
      } else if (pos.x() + size.width() > screen.right()) {
         pos.setX(qMax(pos.x() - size.width(), screen.right() - size.width()));
      }

   } else {
      if (pos.x() + size.width() > screen.right()) {
         pos.setX(screen.right() - size.width());
      }
      pos.setX(qMax(pos.x(), screen.left()));
   }

   if (pos.y() + size.height() > screen.bottom()) {
      pos.setY(pos2.y() - size.height());
   } else if (pos.y() < screen.top()) {
      pos.setY(screen.top());
   }

   if (pos.y() < screen.top()) {
      pos.setY(screen.top());
   }

   if (pos.y() + size.height() > screen.bottom()) {
      pos.setY(screen.bottom() - size.height());
   }

   monthCalendar->move(pos);
}

bool QDateTimeEditPrivate::calendarPopupEnabled() const
{
   return (calendarPopup && (sections & (DateSectionMask)));
}

void QDateTimeEditPrivate::syncCalendarWidget()
{
   Q_Q(QDateTimeEdit);

   if (monthCalendar) {
      const bool sb = monthCalendar->blockSignals(true);
      monthCalendar->setDateRange(q->minimumDate(), q->maximumDate());
      monthCalendar->setDate(q->date());
      monthCalendar->blockSignals(sb);
   }
}

QCalendarPopup::QCalendarPopup(QWidget *parent, QCalendarWidget *cw)
   : QWidget(parent, Qt::Popup)
{
   setAttribute(Qt::WA_WindowPropagation);

   dateChanged = false;

   if (! cw) {
      verifyCalendarInstance();
   } else {
      setCalendarWidget(cw);
   }
}

QCalendarWidget *QCalendarPopup::verifyCalendarInstance()
{
   if (calendar.isNull()) {
      QCalendarWidget *cw = new QCalendarWidget(this);
      cw->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);

#ifdef QT_KEYPAD_NAVIGATION
      if (QApplication::keypadNavigationEnabled()) {
         cw->setHorizontalHeaderFormat(QCalendarWidget::SingleLetterDayNames);
      }
#endif

      setCalendarWidget(cw);
      return cw;

   } else {
      return calendar.data();
   }
}

void QCalendarPopup::setCalendarWidget(QCalendarWidget *cw)
{
   Q_ASSERT(cw);
   QVBoxLayout *widgetLayout = qobject_cast<QVBoxLayout *>(layout());

   if (! widgetLayout) {
      widgetLayout = new QVBoxLayout(this);
      widgetLayout->setMargin(0);
      widgetLayout->setSpacing(0);
   }

   delete calendar.data();
   calendar = QPointer<QCalendarWidget>(cw);
   widgetLayout->addWidget(cw);

   connect(cw, &QCalendarWidget::activated,        this, &QCalendarPopup::dateSelected);
   connect(cw, &QCalendarWidget::clicked,          this, &QCalendarPopup::dateSelected);
   connect(cw, &QCalendarWidget::selectionChanged, this, &QCalendarPopup::dateSelectionChanged);

   cw->setFocus();
}

void QCalendarPopup::setDate(const QDate &date)
{
   oldDate = date;
   verifyCalendarInstance()->setSelectedDate(date);
}

void QCalendarPopup::setDateRange(const QDate &min, const QDate &max)
{
   QCalendarWidget *cw = verifyCalendarInstance();
   cw->setMinimumDate(min);
   cw->setMaximumDate(max);
}

void QCalendarPopup::mousePressEvent(QMouseEvent *event)
{
   QDateTimeEdit *dateTime = qobject_cast<QDateTimeEdit *>(parentWidget());

   if (dateTime) {
      QStyleOptionComboBox opt;
      opt.initFrom(dateTime);

      QRect arrowRect = dateTime->style()->subControlRect(QStyle::CC_ComboBox, &opt,
            QStyle::SC_ComboBoxArrow, dateTime);
      arrowRect.moveTo(dateTime->mapToGlobal(arrowRect .topLeft()));
      if (arrowRect.contains(event->globalPos()) || rect().contains(event->pos())) {
         setAttribute(Qt::WA_NoMouseReplay);
      }
   }

   QWidget::mousePressEvent(event);
}

void QCalendarPopup::mouseReleaseEvent(QMouseEvent *)
{
   emit resetButton();
}

bool QCalendarPopup::event(QEvent *event)
{
   if (event->type() == QEvent::KeyPress) {
      QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

      if (keyEvent->matches(QKeySequence::Cancel)) {
         dateChanged = false;
      }
   }

   return QWidget::event(event);
}

void QCalendarPopup::dateSelectionChanged()
{
   dateChanged = true;
   emit newDateSelected(verifyCalendarInstance()->selectedDate());
}

void QCalendarPopup::dateSelected(const QDate &date)
{
   dateChanged = true;
   emit activated(date);
   close();
}

void QCalendarPopup::hideEvent(QHideEvent *)
{
   emit resetButton();
   if (!dateChanged) {
      emit hidingCalendar(oldDate);
   }
}

void  QDateTimeEdit::_q_resetButton()
{
   Q_D( QDateTimeEdit);
   d->_q_resetButton();
}

#endif // QT_NO_DATETIMEEDIT
