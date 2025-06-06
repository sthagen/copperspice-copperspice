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

#include <qglobal.h>

#ifndef QT_NO_GRAPHICSVIEW

#include <qgraphicsproxywidget.h>
#include <qgraphics_proxywidget_p.h>

#include <qdebug.h>
#include <qevent.h>
#include <qgraphicslayout.h>
#include <qgraphicsscene.h>
#include <qgraphicssceneevent.h>
#include <qgraphicsview.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qpainter.h>
#include <qstyleoption.h>
#include <qtextedit.h>

#include <qapplication_p.h>
#include <qwidget_p.h>

extern bool qt_sendSpontaneousEvent(QObject *, QEvent *);
Q_GUI_EXPORT extern bool qt_tab_all_widgets();

void QGraphicsProxyWidgetPrivate::init()
{
   Q_Q(QGraphicsProxyWidget);
   q->setFocusPolicy(Qt::WheelFocus);
   q->setAcceptDrops(true);
}

void QGraphicsProxyWidgetPrivate::sendWidgetMouseEvent(QGraphicsSceneHoverEvent *event)
{
   QGraphicsSceneMouseEvent mouseEvent(QEvent::GraphicsSceneMouseMove);
   mouseEvent.setPos(event->pos());
   mouseEvent.setScreenPos(event->screenPos());
   mouseEvent.setButton(Qt::NoButton);
   mouseEvent.setButtons(Qt::EmptyFlag);
   mouseEvent.setModifiers(event->modifiers());
   sendWidgetMouseEvent(&mouseEvent);
   event->setAccepted(mouseEvent.isAccepted());
}

void QGraphicsProxyWidgetPrivate::sendWidgetMouseEvent(QGraphicsSceneMouseEvent *event)
{
   if (! event || ! m_proxyWidget || ! m_proxyWidget->isVisible()) {
      return;
   }

   Q_Q(QGraphicsProxyWidget);

   // Find widget position and receiver
   QPointF pos = event->pos();

   QPointer<QWidget> alienWidget = QPointer<QWidget>(m_proxyWidget->childAt(pos.toPoint()));
   QPointer<QWidget> receiver;

   if (alienWidget == nullptr) {
      receiver = m_proxyWidget;
   } else {
      receiver = alienWidget;
   }

   if (QWidgetPrivate::nearestGraphicsProxyWidget(receiver) != q) {
      // another proxywidget will handle the events
      return;
   }

   // Translate QGraphicsSceneMouse events to QMouseEvents
   QEvent::Type type = QEvent::None;

   switch (event->type()) {
      case QEvent::GraphicsSceneMousePress:
         type = QEvent::MouseButtonPress;

         if (!embeddedMouseGrabber) {
            embeddedMouseGrabber = receiver;
         } else {
            receiver = embeddedMouseGrabber;
         }
         break;

      case QEvent::GraphicsSceneMouseRelease:
         type = QEvent::MouseButtonRelease;

         if (embeddedMouseGrabber) {
            receiver = embeddedMouseGrabber;
         }
         break;

      case QEvent::GraphicsSceneMouseDoubleClick:
         type = QEvent::MouseButtonDblClick;
         if (!embeddedMouseGrabber) {
            embeddedMouseGrabber = receiver;
         } else {
            receiver = embeddedMouseGrabber;
         }
         break;

      case QEvent::GraphicsSceneMouseMove:
         type = QEvent::MouseMove;
         if (embeddedMouseGrabber) {
            receiver = embeddedMouseGrabber;
         }
         break;

      default:
         Q_ASSERT_X(false, "QGraphicsProxyWidget", "internal error");
         break;
   }

   if (! lastWidgetUnderMouse) {
      QApplicationPrivate::dispatchEnterLeave(embeddedMouseGrabber ? embeddedMouseGrabber : receiver, nullptr, event->screenPos());
      lastWidgetUnderMouse = receiver;
   }

   // Map event position from us to the receiver
   pos = mapToReceiver(pos, receiver);

   // Send mouse event.
   QMouseEvent mouseEvent(type, pos, receiver->mapTo(receiver->topLevelWidget(), pos.toPoint()),
      receiver->mapToGlobal(pos.toPoint()),
      event->button(), event->buttons(), event->modifiers(), event->source());

   QWidget *embeddedMouseGrabberPtr = (QWidget *)embeddedMouseGrabber;

   QApplicationPrivate::sendMouseEvent(receiver, &mouseEvent, alienWidget, m_proxyWidget,
      &embeddedMouseGrabberPtr, lastWidgetUnderMouse, event->spontaneous());

   embeddedMouseGrabber = embeddedMouseGrabberPtr;

   // Handle enter/leave events when last button is released from mouse
   // grabber child widget.
   if (embeddedMouseGrabber && type == QEvent::MouseButtonRelease && ! event->buttons()) {

      if (q->rect().contains(event->pos()) && q->acceptHoverEvents()) {
         lastWidgetUnderMouse = alienWidget ? alienWidget : m_proxyWidget;
      } else {
         // released on the frame our outside the item, or doesn't accept hover events.
         lastWidgetUnderMouse = nullptr;
      }

      QApplicationPrivate::dispatchEnterLeave(lastWidgetUnderMouse, embeddedMouseGrabber, event->screenPos());
      embeddedMouseGrabber = nullptr;

#ifndef QT_NO_CURSOR
      // ### Restore the cursor, do not override
      if (! lastWidgetUnderMouse) {
         q->unsetCursor();
      }
#endif
   }

   event->setAccepted(mouseEvent.isAccepted());
}

void QGraphicsProxyWidgetPrivate::sendWidgetKeyEvent(QKeyEvent *event)
{
   Q_Q(QGraphicsProxyWidget);

   if (! event || ! m_proxyWidget || ! m_proxyWidget->isVisible()) {
      return;
   }

   QPointer<QWidget> receiver = QPointer<QWidget>(m_proxyWidget->focusWidget());
   if (receiver == nullptr) {
      receiver = m_proxyWidget;
   }
   Q_ASSERT(receiver);

   do {
      bool res = QApplication::sendEvent(receiver, event);
      if ((res && event->isAccepted()) || (q->isWindow() && receiver == m_proxyWidget)) {
         break;
      }
      receiver = receiver->parentWidget();

   } while (receiver);
}

