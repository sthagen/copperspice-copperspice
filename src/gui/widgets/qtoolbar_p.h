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

#ifndef QTOOLBAR_P_H
#define QTOOLBAR_P_H

#include <qtoolbar.h>

#include <qaction.h>
#include <qbasictimer.h>
#include <qlayoutitem.h>

#include <qwidget_p.h>

#ifndef QT_NO_TOOLBAR

class QTimer;
class QToolBarLayout;

class QToolBarPrivate : public QWidgetPrivate
{
   Q_DECLARE_PUBLIC(QToolBar)

 public:
   QToolBarPrivate()
      : explicitIconSize(false), explicitToolButtonStyle(false), movable(true), floatable(true),
        allowedAreas(Qt::AllToolBarAreas), orientation(Qt::Horizontal),
        toolButtonStyle(Qt::ToolButtonIconOnly), m_toolbarLayout(nullptr), state(nullptr)
#ifdef Q_OS_DARWIN
      , macWindowDragging(false)
#endif
   {
   }

   void init();
   void actionTriggered();
   void _q_toggleView(bool b);
   void _q_updateIconSize(const QSize &sz);
   void _q_updateToolButtonStyle(Qt::ToolButtonStyle style);

   bool explicitIconSize;
   bool explicitToolButtonStyle;
   bool movable;
   bool floatable;
   Qt::ToolBarAreas allowedAreas;
   Qt::Orientation orientation;
   Qt::ToolButtonStyle toolButtonStyle;
   QSize iconSize;

   QAction *toggleViewAction;

   QToolBarLayout *m_toolbarLayout;

   struct DragState {
      QPoint pressPos;
      bool dragging;
      bool moving;
      QLayoutItem *widgetItem;
   };
   DragState *state;

#ifdef Q_OS_DARWIN
   bool macWindowDragging;
   QPoint macWindowDragPressPosition;
#endif

   bool mousePressEvent(QMouseEvent *e);
   bool mouseReleaseEvent(QMouseEvent *e);
   bool mouseMoveEvent(QMouseEvent *e);

   void updateWindowFlags(bool floating, bool unplug = false);
   void setWindowState(bool floating, bool unplug = false, const QRect &rect = QRect());
   void initDrag(const QPoint &pos);
   void startDrag(bool moving = false);
   void endDrag();

   void unplug(const QRect &r);
   void plug(const QRect &r);

   QBasicTimer waitForPopupTimer;
};

#endif // QT_NO_TOOLBAR

#endif // QDYNAMICTOOLBAR_P_H
