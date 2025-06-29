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

#include <qpixmapfilter_p.h>

#include <qdebug.h>
#include <qglobal.h>
#include <qmath.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qvarlengtharray.h>

#include <qapplication_p.h>
#include <qdrawhelper_p.h>
#include <qmath_p.h>
#include <qmemrotate_p.h>
#include <qpaintengine_raster_p.h>
#include <qpaintengineex_p.h>

#ifndef QT_NO_GRAPHICSEFFECT

class QPixmapFilterPrivate
{
   Q_DECLARE_PUBLIC(QPixmapFilter)

 public:
   QPixmapFilter::FilterType type;

 protected:
   QPixmapFilter *q_ptr;
};

QPixmapFilter::QPixmapFilter(FilterType type, QObject *parent)
   : QObject(parent), d_ptr(new QPixmapFilterPrivate)
{
   d_ptr->q_ptr   = this;
   d_func()->type = type;
}

QPixmapFilter::QPixmapFilter(QPixmapFilterPrivate &d, QPixmapFilter::FilterType type, QObject *parent)
   : QObject(parent), d_ptr(&d)
{
   d_ptr->q_ptr   = this;
   d_func()->type = type;
}

QPixmapFilter::~QPixmapFilter()
{
}

QPixmapFilter::FilterType QPixmapFilter::type() const
{
   Q_D(const QPixmapFilter);
   return d->type;
}

QRectF QPixmapFilter::boundingRectFor(const QRectF &rect) const
{
   return rect;
}

class QPixmapConvolutionFilterPrivate : public QPixmapFilterPrivate
{
 public:
   QPixmapConvolutionFilterPrivate()
      : convolutionKernel(nullptr), kernelWidth(0), kernelHeight(0), convoluteAlpha(false)
   {
   }

   ~QPixmapConvolutionFilterPrivate() {
      delete[] convolutionKernel;
   }

   qreal *convolutionKernel;
   int kernelWidth;
   int kernelHeight;
   bool convoluteAlpha;
};

QPixmapConvolutionFilter::QPixmapConvolutionFilter(QObject *parent)
   : QPixmapFilter(*new QPixmapConvolutionFilterPrivate, ConvolutionFilter, parent)
{
   Q_D(QPixmapConvolutionFilter);
   d->convoluteAlpha = true;
}

QPixmapConvolutionFilter::~QPixmapConvolutionFilter()
{
}

void QPixmapConvolutionFilter::setConvolutionKernel(const qreal *kernel, int rows, int columns)
{
   Q_D(QPixmapConvolutionFilter);
   delete [] d->convolutionKernel;
   d->convolutionKernel = new qreal[rows * columns];
   memcpy(d->convolutionKernel, kernel, sizeof(qreal) * rows * columns);
   d->kernelWidth = columns;
   d->kernelHeight = rows;
}

const qreal *QPixmapConvolutionFilter::convolutionKernel() const
{
   Q_D(const QPixmapConvolutionFilter);
   return d->convolutionKernel;
}

int QPixmapConvolutionFilter::rows() const
{
   Q_D(const QPixmapConvolutionFilter);
   return d->kernelHeight;
}

int QPixmapConvolutionFilter::columns() const
{
   Q_D(const QPixmapConvolutionFilter);
   return d->kernelWidth;
}

QRectF QPixmapConvolutionFilter::boundingRectFor(const QRectF &rect) const
{
   Q_D(const QPixmapConvolutionFilter);
   return rect.adjusted(-d->kernelWidth / 2, -d->kernelHeight / 2, (d->kernelWidth - 1) / 2, (d->kernelHeight - 1) / 2);
}

