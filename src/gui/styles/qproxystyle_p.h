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

#ifndef QPROXYSTYLE_P_H
#define QPROXYSTYLE_P_H

#include <qproxystyle.h>

#include <qcommonstyle.h>

#include <qcommonstyle_p.h>

#ifndef QT_NO_STYLE_PROXY

class QProxyStylePrivate : public QCommonStylePrivate
{
   Q_DECLARE_PUBLIC(QProxyStyle)

 public:
   void ensureBaseStyle() const;

 private:
   QProxyStylePrivate()
      :QCommonStylePrivate(), baseStyle(nullptr)
   {
   }

   mutable QPointer <QStyle> baseStyle;
};

#endif // QT_NO_STYLE_PROXY

#endif //QPROXYSTYLE_P_H
