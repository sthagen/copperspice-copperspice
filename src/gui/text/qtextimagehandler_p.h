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

#ifndef QTEXTIMAGEHANDLER_P_H
#define QTEXTIMAGEHANDLER_P_H

#include <qabstracttextdocumentlayout.h>
#include <qobject.h>

class QTextImageFormat;

class QTextImageHandler : public QObject, public QTextObjectInterface
{
   GUI_CS_OBJECT(QTextImageHandler)
   CS_INTERFACES(QTextObjectInterface)

 public:
   explicit QTextImageHandler(QObject *parent = nullptr);

   QSizeF intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat &format) override;
   void drawObject(QPainter *p, const QRectF &rect, QTextDocument *doc, int posInDocument,
      const QTextFormat &format) override;

   QImage image(QTextDocument *doc, const QTextImageFormat &imageFormat);
};

#endif
