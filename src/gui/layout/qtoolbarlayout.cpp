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

#include <qaction.h>
#include <qapplication.h>
#include <qdebug.h>
#include <qmath.h>
#include <qmenu.h>
#include <qstyleoption.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qwidgetaction.h>

#ifdef Q_OS_DARWIN
#include <qplatform_nativeinterface.h>
#endif

#include <qmainwindowlayout_p.h>
#include <qtoolbarextension_p.h>
#include <qtoolbarlayout_p.h>
#include <qtoolbarseparator_p.h>

#ifndef QT_NO_TOOLBAR

// qmainwindow.cpp
extern QMainWindowLayout *qt_mainwindow_layout(const QMainWindow *window);

QToolBarItem::QToolBarItem(QWidget *widget)
   : QWidgetItem(widget), action(nullptr), customWidget(false)
{
}

bool QToolBarItem::isEmpty() const
{
   return action == nullptr || !action->isVisible();
}

QToolBarLayout::QToolBarLayout(QWidget *parent)
   : QLayout(parent), expanded(false), animating(false), dirty(true),
     expanding(false), empty(true), expandFlag(false), popupMenu(nullptr)
{
   QToolBar *tb = qobject_cast<QToolBar *>(parent);
   if (! tb) {
      return;
   }

   extension = new QToolBarExtension(tb);
   extension->setFocusPolicy(Qt::NoFocus);
   extension->hide();
   QObject::connect(tb, &QToolBar::orientationChanged, extension, &QToolBarExtension::setOrientation);

   setUsePopupMenu(qobject_cast<QMainWindow *>(tb->parentWidget()) == nullptr);
}

QToolBarLayout::~QToolBarLayout()
{
   while (!items.isEmpty()) {
      QToolBarItem *item = items.takeFirst();
      if (QWidgetAction *widgetAction = qobject_cast<QWidgetAction *>(item->action)) {
         if (item->customWidget) {
            widgetAction->releaseWidget(item->widget());
         }
      }

      delete item;
   }
}

void QToolBarLayout::updateMarginAndSpacing()
{
   QToolBar *tb = qobject_cast<QToolBar *>(parentWidget());

   if (!tb) {
      return;
   }

   QStyle *style = tb->style();
   QStyleOptionToolBar opt;
   tb->initStyleOption(&opt);

   setMargin(style->pixelMetric(QStyle::PM_ToolBarItemMargin, &opt, tb)
      + style->pixelMetric(QStyle::PM_ToolBarFrameWidth, &opt, tb));

   setSpacing(style->pixelMetric(QStyle::PM_ToolBarItemSpacing, &opt, tb));
}

bool QToolBarLayout::hasExpandFlag() const
{
   return expandFlag;
}

void QToolBarLayout::setUsePopupMenu(bool set)
{
   if (! dirty && ((popupMenu == nullptr) == set)) {
      invalidate();
   }

   if (! set) {
      QObject::connect(extension, &QToolBarExtension::clicked, this,
                  &QToolBarLayout::setExpanded, Qt::UniqueConnection);

      extension->setPopupMode(QToolButton::DelayedPopup);
      extension->setMenu(nullptr);
      delete popupMenu;
      popupMenu = nullptr;

   } else {
      QObject::disconnect(extension, &QToolBarExtension::clicked, this, &QToolBarLayout::setExpanded);
      extension->setPopupMode(QToolButton::InstantPopup);

      if (! popupMenu) {
         popupMenu = new QMenu(extension);
      }

      extension->setMenu(popupMenu);
   }
}

void QToolBarLayout::checkUsePopupMenu()
{
   QToolBar *tb = static_cast<QToolBar *>(parent());
   QMainWindow *mw = qobject_cast<QMainWindow *>(tb->parent());
   Qt::Orientation o = tb->orientation();
   setUsePopupMenu(!mw || tb->isFloating() || perp(o, expandedSize(mw->size())) >= perp(o, mw->size()));
}

void QToolBarLayout::addItem(QLayoutItem *)
{
   qWarning("QToolBarLayout::addItem() Use QToolBarLayout::addAction() instead");
   return;
}