void QGraphicsProxyWidgetPrivate::removeSubFocusHelper(QWidget *widget, Qt::FocusReason reason)
{
   QFocusEvent event(QEvent::FocusOut, reason);
   QPointer<QWidget> widgetGuard = QPointer<QWidget>(widget);
   QApplication::sendEvent(widget, &event);

   if (widgetGuard && event.isAccepted()) {
      QApplication::sendEvent(widget->style(), &event);
   }
}

QWidget *QGraphicsProxyWidgetPrivate::findFocusChild(QWidget *child, bool next) const
{
   if (! m_proxyWidget) {
      return nullptr;
   }

   // Run around the focus chain until we find a widget that can take tab focus.
   if (! child) {
      child = next ? m_proxyWidget.data() : m_proxyWidget->d_func()->focus_prev;

   } else {
      child = next ? child->d_func()->focus_next : child->d_func()->focus_prev;

      if ((next && child == m_proxyWidget) || (! next && child == m_proxyWidget->d_func()->focus_prev)) {
         return nullptr;
      }
   }

   if (! child) {
      return nullptr;
   }

   QWidget *oldChild = child;
   uint focus_flag = qt_tab_all_widgets() ? Qt::TabFocus : Qt::StrongFocus;

   do {
      if (child->isEnabled() && child->isVisibleTo(m_proxyWidget) && ((child->focusPolicy() & focus_flag) == focus_flag)
            && ! (child->d_func()->extra && child->d_func()->extra->focus_proxy)) {
         return child;
      }

      child = next ? child->d_func()->focus_next : child->d_func()->focus_prev;

   } while (child != oldChild && ! (next && child == m_proxyWidget) && ! (! next && child == m_proxyWidget->d_func()->focus_prev));

   return nullptr;
}

void QGraphicsProxyWidgetPrivate::_q_removeWidgetSlot()
{
   Q_Q(QGraphicsProxyWidget);

   if (! m_proxyWidget.isNull()) {
      if (QWExtra *extra = m_proxyWidget->d_func()->extra) {
         extra->proxyWidget = nullptr;
      }
   }

   m_proxyWidget = nullptr;
   delete q;
}

void QGraphicsProxyWidgetPrivate::updateWidgetGeometryFromProxy()
{
}

void QGraphicsProxyWidgetPrivate::updateProxyGeometryFromWidget()
{
   Q_Q(QGraphicsProxyWidget);

   if (! m_proxyWidget) {
      return;
   }

   QRectF widgetGeometry = m_proxyWidget->geometry();
   QWidget *parentWidget = m_proxyWidget->parentWidget();

   if (m_proxyWidget->isWindow()) {
      QGraphicsProxyWidget *proxyParent = nullptr;

      if (parentWidget && (proxyParent = qobject_cast<QGraphicsProxyWidget *>(q->parentWidget()))) {
         // Nested window proxy (e.g., combobox popup), map widget to the
         // parent widget's global coordinates, and map that to the parent
         // proxy's child coordinates.
         widgetGeometry.moveTo(proxyParent->subWidgetRect(parentWidget).topLeft()
            + parentWidget->mapFromGlobal(m_proxyWidget->pos()));
      }
   }

   // Adjust to size hint if the widget has never been resized.
   if (! m_proxyWidget->size().isValid()) {
      widgetGeometry.setSize(m_proxyWidget->sizeHint());
   }

   // Assign new geometry.
   posChangeMode = QGraphicsProxyWidgetPrivate::WidgetToProxyMode;
   sizeChangeMode = QGraphicsProxyWidgetPrivate::WidgetToProxyMode;
   q->setGeometry(widgetGeometry);
   posChangeMode = QGraphicsProxyWidgetPrivate::NoMode;
   sizeChangeMode = QGraphicsProxyWidgetPrivate::NoMode;
}

void QGraphicsProxyWidgetPrivate::updateProxyInputMethodAcceptanceFromWidget()
{
   Q_Q(QGraphicsProxyWidget);

   if (! m_proxyWidget) {
      return;
   }

   QWidget *focusWidget = m_proxyWidget->focusWidget();
   if (! focusWidget) {
      focusWidget = m_proxyWidget;
   }

   q->setFlag(QGraphicsItem::ItemAcceptsInputMethod, focusWidget->testAttribute(Qt::WA_InputMethodEnabled));
}

void QGraphicsProxyWidgetPrivate::embedSubWindow(QWidget *subWin)
{
   QWExtra *extra;

   if (! ((extra = subWin->d_func()->extra) && extra->proxyWidget)) {
      QGraphicsProxyWidget *subProxy = new QGraphicsProxyWidget(q_func(), subWin->windowFlags());
      subProxy->d_func()->setWidget_helper(subWin, false);
   }
}

void QGraphicsProxyWidgetPrivate::unembedSubWindow(QWidget *subWin)
{
   for (QGraphicsItem *child : children) {
      if (child->isWidget()) {
         if (QGraphicsProxyWidget *proxy = qobject_cast<QGraphicsProxyWidget *>(static_cast<QGraphicsWidget *>(child))) {
            if (proxy->widget() == subWin) {
               proxy->setWidget(nullptr);
               m_itemScene->removeItem(proxy);
               delete proxy;

               return;
            }
         }
      }
   }
}

bool QGraphicsProxyWidgetPrivate::isProxyWidget() const
{
   return true;
}

QPointF QGraphicsProxyWidgetPrivate::mapToReceiver(const QPointF &pos, const QWidget *receiver) const
{
   QPointF p = pos;

   // Map event position from us to the receiver, preserving its
   // precision (don't use QWidget::mapFrom here).
   while (receiver && receiver != m_proxyWidget) {
      p -= QPointF(receiver->pos());
      receiver = receiver->parentWidget();
   }

   return p;
}

