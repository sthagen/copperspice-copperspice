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

#include <qcosmeticstroker_p.h>

#include <qdebug.h>

#include <qpainterpath_p.h>
#include <qrgba64_p.h>

#include <math.h>

#define toF26Dot6(x) ((int)((x)*64.))

static inline uint sourceOver(uint d, uint color)
{
   return color + BYTE_MUL(d, qAlpha(~color));
}

static inline int F16Dot16FixedDiv(int x, int y)
{
   if (qAbs(x) > 0x7fff) {
      return (((qint64)x) << 16) / y;
   }

   return (x << 16) / y;
}

typedef void (*DrawPixel)(QCosmeticStroker *stroker, int x, int y, int coverage);

namespace {

struct Dasher {
   QCosmeticStroker *stroker;
   int *pattern;
   int offset;
   int dashIndex;
   int dashOn;

   Dasher(QCosmeticStroker *s, bool reverse, int start, int stop)
      : stroker(s) {
      int delta = stop - start;
      if (reverse) {
         pattern = stroker->reversePattern;
         offset = stroker->patternLength - stroker->patternOffset - delta - ((start & 63) - 32);
         dashOn = 0;
      } else {
         pattern = stroker->pattern;
         offset = stroker->patternOffset - ((start & 63) - 32);
         dashOn = 1;
      }
      offset %= stroker->patternLength;
      if (offset < 0) {
         offset += stroker->patternLength;
      }

      dashIndex = 0;
      while (offset >= pattern[dashIndex]) {
         ++dashIndex;
      }

      stroker->patternOffset += delta;
      stroker->patternOffset %= stroker->patternLength;
   }

   bool on() const {
      return (dashIndex + dashOn) & 1;
   }
   void adjust() {
      offset += 64;
      if (offset >= pattern[dashIndex]) {
         ++dashIndex;
         dashIndex %= stroker->patternSize;
      }
      offset %= stroker->patternLength;
   }
};

struct NoDasher {
   NoDasher(QCosmeticStroker *, bool, int, int)
   {}

   bool on() const {
      return true;
   }

   void adjust(int = 0) {}
};

}   // namespace

/*
 * The return value is the result of the clipLine() call performed at the start
 * of each of the two functions, aka "false" means completely outside the devices
 * rect.
 */
template <DrawPixel drawPixel, class Dasher>
static bool drawLine(QCosmeticStroker *stroker, qreal x1, qreal y1, qreal x2, qreal y2, int caps);

template <DrawPixel drawPixel, class Dasher>
static bool drawLineAA(QCosmeticStroker *stroker, qreal x1, qreal y1, qreal x2, qreal y2, int caps);

inline void drawPixel(QCosmeticStroker *stroker, int x, int y, int coverage)
{
   const QRect &cl = stroker->clip;
   if (x < cl.x() || x > cl.right() || y < cl.y() || y > cl.bottom()) {
      return;
   }

   if (stroker->current_span > 0) {
      const int lastx = stroker->spans[stroker->current_span - 1].x + stroker->spans[stroker->current_span - 1].len ;
      const int lasty = stroker->spans[stroker->current_span - 1].y;

      if (stroker->current_span == QCosmeticStroker::SpanCount || y < lasty || (y == lasty && x < lastx)) {
         stroker->blend(stroker->current_span, stroker->spans, &stroker->state->penData);
         stroker->current_span = 0;
      }
   }

   stroker->spans[stroker->current_span].x = ushort(x);
   stroker->spans[stroker->current_span].len = 1;
   stroker->spans[stroker->current_span].y = y;
   stroker->spans[stroker->current_span].coverage = coverage * stroker->opacity >> 8;
   ++stroker->current_span;
}

inline void drawPixelARGB32(QCosmeticStroker *stroker, int x, int y, int coverage)
{
   const QRect &cl = stroker->clip;
   if (x < cl.x() || x > cl.right() || y < cl.y() || y > cl.bottom()) {
      return;
   }

   int offset = x + stroker->ppl * y;
   uint c = BYTE_MUL(stroker->color, coverage);
   stroker->pixels[offset] = sourceOver(stroker->pixels[offset], c);
}