// Convolutes the image
static void convolute(QImage *destImage, const QPointF &pos, const QImage &srcImage, const QRectF &srcRect,
      QPainter::CompositionMode mode, qreal *kernel, int kernelWidth, int kernelHeight )
{
   const QImage processImage = (srcImage.format() != QImage::Format_ARGB32_Premultiplied )
         ? srcImage.convertToFormat(QImage::Format_ARGB32_Premultiplied) : srcImage;

   // TODO: support also other formats directly without copying

   int *fixedKernel = new int[kernelWidth * kernelHeight];

   for (int i = 0; i < kernelWidth * kernelHeight; i++) {
      fixedKernel[i] = (int)(65536 * kernel[i]);
   }

   QRectF trect = srcRect.isNull() ? processImage.rect() : srcRect;
   trect.moveTo(pos);

   QRectF bounded = trect.adjusted(-kernelWidth / 2, -kernelHeight / 2, (kernelWidth - 1) / 2, (kernelHeight - 1) / 2);
   QRect rect = bounded.toAlignedRect();
   QRect targetRect = rect.intersected(destImage->rect());

   QRectF srect = srcRect.isNull() ? processImage.rect() : srcRect;
   QRectF sbounded = srect.adjusted(-kernelWidth / 2, -kernelHeight / 2, (kernelWidth - 1) / 2, (kernelHeight - 1) / 2);
   QPoint srcStartPoint = sbounded.toAlignedRect().topLeft() + (targetRect.topLeft() - rect.topLeft());

   const uint *sourceStart = (const uint *)processImage.scanLine(0);
   uint *outputStart = (uint *)destImage->scanLine(0);

   int yk = srcStartPoint.y();

   for (int y = targetRect.top(); y <= targetRect.bottom(); y++) {
      uint *output = outputStart + (destImage->bytesPerLine() / sizeof(uint)) * y + targetRect.left();
      int xk = srcStartPoint.x();

      for (int x = targetRect.left(); x <= targetRect.right(); x++) {
         int r = 0;
         int g = 0;
         int b = 0;
         int a = 0;

         // some out of bounds pre-checking to avoid inner-loop ifs
         int kernely = -kernelHeight / 2;
         int starty = 0;
         int endy = kernelHeight;

         if (yk + kernely + endy >= srcImage.height()) {
            endy = kernelHeight - ((yk + kernely + endy) - srcImage.height()) - 1;
         }

         if (yk + kernely < 0) {
            starty = -(yk + kernely);
         }

         int kernelx = -kernelWidth / 2;
         int startx = 0;
         int endx = kernelWidth;

         if (xk + kernelx + endx >= srcImage.width()) {
            endx = kernelWidth - ((xk + kernelx + endx) - srcImage.width()) - 1;
         }

         if (xk + kernelx < 0) {
            startx = -(xk + kernelx);
         }

         for (int ys = starty; ys < endy; ys ++) {
            const uint *pix = sourceStart + (processImage.bytesPerLine() / sizeof(uint)) * (yk + kernely + ys) +
                  ((xk + kernelx + startx));

            const uint *endPix = pix + endx - startx;
            int kernelPos = ys * kernelWidth + startx;

            while (pix < endPix) {
               int factor = fixedKernel[kernelPos++];
               a += (((*pix) & 0xff000000) >> 24) * factor;
               r += (((*pix) & 0x00ff0000) >> 16) * factor;
               g += (((*pix) & 0x0000ff00) >> 8 ) * factor;
               b += (((*pix) & 0x000000ff)    ) * factor;
               ++pix;
            }
         }

         r = qBound((int)0, r >> 16, (int)255);
         g = qBound((int)0, g >> 16, (int)255);
         b = qBound((int)0, b >> 16, (int)255);
         a = qBound((int)0, a >> 16, (int)255);

         // composition mode checking could be moved outside of loop

         if (mode == QPainter::CompositionMode_Source) {
            uint color = (a << 24) + (r << 16) + (g << 8) + b;
            *output++ = color;

         } else {
            uint current = *output;
            uchar ca = (current & 0xff000000) >> 24;
            uchar cr = (current & 0x00ff0000) >> 16;
            uchar cg = (current & 0x0000ff00) >> 8;
            uchar cb = (current & 0x000000ff);

            uint color =
               (((ca * (255 - a) >> 8) + a) << 24) +
               (((cr * (255 - a) >> 8) + r) << 16) +
               (((cg * (255 - a) >> 8) + g) << 8) +
               (((cb * (255 - a) >> 8) + b));

            *output++ = color;;
         }

         ++xk;
      }

      ++yk;
   }
   delete[] fixedKernel;
}