QGraphicsProxyWidget::QGraphicsProxyWidget(QGraphicsItem *parent, Qt::WindowFlags flags)
   : QGraphicsWidget(*new QGraphicsProxyWidgetPrivate, parent, flags)
{
   Q_D(QGraphicsProxyWidget);
   d->init();
}

QGraphicsProxyWidget::~QGraphicsProxyWidget()
{
   Q_D(QGraphicsProxyWidget);

   if (d->m_proxyWidget) {
      d->m_proxyWidget->removeEventFilter(this);
      QObject::disconnect(d->m_proxyWidget.data(), &QObject::destroyed, this, &QGraphicsProxyWidget::_q_removeWidgetSlot);
      delete d->m_proxyWidget;
   }
}

void QGraphicsProxyWidget::setWidget(QWidget *widget)
{
   Q_D(QGraphicsProxyWidget);
   d->setWidget_helper(widget, true);
}

void QGraphicsProxyWidgetPrivate::setWidget_helper(QWidget *newWidget, bool autoShow)
{
   Q_Q(QGraphicsProxyWidget);

   if (newWidget == m_proxyWidget) {
      return;
   }

   if (m_proxyWidget) {
      QObject::disconnect(m_proxyWidget.data(), &QObject::destroyed, q, &QGraphicsProxyWidget::_q_removeWidgetSlot);

      m_proxyWidget->removeEventFilter(q);
      m_proxyWidget->setAttribute(Qt::WA_DontShowOnScreen, false);
      m_proxyWidget->d_func()->extra->proxyWidget = nullptr;

      resolveFont(inheritedFontResolveMask);
      resolvePalette(inheritedPaletteResolveMask);
      m_proxyWidget->update();

      for (QGraphicsItem *child : q->childItems()) {
         if (child->d_ptr->isProxyWidget()) {
            QGraphicsProxyWidget *childProxy = static_cast<QGraphicsProxyWidget *>(child);
            QWidget *parent = childProxy->widget();

            while (parent->parentWidget() != nullptr) {
               if (parent == m_proxyWidget) {
                  break;
               }
               parent = parent->parentWidget();
            }

            if (! childProxy->widget() || parent != m_proxyWidget) {
               continue;
            }

            childProxy->setWidget(nullptr);
            delete childProxy;
         }
      }

      m_proxyWidget = nullptr;

#ifndef QT_NO_CURSOR
      q->unsetCursor();
#endif

      q->setAcceptHoverEvents(false);
      if (! newWidget) {
         q->update();
      }
   }

   if (! newWidget) {
      return;
   }

   if (!newWidget->isWindow()) {
      QWExtra *extra = newWidget->parentWidget()->d_func()->extra;
      if (!extra || !extra->proxyWidget)  {
         qWarning("QGraphicsProxyWidget::setWidget() Unable to embed widget, %p must be a toplevel widget "
            "or a child of an embedded widget", static_cast<void *>(newWidget));

         return;
      }
   }

   // Register this proxy within the widget's private.
   // ### This is a bit backdoorish
   QWExtra *extra = newWidget->d_func()->extra;
   if (! extra) {
      newWidget->d_func()->createExtra();
      extra = newWidget->d_func()->extra;
   }

   QGraphicsProxyWidget **proxyWidget = &extra->proxyWidget;
   if (*proxyWidget) {
      if (*proxyWidget != q) {
         qWarning("QGraphicsProxyWidget::setWidget() Unable to embed widget, %p is already embedded",
               static_cast<void *>(newWidget));
      }
      return;
   }
   *proxyWidget = q;

   newWidget->setAttribute(Qt::WA_DontShowOnScreen);
   newWidget->ensurePolished();

   // Do not wait for this widget to close before the app closes ###
   // should not this widget inherit the attribute?
   newWidget->setAttribute(Qt::WA_QuitOnClose, false);
   q->setAcceptHoverEvents(true);

   if (newWidget->testAttribute(Qt::WA_NoSystemBackground)) {
      q->setAttribute(Qt::WA_NoSystemBackground);
   }

   if (newWidget->testAttribute(Qt::WA_OpaquePaintEvent)) {
      q->setAttribute(Qt::WA_OpaquePaintEvent);
   }

   m_proxyWidget = newWidget;

   // Changes only go from the widget to the proxy.
   enabledChangeMode = QGraphicsProxyWidgetPrivate::WidgetToProxyMode;
   visibleChangeMode = QGraphicsProxyWidgetPrivate::WidgetToProxyMode;
   posChangeMode     = QGraphicsProxyWidgetPrivate::WidgetToProxyMode;
   sizeChangeMode    = QGraphicsProxyWidgetPrivate::WidgetToProxyMode;

   if ((autoShow && ! newWidget->testAttribute(Qt::WA_WState_ExplicitShowHide)) ||
         ! newWidget->testAttribute(Qt::WA_WState_Hidden)) {
      newWidget->show();
   }

   // Copy the state from the widget onto the proxy.
#ifndef QT_NO_CURSOR
   if (newWidget->testAttribute(Qt::WA_SetCursor)) {
      q->setCursor(m_proxyWidget->cursor());
   }
#endif

   q->setEnabled(newWidget->isEnabled());
   q->setVisible(newWidget->isVisible());
   q->setLayoutDirection(newWidget->layoutDirection());

   if (newWidget->testAttribute(Qt::WA_SetStyle)) {
      q->setStyle(m_proxyWidget->style());
   }

   resolveFont(inheritedFontResolveMask);
   resolvePalette(inheritedPaletteResolveMask);

   if (!newWidget->testAttribute(Qt::WA_Resized)) {
      newWidget->adjustSize();
   }

   int left, top, right, bottom;
   newWidget->getContentsMargins(&left, &top, &right, &bottom);
   q->setContentsMargins(left, top, right, bottom);
   q->setWindowTitle(newWidget->windowTitle());

   // size policies and constraints..
   q->setSizePolicy(newWidget->sizePolicy());

   QSize sz = newWidget->minimumSize();
   q->setMinimumSize(sz.isNull() ? QSizeF() : QSizeF(sz));
   sz = newWidget->maximumSize();
   q->setMaximumSize(sz.isNull() ? QSizeF() : QSizeF(sz));

   updateProxyGeometryFromWidget();
   updateProxyInputMethodAcceptanceFromWidget();

   // Hook up the event filter to keep the state up to date.
   newWidget->installEventFilter(q);
   QObject::connect(newWidget, &QObject::destroyed, q, &QGraphicsProxyWidget::_q_removeWidgetSlot);

   // Changes no longer go only from the widget to the proxy.
   enabledChangeMode = QGraphicsProxyWidgetPrivate::NoMode;
   visibleChangeMode = QGraphicsProxyWidgetPrivate::NoMode;
   posChangeMode     = QGraphicsProxyWidgetPrivate::NoMode;
   sizeChangeMode    = QGraphicsProxyWidgetPrivate::NoMode;
}