QLayoutItem *QToolBarLayout::itemAt(int index) const
{
   if (index < 0 || index >= items.count()) {
      return nullptr;
   }

   return items.at(index);
}

QLayoutItem *QToolBarLayout::takeAt(int index)
{
   if (index < 0 || index >= items.count()) {
      return nullptr;
   }

   QToolBarItem *item = items.takeAt(index);

   if (popupMenu) {
      popupMenu->removeAction(item->action);
   }

   QWidgetAction *widgetAction = qobject_cast<QWidgetAction *>(item->action);

   if (widgetAction != nullptr && item->customWidget) {
      widgetAction->releaseWidget(item->widget());
   } else {
      // destroy the QToolButton/QToolBarSeparator
      item->widget()->hide();
      item->widget()->deleteLater();
   }

   invalidate();
   return item;
}

void QToolBarLayout::insertAction(int index, QAction *action)
{
   index = qMax(0, index);
   index = qMin(items.count(), index);

   QToolBarItem *item = createItem(action);
   if (item) {
      items.insert(index, item);
      invalidate();
   }
}

int QToolBarLayout::indexOf(QAction *action) const
{
   for (int i = 0; i < items.count(); ++i) {
      if (items.at(i)->action == action) {
         return i;
      }
   }

   return -1;
}

int QToolBarLayout::count() const
{
   return items.count();
}

bool QToolBarLayout::isEmpty() const
{
   if (dirty) {
      updateGeomArray();
   }
   return empty;
}

void QToolBarLayout::invalidate()
{
   dirty = true;
   QLayout::invalidate();
}

Qt::Orientations QToolBarLayout::expandingDirections() const
{
   if (dirty) {
      updateGeomArray();
   }

   QToolBar *tb = qobject_cast<QToolBar *>(parentWidget());

   if (!tb) {
      return Qt::EmptyFlag;
   }

   Qt::Orientation o = tb->orientation();
   return expanding ? Qt::Orientations(o) : Qt::EmptyFlag;
}

bool QToolBarLayout::movable() const
{
   QToolBar *tb = qobject_cast<QToolBar *>(parentWidget());
   if (!tb) {
      return false;
   }

   QMainWindow *win = qobject_cast<QMainWindow *>(tb->parentWidget());

   return tb->isMovable() && win != nullptr;
}

void QToolBarLayout::updateGeomArray() const
{
   if (! dirty) {
      return;
   }

   QToolBarLayout *that = const_cast<QToolBarLayout *>(this);

   QToolBar *tb = qobject_cast<QToolBar *>(parentWidget());

   if (! tb) {
      return;
   }

   QStyle *style = tb->style();
   QStyleOptionToolBar opt;
   tb->initStyleOption(&opt);

   const int handleExtent = movable() ? style->pixelMetric(QStyle::PM_ToolBarHandleExtent, &opt, tb) : 0;

   const int margin  = this->margin();
   const int spacing = this->spacing();

   const int extensionExtent = style->pixelMetric(QStyle::PM_ToolBarExtensionExtent, &opt, tb);
   Qt::Orientation o = tb->orientation();

   that->minSize = QSize(0, 0);
   that->m_toolBarLayoutHint = QSize(0, 0);

   rperp(o, that->minSize) = style->pixelMetric(QStyle::PM_ToolBarHandleExtent, &opt, tb);
   rperp(o, that->m_toolBarLayoutHint) = style->pixelMetric(QStyle::PM_ToolBarHandleExtent, &opt, tb);

   that->expanding = false;
   that->empty = false;

   QVector<QLayoutStruct> a(items.count() + 1); // + 1 for the stretch

   int count = 0;

   for (int i = 0; i < items.count(); ++i) {
      QToolBarItem *item = items.at(i);

      QSize max  = item->maximumSize();
      QSize min  = item->minimumSize();
      QSize hint = item->sizeHint();

      Qt::Orientations exp = item->expandingDirections();
      bool itemEmpty = item->isEmpty();

      that->expanding = expanding || exp & o;

      if (item->widget()) {
         if (cs_enum_cast(item->widget()->sizePolicy().horizontalPolicy()) & cs_enum_cast(QSizePolicy::ExpandFlag)) {
            that->expandFlag = true;
         }
      }

      if (! itemEmpty) {
         if (count == 0) {
            // minimum size only displays one widget
            rpick(o, that->minSize) += pick(o, min);
         }

         int s = perp(o, minSize);
         rperp(o, that->minSize) = qMax(s, perp(o, min));

         // only add spacing before item, not before the first one
         rpick(o, that->m_toolBarLayoutHint) += (count == 0 ? 0 : spacing) + pick(o, hint);

         s = perp(o, that->m_toolBarLayoutHint);
         rperp(o, that->m_toolBarLayoutHint) = qMax(s, perp(o, hint));

         ++count;
      }

      a[i].sizeHint    = pick(o, hint);
      a[i].maximumSize = pick(o, max);
      a[i].minimumSize = pick(o, min);
      a[i].expansive   = exp & o;

      if (o == Qt::Horizontal) {
         a[i].stretch = item->widget()->sizePolicy().horizontalStretch();
      } else {
         a[i].stretch = item->widget()->sizePolicy().verticalStretch();
      }

      a[i].empty = itemEmpty;
   }

   that->geomArray = a;
   that->empty = count == 0;

   rpick(o, that->minSize) += handleExtent;
   that->minSize += QSize(2 * margin, 2 * margin);

   if (items.count() > 1) {
      rpick(o, that->minSize) += spacing + extensionExtent;
   }

   rpick(o, that->m_toolBarLayoutHint) += handleExtent;
   that->m_toolBarLayoutHint += QSize(2 * margin, 2 * margin);
   that->dirty = false;
}