inline void drawPixelARGB32Opaque(QCosmeticStroker *stroker, int x, int y, int)
{
   const QRect &cl = stroker->clip;
   if (x < cl.x() || x > cl.right() || y < cl.y() || y > cl.bottom()) {
      return;
   }

   int offset = x + stroker->ppl * y;
   stroker->pixels[offset] = sourceOver(stroker->pixels[offset], stroker->color);
}

enum StrokeSelection {
   Aliased = 0,
   AntiAliased = 1,
   Solid = 0,
   Dashed = 2,
   RegularDraw = 0,
   FastDraw = 4
};

static StrokeLine strokeLine(int strokeSelection)
{
   StrokeLine stroke;

   switch (strokeSelection) {
      case Aliased|Solid|RegularDraw:
         stroke = &QT_PREPEND_NAMESPACE(drawLine)<drawPixel, NoDasher>;
         break;

      case Aliased|Solid|FastDraw:
         stroke = &QT_PREPEND_NAMESPACE(drawLine)<drawPixelARGB32Opaque, NoDasher>;
         break;

      case Aliased|Dashed|RegularDraw:
         stroke = &QT_PREPEND_NAMESPACE(drawLine)<drawPixel, Dasher>;
         break;

      case Aliased|Dashed|FastDraw:
         stroke = &QT_PREPEND_NAMESPACE(drawLine)<drawPixelARGB32Opaque, Dasher>;
         break;

      case AntiAliased|Solid|RegularDraw:
         stroke = &QT_PREPEND_NAMESPACE(drawLineAA)<drawPixel, NoDasher>;
         break;

      case AntiAliased|Solid|FastDraw:
         stroke = &QT_PREPEND_NAMESPACE(drawLineAA)<drawPixelARGB32, NoDasher>;
         break;

      case AntiAliased|Dashed|RegularDraw:
         stroke = &QT_PREPEND_NAMESPACE(drawLineAA)<drawPixel, Dasher>;
         break;

      case AntiAliased|Dashed|FastDraw:
         stroke = &QT_PREPEND_NAMESPACE(drawLineAA)<drawPixelARGB32, Dasher>;
         break;

      default:
         Q_ASSERT(false);
         stroke = nullptr;
   }
   return stroke;
}

void QCosmeticStroker::setup()
{
   blend = state->penData.blend;
   if (state->clip && state->clip->enabled && state->clip->hasRectClip && !state->clip->clipRect.isEmpty()) {
      clip &= state->clip->clipRect;
      blend = state->penData.unclipped_blend;
   }

   int strokeSelection = 0;
   if (blend == state->penData.unclipped_blend && state->penData.type == QSpanData::Solid
            && (state->penData.rasterBuffer->format == QImage::Format_ARGB32_Premultiplied
            || state->penData.rasterBuffer->format == QImage::Format_RGB32)
            && state->compositionMode() == QPainter::CompositionMode_SourceOver) {
      strokeSelection |= FastDraw;
   }

   if (state->renderHints & QPainter::Antialiasing) {
      strokeSelection |= AntiAliased;
   }

   const QVector<qreal> &penPattern = state->lastPen.dashPattern();
   if (penPattern.isEmpty()) {
      Q_ASSERT(!pattern && !reversePattern);
      pattern        = nullptr;
      reversePattern = nullptr;
      patternLength  = 0;
      patternSize    = 0;

   } else {
      pattern = (int *)malloc(penPattern.size() * sizeof(int));
      reversePattern = (int *)malloc(penPattern.size() * sizeof(int));
      patternSize    = penPattern.size();

      patternLength = 0;

      for (int i = 0; i < patternSize; ++i) {
         patternLength += (int) qMax(1.0, penPattern.at(i) * 64.0);
         pattern[i] = patternLength;
      }

      patternLength = 0;

      for (int i = 0; i < patternSize; ++i) {
         patternLength += (int) qMax(1.0, penPattern.at(patternSize - 1 - i) * 64.0);
         reversePattern[i] = patternLength;
      }

      strokeSelection |= Dashed;
   }

   stroke = strokeLine(strokeSelection);

   qreal width = state->lastPen.widthF();

   if (width == 0) {
      opacity = 256;
   } else if (qt_pen_is_cosmetic(state->lastPen, state->renderHints))  {
      opacity = (int) 256 * width;
   } else {
      opacity = (int) 256 * width * state->txscale;
   }

   opacity = qBound(0, opacity, 256);

   drawCaps = state->lastPen.capStyle() != Qt::FlatCap;

   if (strokeSelection & FastDraw) {
      color = multiplyAlpha256(state->penData.solid.color, opacity).toArgb32();
      QRasterBuffer *buffer = state->penData.rasterBuffer;
      pixels = (uint *)buffer->buffer();
      ppl = buffer->bytesPerLine() >> 2;
   }

   // line drawing produces different results with different clips, so
   // we need to clip consistently when painting to the same device

   // setup FP clip bounds
   xmin = deviceRect.left() - 1;
   xmax = deviceRect.right() + 2;
   ymin = deviceRect.top() - 1;
   ymax = deviceRect.bottom() + 2;

   lastPixel.x = -1;
   lastPixel.y = -1;
}

