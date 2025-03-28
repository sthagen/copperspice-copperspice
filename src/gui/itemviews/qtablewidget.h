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

#ifndef QTABLEWIDGET_H
#define QTABLEWIDGET_H

#include <qtableview.h>
#include <qvariant.h>
#include <qvector.h>

#ifndef QT_NO_TABLEWIDGET

class QTableModel;
class QTableWidget;
class QWidgetItemData;

class QTableWidgetItemPrivate;

class Q_GUI_EXPORT QTableWidgetSelectionRange
{
 public:
   QTableWidgetSelectionRange();
   QTableWidgetSelectionRange(int top, int left, int bottom, int right);

   ~QTableWidgetSelectionRange();

   QTableWidgetSelectionRange(const QTableWidgetSelectionRange &other);

   int topRow() const {
      return m_tableWidgetTop;
   }

   int bottomRow() const {
      return m_tableWidgetBottom;
   }

   int leftColumn() const {
      return m_tableWidgetLeft;
   }

   int rightColumn() const {
      return m_tableWidgetRight;
   }

   int rowCount() const {
      return m_tableWidgetBottom - m_tableWidgetTop + 1;
   }

   int columnCount() const {
      return m_tableWidgetRight - m_tableWidgetLeft + 1;
   }

 private:
   int m_tableWidgetTop;
   int m_tableWidgetLeft;
   int m_tableWidgetBottom;
   int m_tableWidgetRight;
};

class Q_GUI_EXPORT QTableWidgetItem
{
 public:
   enum ItemType {
      Type = 0,
      UserType = 1000
   };

   explicit QTableWidgetItem(int type = Type);
   explicit QTableWidgetItem(const QString &text, int type = Type);
   explicit QTableWidgetItem(const QIcon &icon, const QString &text, int type = Type);

   virtual ~QTableWidgetItem();

   QTableWidgetItem(const QTableWidgetItem &other);

   virtual QTableWidgetItem *clone() const;

   QTableWidget *tableWidget() const {
      return view;
   }

   inline int row() const;
   inline int column() const;

   inline void setSelected(bool select);
   inline bool isSelected() const;

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

   void setBackgroundColor(const QColor &color) {
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
      setData(Qt::CheckStateRole, state);
   }

   QSize sizeHint() const {
      return data(Qt::SizeHintRole).value<QSize>();
   }

   void setSizeHint(const QSize &size) {
      setData(Qt::SizeHintRole, size);
   }

   virtual QVariant data(int role) const;
   virtual void setData(int role, const QVariant &value);

   virtual bool operator<(const QTableWidgetItem &other) const;

#ifndef QT_NO_DATASTREAM
   virtual void read(QDataStream &in);
   virtual void write(QDataStream &out) const;
#endif

   QTableWidgetItem &operator=(const QTableWidgetItem &other);

   int type() const {
      return rtti;
   }

 private:
   int rtti;
   QVector<QWidgetItemData> values;
   QTableWidget *view;
   QTableWidgetItemPrivate *d;
   Qt::ItemFlags itemFlags;

   friend class QTableWidget;
   friend class QTableModel;
};

inline void QTableWidgetItem::setText(const QString &text)
{
   setData(Qt::DisplayRole, text);
}

inline void QTableWidgetItem::setIcon(const QIcon &icon)
{
   setData(Qt::DecorationRole, icon);
}

inline void QTableWidgetItem::setStatusTip(const QString &statusTip)
{
   setData(Qt::StatusTipRole, statusTip);
}

#ifndef QT_NO_TOOLTIP
inline void QTableWidgetItem::setToolTip(const QString &toolTip)
{
   setData(Qt::ToolTipRole, toolTip);
}
#endif

#ifndef QT_NO_WHATSTHIS
inline void QTableWidgetItem::setWhatsThis(const QString &whatsThis)
{
   setData(Qt::WhatsThisRole, whatsThis);
}
#endif

inline void QTableWidgetItem::setFont(const QFont &font)
{
   setData(Qt::FontRole, font);
}

Q_GUI_EXPORT QDataStream &operator>>(QDataStream &in, QTableWidgetItem &item);
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &out, const QTableWidgetItem &item);

class QTableWidgetPrivate;