static bool defaultWidgetAction(QToolBarItem *item)
{
   QWidgetAction *a = qobject_cast<QWidgetAction *>(item->action);
   return a != nullptr && a->defaultWidget() == item->widget();
}

void QToolBarLayout::updateMacBorderMetrics()
{
#ifdef Q_OS_DARWIN
   QToolBar *tb = qobject_cast<QToolBar *>(parentWidget());
   if (!tb) {
      return;
   }

   QRect rect = geometry();

   QMainWindow *mainWindow = qobject_cast<QMainWindow *>(tb->parentWidget());
   if (!mainWindow || !mainWindow->isWindow() || !mainWindow->unifiedTitleAndToolBarOnMac()) {
      return;
   }

   QPlatformNativeInterface *nativeInterface = QApplication::platformNativeInterface();
   QPlatformNativeInterface::FP_Integration function =
      nativeInterface->nativeResourceFunctionForIntegration("registerContentBorderArea");

   if (! function) {
      return;   // Not Cocoa platform plugin.
   }

   QPoint upper = tb->mapToParent(rect.topLeft());
   QPoint lower = tb->mapToParent(rect.bottomLeft() + QPoint(0, 1));

   typedef void (*RegisterContentBorderAreaFunction)(QWindow * window, void *identifier, int upper, int lower);
   if (mainWindow->toolBarArea(tb) == Qt::TopToolBarArea) {
      (reinterpret_cast<RegisterContentBorderAreaFunction>(function))(tb->window()->windowHandle(), tb, upper.y(), lower.y());
   } else {
      (reinterpret_cast<RegisterContentBorderAreaFunction>(function))(tb->window()->windowHandle(), tb, 0, 0);
   }
#endif
}