QWidget *QGraphicsProxyWidget::widget() const
{
   Q_D(const QGraphicsProxyWidget);
   return d->m_proxyWidget;
}

QRectF QGraphicsProxyWidget::subWidgetRect(const QWidget *widget) const
{
   Q_D(const QGraphicsProxyWidget);

   if (! widget || ! d->m_proxyWidget) {
      return QRectF();
   }

   if (d->m_proxyWidget == widget || d->m_proxyWidget->isAncestorOf(widget)) {
      return QRectF(widget->mapTo(d->m_proxyWidget, QPoint(0, 0)), widget->size());
   }

   return QRectF();
}

void QGraphicsProxyWidget::setGeometry(const QRectF &rect)
{
   Q_D(QGraphicsProxyWidget);

   bool proxyResizesWidget = !d->posChangeMode && !d->sizeChangeMode;

   if (proxyResizesWidget) {
      d->posChangeMode = QGraphicsProxyWidgetPrivate::ProxyToWidgetMode;
      d->sizeChangeMode = QGraphicsProxyWidgetPrivate::ProxyToWidgetMode;
   }

   QGraphicsWidget::setGeometry(rect);

   if (proxyResizesWidget) {
      d->posChangeMode = QGraphicsProxyWidgetPrivate::NoMode;
      d->sizeChangeMode = QGraphicsProxyWidgetPrivate::NoMode;
   }
}

QVariant QGraphicsProxyWidget::itemChange(GraphicsItemChange change, const QVariant &value)
{
   Q_D(QGraphicsProxyWidget);

   switch (change) {
      case ItemPositionChange:
         // The item's position is either changed directly on the proxy, in
         // which case the position change should propagate to the widget,
         // otherwise it happens as a side effect when filtering QEvent::Move.
         if (!d->posChangeMode) {
            d->posChangeMode = QGraphicsProxyWidgetPrivate::ProxyToWidgetMode;
         }
         break;

      case ItemPositionHasChanged:
         // Move the CS widget if we are in widget-to-proxy mode, otherwise the widget has already moved.
         if (d->m_proxyWidget && d->posChangeMode != QGraphicsProxyWidgetPrivate::WidgetToProxyMode) {
            d->m_proxyWidget->move(value.toPoint());
         }

         if (d->posChangeMode == QGraphicsProxyWidgetPrivate::ProxyToWidgetMode) {
            d->posChangeMode = QGraphicsProxyWidgetPrivate::NoMode;
         }
         break;

      case ItemVisibleChange:
         if (!d->visibleChangeMode) {
            d->visibleChangeMode = QGraphicsProxyWidgetPrivate::ProxyToWidgetMode;
         }
         break;

      case ItemVisibleHasChanged:
         if (d->m_proxyWidget && d->visibleChangeMode != QGraphicsProxyWidgetPrivate::WidgetToProxyMode) {
            d->m_proxyWidget->setVisible(isVisible());
         }

         if (d->visibleChangeMode == QGraphicsProxyWidgetPrivate::ProxyToWidgetMode) {
            d->visibleChangeMode = QGraphicsProxyWidgetPrivate::NoMode;
         }
         break;

      case ItemEnabledChange:
         if (! d->enabledChangeMode) {
            d->enabledChangeMode = QGraphicsProxyWidgetPrivate::ProxyToWidgetMode;
         }
         break;

      case ItemEnabledHasChanged:
         if (d->m_proxyWidget && d->enabledChangeMode != QGraphicsProxyWidgetPrivate::WidgetToProxyMode) {
            d->m_proxyWidget->setEnabled(isEnabled());
         }

         if (d->enabledChangeMode == QGraphicsProxyWidgetPrivate::ProxyToWidgetMode) {
            d->enabledChangeMode = QGraphicsProxyWidgetPrivate::NoMode;
         }
         break;

      default:
         break;
   }
   return QGraphicsWidget::itemChange(change, value);
}