// returns true if the whole line gets clipped away
bool QCosmeticStroker::clipLine(qreal &x1, qreal &y1, qreal &x2, qreal &y2)
{
   // basic/rough clipping is done in floating point coordinates to avoid
   // integer overflow problems.
   if (x1 < xmin) {
      if (x2 <= xmin) {
         goto clipped;
      }

      y1 += (y2 - y1) / (x2 - x1) * (xmin - x1);
      x1 = xmin;

   } else if (x1 > xmax) {
      if (x2 >= xmax) {
         goto clipped;
      }
      y1 += (y2 - y1) / (x2 - x1) * (xmax - x1);
      x1 = xmax;
   }

   if (x2 < xmin) {
      lastPixel.x = -1;
      y2 += (y2 - y1) / (x2 - x1) * (xmin - x2);
      x2 = xmin;
   } else if (x2 > xmax) {
      lastPixel.x = -1;
      y2 += (y2 - y1) / (x2 - x1) * (xmax - x2);
      x2 = xmax;
   }

   if (y1 < ymin) {
      if (y2 <= ymin) {
         goto clipped;
      }
      x1 += (x2 - x1) / (y2 - y1) * (ymin - y1);
      y1 = ymin;
   } else if (y1 > ymax) {
      if (y2 >= ymax) {
         goto clipped;
      }
      x1 += (x2 - x1) / (y2 - y1) * (ymax - y1);
      y1 = ymax;
   }
   if (y2 < ymin) {
      lastPixel.x = -1;
      x2 += (x2 - x1) / (y2 - y1) * (ymin - y2);
      y2 = ymin;
   } else if (y2 > ymax) {
      lastPixel.x = -1;
      x2 += (x2 - x1) / (y2 - y1) * (ymax - y2);
      y2 = ymax;
   }

   return false;

clipped:
   lastPixel.x = -1;
   return true;
}

void QCosmeticStroker::drawLine(const QPointF &p1, const QPointF &p2)
{
   if (p1 == p2) {
      drawPoints(&p1, 1);
      return;
   }

   QPointF start = p1 * state->matrix;
   QPointF end = p2 * state->matrix;

   patternOffset = state->lastPen.dashOffset() * 64;
   lastPixel.x = -1;

   stroke(this, start.x(), start.y(), end.x(), end.y(), drawCaps ? CapBegin | CapEnd : 0);

   blend(current_span, spans, &state->penData);
   current_span = 0;
}

void QCosmeticStroker::drawPoints(const QPoint *pointPtr, int pointCount)
{
   const QPoint *end = pointPtr + pointCount;

   while (pointPtr < end) {
      QPointF p = QPointF(*pointPtr) * state->matrix;
      drawPixel(this, qRound(p.x()), qRound(p.y()), 255);
      ++pointPtr;
   }

   blend(current_span, spans, &state->penData);
   current_span = 0;
}

void QCosmeticStroker::drawPoints(const QPointF *pointPtr, int pointCount)
{
   const QPointF *end = pointPtr + pointCount;

   while (pointPtr < end) {
      QPointF p = (*pointPtr) * state->matrix;
      drawPixel(this, qRound(p.x()), qRound(p.y()), 255);
      ++pointPtr;
   }

   blend(current_span, spans, &state->penData);
   current_span = 0;
}