void QPixmapConvolutionFilter::draw(QPainter *painter, const QPointF &p, const QPixmap &src, const QRectF &srcRect) const
{
   Q_D(const QPixmapConvolutionFilter);

   if (! painter->isActive()) {
      return;
   }

   if (d->kernelWidth <= 0 || d->kernelHeight <= 0) {
      return;
   }

   if (src.isNull()) {
      return;
   }

   // raster implementation
   QImage *target = nullptr;

   if (painter->paintEngine()->paintDevice()->devType() == QInternal::Image) {
      target = static_cast<QImage *>(painter->paintEngine()->paintDevice());

      QTransform mat = painter->combinedTransform();

      if (mat.type() > QTransform::TxTranslate) {
         // Disabled because of transformation
         target = nullptr;

      } else {
         QRasterPaintEngine *pe = static_cast<QRasterPaintEngine *>(painter->paintEngine());
         if (pe->clipType() == QRasterPaintEngine::ComplexClip)
            // disabled because of complex clipping...
         {
            target = nullptr;

         } else {
            QRectF clip = pe->clipBoundingRect();
            QRectF rect = boundingRectFor(srcRect.isEmpty() ? src.rect() : srcRect);
            QTransform x = painter->deviceTransform();
            if (!clip.contains(rect.translated(x.dx() + p.x(), x.dy() + p.y()))) {
               target = nullptr;
            }

         }
      }
   }

   if (target) {
      QTransform x = painter->deviceTransform();
      QPointF offset(x.dx(), x.dy());

      convolute(target, p + offset, src.toImage(), srcRect, QPainter::CompositionMode_SourceOver,
            d->convolutionKernel, d->kernelWidth, d->kernelHeight);

   } else {
      QRect srect = srcRect.isNull() ? src.rect() : srcRect.toRect();
      QRect rect = boundingRectFor(srect).toRect();
      QImage result = QImage(rect.size(), QImage::Format_ARGB32_Premultiplied);
      QPoint offset = srect.topLeft() - rect.topLeft();

      convolute(&result, offset, src.toImage(), srect, QPainter::CompositionMode_Source, d->convolutionKernel,
            d->kernelWidth, d->kernelHeight);

      painter->drawImage(p - offset, result);
   }
}

class QPixmapBlurFilterPrivate : public QPixmapFilterPrivate
{
 public:
   QPixmapBlurFilterPrivate() : radius(5), hints(QGraphicsBlurEffect::PerformanceHint) {}

   qreal radius;
   QGraphicsBlurEffect::BlurHints hints;
};

QPixmapBlurFilter::QPixmapBlurFilter(QObject *parent)
   : QPixmapFilter(*new QPixmapBlurFilterPrivate, BlurFilter, parent)
{
}

QPixmapBlurFilter::~QPixmapBlurFilter()
{
}

void QPixmapBlurFilter::setRadius(qreal radius)
{
   Q_D(QPixmapBlurFilter);
   d->radius = radius;
}

qreal QPixmapBlurFilter::radius() const
{
   Q_D(const QPixmapBlurFilter);
   return d->radius;
}

void QPixmapBlurFilter::setBlurHints(QGraphicsBlurEffect::BlurHints hints)
{
   Q_D(QPixmapBlurFilter);
   d->hints = hints;
}

QGraphicsBlurEffect::BlurHints QPixmapBlurFilter::blurHints() const
{
   Q_D(const QPixmapBlurFilter);
   return d->hints;
}

const qreal radiusScale = qreal(2.5);

QRectF QPixmapBlurFilter::boundingRectFor(const QRectF &rect) const
{
   Q_D(const QPixmapBlurFilter);
   const qreal delta = radiusScale * d->radius + 1;
   return rect.adjusted(-delta, -delta, delta, delta);
}

template <int shift>
inline int qt_static_shift(int value)
{
   if constexpr (shift == 0) {
      return value;

   } else if constexpr (shift > 0) {
      return value << (uint(shift) & 0x1f);

   } else {
      return value >> (uint(-shift) & 0x1f);
   }
}