bool QGraphicsProxyWidget::event(QEvent *event)
{
   Q_D(QGraphicsProxyWidget);

   if (! d->m_proxyWidget) {
      return QGraphicsWidget::event(event);
   }

   switch (event->type()) {
      case QEvent::StyleChange:
         // Propagate style changes to the embedded widget.
         if (!d->styleChangeMode) {
            d->styleChangeMode = QGraphicsProxyWidgetPrivate::ProxyToWidgetMode;
            d->m_proxyWidget->setStyle(style());
            d->styleChangeMode = QGraphicsProxyWidgetPrivate::NoMode;
         }
         break;

      case QEvent::FontChange: {
         // Propagate to widget.
         QWidgetPrivate *wd = d->m_proxyWidget->d_func();
         int mask = d->m_graphicsFont.resolve() | d->inheritedFontResolveMask;
         wd->inheritedFontResolveMask = mask;
         wd->resolveFont();
         break;
      }

      case QEvent::PaletteChange: {
         // Propagate to widget.
         QWidgetPrivate *wd = d->m_proxyWidget->d_func();
         int mask = d->m_graphicsPalette.resolve() | d->inheritedPaletteResolveMask;
         wd->inheritedPaletteResolveMask = mask;
         wd->resolvePalette();
         break;
      }

      case QEvent::InputMethod: {
         inputMethodEvent(static_cast<QInputMethodEvent *>(event));

         if (event->isAccepted()) {
            return true;
         }

         return false;
      }

      case QEvent::ShortcutOverride: {
         QWidget *focusWidget = d->m_proxyWidget->focusWidget();

         while (focusWidget) {
            QApplication::sendEvent(focusWidget, event);

            if (event->isAccepted()) {
               return true;
            }

            focusWidget = focusWidget->parentWidget();
         }

         return false;
      }

      case QEvent::KeyPress: {
         QKeyEvent *k = static_cast<QKeyEvent *>(event);

         if (k->key() == Qt::Key_Tab || k->key() == Qt::Key_Backtab) {
            if (!(k->modifiers() & (Qt::ControlModifier | Qt::AltModifier))) {
               // Add MetaModifier?
               QWidget *focusWidget = d->m_proxyWidget->focusWidget();

               while (focusWidget) {
                  bool res = QApplication::sendEvent(focusWidget, event);

                  if ((res && event->isAccepted()) || (isWindow() && focusWidget == d->m_proxyWidget)) {
                     event->accept();
                     break;
                  }

                  focusWidget = focusWidget->parentWidget();
               }

               return true;
            }
         }
         break;
      }

#ifndef QT_NO_TOOLTIP
      case QEvent::GraphicsSceneHelp: {
         // Propagate the help event (for tooltip) to the widget under mouse
         if (d->lastWidgetUnderMouse) {
            QGraphicsSceneHelpEvent *he = static_cast<QGraphicsSceneHelpEvent *>(event);
            QPoint pos = d->mapToReceiver(mapFromScene(he->scenePos()), d->lastWidgetUnderMouse).toPoint();
            QHelpEvent e(QEvent::ToolTip, pos, he->screenPos());
            QApplication::sendEvent(d->lastWidgetUnderMouse, &e);
            event->setAccepted(e.isAccepted());
            return e.isAccepted();
         }
         break;
      }

      case QEvent::ToolTipChange: {
         // Propagate tooltip change to the widget
         if (! d->tooltipChangeMode) {
            d->tooltipChangeMode = QGraphicsProxyWidgetPrivate::ProxyToWidgetMode;
            d->m_proxyWidget->setToolTip(toolTip());
            d->tooltipChangeMode = QGraphicsProxyWidgetPrivate::NoMode;
         }
         break;
      }
#endif

      case QEvent::TouchBegin:
      case QEvent::TouchUpdate:
      case QEvent::TouchEnd: {
         if (event->spontaneous()) {
            qt_sendSpontaneousEvent(d->m_proxyWidget, event);
         } else {
            QApplication::sendEvent(d->m_proxyWidget, event);
         }

         if (event->isAccepted()) {
            return true;
         }

         break;
      }

      default:
         break;
   }

   return QGraphicsWidget::event(event);
}

bool QGraphicsProxyWidget::eventFilter(QObject *object, QEvent *event)
{
   Q_D(QGraphicsProxyWidget);

   if (object == d->m_proxyWidget) {
      switch (event->type()) {
         case QEvent::LayoutRequest:
            updateGeometry();
            break;

         case QEvent::Resize:
            // If the widget resizes itself, we resize the proxy too.
            // Prevent feed-back by checking the geometry change mode.
            if (!d->sizeChangeMode) {
               d->updateProxyGeometryFromWidget();
            }
            break;

         case QEvent::Move:
            // If the widget moves itself, we move the proxy too.  Prevent
            // feed-back by checking the geometry change mode.
            if (!d->posChangeMode) {
               d->updateProxyGeometryFromWidget();
            }
            break;

         case QEvent::Hide:
         case QEvent::Show:
            // If the widget toggles its visible state, the proxy will follow.
            if (!d->visibleChangeMode) {
               d->visibleChangeMode = QGraphicsProxyWidgetPrivate::WidgetToProxyMode;
               setVisible(event->type() == QEvent::Show);
               d->visibleChangeMode = QGraphicsProxyWidgetPrivate::NoMode;
            }
            break;

         case QEvent::EnabledChange:
            // If the widget toggles its enabled state, the proxy will follow.
            if (!d->enabledChangeMode) {
               d->enabledChangeMode = QGraphicsProxyWidgetPrivate::WidgetToProxyMode;
               setEnabled(d->m_proxyWidget->isEnabled());
               d->enabledChangeMode = QGraphicsProxyWidgetPrivate::NoMode;
            }
            break;

         case QEvent::StyleChange:
            // Propagate style changes to the proxy.
            if (!d->styleChangeMode) {
               d->styleChangeMode = QGraphicsProxyWidgetPrivate::WidgetToProxyMode;
               setStyle(d->m_proxyWidget->style());
               d->styleChangeMode = QGraphicsProxyWidgetPrivate::NoMode;
            }
            break;

#ifndef QT_NO_TOOLTIP
         case QEvent::ToolTipChange:
            // Propagate tooltip change to the proxy.
            if (!d->tooltipChangeMode) {
               d->tooltipChangeMode = QGraphicsProxyWidgetPrivate::WidgetToProxyMode;
               setToolTip(d->m_proxyWidget->toolTip());
               d->tooltipChangeMode = QGraphicsProxyWidgetPrivate::NoMode;
            }
            break;
#endif

         default:
            break;
      }
   }
   return QGraphicsWidget::eventFilter(object, event);
}

void QGraphicsProxyWidget::showEvent(QShowEvent *event)
{
   (void) event;
}

void QGraphicsProxyWidget::hideEvent(QHideEvent *event)
{
   (void) event;
}

#ifndef QT_NO_CONTEXTMENU