void QToolBarLayout::setGeometry(const QRect &rect)
{
   QToolBar *tb = qobject_cast<QToolBar *>(parentWidget());
   if (!tb) {
      return;
   }
   QStyle *style = tb->style();
   QStyleOptionToolBar opt;
   tb->initStyleOption(&opt);
   const int margin = this->margin();
   const int extensionExtent = style->pixelMetric(QStyle::PM_ToolBarExtensionExtent, &opt, tb);
   Qt::Orientation o = tb->orientation();

   QLayout::setGeometry(rect);

   updateMacBorderMetrics();
   bool ranOutOfSpace = false;
   if (!animating) {
      ranOutOfSpace = layoutActions(rect.size());
   }

   if (expanded || animating || ranOutOfSpace) {
      Qt::ToolBarArea area = Qt::TopToolBarArea;
      if (QMainWindow *win = qobject_cast<QMainWindow *>(tb->parentWidget())) {
         area = win->toolBarArea(tb);
      }
      QSize hint = sizeHint();

      QPoint pos;
      rpick(o, pos) = pick(o, rect.bottomRight()) - margin - extensionExtent + 2;
      if (area == Qt::LeftToolBarArea || area == Qt::TopToolBarArea) {
         rperp(o, pos) = perp(o, rect.topLeft()) + margin;
      } else {
         rperp(o, pos) = perp(o, rect.bottomRight()) - margin - (perp(o, hint) - 2 * margin) + 1;
      }
      QSize size;
      rpick(o, size) = extensionExtent;
      rperp(o, size) = perp(o, hint) - 2 * margin;
      QRect r(pos, size);

      if (o == Qt::Horizontal) {
         r = QStyle::visualRect(parentWidget()->layoutDirection(), rect, r);
      }

      extension->setGeometry(r);

      if (extension->isHidden()) {
         extension->show();
      }

   } else {
      if (!extension->isHidden()) {
         extension->hide();
      }
   }
}

bool QToolBarLayout::layoutActions(const QSize &size)
{
   if (dirty) {
      updateGeomArray();
   }

   QRect rect(0, 0, size.width(), size.height());

   QList<QWidget *> showWidgets, hideWidgets;

   QToolBar *tb = qobject_cast<QToolBar *>(parentWidget());
   if (! tb) {
      return false;
   }

   QStyle *style = tb->style();
   QStyleOptionToolBar opt;

   tb->initStyleOption(&opt);
   const int handleExtent = movable()
      ? style->pixelMetric(QStyle::PM_ToolBarHandleExtent, &opt, tb) : 0;

   const int margin = this->margin();
   const int spacing = this->spacing();
   const int extensionExtent = style->pixelMetric(QStyle::PM_ToolBarExtensionExtent, &opt, tb);

   Qt::Orientation o = tb->orientation();
   bool extensionMenuContainsOnlyWidgetActions = true;

   int space = pick(o, rect.size()) - 2 * margin - handleExtent;

   if (space <= 0) {
      return false;   // nothing to do.
   }

   if (popupMenu) {
      popupMenu->clear();
   }

   bool ranOutOfSpace = false;

   int rows   = 0;
   int rowPos = perp(o, rect.topLeft()) + margin;

   int i = 0;

   while (i < items.count()) {
      QVector<QLayoutStruct> a = geomArray;

      int start = i;
      int prev  = -1;

      int totalSize = 0;

      int rowHeight = 0;
      int count     = 0;

      int maximumSize   = 0;
      bool expansiveRow = false;

      for (; i < items.count(); ++i) {
         if (a[i].empty) {
            continue;
         }

         int newSize = totalSize + (count == 0 ? 0 : spacing) + a[i].minimumSize;

         if (prev != -1 && newSize > space) {
            if (rows == 0) {
               ranOutOfSpace = true;
            }

            // do we have to move the previous item to the next line to make space for
            // the extension button?

            if (count > 1 && totalSize + spacing + extensionExtent > space) {
               i = prev;
            }

            break;
         }

         if (expanded) {
            rowHeight = qMax(rowHeight, perp(o, items.at(i)->sizeHint()));
         }

         expansiveRow = expansiveRow || a[i].expansive;
         totalSize    = newSize;
         maximumSize += spacing + (a[i].expansive ? a[i].maximumSize : a[i].smartSizeHint());
         prev = i;

         ++count;
      }

      // stretch at the end
      a[i].sizeHint = 0;
      a[i].maximumSize = QWIDGETSIZE_MAX;
      a[i].minimumSize = 0;
      a[i].expansive = true;
      a[i].stretch = 0;
      a[i].empty = true;

      if (expansiveRow && maximumSize < space) {
         expansiveRow = false;
         a[i].maximumSize = space - maximumSize;
      }

      qGeomCalc(a, start, i - start + (expansiveRow ? 0 : 1), 0,
            space - (ranOutOfSpace ? (extensionExtent + spacing) : 0), spacing);

      for (int j = start; j < i; ++j) {
         QToolBarItem *item = items.at(j);

         if (a[j].empty) {
            if (!item->widget()->isHidden()) {
               hideWidgets << item->widget();
            }
            continue;
         }

         QPoint pos;
         rpick(o, pos) = margin + handleExtent + a[j].pos;
         rperp(o, pos) = rowPos;

         QSize toolBarItemSize;
         rpick(o, toolBarItemSize) = a[j].size;

         if (expanded) {
            rperp(o, toolBarItemSize) = rowHeight;
         } else {
            rperp(o, toolBarItemSize) = perp(o, rect.size()) - 2 * margin;
         }

         QRect r(pos, toolBarItemSize);

         if (o == Qt::Horizontal) {
            r = QStyle::visualRect(parentWidget()->layoutDirection(), rect, r);
         }

         item->setGeometry(r);

         if (item->widget()->isHidden()) {
            showWidgets << item->widget();
         }
      }

      if (!expanded) {
         for (int j = i; j < items.count(); ++j) {
            QToolBarItem *item = items.at(j);
            if (!item->widget()->isHidden()) {
               hideWidgets << item->widget();
            }
            if (popupMenu) {
               if (!defaultWidgetAction(item)) {
                  popupMenu->addAction(item->action);
                  extensionMenuContainsOnlyWidgetActions = false;
               }
            }
         }
         break;
      }

      rowPos += rowHeight + spacing;
      ++rows;
   }

   // if we are using a popup menu, not the expadning toolbar effect, we cannot move custom
   // widgets into the menu. If only custom widget actions are chopped off, the popup menu
   // is empty. So we show the little extension button to show something is chopped off,
   // but we make it disabled.

   extension->setEnabled(popupMenu == nullptr || !extensionMenuContainsOnlyWidgetActions);

   // do the show/hide here because it triggers more calls to setGeometry
   for (int j = 0; j < showWidgets.count(); ++j) {
      showWidgets.at(j)->show();
   }

   for (int j = 0; j < hideWidgets.count(); ++j) {
      hideWidgets.at(j)->hide();
   }

   return ranOutOfSpace;
}