template <int aprec, int zprec>
inline void qt_blurinner(uchar *bptr, int &zR, int &zG, int &zB, int &zA, int alpha)
{
   QRgb *pixel = (QRgb *)bptr;

#define Z_MASK (0xff << zprec)
   const int A_zprec = qt_static_shift < zprec - 24 > (*pixel) & Z_MASK;
   const int R_zprec = qt_static_shift < zprec - 16 > (*pixel) & Z_MASK;
   const int G_zprec = qt_static_shift < zprec - 8 > (*pixel)  & Z_MASK;
   const int B_zprec = qt_static_shift<zprec>(*pixel)      & Z_MASK;
#undef Z_MASK

   const int zR_zprec = zR >> aprec;
   const int zG_zprec = zG >> aprec;
   const int zB_zprec = zB >> aprec;
   const int zA_zprec = zA >> aprec;

   zR += alpha * (R_zprec - zR_zprec);
   zG += alpha * (G_zprec - zG_zprec);
   zB += alpha * (B_zprec - zB_zprec);
   zA += alpha * (A_zprec - zA_zprec);

#define ZA_MASK (0xff << (zprec + aprec))
   *pixel =
      qt_static_shift < 24 - zprec - aprec > (zA & ZA_MASK)
      | qt_static_shift < 16 - zprec - aprec > (zR & ZA_MASK)
      | qt_static_shift < 8 - zprec - aprec > (zG & ZA_MASK)
      | qt_static_shift < -zprec - aprec > (zB & ZA_MASK);
#undef ZA_MASK
}

const int alphaIndex = (QSysInfo::ByteOrder == QSysInfo::BigEndian ? 0 : 3);

template <int aprec, int zprec>
inline void qt_blurinner_alphaOnly(uchar *bptr, int &z, int alpha)
{
   const int A_zprec = int(*(bptr)) << zprec;
   const int z_zprec = z >> aprec;
   z += alpha * (A_zprec - z_zprec);
   *(bptr) = z >> (zprec + aprec);
}

template <int aprec, int zprec, bool alphaOnly>
inline void qt_blurrow(QImage &im, int line, int alpha)
{
   uchar *bptr = im.scanLine(line);

   int zR = 0, zG = 0, zB = 0, zA = 0;

   if (alphaOnly && im.format() != QImage::Format_Indexed8) {
      bptr += alphaIndex;
   }

   const int stride = im.depth() >> 3;
   const int im_width = im.width();
   for (int index = 0; index < im_width; ++index) {
      if (alphaOnly) {
         qt_blurinner_alphaOnly<aprec, zprec>(bptr, zA, alpha);
      } else {
         qt_blurinner<aprec, zprec>(bptr, zR, zG, zB, zA, alpha);
      }
      bptr += stride;
   }

   bptr -= stride;

   for (int index = im_width - 2; index >= 0; --index) {
      bptr -= stride;
      if (alphaOnly) {
         qt_blurinner_alphaOnly<aprec, zprec>(bptr, zA, alpha);
      } else {
         qt_blurinner<aprec, zprec>(bptr, zR, zG, zB, zA, alpha);
      }
   }
}