void QGraphicsProxyWidget::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
   Q_D(QGraphicsProxyWidget);

   if (! event || ! d->m_proxyWidget || ! d->m_proxyWidget->isVisible() || ! hasFocus()) {
      return;
   }

   // Find widget position and receiver.
   QPointF pos = event->pos();
   QPointer<QWidget> alienWidget = QPointer<QWidget>(d->m_proxyWidget->childAt(pos.toPoint()));

   QPointer<QWidget> receiver;

   if (alienWidget == nullptr) {
      receiver = d->m_proxyWidget;
   } else {
      receiver = alienWidget;
   }

   // Map event position from us to the receiver
   pos = d->mapToReceiver(pos, receiver);

   QPoint globalPos = receiver->mapToGlobal(pos.toPoint());

   //If the receiver by-pass the proxy its popups
   //will be top level QWidgets therefore they need
   //the screen position. mapToGlobal expect the widget to
   //have proper coordinates in regards of the windowing system
   //but it's not true because the widget is embedded.

   if (bypassGraphicsProxyWidget(receiver)) {
      globalPos = event->screenPos();
   }

   // Send mouse event. ### Doesn't propagate the event.
   QContextMenuEvent contextMenuEvent(QContextMenuEvent::Reason(event->reason()),
      pos.toPoint(), globalPos, event->modifiers());
   QApplication::sendEvent(receiver, &contextMenuEvent);

   event->setAccepted(contextMenuEvent.isAccepted());
}
#endif

#ifndef QT_NO_DRAGANDDROP

void QGraphicsProxyWidget::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
   Q_D(QGraphicsProxyWidget);

   if (! d->m_proxyWidget) {
      return;
   }

   QDragEnterEvent proxyDragEnter(event->pos().toPoint(), event->dropAction(), event->mimeData(),
         event->buttons(), event->modifiers());

   proxyDragEnter.setAccepted(event->isAccepted());
   QApplication::sendEvent(d->m_proxyWidget, &proxyDragEnter);
   event->setAccepted(proxyDragEnter.isAccepted());

   if (proxyDragEnter.isAccepted()) {
      // we discard answerRect
      event->setDropAction(proxyDragEnter.dropAction());
   }
}

void QGraphicsProxyWidget::dragLeaveEvent(QGraphicsSceneDragDropEvent *event)
{
   (void) event;

   Q_D(QGraphicsProxyWidget);

   if (! d->m_proxyWidget || ! d->dragDropWidget) {
      return;
   }

   QDragLeaveEvent proxyDragLeave;
   QApplication::sendEvent(d->dragDropWidget, &proxyDragLeave);
   d->dragDropWidget = nullptr;
}

void QGraphicsProxyWidget::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
   Q_D(QGraphicsProxyWidget);

   if (! d->m_proxyWidget) {
      return;
   }

   QPointF p = event->pos();
   event->ignore();

   QPointer<QWidget> subWidget = QPointer<QWidget>(d->m_proxyWidget->childAt(p.toPoint()));
   QPointer<QWidget> receiver;

   if (subWidget == nullptr) {
      receiver = d->m_proxyWidget;
   } else {
      receiver = subWidget;
   }

   bool eventDelivered = false;

   for (; receiver; receiver = receiver->parentWidget()) {
      if (!receiver->isEnabled() || !receiver->acceptDrops()) {
         continue;
      }

      // Map event position from us to the receiver
      QPoint receiverPos = d->mapToReceiver(p, receiver).toPoint();

      if (receiver != d->dragDropWidget) {
         // Try to enter before we leave
         QDragEnterEvent dragEnter(receiverPos, event->possibleActions(), event->mimeData(), event->buttons(),
            event->modifiers());

         dragEnter.setDropAction(event->proposedAction());
         QApplication::sendEvent(receiver, &dragEnter);
         event->setAccepted(dragEnter.isAccepted());
         event->setDropAction(dragEnter.dropAction());

         if (!event->isAccepted()) {
            // propagate to the parent widget
            continue;
         }

         d->lastDropAction = event->dropAction();

         if (d->dragDropWidget) {
            QDragLeaveEvent dragLeave;
            QApplication::sendEvent(d->dragDropWidget, &dragLeave);
         }
         d->dragDropWidget = receiver;
      }

      QDragMoveEvent dragMove(receiverPos, event->possibleActions(), event->mimeData(), event->buttons(), event->modifiers());
      event->setDropAction(d->lastDropAction);
      QApplication::sendEvent(receiver, &dragMove);
      event->setAccepted(dragMove.isAccepted());
      event->setDropAction(dragMove.dropAction());

      if (event->isAccepted()) {
         d->lastDropAction = event->dropAction();
      }

      eventDelivered = true;
      break;
   }

   if (!eventDelivered) {
      if (d->dragDropWidget) {
         // Leave the last drag drop item
         QDragLeaveEvent dragLeave;
         QApplication::sendEvent(d->dragDropWidget, &dragLeave);
         d->dragDropWidget = nullptr;
      }

      // Propagate
      event->setDropAction(Qt::IgnoreAction);
   }
}

void QGraphicsProxyWidget::dropEvent(QGraphicsSceneDragDropEvent *event)
{
   Q_D(QGraphicsProxyWidget);

   if (d->m_proxyWidget && d->dragDropWidget) {
      QPoint widgetPos = d->mapToReceiver(event->pos(), d->dragDropWidget).toPoint();
      QDropEvent dropEvent(widgetPos, event->possibleActions(), event->mimeData(), event->buttons(), event->modifiers());
      QApplication::sendEvent(d->dragDropWidget, &dropEvent);
      event->setAccepted(dropEvent.isAccepted());
      d->dragDropWidget = nullptr;
   }
}

#endif  // QT_NO_DRAGANDDROP

void QGraphicsProxyWidget::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
   (void) event;
}

void QGraphicsProxyWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
   Q_D(QGraphicsProxyWidget);

   // If hoverMove was compressed away, make sure we update properly here.
   if (d->lastWidgetUnderMouse) {
      QApplicationPrivate::dispatchEnterLeave(nullptr, d->lastWidgetUnderMouse, event->screenPos());
      d->lastWidgetUnderMouse = nullptr;
   }
}