QSize QToolBarLayout::expandedSize(const QSize &size) const
{
   if (dirty) {
      updateGeomArray();
   }

   QToolBar *tb = qobject_cast<QToolBar *>(parentWidget());
   if (!tb) {
      return QSize(0, 0);
   }

   QMainWindow *win = qobject_cast<QMainWindow *>(tb->parentWidget());
   Qt::Orientation o = tb->orientation();

   QStyle *style = tb->style();
   QStyleOptionToolBar opt;
   tb->initStyleOption(&opt);

   const int handleExtent = movable() ? style->pixelMetric(QStyle::PM_ToolBarHandleExtent, &opt, tb) : 0;
   const int margin  = this->margin();
   const int spacing = this->spacing();

   const int extensionExtent = style->pixelMetric(QStyle::PM_ToolBarExtensionExtent, &opt, tb);

   int total_w = 0;
   int count   = 0;

   for (int x = 0; x < items.count(); ++x) {
      if (!geomArray[x].empty) {
         total_w += (count == 0 ? 0 : spacing) + geomArray[x].minimumSize;
         ++count;
      }
   }

   if (count == 0) {
      return QSize(0, 0);
   }

   int min_w = pick(o, size);
   int rows  = (int)qSqrt(qreal(count));

   if (rows == 1) {
      ++rows;   // we want to expand to at least two rows
   }

   int space = total_w / rows + spacing + extensionExtent;
   space = qMax(space, min_w - 2 * margin - handleExtent);

   if (win != nullptr) {
      space = qMin(space, pick(o, win->size()) - 2 * margin - handleExtent);
   }

   int w = 0;
   int h = 0;
   int i = 0;

   while (i < items.count()) {
      int countItems = 0;

      int totalsize = 0;
      int prev      = -1;
      int rowHeight = 0;

      for (; i < items.count(); ++i) {
         if (geomArray[i].empty) {
            continue;
         }

         int newSize = totalsize + (countItems == 0 ? 0 : spacing) + geomArray[i].minimumSize;
         rowHeight = qMax(rowHeight, perp(o, items.at(i)->sizeHint()));

         if (prev != -1 && newSize > space) {
            if (countItems > 1 && totalsize + spacing + extensionExtent > space) {
               totalsize -= spacing + geomArray[prev].minimumSize;
               i = prev;
            }

            break;
         }

         totalsize = newSize;
         prev = i;

         ++countItems;
      }

      w = qMax(totalsize, w);
      h += rowHeight + spacing;
   }

   w += 2 * margin + handleExtent + spacing + extensionExtent;
   w = qMax(w, min_w);

   if (win != nullptr) {
      w = qMin(w, pick(o, win->size()));
   }
   h += 2 * margin - spacing; //there is no spacing before the first row

   QSize result;
   rpick(o, result) = w;
   rperp(o, result) = h;

   return result;
}

