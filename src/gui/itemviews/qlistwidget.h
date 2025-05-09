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

#ifndef QLISTWIDGET_H
#define QLISTWIDGET_H

#include <qitemselectionmodel.h>
#include <qlistview.h>
#include <qvariant.h>
#include <qvector.h>

#ifndef QT_NO_LISTWIDGET

class QEvent;
class QListModel;
class QListWidget;
class QWidgetItemData;

class QListWidgetItemPrivate;
class QListWidgetPrivate;

class Q_GUI_EXPORT QListWidgetItem
{
   friend class QListModel;
   friend class QListWidget;

 public:
   enum ItemType {
      Type     = 0,
      UserType = 1000
   };

   explicit QListWidgetItem(QListWidget *view = nullptr, int type = Type);
   explicit QListWidgetItem(const QString &text, QListWidget *view = nullptr, int type = Type);
   explicit QListWidgetItem(const QIcon &icon, const QString &text, QListWidget *view = nullptr, int type = Type);

   virtual ~QListWidgetItem();

   QListWidgetItem(const QListWidgetItem &other);

   virtual QListWidgetItem *clone() const;

   QListWidget *listWidget() const {
      return m_view;
   }

   inline void setSelected(bool select);
   inline bool isSelected() const;

   inline void setHidden(bool hide);
   inline bool isHidden() const;

   Qt::ItemFlags flags() const {
      return itemFlags;
   }
   void setFlags(Qt::ItemFlags flags);

   QString text() const {
      return data(Qt::DisplayRole).toString();
   }
   inline void setText(const QString &text);

   QIcon icon() const {
      return data(Qt::DecorationRole).value<QIcon>();
   }
   inline void setIcon(const QIcon &icon);

   QString statusTip() const {
      return data(Qt::StatusTipRole).toString();
   }
   inline void setStatusTip(const QString &statusTip);

#ifndef QT_NO_TOOLTIP
   QString toolTip() const {
      return data(Qt::ToolTipRole).toString();
   }
   inline void setToolTip(const QString &toolTip);
#endif

#ifndef QT_NO_WHATSTHIS
   QString whatsThis() const {
      return data(Qt::WhatsThisRole).toString();
   }
   inline void setWhatsThis(const QString &whatsThis);
#endif

   QFont font() const {
      return data(Qt::FontRole).value<QFont>();
   }

   inline void setFont(const QFont &font);

   int textAlignment() const {
      return data(Qt::TextAlignmentRole).toInt();
   }
   void setTextAlignment(int alignment) {
      setData(Qt::TextAlignmentRole, alignment);
   }

   QColor backgroundColor() const {
      return data(Qt::BackgroundColorRole).value<QColor>();
   }

   virtual void setBackgroundColor(const QColor &color) {
      setData(Qt::BackgroundColorRole, color);
   }

   QBrush background() const {
      return data(Qt::BackgroundRole).value<QBrush>();
   }

   void setBackground(const QBrush &brush) {
      setData(Qt::BackgroundRole, brush);
   }

   QColor textColor() const {
      return data(Qt::TextColorRole).value<QColor>();
   }

   void setTextColor(const QColor &color) {
      setData(Qt::TextColorRole, color);
   }

   QBrush foreground() const {
      return data(Qt::ForegroundRole).value<QBrush>();
   }

   void setForeground(const QBrush &brush) {
      setData(Qt::ForegroundRole, brush);
   }

   Qt::CheckState checkState() const {
      return static_cast<Qt::CheckState>(data(Qt::CheckStateRole).toInt());
   }

   void setCheckState(Qt::CheckState state) {
      setData(Qt::CheckStateRole, static_cast<int>(state));
   }

   QSize sizeHint() const {
      return data(Qt::SizeHintRole).value<QSize>();
   }

   void setSizeHint(const QSize &size) {
      setData(Qt::SizeHintRole, size);
   }

   virtual QVariant data(int role) const;
   virtual void setData(int role, const QVariant &value);

   virtual bool operator<(const QListWidgetItem &other) const;

   virtual void read(QDataStream &in);
   virtual void write(QDataStream &out) const;

   QListWidgetItem &operator=(const QListWidgetItem &other);

   int type() const {
      return rtti;
   }

 private:
   int rtti;
   QVector<void *> dummy;
   QListWidget *m_view;
   QListWidgetItemPrivate *d;
   Qt::ItemFlags itemFlags;
};

inline void QListWidgetItem::setText(const QString &text)
{
   setData(Qt::DisplayRole, text);
}

inline void QListWidgetItem::setIcon(const QIcon &icon)
{
   setData(Qt::DecorationRole, icon);
}