void QGraphicsProxyWidget::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
   Q_D(QGraphicsProxyWidget);

#if defined(CS_SHOW_DEBUG_GUI_GRAPHICSVIEW)
   qDebug("QGraphicsProxyWidget::hoverMoveEvent");
#endif

   // Ignore events on the window frame.
   if (! d->m_proxyWidget || !rect().contains(event->pos())) {
      if (d->lastWidgetUnderMouse) {
         QApplicationPrivate::dispatchEnterLeave(nullptr, d->lastWidgetUnderMouse, event->screenPos());
         d->lastWidgetUnderMouse = nullptr;
      }
      return;
   }

   d->embeddedMouseGrabber = nullptr;
   d->sendWidgetMouseEvent(event);
}

void QGraphicsProxyWidget::grabMouseEvent(QEvent *event)
{
   (void) event;
}

void QGraphicsProxyWidget::ungrabMouseEvent(QEvent *event)
{
   (void) event;

   Q_D(QGraphicsProxyWidget);
   d->embeddedMouseGrabber = nullptr;
}

void QGraphicsProxyWidget::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
   Q_D(QGraphicsProxyWidget);

#if defined(CS_SHOW_DEBUG_GUI_GRAPHICSVIEW)
   qDebug("QGraphicsProxyWidget::mouseMoveEvent");
#endif

   d->sendWidgetMouseEvent(event);
}

void QGraphicsProxyWidget::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
   Q_D(QGraphicsProxyWidget);

#if defined(CS_SHOW_DEBUG_GUI_GRAPHICSVIEW)
   qDebug("QGraphicsProxyWidget::mousePressEvent");
#endif

   d->sendWidgetMouseEvent(event);
}

void QGraphicsProxyWidget::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
   Q_D(QGraphicsProxyWidget);

#if defined(CS_SHOW_DEBUG_GUI_GRAPHICSVIEW)
   qDebug("QGraphicsProxyWidget::mouseDoubleClickEvent");
#endif

   d->sendWidgetMouseEvent(event);
}

#ifndef QT_NO_WHEELEVENT

void QGraphicsProxyWidget::wheelEvent(QGraphicsSceneWheelEvent *event)
{
   Q_D(QGraphicsProxyWidget);

#if defined(CS_SHOW_DEBUG_GUI_GRAPHICSVIEW)
   qDebug("QGraphicsProxyWidget::wheelEvent");
#endif

   if (! d->m_proxyWidget) {
      return;
   }

   QPointF pos = event->pos();
   QPointer<QWidget> receiver = QPointer<QWidget>(d->m_proxyWidget->childAt(pos.toPoint()));

   if (receiver == nullptr) {
      receiver = d->m_proxyWidget;
   }

   // Map event position from us to the receiver
   pos = d->mapToReceiver(pos, receiver);

   // Send mouse event.
   QWheelEvent wheelEvent(pos.toPoint(), event->screenPos(), event->delta(),
         event->buttons(), event->modifiers(), event->orientation());

   QPointer<QWidget> focusWidget = QPointer<QWidget>(d->m_proxyWidget->focusWidget());

   extern bool qt_sendSpontaneousEvent(QObject *, QEvent *);
   qt_sendSpontaneousEvent(receiver, &wheelEvent);
   event->setAccepted(wheelEvent.isAccepted());

   // ### Remove, this should be done by proper focusIn/focusOut events.
   if (focusWidget && ! focusWidget->hasFocus()) {
      focusWidget->update();
      focusWidget = d->m_proxyWidget->focusWidget();

      if (focusWidget && focusWidget->hasFocus()) {
         focusWidget->update();
      }
   }
}

#endif // QT_NO_WHEELEVENT

void QGraphicsProxyWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
   Q_D(QGraphicsProxyWidget);

#if defined(CS_SHOW_DEBUG_GUI_GRAPHICSVIEW)
   qDebug("QGraphicsProxyWidget::mouseReleaseEvent");
#endif

   d->sendWidgetMouseEvent(event);
}

void QGraphicsProxyWidget::keyPressEvent(QKeyEvent *event)
{
   Q_D(QGraphicsProxyWidget);

#if defined(CS_SHOW_DEBUG_GUI_GRAPHICSVIEW)
   qDebug("QGraphicsProxyWidget::keyPressEvent");
#endif

   d->sendWidgetKeyEvent(event);
}

void QGraphicsProxyWidget::keyReleaseEvent(QKeyEvent *event)
{
   Q_D(QGraphicsProxyWidget);

#if defined(CS_SHOW_DEBUG_GUI_GRAPHICSVIEW)
   qDebug("QGraphicsProxyWidget::keyReleaseEvent");
#endif

   d->sendWidgetKeyEvent(event);
}

void QGraphicsProxyWidget::focusInEvent(QFocusEvent *event)
{
#if defined(CS_SHOW_DEBUG_GUI_GRAPHICSVIEW)
   qDebug("QGraphicsProxyWidget::focusInEvent");
#endif

   Q_D(QGraphicsProxyWidget);

   if (d->focusFromWidgetToProxy) {
      // Prevent recursion when the proxy autogains focus through the
      // embedded widget calling setFocus(). ### Could be done with event
      // filter on FocusIn instead?
      return;
   }

   d->proxyIsGivingFocus = true;

   switch (event->reason()) {
      case Qt::TabFocusReason: {
         if (QWidget *focusChild = d->findFocusChild(nullptr, true)) {
            focusChild->setFocus(event->reason());
         }
         break;
      }

      case Qt::BacktabFocusReason:
         if (QWidget *focusChild = d->findFocusChild(nullptr, false)) {
            focusChild->setFocus(event->reason());
         }
         break;

      default:
         if (d->m_proxyWidget && d->m_proxyWidget->focusWidget()) {
            d->m_proxyWidget->focusWidget()->setFocus(event->reason());
         }
         break;
   }

   d->proxyIsGivingFocus = false;
}

