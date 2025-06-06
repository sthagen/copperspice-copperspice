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

#ifndef QCOMBOBOX_P_H
#define QCOMBOBOX_P_H

#include <qcombobox.h>

#ifndef QT_NO_COMBOBOX

#include <qabstractslider.h>
#include <qapplication.h>
#include <qcompleter.h>
#include <qdebug.h>
#include <qevent.h>
#include <qitemdelegate.h>
#include <qlineedit.h>
#include <qlistview.h>
#include <qpainter.h>
#include <qpair.h>
#include <qpointer.h>
#include <qstandarditemmodel.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qtimer.h>
#include <qwidget_p.h>

#include <limits.h>

class QAction;
class QPlatformMenu;
class QStandardItemModel;

class QComboBoxListView : public QListView
{
   GUI_CS_OBJECT(QComboBoxListView)

 public:
   QComboBoxListView(QComboBox *cmb = nullptr)
      : combo(cmb)
   {
   }

 protected:
   void resizeEvent(QResizeEvent *event)  override {
      resizeContents(viewport()->width(), contentsSize().height());
      QListView::resizeEvent(event);
   }

   QStyleOptionViewItem viewOptions() const override {
      QStyleOptionViewItem option = QListView::viewOptions();
      option.showDecorationSelected = true;
      if (combo) {
         option.font = combo->font();
      }
      return option;
   }

   void paintEvent(QPaintEvent *e) override {
      if (combo) {
         QStyleOptionComboBox opt;
         opt.initFrom(combo);
         opt.editable = combo->isEditable();
         if (combo->style()->styleHint(QStyle::SH_ComboBox_Popup, &opt, combo)) {
            //we paint the empty menu area to avoid having blank space that can happen when scrolling
            QStyleOptionMenuItem menuOpt;
            menuOpt.initFrom(this);
            menuOpt.palette = palette();
            menuOpt.state = QStyle::State_None;
            menuOpt.checkType = QStyleOptionMenuItem::NotCheckable;
            menuOpt.menuRect = e->rect();
            menuOpt.maxIconWidth = 0;
            menuOpt.tabWidth = 0;
            QPainter p(viewport());
            combo->style()->drawControl(QStyle::CE_MenuEmptyArea, &menuOpt, &p, this);
         }
      }
      QListView::paintEvent(e);
   }

 private:
   QComboBox *combo;
};

class QComboBoxPrivateScroller : public QWidget
{
   GUI_CS_OBJECT(QComboBoxPrivateScroller)

 public:
   QComboBoxPrivateScroller(QAbstractSlider::SliderAction action, QWidget *parent)
      : QWidget(parent), sliderAction(action) {
      setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
      setAttribute(Qt::WA_NoMousePropagation);
   }

   QSize sizeHint() const override {
      return QSize(20, style()->pixelMetric(QStyle::PM_MenuScrollerHeight));
   }

   GUI_CS_SIGNAL_1(Public, void doScroll(int action))
   GUI_CS_SIGNAL_2(doScroll, action)

 protected:
   void stopTimer() {
      timer.stop();
   }

   void startTimer() {
      timer.start(100, this);
      fast = false;
   }

   void enterEvent(QEvent *) override {
      startTimer();
   }

   void leaveEvent(QEvent *) override {
      stopTimer();
   }

   void timerEvent(QTimerEvent *e) override {
      if (e->timerId() == timer.timerId()) {
         emit doScroll(sliderAction);
         if (fast) {
            emit doScroll(sliderAction);
            emit doScroll(sliderAction);
         }
      }
   }

   void hideEvent(QHideEvent *) override {
      stopTimer();
   }

   void mouseMoveEvent(QMouseEvent *e) override {
      // Enable fast scrolling if the cursor is directly above or below the popup.
      const int mouseX = e->pos().x();
      const int mouseY = e->pos().y();
      const bool horizontallyInside = pos().x() < mouseX && mouseX < rect().right() + 1;
      const bool verticallyOutside = (sliderAction == QAbstractSlider::SliderSingleStepAdd) ?
         rect().bottom() + 1 < mouseY : mouseY < pos().y();

      fast = horizontallyInside && verticallyOutside;
   }

