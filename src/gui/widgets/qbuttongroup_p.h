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

#ifndef QBUTTONGROUP_P_H
#define QBUTTONGROUP_P_H

#include <qbuttongroup.h>

#ifndef QT_NO_BUTTONGROUP

#include <qhash.h>
#include <qlist.h>
#include <qpointer.h>

class QButtonGroupPrivate
{
   Q_DECLARE_PUBLIC(QButtonGroup)

 public:
   QButtonGroupPrivate()
      : exclusive(true)
   { }

   virtual ~QButtonGroupPrivate()
   { }

   QList<QAbstractButton *> buttonList;
   QPointer<QAbstractButton> checkedButton;
   void detectCheckedButton();

   bool exclusive;
   QHash<QAbstractButton *, int> mapping;

 protected:
   QButtonGroup *q_ptr;

};

#endif // QT_NO_BUTTONGROUP

#endif // QBUTTONGROUP_P_H