template <int aprec, int zprec, bool alphaOnly>
void expblur(QImage &img, qreal radius, bool improvedQuality = false, int transposed = 0)
{
   // halve the radius if we're using two passes
   if (improvedQuality) {
      radius *= qreal(0.5);
   }

   Q_ASSERT(img.format() == QImage::Format_ARGB32_Premultiplied
      || img.format() == QImage::Format_RGB32
      || img.format() == QImage::Format_Indexed8
      || img.format() == QImage::Format_Grayscale8);

   // choose the alpha such that pixels at radius distance from a fully
   // saturated pixel will have an alpha component of no greater than
   // the cutOffIntensity
   const qreal cutOffIntensity = 2;
   int alpha = radius <= qreal(1e-5)
      ? ((1 << aprec) - 1)
      : qRound((1 << aprec) * (1 - qPow(cutOffIntensity * (1 / qreal(255)), 1 / radius)));

   int img_height = img.height();
   for (int row = 0; row < img_height; ++row) {
      for (int i = 0; i <= int(improvedQuality); ++i) {
         qt_blurrow<aprec, zprec, alphaOnly>(img, row, alpha);
      }
   }

   QImage temp(img.height(), img.width(), img.format());
   if (transposed >= 0) {
      if (img.depth() == 8) {
         qt_memrotate270(reinterpret_cast<const quint8 *>(img.bits()),
            img.width(), img.height(), img.bytesPerLine(),
            reinterpret_cast<quint8 *>(temp.bits()),
            temp.bytesPerLine());
      } else {
         qt_memrotate270(reinterpret_cast<const quint32 *>(img.bits()),
            img.width(), img.height(), img.bytesPerLine(),
            reinterpret_cast<quint32 *>(temp.bits()),
            temp.bytesPerLine());
      }
   } else {
      if (img.depth() == 8) {
         qt_memrotate90(reinterpret_cast<const quint8 *>(img.bits()),
            img.width(), img.height(), img.bytesPerLine(),
            reinterpret_cast<quint8 *>(temp.bits()),
            temp.bytesPerLine());
      } else {
         qt_memrotate90(reinterpret_cast<const quint32 *>(img.bits()),
            img.width(), img.height(), img.bytesPerLine(),
            reinterpret_cast<quint32 *>(temp.bits()),
            temp.bytesPerLine());
      }
   }

   img_height = temp.height();
   for (int row = 0; row < img_height; ++row) {
      for (int i = 0; i <= int(improvedQuality); ++i) {
         qt_blurrow<aprec, zprec, alphaOnly>(temp, row, alpha);
      }
   }

   if (transposed == 0) {
      if (img.depth() == 8) {
         qt_memrotate90(reinterpret_cast<const quint8 *>(temp.bits()),
            temp.width(), temp.height(), temp.bytesPerLine(),
            reinterpret_cast<quint8 *>(img.bits()),
            img.bytesPerLine());
      } else {
         qt_memrotate90(reinterpret_cast<const quint32 *>(temp.bits()),
            temp.width(), temp.height(), temp.bytesPerLine(),
            reinterpret_cast<quint32 *>(img.bits()),
            img.bytesPerLine());
      }
   } else {
      img = temp;
   }
}
#define AVG(a,b)  ( ((((a)^(b)) & 0xfefefefeUL) >> 1) + ((a)&(b)) )
#define AVG16(a,b)  ( ((((a)^(b)) & 0xf7deUL) >> 1) + ((a)&(b)) )