   void paintEvent(QPaintEvent *) override {
      QPainter p(this);
      QStyleOptionMenuItem menuOpt;
      menuOpt.initFrom(this);
      menuOpt.checkType = QStyleOptionMenuItem::NotCheckable;
      menuOpt.menuRect = rect();
      menuOpt.maxIconWidth = 0;
      menuOpt.tabWidth = 0;
      menuOpt.menuItemType = QStyleOptionMenuItem::Scroller;

      if (sliderAction == QAbstractSlider::SliderSingleStepAdd) {
         menuOpt.state |= QStyle::State_DownArrow;
      }

      p.eraseRect(rect());
      style()->drawControl(QStyle::CE_MenuScroller, &menuOpt, &p);
   }

 private:
   QAbstractSlider::SliderAction sliderAction;
   QBasicTimer timer;
   bool fast;
};

class QComboBoxPrivateContainer : public QFrame
{
   GUI_CS_OBJECT(QComboBoxPrivateContainer)

 public:
   QComboBoxPrivateContainer(QAbstractItemView *itemView, QComboBox *parent);
   QAbstractItemView *itemView() const;
   void setItemView(QAbstractItemView *itemView);
   int spacing() const;
   int topMargin() const;
   int bottomMargin() const {
      return topMargin();
   }
   void updateTopBottomMargin();

   QTimer blockMouseReleaseTimer;
   QBasicTimer adjustSizeTimer;
   QPoint initialClickPosition;

   GUI_CS_SLOT_1(Public, void scrollItemView(int action))
   GUI_CS_SLOT_2(scrollItemView)

   GUI_CS_SLOT_1(Public, void updateScrollers())
   GUI_CS_SLOT_2(updateScrollers)

   GUI_CS_SLOT_1(Public, void viewDestroyed())
   GUI_CS_SLOT_2(viewDestroyed)

   GUI_CS_SIGNAL_1(Public, void itemSelected(const QModelIndex &index))
   GUI_CS_SIGNAL_2(itemSelected, index)

   GUI_CS_SIGNAL_1(Public, void resetButton())
   GUI_CS_SIGNAL_2(resetButton)

 protected:
   void changeEvent(QEvent *e) override;
   bool eventFilter(QObject *o, QEvent *e) override;
   void mousePressEvent(QMouseEvent *e) override;
   void mouseReleaseEvent(QMouseEvent *e) override;
   void showEvent(QShowEvent *e) override;
   void hideEvent(QHideEvent *e) override;
   void timerEvent(QTimerEvent *timerEvent) override;
   void leaveEvent(QEvent *e) override;
   void resizeEvent(QResizeEvent *e) override;
   QStyleOptionComboBox comboStyleOption() const;

 private:
   QComboBox *combo;
   QAbstractItemView *view;
   QComboBoxPrivateScroller *top;
   QComboBoxPrivateScroller *bottom;
   bool maybeIgnoreMouseButtonRelease;
   QElapsedTimer popupTimer;
   friend class QComboBox;
   friend class QComboBoxPrivate;
};

class QComboMenuDelegate : public QAbstractItemDelegate
{
   GUI_CS_OBJECT(QComboMenuDelegate)

 public:
   QComboMenuDelegate(QObject *parent, QComboBox *cmb) : QAbstractItemDelegate(parent), mCombo(cmb) {}

 protected:
   void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
      QStyleOptionMenuItem opt = getStyleOption(option, index);
      painter->fillRect(option.rect, opt.palette.background());

      mCombo->style()->drawControl(QStyle::CE_MenuItem, &opt, painter, mCombo);
   }

   QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
      QStyleOptionMenuItem opt = getStyleOption(option, index);
      return mCombo->style()->sizeFromContents(QStyle::CT_MenuItem, &opt, option.rect.size(), mCombo);
   }

 private:
   QStyleOptionMenuItem getStyleOption(const QStyleOptionViewItem &option, const QModelIndex &index) const;
   QComboBox *mCombo;
};

// Note that this class is intentionally not using QStyledItemDelegate
// Vista does not use the new theme for combo boxes and there might
// be other side effects from using the new class
class QComboBoxDelegate : public QItemDelegate
{
   GUI_CS_OBJECT(QComboBoxDelegate)

 public:
   QComboBoxDelegate(QObject *parent, QComboBox *cmb) : QItemDelegate(parent), mCombo(cmb) {}

   static bool isSeparator(const QModelIndex &index) {
      return index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("separator");
   }