inline void QListWidgetItem::setStatusTip(const QString &statusTip)
{
   setData(Qt::StatusTipRole, statusTip);
}

#ifndef QT_NO_TOOLTIP
inline void QListWidgetItem::setToolTip(const QString &toolTip)
{
   setData(Qt::ToolTipRole, toolTip);
}
#endif

#ifndef QT_NO_WHATSTHIS
inline void QListWidgetItem::setWhatsThis(const QString &whatsThis)
{
   setData(Qt::WhatsThisRole, whatsThis);
}
#endif

inline void QListWidgetItem::setFont(const QFont &font)
{
   setData(Qt::FontRole, font);
}

Q_GUI_EXPORT QDataStream &operator<<(QDataStream &out, const QListWidgetItem &item);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &in, QListWidgetItem &item);

class Q_GUI_EXPORT QListWidget : public QListView
{
   GUI_CS_OBJECT(QListWidget)

   GUI_CS_PROPERTY_READ(count, count)

   GUI_CS_PROPERTY_READ(currentRow, currentRow)
   GUI_CS_PROPERTY_WRITE(currentRow, cs_setCurrentRow)
   GUI_CS_PROPERTY_NOTIFY(currentRow, currentRowChanged)
   GUI_CS_PROPERTY_USER(currentRow, true)

   GUI_CS_PROPERTY_READ(sortingEnabled, isSortingEnabled)
   GUI_CS_PROPERTY_WRITE(sortingEnabled, setSortingEnabled)

   friend class QListWidgetItem;
   friend class QListModel;

 public:
   explicit QListWidget(QWidget *parent = nullptr);

   QListWidget(const QListWidget &) = delete;
   QListWidget &operator=(const QListWidget &) = delete;

   ~QListWidget();

   QListWidgetItem *item(int row) const;
   int row(const QListWidgetItem *item) const;
   void insertItem(int row, QListWidgetItem *item);
   void insertItem(int row, const QString &label);
   void insertItems(int row, const QStringList &labels);

   void addItem(const QString &label) {
      insertItem(count(), label);
   }

   inline void addItem(QListWidgetItem *item);

   void addItems(const QStringList &labels) {
      insertItems(count(), labels);
   }

   QListWidgetItem *takeItem(int row);
   int count() const;

   QListWidgetItem *currentItem() const;
   void setCurrentItem(QListWidgetItem *item);
   void setCurrentItem(QListWidgetItem *item, QItemSelectionModel::SelectionFlags command);

   int currentRow() const;
   void setCurrentRow(int row);
   void setCurrentRow(int row, QItemSelectionModel::SelectionFlags command);

   // wrapper for overloaded method
   inline void cs_setCurrentRow(int row);

   QListWidgetItem *itemAt(const QPoint &p) const;
   inline QListWidgetItem *itemAt(int x, int y) const;
   QRect visualItemRect(const QListWidgetItem *item) const;

   void sortItems(Qt::SortOrder order = Qt::AscendingOrder);
   void setSortingEnabled(bool enable);
   bool isSortingEnabled() const;

   void editItem(QListWidgetItem *item);
   void openPersistentEditor(QListWidgetItem *item);
   void closePersistentEditor(QListWidgetItem *item);

   QWidget *itemWidget(QListWidgetItem *item) const;
   void setItemWidget(QListWidgetItem *item, QWidget *widget);
   inline void removeItemWidget(QListWidgetItem *item);

   bool isItemSelected(const QListWidgetItem *item) const;
   void setItemSelected(const QListWidgetItem *item, bool select);
   QList<QListWidgetItem *> selectedItems() const;
   QList<QListWidgetItem *> findItems(const QString &text, Qt::MatchFlags flags) const;

   bool isItemHidden(const QListWidgetItem *item) const;
   void setItemHidden(const QListWidgetItem *item, bool hide);
   void dropEvent(QDropEvent *event) override;

   GUI_CS_SLOT_1(Public, void scrollToItem(const QListWidgetItem *item, QAbstractItemView::ScrollHint hint = EnsureVisible))
   GUI_CS_SLOT_2(scrollToItem)

   GUI_CS_SLOT_1(Public, void clear())
   GUI_CS_SLOT_2(clear)

   GUI_CS_SIGNAL_1(Public, void itemPressed(QListWidgetItem *item))
   GUI_CS_SIGNAL_2(itemPressed, item)

   GUI_CS_SIGNAL_1(Public, void itemClicked(QListWidgetItem *item))
   GUI_CS_SIGNAL_2(itemClicked, item)

