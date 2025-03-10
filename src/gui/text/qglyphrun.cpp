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

#include <qglyphrun.h>
#include <qglyphrun_p.h>

#include <qglobal.h>

QGlyphRun::QGlyphRun() : d(new QGlyphRunPrivate)
{
}

QGlyphRun::QGlyphRun(const QGlyphRun &other)
{
   d = other.d;
}

QGlyphRun::~QGlyphRun()
{
   // Required for QExplicitlySharedDataPointer
}

void QGlyphRun::detach()
{
   if (d->ref.load() != 1) {
      d.detach();
   }
}

QGlyphRun &QGlyphRun::operator=(const QGlyphRun &other)
{
   d = other.d;
   return *this;
}

bool QGlyphRun::operator==(const QGlyphRun &other) const
{
   if (d == other.d) {
      return true;
   }

   if ((d->glyphIndexDataSize != other.d->glyphIndexDataSize)
      || (d->glyphPositionDataSize != other.d->glyphPositionDataSize)) {
      return false;
   }

   if (d->glyphIndexData != other.d->glyphIndexData) {
      for (int i = 0; i < d->glyphIndexDataSize; ++i) {
         if (d->glyphIndexData[i] != other.d->glyphIndexData[i]) {
            return false;
         }
      }
   }
   if (d->glyphPositionData != other.d->glyphPositionData) {
      for (int i = 0; i < d->glyphPositionDataSize; ++i) {
         if (d->glyphPositionData[i] != other.d->glyphPositionData[i]) {
            return false;
         }
      }
   }
   return (d->flags == other.d->flags && d->rawFont == other.d->rawFont);
}


QRawFont QGlyphRun::rawFont() const
{
   return d->rawFont;
}

void QGlyphRun::setRawFont(const QRawFont &rawFont)
{
   detach();
   d->rawFont = rawFont;
}

QVector<quint32> QGlyphRun::glyphIndexes() const
{
   if (d->glyphIndexes.constData() == d->glyphIndexData) {
      return d->glyphIndexes;
   } else {
      QVector<quint32> indexes(d->glyphIndexDataSize);
      memcpy(indexes.data(), d->glyphIndexData, d->glyphIndexDataSize * sizeof(quint32));
      return indexes;
   }
}

void QGlyphRun::setGlyphIndexes(const QVector<quint32> &glyphIndexes)
{
   detach();
   d->glyphIndexes = glyphIndexes; // Keep a reference to the QVector to avoid copying
   d->glyphIndexData = glyphIndexes.constData();
   d->glyphIndexDataSize = glyphIndexes.size();
}

QVector<QPointF> QGlyphRun::positions() const
{
   if (d->glyphPositions.constData() == d->glyphPositionData) {
      return d->glyphPositions;
   } else {
      QVector<QPointF> glyphPositions(d->glyphPositionDataSize);
      memcpy(glyphPositions.data(), d->glyphPositionData,
         d->glyphPositionDataSize * sizeof(QPointF));
      return glyphPositions;
   }
}

void QGlyphRun::setPositions(const QVector<QPointF> &positions)
{
   detach();
   d->glyphPositions = positions; // Keep a reference to the vector to avoid copying
   d->glyphPositionData = positions.constData();
   d->glyphPositionDataSize = positions.size();
}

void QGlyphRun::clear()
{
   detach();
   d->rawFont = QRawFont();
   d->flags = Qt::EmptyFlag;

   setPositions(QVector<QPointF>());
   setGlyphIndexes(QVector<quint32>());
}

void QGlyphRun::setRawData(const quint32 *glyphIndexArray, const QPointF *glyphPositionArray, int size)
{
   detach();
   d->glyphIndexes.clear();
   d->glyphPositions.clear();

   d->glyphIndexData = glyphIndexArray;
   d->glyphPositionData = glyphPositionArray;
   d->glyphIndexDataSize = d->glyphPositionDataSize = size;
}

bool QGlyphRun::overline() const
{
   return d->flags & Overline;
}

void QGlyphRun::setOverline(bool overline)
{
   setFlag(Overline, overline);
}

bool QGlyphRun::underline() const
{
   return d->flags & Underline;
}

void QGlyphRun::setUnderline(bool underline)
{
   setFlag(Underline, underline);
}

bool QGlyphRun::strikeOut() const
{
   return d->flags & StrikeOut;
}

void QGlyphRun::setStrikeOut(bool strikeOut)
{
   setFlag(StrikeOut, strikeOut);
}

bool QGlyphRun::isRightToLeft() const
{
   return d->flags & RightToLeft;
}

void QGlyphRun::setRightToLeft(bool rightToLeft)
{
   setFlag(RightToLeft, rightToLeft);
}

QGlyphRun::GlyphRunFlags QGlyphRun::flags() const
{
   return d->flags;
}

void QGlyphRun::setFlag(GlyphRunFlag flag, bool enabled)
{
   if (d->flags.testFlag(flag) == enabled) {
      return;
   }

   detach();
   if (enabled) {
      d->flags |= flag;
   } else {
      d->flags &= ~flag;
   }
}

void QGlyphRun::setFlags(GlyphRunFlags flags)
{
   if (d->flags == flags) {
      return;
   }

   detach();
   d->flags = flags;
}

void QGlyphRun::setBoundingRect(const QRectF &boundingRect)
{
   detach();
   d->boundingRect = boundingRect;
}

QRectF QGlyphRun::boundingRect() const
{
   if (!d->boundingRect.isEmpty() || !d->rawFont.isValid()) {
      return d->boundingRect;
   }

   qreal minX, minY, maxX, maxY;
   minX = minY = maxX = maxY = 0;

   for (int i = 0, n = qMin(d->glyphIndexDataSize, d->glyphPositionDataSize); i < n; ++i) {
      QRectF glyphRect = d->rawFont.boundingRect(d->glyphIndexData[i]);
      glyphRect.translate(d->glyphPositionData[i]);

      if (i == 0) {
         minX = glyphRect.left();
         minY = glyphRect.top();
         maxX = glyphRect.right();
         maxY = glyphRect.bottom();
      } else {
         minX = qMin(glyphRect.left(), minX);
         minY = qMin(glyphRect.top(), minY);
         maxX = qMax(glyphRect.right(), maxX);
         maxY = qMax(glyphRect.bottom(), maxY);
      }
   }

   return QRectF(QPointF(minX, minY), QPointF(maxX, maxY));
}
bool QGlyphRun::isEmpty() const
{
   return d->glyphIndexDataSize == 0;
}