Q_GUI_EXPORT QImage qt_halfScaled(const QImage &source)
{
   if (source.width() < 2 || source.height() < 2) {
      return QImage();
   }

   QImage srcImage = source;

   if (source.format() == QImage::Format_Indexed8 || source.format() == QImage::Format_Grayscale8) {
      // assumes grayscale
      QImage dest(source.width() / 2, source.height() / 2, srcImage.format());

      const uchar *src = reinterpret_cast<const uchar *>(const_cast<const QImage &>(srcImage).bits());
      int sx = srcImage.bytesPerLine();
      int sx2 = sx << 1;

      uchar *dst = reinterpret_cast<uchar *>(dest.bits());
      int dx = dest.bytesPerLine();
      int ww = dest.width();
      int hh = dest.height();

      for (int y = hh; y; --y, dst += dx, src += sx2) {
         const uchar *p1 = src;
         const uchar *p2 = src + sx;
         uchar *q = dst;
         for (int x = ww; x; --x, ++q, p1 += 2, p2 += 2) {
            *q = ((int(p1[0]) + int(p1[1]) + int(p2[0]) + int(p2[1])) + 2) >> 2;
         }
      }

      return dest;
   } else if (source.format() == QImage::Format_ARGB8565_Premultiplied) {
      QImage dest(source.width() / 2, source.height() / 2, srcImage.format());

      const uchar *src = reinterpret_cast<const uchar *>(const_cast<const QImage &>(srcImage).bits());
      int sx = srcImage.bytesPerLine();
      int sx2 = sx << 1;

      uchar *dst = reinterpret_cast<uchar *>(dest.bits());
      int dx = dest.bytesPerLine();
      int ww = dest.width();
      int hh = dest.height();

      for (int y = hh; y; --y, dst += dx, src += sx2) {
         const uchar *p1 = src;
         const uchar *p2 = src + sx;
         uchar *q = dst;
         for (int x = ww; x; --x, q += 3, p1 += 6, p2 += 6) {
            // alpha
            q[0] = AVG(AVG(p1[0], p1[3]), AVG(p2[0], p2[3]));
            // rgb
            const quint16 p16_1 = (p1[2] << 8) | p1[1];
            const quint16 p16_2 = (p1[5] << 8) | p1[4];
            const quint16 p16_3 = (p2[2] << 8) | p2[1];
            const quint16 p16_4 = (p2[5] << 8) | p2[4];
            const quint16 result = AVG16(AVG16(p16_1, p16_2), AVG16(p16_3, p16_4));
            q[1] = result & 0xff;
            q[2] = result >> 8;
         }
      }

      return dest;
   } else if (source.format() != QImage::Format_ARGB32_Premultiplied
      && source.format() != QImage::Format_RGB32) {
      srcImage = source.convertToFormat(QImage::Format_ARGB32_Premultiplied);
   }

   QImage dest(source.width() / 2, source.height() / 2, srcImage.format());

   const quint32 *src = reinterpret_cast<const quint32 *>(const_cast<const QImage &>(srcImage).bits());
   int sx = srcImage.bytesPerLine() >> 2;
   int sx2 = sx << 1;

   quint32 *dst = reinterpret_cast<quint32 *>(dest.bits());
   int dx = dest.bytesPerLine() >> 2;
   int ww = dest.width();
   int hh = dest.height();

   for (int y = hh; y; --y, dst += dx, src += sx2) {
      const quint32 *p1 = src;
      const quint32 *p2 = src + sx;
      quint32 *q = dst;
      for (int x = ww; x; --x, q++, p1 += 2, p2 += 2) {
         *q = AVG(AVG(p1[0], p1[1]), AVG(p2[0], p2[1]));
      }
   }

   return dest;
}

Q_GUI_EXPORT void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0)
{
   if (blurImage.format() != QImage::Format_ARGB32_Premultiplied
      && blurImage.format() != QImage::Format_RGB32) {
      blurImage = blurImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);
   }

   qreal scale = 1;
   if (radius >= 4 && blurImage.width() >= 2 && blurImage.height() >= 2) {
      blurImage = qt_halfScaled(blurImage);
      scale = 2;
      radius *= qreal(0.5);
   }

   if (alphaOnly) {
      expblur<12, 10, true>(blurImage, radius, quality, transposed);
   } else {
      expblur<12, 10, false>(blurImage, radius, quality, transposed);
   }

   if (p) {
      p->scale(scale, scale);
      p->setRenderHint(QPainter::SmoothPixmapTransform);
      p->drawImage(QRect(0, 0, blurImage.width(), blurImage.height()), blurImage);
   }
}

Q_GUI_EXPORT void qt_blurImage(QImage &blurImage, qreal radius, bool quality, int transposed = 0)
{
   if (blurImage.format() == QImage::Format_Indexed8 || blurImage.format() == QImage::Format_Grayscale8) {
      expblur<12, 10, true>(blurImage, radius, quality, transposed);
   } else {
      expblur<12, 10, false>(blurImage, radius, quality, transposed);
   }
}

Q_GUI_EXPORT extern bool qt_scaleForTransform(const QTransform &transform, qreal *scale);

void QPixmapBlurFilter::draw(QPainter *painter, const QPointF &p, const QPixmap &src, const QRectF &rectF) const
{
   Q_D(const QPixmapBlurFilter);

   if (! painter->isActive()) {
      return;
   }

   if (src.isNull()) {
      return;
   }

   QRectF srcRect = rectF;

   if (srcRect.isNull()) {
      srcRect = src.rect();
   }

   if (d->radius <= 1) {
      painter->drawPixmap(srcRect.translated(p), src, srcRect);
      return;
   }

   qreal scaledRadius = radiusScale * d->radius;
   qreal scale;
   if (qt_scaleForTransform(painter->transform(), &scale)) {
      scaledRadius /= scale;
   }

   QImage srcImage;
   QImage destImage;

   if (srcRect == src.rect()) {
      srcImage = src.toImage();

   } else {
      QRect rect = srcRect.toAlignedRect().intersected(src.rect());
      srcImage = src.copy(rect).toImage();
   }

   QTransform transform = painter->worldTransform();
   painter->translate(p);

   qt_blurImage(painter, srcImage, scaledRadius, (d->hints & QGraphicsBlurEffect::QualityHint), false);
   painter->setWorldTransform(transform);
}