void QCosmeticStroker::calculateLastPoint(qreal rx1, qreal ry1, qreal rx2, qreal ry2)
{
   // this is basically the same code as used in the aliased stroke method,
   // but it only determines the direction and last point of a line
   //
   // This is being used to have proper dropout control for closed contours
   // by calculating the direction and last pixel of the last segment in the contour.
   // the info is then used to perform dropout control when drawing the first line segment
   // of the contour
   lastPixel.x = -1;
   lastPixel.y = -1;

   if (clipLine(rx1, ry1, rx2, ry2)) {
      return;
   }

   const int half = legacyRounding ? 31 : 0;
   int x1 = toF26Dot6(rx1) + half;
   int y1 = toF26Dot6(ry1) + half;
   int x2 = toF26Dot6(rx2) + half;
   int y2 = toF26Dot6(ry2) + half;

   int dx = qAbs(x2 - x1);
   int dy = qAbs(y2 - y1);

   if (dx < dy) {
      // vertical
      bool swapped = false;
      if (y1 > y2) {
         swapped = true;
         qSwap(y1, y2);
         qSwap(x1, x2);
      }
      int xinc = F16Dot16FixedDiv(x2 - x1, y2 - y1);
      int x = x1 << 10;

      int y = (y1 + 32) >> 6;
      int ys = (y2 + 32) >> 6;

      int round = (xinc > 0) ? 32 : 0;

      if (y != ys) {
         x += ((y * (1 << 6)) + round - y1) * xinc >> 6;

         if (swapped) {
            lastPixel.x = x >> 16;
            lastPixel.y = y;
            lastDir = QCosmeticStroker::BottomToTop;
         } else {
            lastPixel.x = (x + (ys - y - 1) * xinc) >> 16;
            lastPixel.y = ys - 1;
            lastDir = QCosmeticStroker::TopToBottom;
         }
         lastAxisAligned = qAbs(xinc) < (1 << 14);
      }
   } else {
      // horizontal
      if (!dx) {
         return;
      }

      bool swapped = false;
      if (x1 > x2) {
         swapped = true;
         qSwap(x1, x2);
         qSwap(y1, y2);
      }
      int yinc = F16Dot16FixedDiv(y2 - y1, x2 - x1);
      int y = y1 << 10;

      int x = (x1 + 32) >> 6;
      int xs = (x2 + 32) >> 6;

      int round = (yinc > 0) ? 32 : 0;
      if (x != xs) {
         y += ((x * (1 << 6)) + round - x1) * yinc >> 6;

         if (swapped) {
            lastPixel.x = x;
            lastPixel.y = y >> 16;
            lastDir = QCosmeticStroker::RightToLeft;
         } else {
            lastPixel.x = xs - 1;
            lastPixel.y = (y + (xs - x - 1) * yinc) >> 16;
            lastDir = QCosmeticStroker::LeftToRight;
         }
         lastAxisAligned = qAbs(yinc) < (1 << 14);
      }
   }
}

static inline const QPainterPath::ElementType *subPath(const QPainterPath::ElementType *t,
      const QPainterPath::ElementType *end, const qreal *points, bool *closed)
{
   const QPainterPath::ElementType *start = t;
   ++t;

   // find out if the subpath is closed
   while (t < end) {
      if (*t == QPainterPath::MoveToElement) {
         break;
      }
      ++t;
   }

   int offset = t - start - 1;
   *closed = (points[0] == points[2 * offset] && points[1] == points[2 * offset + 1]);

   return t;
}

