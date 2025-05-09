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

#ifndef QPRINTENGINE_MAC_P_H
#define QPRINTENGINE_MAC_P_H

#include <qglobal.h>

#ifndef QT_NO_PRINTER

#include <qcocoaprintdevice.h>
#include <qpagelayout.h>
#include <qpainter_p.h>
#include <qprintengine.h>
#include <qprinter.h>

#include <qpaintengine_mac_p.h>

class QMacPrintEnginePrivate;
class QPrinterPrivate;

#ifdef __OBJC__
@class NSPrintInfo;
#else
typedef void NSPrintInfo;
#endif

class QMacPrintEngine : public QPaintEngine, public QPrintEngine
{
   Q_DECLARE_PRIVATE(QMacPrintEngine)

 public:
   QMacPrintEngine(QPrinter::PrinterMode mode);

   Qt::HANDLE handle() const;

   bool begin(QPaintDevice *dev) override;
   bool end() override;

   virtual QPaintEngine::Type type() const override {
      return QPaintEngine::MacPrinter;
   }

   QPaintEngine *paintEngine() const;

   void setProperty(PrintEnginePropertyKey key, const QVariant &value) override;
   QVariant property(PrintEnginePropertyKey key) const override;

   QPrinter::PrinterState printerState() const override;

   bool newPage() override;
   bool abort() override;
   int metric(QPaintDevice::PaintDeviceMetric) const override;

   void updateState(const QPaintEngineState &state) override;

   void drawLines(const QLineF *lines, int lineCount) override;
   void drawRects(const QRectF *r, int num) override;
   void drawPoints(const QPointF *p, int pointCount) override;
   void drawEllipse(const QRectF &r) override;
   void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode) override;
   void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) override;
   void drawImage(const QRectF &r, const QImage &pm, const QRectF &sr, Qt::ImageConversionFlags flags) override;
   void drawTextItem(const QPointF &p, const QTextItem &ti) override;
   void drawTiledPixmap(const QRectF &r, const QPixmap &pixmap, const QPointF &s) override;
   void drawPath(const QPainterPath &) override;

 private:
   friend class QCocoaNativeInterface;
};

class QMacPrintEnginePrivate : public QPaintEnginePrivate
{
   Q_DECLARE_PUBLIC(QMacPrintEngine)
 public:
   QPrinter::PrinterMode mode;
   QPrinter::PrinterState state;
   QSharedPointer<QCocoaPrintDevice> m_printDevice;
   QPageLayout m_pageLayout;
   NSPrintInfo *printInfo;
   PMResolution resolution;
   QString outputFilename;
   QString m_creator;
   QPaintEngine *paintEngine;
   QHash<QMacPrintEngine::PrintEnginePropertyKey, QVariant> valueCache;
   uint embedFonts;

   QMacPrintEnginePrivate() : mode(QPrinter::ScreenResolution), state(QPrinter::Idle),
      m_pageLayout(QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, QMarginsF(0, 0, 0, 0))),
      printInfo(nullptr), paintEngine(nullptr), embedFonts(true)
   {
   }

   ~QMacPrintEnginePrivate();

   void initialize();
   void releaseSession();
   bool newPage_helper();
   void setPageSize(const QPageSize &pageSize);
   inline bool isPrintSessionInitialized() const {
      return printInfo != nullptr;
   }

   PMPageFormat format() const {
      return static_cast<PMPageFormat>([printInfo PMPageFormat]);
   }
   PMPrintSession session() const {
      return static_cast<PMPrintSession>([printInfo PMPrintSession]);
   }
   PMPrintSettings settings() const {
      return static_cast<PMPrintSettings>([printInfo PMPrintSettings]);
   }
};

#endif // QT_NO_PRINTER

#endif // QPRINTENGINE_WIN_P_H