// grayscales the image to dest (could be same). If rect is not defined
// destination image size is used to determine the dimension of grayscaling rocess.
static void grayscale(const QImage &image, QImage &dest, const QRect &rect = QRect())
{
   QRect destRect = rect;
   QRect srcRect  = rect;

   if (rect.isNull()) {
      srcRect = dest.rect();
      destRect = dest.rect();
   }

   if (&image != &dest) {
      destRect.moveTo(QPoint(0, 0));
   }

   const unsigned int *data = (const unsigned int *)image.bits();
   unsigned int *outData = (unsigned int *)dest.bits();

   if (dest.size() == image.size() && image.rect() == srcRect) {
      // a bit faster loop for grayscaling everything
      int pixels = dest.width() * dest.height();
      for (int i = 0; i < pixels; ++i) {
         int val = qGray(data[i]);
         outData[i] = qRgba(val, val, val, qAlpha(data[i]));
      }
   } else {
      int yd = destRect.top();
      for (int y = srcRect.top(); y <= srcRect.bottom() && y < image.height(); y++) {
         data = (const unsigned int *)image.scanLine(y);
         outData = (unsigned int *)dest.scanLine(yd++);
         int xd = destRect.left();
         for (int x = srcRect.left(); x <= srcRect.right() && x < image.width(); x++) {
            int val = qGray(data[x]);
            outData[xd++] = qRgba(val, val, val, qAlpha(data[x]));
         }
      }
   }
}

class QPixmapColorizeFilterPrivate : public QPixmapFilterPrivate
{
   Q_DECLARE_PUBLIC(QPixmapColorizeFilter)
 public:
   QColor color;
   qreal strength;
   quint32 opaque : 1;
   quint32 alphaBlend : 1;
   quint32 padding : 30;
};

QPixmapColorizeFilter::QPixmapColorizeFilter(QObject *parent)
   : QPixmapFilter(*new QPixmapColorizeFilterPrivate, ColorizeFilter, parent)
{
   Q_D(QPixmapColorizeFilter);
   d->color = QColor(0, 0, 192);
   d->strength = qreal(1);
   d->opaque = true;
   d->alphaBlend = false;
}

QColor QPixmapColorizeFilter::color() const
{
   Q_D(const QPixmapColorizeFilter);
   return d->color;
}

void QPixmapColorizeFilter::setColor(const QColor &color)
{
   Q_D(QPixmapColorizeFilter);
   d->color = color;
}

qreal QPixmapColorizeFilter::strength() const
{
   Q_D(const QPixmapColorizeFilter);
   return d->strength;
}

void QPixmapColorizeFilter::setStrength(qreal strength)
{
   Q_D(QPixmapColorizeFilter);
   d->strength = qBound(qreal(0), strength, qreal(1));
   d->opaque = !qFuzzyIsNull(d->strength);
   d->alphaBlend = !qFuzzyIsNull(d->strength - 1);
}