class Q_GUI_EXPORT QTableWidget : public QTableView
{
   GUI_CS_OBJECT(QTableWidget)

   GUI_CS_PROPERTY_READ(rowCount, rowCount)
   GUI_CS_PROPERTY_WRITE(rowCount, setRowCount)

   GUI_CS_PROPERTY_READ(columnCount, columnCount)
   GUI_CS_PROPERTY_WRITE(columnCount, setColumnCount)

   friend class QTableModel;

 public:
   explicit QTableWidget(QWidget *parent = nullptr);
   QTableWidget(int rows, int columns, QWidget *parent = nullptr);

   QTableWidget(const QTableWidget &) = delete;
   QTableWidget &operator=(const QTableWidget &) = delete;

   ~QTableWidget();

   void setRowCount(int rows);
   int rowCount() const;

   void setColumnCount(int columns);
   int columnCount() const;

   int row(const QTableWidgetItem *item) const;
   int column(const QTableWidgetItem *item) const;

   QTableWidgetItem *item(int row, int column) const;
   void setItem(int row, int column, QTableWidgetItem *item);
   QTableWidgetItem *takeItem(int row, int column);

   QTableWidgetItem *verticalHeaderItem(int row) const;
   void setVerticalHeaderItem(int row, QTableWidgetItem *item);
   QTableWidgetItem *takeVerticalHeaderItem(int row);

   QTableWidgetItem *horizontalHeaderItem(int column) const;
   void setHorizontalHeaderItem(int column, QTableWidgetItem *item);
   QTableWidgetItem *takeHorizontalHeaderItem(int column);
   void setVerticalHeaderLabels(const QStringList &labels);
   void setHorizontalHeaderLabels(const QStringList &labels);

   int currentRow() const;
   int currentColumn() const;
   QTableWidgetItem *currentItem() const;
   void setCurrentItem(QTableWidgetItem *item);
   void setCurrentItem(QTableWidgetItem *item, QItemSelectionModel::SelectionFlags command);
   void setCurrentCell(int row, int column);
   void setCurrentCell(int row, int column, QItemSelectionModel::SelectionFlags command);

   void sortItems(int column, Qt::SortOrder order = Qt::AscendingOrder);
   void setSortingEnabled(bool enable);
   bool isSortingEnabled() const;

   void editItem(QTableWidgetItem *item);
   void openPersistentEditor(QTableWidgetItem *item);
   void closePersistentEditor(QTableWidgetItem *item);

   QWidget *cellWidget(int row, int column) const;
   void setCellWidget(int row, int column, QWidget *widget);
   inline void removeCellWidget(int row, int column);

   bool isItemSelected(const QTableWidgetItem *item) const;
   void setItemSelected(const QTableWidgetItem *item, bool select);
   void setRangeSelected(const QTableWidgetSelectionRange &range, bool select);

   QList<QTableWidgetSelectionRange> selectedRanges() const;
   QList<QTableWidgetItem *> selectedItems() const;
   QList<QTableWidgetItem *> findItems(const QString &text, Qt::MatchFlags flags) const;

   int visualRow(int logicalRow) const;
   int visualColumn(int logicalColumn) const;

   QTableWidgetItem *itemAt(const QPoint &point) const;
   inline QTableWidgetItem *itemAt(int x, int y) const;
   QRect visualItemRect(const QTableWidgetItem *item) const;

   const QTableWidgetItem *itemPrototype() const;
   void setItemPrototype(const QTableWidgetItem *item);

   GUI_CS_SLOT_1(Public, void scrollToItem(const QTableWidgetItem *item, QAbstractItemView::ScrollHint hint = EnsureVisible))
   GUI_CS_SLOT_2(scrollToItem)

   GUI_CS_SLOT_1(Public, void insertRow(int row))
   GUI_CS_SLOT_2(insertRow)

   GUI_CS_SLOT_1(Public, void insertColumn(int column))
   GUI_CS_SLOT_2(insertColumn)

   GUI_CS_SLOT_1(Public, void removeRow(int row))
   GUI_CS_SLOT_2(removeRow)

   GUI_CS_SLOT_1(Public, void removeColumn(int column))
   GUI_CS_SLOT_2(removeColumn)