void QCosmeticStroker::drawPath(const QVectorPath &path)
{
   if (path.isEmpty()) {
      return;
   }

   const qreal *points = path.points();
   const QPainterPath::ElementType *type = path.elements();

   if (type) {
      const QPainterPath::ElementType *end = type + path.elementCount();

      while (type < end) {
         Q_ASSERT(type == path.elements() || *type == QPainterPath::MoveToElement);

         QPointF p = QPointF(points[0], points[1]) * state->matrix;
         patternOffset = state->lastPen.dashOffset() * 64;

         lastPixel.x = INT_MIN;
         lastPixel.y = INT_MIN;

         bool closed;
         const QPainterPath::ElementType *e = subPath(type, end, points, &closed);

         if (closed) {
            const qreal *ptr = points + 2 * (e - type);

            QPointF p1 = QPointF(ptr[-4], ptr[-3]) * state->matrix;
            QPointF p2 = QPointF(ptr[-2], ptr[-1]) * state->matrix;

            calculateLastPoint(p1.x(), p1.y(), p2.x(), p2.y());
         }

         int caps = (!closed & drawCaps) ? CapBegin : NoCaps;

         points += 2;
         ++type;

         while (type < e) {
            QPointF p2 = QPointF(points[0], points[1]) * state->matrix;

            switch (*type) {
               case QPainterPath::MoveToElement:
                  Q_ASSERT(!"Logic error");
                  break;

               case QPainterPath::LineToElement:
                  if (!closed && drawCaps && type == e - 1) {
                     caps |= CapEnd;
                  }
                  stroke(this, p.x(), p.y(), p2.x(), p2.y(), caps);
                  p = p2;
                  points += 2;
                  ++type;
                  break;

               case QPainterPath::CurveToElement: {
                  if (!closed && drawCaps && type == e - 3) {
                     caps |= CapEnd;
                  }
                  QPointF p3 = QPointF(points[2], points[3]) * state->matrix;
                  QPointF p4 = QPointF(points[4], points[5]) * state->matrix;
                  renderCubic(p, p2, p3, p4, caps);
                  p = p4;
                  type += 3;
                  points += 6;
                  break;
               }
               case QPainterPath::CurveToDataElement:
                  Q_ASSERT(!"QPainterPath::toSubpathPolygons(), bad element type");
                  break;
            }
            caps = NoCaps;
         }
      }

   } else {
      // !type, simple polygon
      QPointF p = QPointF(points[0], points[1]) * state->matrix;
      QPointF movedTo = p;

      patternOffset = state->lastPen.dashOffset() * 64;
      lastPixel.x = INT_MIN;
      lastPixel.y = INT_MIN;

      const qreal *begin = points;
      const qreal *end = points + 2 * path.elementCount();
      // handle closed path case
      bool closed = path.hasImplicitClose() || (points[0] == end[-2] && points[1] == end[-1]);
      int caps = (!closed & drawCaps) ? CapBegin : NoCaps;

      if (closed) {
         QPointF p2 = QPointF(end[-2], end[-1]) * state->matrix;
         calculateLastPoint(p2.x(), p2.y(), p.x(), p.y());
      }

      bool fastPenAliased = (state->flags.fast_pen && !state->flags.antialiased);
      points += 2;
      while (points < end) {
         QPointF p2 = QPointF(points[0], points[1]) * state->matrix;

         if (!closed && drawCaps && points == end - 2) {
            caps |= CapEnd;
         }

         bool moveNextStart = stroke(this, p.x(), p.y(), p2.x(), p2.y(), caps);

         /* fix for gaps in polylines with fastpen and aliased in a sequence
            of points with small distances: if current point p2 has been dropped
            out, keep last non dropped point p.

            However, if the line was completely outside the devicerect, we
            still need to update p to avoid drawing the line after this one from
            a bad starting position.
         */

         if (!fastPenAliased || moveNextStart || points == begin + 2 || points == end - 2)  {
            p = p2;
         }

         points += 2;
         caps = NoCaps;
      }
      if (path.hasImplicitClose()) {
         stroke(this, p.x(), p.y(), movedTo.x(), movedTo.y(), NoCaps);
      }
   }

   blend(current_span, spans, &state->penData);
   current_span = 0;
}

void QCosmeticStroker::renderCubic(const QPointF &p1, const QPointF &p2, const QPointF &p3, const QPointF &p4, int caps)
{
   const int maxSubDivisions = 6;
   PointF points[3 * maxSubDivisions + 4];

   points[3].x = p1.x();
   points[3].y = p1.y();
   points[2].x = p2.x();
   points[2].y = p2.y();
   points[1].x = p3.x();
   points[1].y = p3.y();
   points[0].x = p4.x();
   points[0].y = p4.y();

   PointF *p = points;
   int level = maxSubDivisions;

   renderCubicSubdivision(p, level, caps);
}