   static void setSeparator(QAbstractItemModel *model, const QModelIndex &index) {
      model->setData(index, QString::fromLatin1("separator"), Qt::AccessibleDescriptionRole);

      if (QStandardItemModel *m = qobject_cast<QStandardItemModel *>(model))
         if (QStandardItem *item = m->itemFromIndex(index)) {
            item->setFlags(item->flags() & ~(Qt::ItemIsSelectable | Qt::ItemIsEnabled));
         }
   }

 protected:
   void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {

      if (isSeparator(index)) {
         QRect rect = option.rect;

         if (const QAbstractItemView *view = qobject_cast<const QAbstractItemView *>(option.widget)) {
            rect.setWidth(view->viewport()->width());
         }

         QStyleOption opt;
         opt.rect = rect;
         mCombo->style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &opt, painter, mCombo);

      } else {
         QItemDelegate::paint(painter, option, index);
      }
   }

   QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
      if (isSeparator(index)) {
         int pm = mCombo->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, nullptr, mCombo);
         return QSize(pm, pm);
      }

      return QItemDelegate::sizeHint(option, index);
   }

 private:
   QComboBox *mCombo;
};

class QComboBoxPrivate : public QWidgetPrivate
{
   Q_DECLARE_PUBLIC(QComboBox)

 public:
   QComboBoxPrivate();
   ~QComboBoxPrivate();

   void init();
   QComboBoxPrivateContainer *viewContainer();
   void updateLineEditGeometry();
   Qt::MatchFlags matchFlags() const;
   void _q_editingFinished();
   void _q_returnPressed();
   void _q_complete();
   void _q_itemSelected(const QModelIndex &item);
   bool contains(const QString &text, int role);
   void emitActivated(const QModelIndex &);
   void _q_emitHighlighted(const QModelIndex &);
   void _q_emitCurrentIndexChanged(const QModelIndex &index);
   void _q_modelDestroyed();
   void _q_modelReset();

#ifndef QT_NO_COMPLETER
   void _q_completerActivated(const QModelIndex &index);
#endif

   void _q_resetButton();
   void _q_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
   void _q_updateIndexBeforeChange();
   void _q_rowsInserted(const QModelIndex &parent, int start, int end);
   void _q_rowsRemoved(const QModelIndex &parent, int start, int end);
   void updateArrow(QStyle::StateFlag state);
   bool updateHoverControl(const QPoint &pos);
   QRect popupGeometry(int screen = -1) const;
   QStyle::SubControl newHoverControl(const QPoint &pos);
   int computeWidthHint() const;
   QSize recomputeSizeHint(QSize &sh) const;
   void adjustComboBoxSize();
   QString itemText(const QModelIndex &index) const;
   QIcon itemIcon(const QModelIndex &index) const;
   int itemRole() const;
   void updateLayoutDirection();
   void setCurrentIndex(const QModelIndex &index);
   void updateDelegate(bool force = false);
   void keyboardSearchString(const QString &text);
   void modelChanged();
   void updateViewContainerPaletteAndOpacity();
   void updateFocusPolicy();
   void showPopupFromMouseEvent(QMouseEvent *e);
#ifdef Q_OS_DARWIN
   void cleanupNativePopup();
   bool showNativePopup();
#endif

   QAbstractItemModel *model;
   QLineEdit *lineEdit;
   QComboBoxPrivateContainer *container;
   QComboBox::InsertPolicy insertPolicy;
   QComboBox::SizeAdjustPolicy sizeAdjustPolicy;
   int minimumContentsLength;
   QSize iconSize;
   uint shownOnce : 1;
   uint autoCompletion : 1;
   uint duplicatesEnabled : 1;
   uint frame : 1;
   uint padding : 26;
   int maxVisibleItems;
   int maxCount;
   int modelColumn;
   bool inserting;
   mutable QSize minimumSizeHint;
   mutable QSize sizeHint;
   QStyle::StateFlag arrowState;
   QStyle::SubControl hoverControl;
   QRect hoverRect;
   QPersistentModelIndex currentIndex;
   QPersistentModelIndex root;
   Qt::CaseSensitivity autoCompletionCaseSensitivity;
   int indexBeforeChange;

#ifdef Q_OS_DARWIN
   QPlatformMenu *m_platformMenu;
#endif

#ifndef QT_NO_COMPLETER
   QPointer<QCompleter> completer;
#endif

   static QPalette viewContainerPalette(QComboBox *cmb) {
      return cmb->d_func()->viewContainer()->palette();
   }
};

#endif // QT_NO_COMBOBOX

#endif