   GUI_CS_SLOT_1(Public, void clear())
   GUI_CS_SLOT_2(clear)

   GUI_CS_SLOT_1(Public, void clearContents())
   GUI_CS_SLOT_2(clearContents)

   GUI_CS_SIGNAL_1(Public, void itemPressed(QTableWidgetItem *item))
   GUI_CS_SIGNAL_2(itemPressed, item)
   GUI_CS_SIGNAL_1(Public, void itemClicked(QTableWidgetItem *item))
   GUI_CS_SIGNAL_2(itemClicked, item)
   GUI_CS_SIGNAL_1(Public, void itemDoubleClicked(QTableWidgetItem *item))
   GUI_CS_SIGNAL_2(itemDoubleClicked, item)

   GUI_CS_SIGNAL_1(Public, void itemActivated(QTableWidgetItem *item))
   GUI_CS_SIGNAL_2(itemActivated, item)

   GUI_CS_SIGNAL_1(Public, void itemEntered(QTableWidgetItem *item))
   GUI_CS_SIGNAL_2(itemEntered, item)

   GUI_CS_SIGNAL_1(Public, void itemChanged(QTableWidgetItem *item))
   GUI_CS_SIGNAL_2(itemChanged, item)

   GUI_CS_SIGNAL_1(Public, void currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *previous))
   GUI_CS_SIGNAL_2(currentItemChanged, current, previous)

   GUI_CS_SIGNAL_1(Public, void itemSelectionChanged())
   GUI_CS_SIGNAL_2(itemSelectionChanged)

   GUI_CS_SIGNAL_1(Public, void cellPressed(int row, int column))
   GUI_CS_SIGNAL_2(cellPressed, row, column)
   GUI_CS_SIGNAL_1(Public, void cellClicked(int row, int column))
   GUI_CS_SIGNAL_2(cellClicked, row, column)
   GUI_CS_SIGNAL_1(Public, void cellDoubleClicked(int row, int column))
   GUI_CS_SIGNAL_2(cellDoubleClicked, row, column)

   GUI_CS_SIGNAL_1(Public, void cellActivated(int row, int column))
   GUI_CS_SIGNAL_2(cellActivated, row, column)
   GUI_CS_SIGNAL_1(Public, void cellEntered(int row, int column))
   GUI_CS_SIGNAL_2(cellEntered, row, column)
   GUI_CS_SIGNAL_1(Public, void cellChanged(int row, int column))
   GUI_CS_SIGNAL_2(cellChanged, row, column)

   GUI_CS_SIGNAL_1(Public, void currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn))
   GUI_CS_SIGNAL_2(currentCellChanged, currentRow, currentColumn, previousRow, previousColumn)

 protected:
   bool event(QEvent *event) override;
   virtual QStringList mimeTypes() const;
   virtual QMimeData *mimeData(const QList<QTableWidgetItem *> &items) const;

   virtual bool dropMimeData(int row, int column, const QMimeData *data, Qt::DropAction action);
   virtual Qt::DropActions supportedDropActions() const;
   QList<QTableWidgetItem *> items(const QMimeData *data) const;

   QModelIndex indexFromItem(QTableWidgetItem *item) const;
   QTableWidgetItem *itemFromIndex(const QModelIndex &index) const;
   void dropEvent(QDropEvent *event) override;

 private:
   void setModel(QAbstractItemModel *model) override;

   Q_DECLARE_PRIVATE(QTableWidget)

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

inline void QTableWidget::removeCellWidget(int row, int column)
{
   setCellWidget(row, column, nullptr);
}

inline QTableWidgetItem *QTableWidget::itemAt(int x, int y) const
{
   return itemAt(QPoint(x, y));
}

inline int QTableWidgetItem::row() const
{
   return (view ? view->row(this) : -1);
}

inline int QTableWidgetItem::column() const
{
   return (view ? view->column(this) : -1);
}

inline void QTableWidgetItem::setSelected(bool select)
{
   if (view) {
      view->setItemSelected(this, select);
   }
}

inline bool QTableWidgetItem::isSelected() const
{
   return (view ? view->isItemSelected(this) : false);
}

#endif // QT_NO_TABLEWIDGET

#endif // QTABLEWIDGET_H