static void splitCubic(QCosmeticStroker::PointF *points)
{
   const qreal half = .5;
   qreal  a, b, c, d;

   points[6].x = points[3].x;
   c = points[1].x;
   d = points[2].x;
   points[1].x = a = ( points[0].x + c ) * half;
   points[5].x = b = ( points[3].x + d ) * half;
   c = ( c + d ) * half;
   points[2].x = a = ( a + c ) * half;
   points[4].x = b = ( b + c ) * half;
   points[3].x = ( a + b ) * half;

   points[6].y = points[3].y;
   c = points[1].y;
   d = points[2].y;
   points[1].y = a = ( points[0].y + c ) * half;
   points[5].y = b = ( points[3].y + d ) * half;
   c = ( c + d ) * half;
   points[2].y = a = ( a + c ) * half;
   points[4].y = b = ( b + c ) * half;
   points[3].y = ( a + b ) * half;
}

void QCosmeticStroker::renderCubicSubdivision(QCosmeticStroker::PointF *points, int level, int caps)
{
   if (level) {
      qreal dx = points[3].x - points[0].x;
      qreal dy = points[3].y - points[0].y;
      qreal len = ((qreal).25) * (qAbs(dx) + qAbs(dy));

      if (qAbs(dx * (points[0].y - points[2].y) - dy * (points[0].x - points[2].x)) >= len ||
         qAbs(dx * (points[0].y - points[1].y) - dy * (points[0].x - points[1].x)) >= len) {
         splitCubic(points);

         --level;
         renderCubicSubdivision(points + 3, level, caps & CapBegin);
         renderCubicSubdivision(points, level, caps & CapEnd);
         return;
      }
   }

   stroke(this, points[3].x, points[3].y, points[0].x, points[0].y, caps);
}

static inline int swapCaps(int caps)
{
   return ((caps & QCosmeticStroker::CapBegin) << 1) |
      ((caps & QCosmeticStroker::CapEnd) >> 1);
}

// adjust line by half a pixel
static inline void capAdjust(int caps, int &x1, int &x2, int &y, int yinc)
{
   if (caps & QCosmeticStroker::CapBegin) {
      x1 -= 32;
      y -= yinc >> 1;
   }
   if (caps & QCosmeticStroker::CapEnd) {
      x2 += 32;
   }
}

/*
  The hard part about this is dropout control and avoiding douple drawing of points when
  the drawing shifts from horizontal to vertical or back.
  */