   GUI_CS_SIGNAL_1(Public, void itemDoubleClicked(QListWidgetItem *item))
   GUI_CS_SIGNAL_2(itemDoubleClicked, item)

   GUI_CS_SIGNAL_1(Public, void itemActivated(QListWidgetItem *item))
   GUI_CS_SIGNAL_2(itemActivated, item)

   GUI_CS_SIGNAL_1(Public, void itemEntered(QListWidgetItem *item))
   GUI_CS_SIGNAL_2(itemEntered, item)

   GUI_CS_SIGNAL_1(Public, void itemChanged(QListWidgetItem *item))
   GUI_CS_SIGNAL_2(itemChanged, item)

   GUI_CS_SIGNAL_1(Public, void currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous))
   GUI_CS_SIGNAL_2(currentItemChanged, current, previous)

   GUI_CS_SIGNAL_1(Public, void currentTextChanged(const QString &currentText))
   GUI_CS_SIGNAL_2(currentTextChanged, currentText)

   GUI_CS_SIGNAL_1(Public, void currentRowChanged(int currentRow))
   GUI_CS_SIGNAL_2(currentRowChanged, currentRow)

   GUI_CS_SIGNAL_1(Public, void itemSelectionChanged())
   GUI_CS_SIGNAL_2(itemSelectionChanged)

 protected:
   bool event(QEvent *event) override;
   virtual QStringList mimeTypes() const;
   virtual QMimeData *mimeData(const QList<QListWidgetItem *> &items) const;

#ifndef QT_NO_DRAGANDDROP
   virtual bool dropMimeData(int index, const QMimeData *data, Qt::DropAction action);
   virtual Qt::DropActions supportedDropActions() const;
#endif

   QList<QListWidgetItem *> items(const QMimeData *data) const;

   QModelIndex indexFromItem(QListWidgetItem *item) const;
   QListWidgetItem *itemFromIndex(const QModelIndex &index) const;

 private:
   void setModel(QAbstractItemModel *model) override;
   Qt::SortOrder sortOrder() const;

   Q_DECLARE_PRIVATE(QListWidget)

   GUI_CS_SLOT_1(Private, void _q_emitItemPressed(const QModelIndex &index))
   GUI_CS_SLOT_2(_q_emitItemPressed)

   GUI_CS_SLOT_1(Private, void _q_emitItemClicked(const QModelIndex &index))
   GUI_CS_SLOT_2(_q_emitItemClicked)

   GUI_CS_SLOT_1(Private, void _q_emitItemDoubleClicked(const QModelIndex &index))
   GUI_CS_SLOT_2(_q_emitItemDoubleClicked)

   GUI_CS_SLOT_1(Private, void _q_emitItemActivated(const QModelIndex &index))
   GUI_CS_SLOT_2(_q_emitItemActivated)

   GUI_CS_SLOT_1(Private, void _q_emitItemEntered(const QModelIndex &index))
   GUI_CS_SLOT_2(_q_emitItemEntered)

   GUI_CS_SLOT_1(Private, void _q_emitItemChanged(const QModelIndex &index))
   GUI_CS_SLOT_2(_q_emitItemChanged)

   GUI_CS_SLOT_1(Private, void _q_emitCurrentItemChanged(const QModelIndex &previous, const QModelIndex &current))
   GUI_CS_SLOT_2(_q_emitCurrentItemChanged)

   GUI_CS_SLOT_1(Private, void _q_sort())
   GUI_CS_SLOT_2(_q_sort)

   GUI_CS_SLOT_1(Private, void _q_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight))
   GUI_CS_SLOT_2(_q_dataChanged)
};

void QListWidget::cs_setCurrentRow(int row)
{
   setCurrentRow(row);
}

void QListWidget::removeItemWidget(QListWidgetItem *item)
{
   setItemWidget(item, nullptr);
}

void QListWidget::addItem(QListWidgetItem *item)
{
   insertItem(count(), item);
}

QListWidgetItem *QListWidget::itemAt(int x, int y) const
{
   return itemAt(QPoint(x, y));
}

void QListWidgetItem::setSelected(bool select)
{
   if (m_view != nullptr) {
      m_view->setItemSelected(this, select);
   }
}

bool QListWidgetItem::isSelected() const
{
   return (m_view ? m_view->isItemSelected(this) : false);
}

void QListWidgetItem::setHidden(bool hide)
{
   if (m_view != nullptr) {
      m_view->setItemHidden(this, hide);
   }
}

bool QListWidgetItem::isHidden() const
{
   return (m_view ? m_view->isItemHidden(this) : false);
}

#endif // QT_NO_LISTWIDGET

#endif // QLISTWIDGET_H