void QGraphicsProxyWidget::focusOutEvent(QFocusEvent *event)
{
#if defined(CS_SHOW_DEBUG_GUI_GRAPHICSVIEW)
   qDebug("QGraphicsProxyWidget::focusOutEvent");
#endif

   Q_D(QGraphicsProxyWidget);

   if (d->m_proxyWidget) {
      // We need to explicitly remove subfocus from the embedded widget's
      // focus widget.
      if (QWidget *focusWidget = d->m_proxyWidget->focusWidget()) {
         d->removeSubFocusHelper(focusWidget, event->reason());
      }
   }
}

bool QGraphicsProxyWidget::focusNextPrevChild(bool next)
{
   Q_D(QGraphicsProxyWidget);

   if (! d->m_proxyWidget || ! d->m_itemScene) {
      return QGraphicsWidget::focusNextPrevChild(next);
   }

   Qt::FocusReason reason = next ? Qt::TabFocusReason : Qt::BacktabFocusReason;
   QWidget *lastFocusChild = d->m_proxyWidget->focusWidget();

   if (QWidget *newFocusChild = d->findFocusChild(lastFocusChild, next)) {
      newFocusChild->setFocus(reason);
      return true;
   }

   return QGraphicsWidget::focusNextPrevChild(next);
}

QVariant QGraphicsProxyWidget::inputMethodQuery(Qt::InputMethodQuery query) const
{
   Q_D(const QGraphicsProxyWidget);

   if (! d->m_proxyWidget || !hasFocus()) {
      return QVariant();
   }

   QWidget *focusWidget = widget()->focusWidget();
   if (! focusWidget) {
      focusWidget = d->m_proxyWidget;
   }

   QVariant v = focusWidget->inputMethodQuery(query);
   QPointF focusWidgetPos = subWidgetRect(focusWidget).topLeft();

   switch (v.type()) {
      case QVariant::RectF:
         v = v.toRectF().translated(focusWidgetPos);
         break;

      case QVariant::PointF:
         v = v.toPointF() + focusWidgetPos;
         break;

      case QVariant::Rect:
         v = v.toRect().translated(focusWidgetPos.toPoint());
         break;

      case QVariant::Point:
         v = v.toPoint() + focusWidgetPos.toPoint();
         break;

      default:
         break;
   }
   return v;

}

void QGraphicsProxyWidget::inputMethodEvent(QInputMethodEvent *event)
{
   // Forward input method events if the focus widget enables input methods.
   Q_D(const QGraphicsProxyWidget);

   QWidget *focusWidget = d->m_proxyWidget->focusWidget();

   if (focusWidget && focusWidget->testAttribute(Qt::WA_InputMethodEnabled)) {
      QApplication::sendEvent(focusWidget, event);
   }
}

QSizeF QGraphicsProxyWidget::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
   Q_D(const QGraphicsProxyWidget);

   if (! d->m_proxyWidget) {
      return QGraphicsWidget::sizeHint(which, constraint);
   }

   QSizeF sh;

   switch (which) {
      case Qt::PreferredSize:
         if (QLayout *l = d->m_proxyWidget->layout()) {
            sh = l->sizeHint();
         } else {
            sh = d->m_proxyWidget->sizeHint();
         }
         break;

      case Qt::MinimumSize:
         if (QLayout *l = d->m_proxyWidget->layout()) {
            sh = l->minimumSize();
         } else {
            sh = d->m_proxyWidget->minimumSizeHint();
         }
         break;

      case Qt::MaximumSize:
         if (QLayout *l = d->m_proxyWidget->layout()) {
            sh = l->maximumSize();
         } else {
            sh = QSizeF(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
         }
         break;

      case Qt::MinimumDescent:
         sh = constraint;
         break;

      default:
         break;
   }

   return sh;
}

void QGraphicsProxyWidget::resizeEvent(QGraphicsSceneResizeEvent *event)
{
   Q_D(QGraphicsProxyWidget);

   if (d->m_proxyWidget) {
      if (d->sizeChangeMode != QGraphicsProxyWidgetPrivate::WidgetToProxyMode) {
         d->m_proxyWidget->resize(event->newSize().toSize());
      }
   }

   QGraphicsWidget::resizeEvent(event);
}

void QGraphicsProxyWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
   (void) widget;

   Q_D(QGraphicsProxyWidget);

   if (! d->m_proxyWidget || ! d->m_proxyWidget->isVisible()) {
      return;
   }

   // Filter out repaints on the window frame.
   const QRect exposedWidgetRect = (option->exposedRect & rect()).toAlignedRect();

   if (exposedWidgetRect.isEmpty()) {
      return;
   }

   d->m_proxyWidget->render(painter, exposedWidgetRect.topLeft(), exposedWidgetRect);
}

int QGraphicsProxyWidget::type() const
{
   return Type;
}

QGraphicsProxyWidget *QGraphicsProxyWidget::createProxyForChildWidget(QWidget *child)
{
   QGraphicsProxyWidget *proxy = child->graphicsProxyWidget();

   if (proxy) {
      return proxy;
   }

   if (! child->parentWidget()) {
      qWarning("QGraphicsProxyWidget::createProxyForChildWidget() Toplevel widget must be in a QGraphicsScene");
      return nullptr;
   }

   QGraphicsProxyWidget *parentProxy = createProxyForChildWidget(child->parentWidget());
   if (! parentProxy) {
      return nullptr;
   }

   if (! QMetaObject::invokeMethod(parentProxy, "newProxyWidget",  Qt::DirectConnection,
         Q_RETURN_ARG(QGraphicsProxyWidget *, proxy), Q_ARG(const QWidget *, child)))  {
      return nullptr;
   }

   proxy->setParent(parentProxy);
   proxy->setWidget(child);

   return proxy;
}

QGraphicsProxyWidget *QGraphicsProxyWidget::newProxyWidget(const QWidget *)
{
   return new QGraphicsProxyWidget(this);
}

void QGraphicsProxyWidget::_q_removeWidgetSlot()
{
   Q_D(QGraphicsProxyWidget);
   d->_q_removeWidgetSlot();
}

#endif // QT_NO_GRAPHICSVIEW