template <DrawPixel drawPixel, class Dasher>
static bool drawLine(QCosmeticStroker *stroker, qreal rx1, qreal ry1, qreal rx2, qreal ry2, int caps)
{
   bool didDraw = qAbs(rx2 - rx1) + qAbs(ry2 - ry1) >= 1.0;
   if (stroker->clipLine(rx1, ry1, rx2, ry2)) {
      return true;
   }

   const int half = stroker->legacyRounding ? 31 : 0;
   int x1 = toF26Dot6(rx1) + half;
   int y1 = toF26Dot6(ry1) + half;
   int x2 = toF26Dot6(rx2) + half;
   int y2 = toF26Dot6(ry2) + half;

   int dx = qAbs(x2 - x1);
   int dy = qAbs(y2 - y1);

   QCosmeticStroker::Point last = stroker->lastPixel;

   if (dx < dy) {
      // vertical
      QCosmeticStroker::Direction dir = QCosmeticStroker::TopToBottom;

      bool swapped = false;
      if (y1 > y2) {
         swapped = true;
         qSwap(y1, y2);
         qSwap(x1, x2);
         caps = swapCaps(caps);
         dir = QCosmeticStroker::BottomToTop;
      }

      int xinc = F16Dot16FixedDiv(x2 - x1, y2 - y1);
      int x = x1 * (1 << 10);

      if ((stroker->lastDir ^ QCosmeticStroker::VerticalMask) == dir) {
         caps |= swapped ? QCosmeticStroker::CapEnd : QCosmeticStroker::CapBegin;
      }

      capAdjust(caps, y1, y2, x, xinc);

      int y = (y1 + 32) >> 6;
      int ys = (y2 + 32) >> 6;
      int round = (xinc > 0) ? 32 : 0;

      if (y != ys) {
         x += ((y * (1 << 6)) + round - y1) * xinc >> 6;

         // calculate first and last pixel and perform dropout control
         QCosmeticStroker::Point first;
         first.x = x >> 16;
         first.y = y;
         last.x = (x + (ys - y - 1) * xinc) >> 16;
         last.y = ys - 1;
         if (swapped) {
            qSwap(first, last);
         }

         bool axisAligned = qAbs(xinc) < (1 << 14);
         if (stroker->lastPixel.x >= 0) {
            if (first.x == stroker->lastPixel.x &&
               first.y == stroker->lastPixel.y) {
               // remove duplicated pixel
               if (swapped) {
                  --ys;
               } else {
                  ++y;
                  x += xinc;
               }
            } else if (stroker->lastDir != dir &&
               (((axisAligned && stroker->lastAxisAligned) &&
                     stroker->lastPixel.x != first.x && stroker->lastPixel.y != first.y) ||
                  (qAbs(stroker->lastPixel.x - first.x) > 1 ||
                     qAbs(stroker->lastPixel.y - first.y) > 1))) {
               // have a missing pixel, insert it
               if (swapped) {
                  ++ys;
               } else {
                  --y;
                  x -= xinc;
               }
            }
         }
         stroker->lastDir = dir;
         stroker->lastAxisAligned = axisAligned;

         Dasher dasher(stroker, swapped, y * (1 << 6), ys * (1 << 6));

         do {
            if (dasher.on()) {
               drawPixel(stroker, x >> 16, y, 255);
            }
            dasher.adjust();
            x += xinc;
         } while (++y < ys);

         didDraw = true;
      }
   } else {
      // horizontal
      if (!dx) {
         return true;
      }

      QCosmeticStroker::Direction dir = QCosmeticStroker::LeftToRight;

      bool swapped = false;
      if (x1 > x2) {
         swapped = true;
         qSwap(x1, x2);
         qSwap(y1, y2);
         caps = swapCaps(caps);
         dir = QCosmeticStroker::RightToLeft;
      }
      int yinc = F16Dot16FixedDiv(y2 - y1, x2 - x1);
      int y = y1 * (1 << 10);

      if ((stroker->lastDir ^ QCosmeticStroker::HorizontalMask) == dir) {
         caps |= swapped ? QCosmeticStroker::CapEnd : QCosmeticStroker::CapBegin;
      }

      capAdjust(caps, x1, x2, y, yinc);

      int x = (x1 + 32) >> 6;
      int xs = (x2 + 32) >> 6;
      int round = (yinc > 0) ? 32 : 0;

      if (x != xs) {
         y += ((x * (1 << 6)) + round - x1) * yinc >> 6;

         // calculate first and last pixel to perform dropout control
         QCosmeticStroker::Point first;
         first.x = x;
         first.y = y >> 16;
         last.x = xs - 1;
         last.y = (y + (xs - x - 1) * yinc) >> 16;
         if (swapped) {
            qSwap(first, last);
         }

         bool axisAligned = qAbs(yinc) < (1 << 14);
         if (stroker->lastPixel.x >= 0) {
            if (first.x == stroker->lastPixel.x && first.y == stroker->lastPixel.y) {
               // remove duplicated pixel
               if (swapped) {
                  --xs;
               } else {
                  ++x;
                  y += yinc;
               }
            } else if (stroker->lastDir != dir &&
               (((axisAligned && stroker->lastAxisAligned) &&
                     stroker->lastPixel.x != first.x && stroker->lastPixel.y != first.y) ||
                  (qAbs(stroker->lastPixel.x - first.x) > 1 ||
                     qAbs(stroker->lastPixel.y - first.y) > 1))) {
               // have a missing pixel, insert it
               if (swapped) {
                  ++xs;
               } else {
                  --x;
                  y -= yinc;
               }
            }
         }
         stroker->lastDir = dir;
         stroker->lastAxisAligned = axisAligned;

         Dasher dasher(stroker, swapped, x * (1 << 6), xs * (1 << 6));

         do {
            if (dasher.on()) {
               drawPixel(stroker, x, y >> 16, 255);
            }
            dasher.adjust();
            y += yinc;
         } while (++x < xs);

         didDraw = true;
      }
   }

   stroker->lastPixel = last;
   return didDraw;
}