void QPixmapColorizeFilter::draw(QPainter *painter, const QPointF &dest, const QPixmap &src, const QRectF &srcRect) const
{
   Q_D(const QPixmapColorizeFilter);

   if (src.isNull()) {
      return;
   }

   // raster implementation

   if (!d->opaque) {
      painter->drawPixmap(dest, src, srcRect);
      return;
   }

   QImage srcImage;
   QImage destImage;

   if (srcRect.isNull()) {
      srcImage = src.toImage();
      srcImage = srcImage.convertToFormat(srcImage.hasAlphaChannel() ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32);
      destImage = QImage(srcImage.size(), srcImage.format());
   } else {
      QRect rect = srcRect.toAlignedRect().intersected(src.rect());

      srcImage = src.copy(rect).toImage();
      srcImage = srcImage.convertToFormat(srcImage.hasAlphaChannel() ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32);
      destImage = QImage(rect.size(), srcImage.format());
   }

   // do colorizing
   QPainter destPainter(&destImage);
   grayscale(srcImage, destImage, srcImage.rect());
   destPainter.setCompositionMode(QPainter::CompositionMode_Screen);
   destPainter.fillRect(srcImage.rect(), d->color);
   destPainter.end();

   if (d->alphaBlend) {
      // alpha blending srcImage and destImage
      QImage buffer = srcImage;
      QPainter bufPainter(&buffer);
      bufPainter.setOpacity(d->strength);
      bufPainter.drawImage(0, 0, destImage);
      bufPainter.end();
      destImage = buffer;
   }

   if (srcImage.hasAlphaChannel()) {
      destImage.setAlphaChannel(srcImage.alphaChannel());
   }

   painter->drawImage(dest, destImage);
}

class QPixmapDropShadowFilterPrivate : public QPixmapFilterPrivate
{
 public:
   QPixmapDropShadowFilterPrivate()
      : offset(8, 8), color(63, 63, 63, 180), radius(1) {}

   QPointF offset;
   QColor color;
   qreal radius;
};

QPixmapDropShadowFilter::QPixmapDropShadowFilter(QObject *parent)
   : QPixmapFilter(*new QPixmapDropShadowFilterPrivate, DropShadowFilter, parent)
{
}

QPixmapDropShadowFilter::~QPixmapDropShadowFilter()
{
}

qreal QPixmapDropShadowFilter::blurRadius() const
{
   Q_D(const QPixmapDropShadowFilter);
   return d->radius;
}

void QPixmapDropShadowFilter::setBlurRadius(qreal radius)
{
   Q_D(QPixmapDropShadowFilter);
   d->radius = radius;
}

QColor QPixmapDropShadowFilter::color() const
{
   Q_D(const QPixmapDropShadowFilter);
   return d->color;
}

void QPixmapDropShadowFilter::setColor(const QColor &color)
{
   Q_D(QPixmapDropShadowFilter);
   d->color = color;
}

QPointF QPixmapDropShadowFilter::offset() const
{
   Q_D(const QPixmapDropShadowFilter);
   return d->offset;
}

void QPixmapDropShadowFilter::setOffset(const QPointF &offset)
{
   Q_D(QPixmapDropShadowFilter);
   d->offset = offset;
}

QRectF QPixmapDropShadowFilter::boundingRectFor(const QRectF &rect) const
{
   Q_D(const QPixmapDropShadowFilter);
   return rect.united(rect.translated(d->offset).adjusted(-d->radius, -d->radius, d->radius, d->radius));
}

void QPixmapDropShadowFilter::draw(QPainter *p, const QPointF &pos, const QPixmap &px, const QRectF &src) const
{
   Q_D(const QPixmapDropShadowFilter);

   if (px.isNull()) {
      return;
   }

   QImage tmp(px.size(), QImage::Format_ARGB32_Premultiplied);
   tmp.fill(0);

   QPainter tmpPainter(&tmp);
   tmpPainter.setCompositionMode(QPainter::CompositionMode_Source);
   tmpPainter.drawPixmap(d->offset, px);
   tmpPainter.end();

   // blur the alpha channel
   QImage blurred(tmp.size(), QImage::Format_ARGB32_Premultiplied);
   blurred.fill(0);
   QPainter blurPainter(&blurred);
   qt_blurImage(&blurPainter, tmp, d->radius, false, true);
   blurPainter.end();

   tmp = blurred;

   // blacken the image...
   tmpPainter.begin(&tmp);
   tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
   tmpPainter.fillRect(tmp.rect(), d->color);
   tmpPainter.end();

   // draw the blurred drop shadow...
   p->drawImage(pos, tmp);

   // Draw the actual pixmap...
   p->drawPixmap(pos, px, src);
}

#endif