void QToolBarLayout::setExpanded(bool exp)
{
   QWidget *tb = qobject_cast<QToolBar *>(parentWidget());
   if (!tb) {
      return;
   }
   if (exp == expanded && !tb->isWindow()) {
      return;
   }

   expanded = exp;
   extension->setChecked(expanded);

   if (QMainWindow *win = qobject_cast<QMainWindow *>(tb->parentWidget())) {
#ifdef QT_NO_DOCKWIDGET
      animating = false;
#else
      animating = !tb->isWindow() && win->isAnimated();
#endif
      QMainWindowLayout *layout = qt_mainwindow_layout(win);

      if (expanded) {
         tb->raise();
      } else {
         QList<int> path = layout->layoutState.indexOf(tb);

         if (!path.isEmpty()) {
            QRect rect = layout->layoutState.itemRect(path);
            layoutActions(rect.size());
         }
      }
      layout->layoutState.toolBarAreaLayout.apply(animating);
   }
}

QSize QToolBarLayout::minimumSize() const
{
   if (dirty) {
      updateGeomArray();
   }

   return minSize;
}

QSize QToolBarLayout::sizeHint() const
{
   if (dirty) {
      updateGeomArray();
   }

   return m_toolBarLayoutHint;
}

QToolBarItem *QToolBarLayout::createItem(QAction *action)
{
   bool customWidget = false;
   bool standardButtonWidget = false;

   QWidget *widget = nullptr;
   QToolBar *tb    = qobject_cast<QToolBar *>(parentWidget());

   if (! tb) {
      return nullptr;
   }

   if (QWidgetAction *widgetAction = qobject_cast<QWidgetAction *>(action)) {
      widget = widgetAction->requestWidget(tb);

      if (widget != nullptr) {
         widget->setAttribute(Qt::WA_LayoutUsesWidgetRect);
         customWidget = true;
      }

   } else if (action->isSeparator()) {
      QToolBarSeparator *sep = new QToolBarSeparator(tb);
      connect(tb, &QToolBar::orientationChanged, sep, &QToolBarSeparator::setOrientation);
      widget = sep;
   }

   if (! widget) {
      QToolButton *button = new QToolButton(tb);
      button->setAutoRaise(true);
      button->setFocusPolicy(Qt::NoFocus);
      button->setIconSize(tb->iconSize());
      button->setToolButtonStyle(tb->toolButtonStyle());

      QObject::connect(tb, &QToolBar::iconSizeChanged,        button, &QToolButton::setIconSize);
      QObject::connect(tb, &QToolBar::toolButtonStyleChanged, button, &QToolButton::setToolButtonStyle);

      button->setDefaultAction(action);
      QObject::connect(button, &QToolButton::triggered, tb, &QToolBar::actionTriggered);

      widget = button;
      standardButtonWidget = true;
   }

   widget->hide();
   QToolBarItem *result = new QToolBarItem(widget);
   if (standardButtonWidget) {
      result->setAlignment(Qt::AlignJustify);
   }
   result->customWidget = customWidget;
   result->action = action;
   return result;
}


#endif // QT_NO_TOOLBAR