template <DrawPixel drawPixel, class Dasher>
static bool drawLineAA(QCosmeticStroker *stroker, qreal rx1, qreal ry1, qreal rx2, qreal ry2, int caps)
{
   if (stroker->clipLine(rx1, ry1, rx2, ry2)) {
      return true;
   }

   int x1 = toF26Dot6(rx1);
   int y1 = toF26Dot6(ry1);
   int x2 = toF26Dot6(rx2);
   int y2 = toF26Dot6(ry2);

   int dx = x2 - x1;
   int dy = y2 - y1;

   if (qAbs(dx) < qAbs(dy)) {
      // vertical

      int xinc = F16Dot16FixedDiv(dx, dy);

      bool swapped = false;
      if (y1 > y2) {
         qSwap(y1, y2);
         qSwap(x1, x2);
         swapped = true;
         caps = swapCaps(caps);
      }

      int x = (x1 - 32) * (1 << 10);
      x -= ( ((y1 & 63) - 32)  * xinc ) >> 6;

      capAdjust(caps, y1, y2, x, xinc);

      Dasher dasher(stroker, swapped, y1, y2);

      int y = y1 >> 6;
      int ys = y2 >> 6;

      int alphaStart, alphaEnd;
      if (y == ys) {
         alphaStart = y2 - y1;
         Q_ASSERT(alphaStart >= 0 && alphaStart < 64);
         alphaEnd = 0;
      } else {
         alphaStart = 64 - (y1 & 63);
         alphaEnd = (y2 & 63);
      }

      // draw first pixel
      if (dasher.on()) {
         uint alpha = (quint8)(x >> 8);
         drawPixel(stroker, x >> 16, y, (255 - alpha) * alphaStart >> 6);
         drawPixel(stroker, (x >> 16) + 1, y, alpha * alphaStart >> 6);
      }

      dasher.adjust();
      x += xinc;
      ++y;

      if (y < ys) {
         do {
            if (dasher.on()) {
               uint alpha = (quint8)(x >> 8);
               drawPixel(stroker, x >> 16, y, (255 - alpha));
               drawPixel(stroker, (x >> 16) + 1, y, alpha);
            }
            dasher.adjust();
            x += xinc;
         } while (++y < ys);
      }
      // draw last pixel
      if (alphaEnd && dasher.on()) {
         uint alpha = (quint8)(x >> 8);
         drawPixel(stroker, x >> 16, y, (255 - alpha) * alphaEnd >> 6);
         drawPixel(stroker, (x >> 16) + 1, y, alpha * alphaEnd >> 6);
      }
   } else {
      // horizontal
      if (!dx) {
         return true;
      }

      int yinc = F16Dot16FixedDiv(dy, dx);

      bool swapped = false;
      if (x1 > x2) {
         qSwap(x1, x2);
         qSwap(y1, y2);
         swapped = true;
         caps = swapCaps(caps);
      }

      int y = (y1 - 32) * (1 << 10);
      y -= ( ((x1 & 63) - 32)  * yinc ) >> 6;

      capAdjust(caps, x1, x2, y, yinc);

      Dasher dasher(stroker, swapped, x1, x2);

      int x  = x1 >> 6;
      int xs = x2 >> 6;

      int alphaStart, alphaEnd;

      if (x == xs) {
         alphaStart = x2 - x1;
         Q_ASSERT(alphaStart >= 0 && alphaStart < 64);
         alphaEnd = 0;
      } else {
         alphaStart = 64 - (x1 & 63);
         alphaEnd = (x2 & 63);
      }

      // draw first pixel
      if (dasher.on()) {
         uint alpha = (quint8)(y >> 8);
         drawPixel(stroker, x, y >> 16, (255 - alpha) * alphaStart >> 6);
         drawPixel(stroker, x, (y >> 16) + 1, alpha * alphaStart >> 6);
      }
      dasher.adjust();
      y += yinc;
      ++x;
      // draw line
      if (x < xs) {
         do {
            if (dasher.on()) {
               uint alpha = (quint8)(y >> 8);
               drawPixel(stroker, x, y >> 16, (255 - alpha));
               drawPixel(stroker, x, (y >> 16) + 1, alpha);
            }
            dasher.adjust();
            y += yinc;
         } while (++x < xs);
      }
      // draw last pixel
      if (alphaEnd && dasher.on()) {
         uint alpha = (quint8)(y >> 8);
         drawPixel(stroker, x, y >> 16, (255 - alpha) * alphaEnd >> 6);
         drawPixel(stroker, x, (y >> 16) + 1, alpha * alphaEnd >> 6);
      }
   }
   return true;
}
