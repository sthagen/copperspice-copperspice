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

#ifndef QTREEVIEW_P_H
#define QTREEVIEW_P_H

#include <qabstractitemmodel.h>
#include <qvariantanimation.h>

#include <qabstractitemview_p.h>

#ifndef QT_NO_TREEVIEW

struct QTreeViewItem {
   QTreeViewItem()
      : parentItem(-1), expanded(false), spanning(false), hasChildren(false),
        hasMoreSiblings(false), total(0), level(0), height(0)
   {
   }

   QModelIndex index; // we remove items whenever the indexes are invalidated

   int parentItem; // parent item index in viewItems
   uint expanded : 1;
   uint spanning : 1;
   uint hasChildren : 1; // if the item has visible children (even if collapsed)
   uint hasMoreSiblings : 1;
   uint total : 28; // total number of children visible
   uint level : 16; // indentation
   int height : 16; // row height
};

class Q_GUI_EXPORT QTreeViewPrivate : public QAbstractItemViewPrivate
{
 public:
   QTreeViewPrivate()
      : QAbstractItemViewPrivate(), indent(20), defaultItemHeight(-1), lastViewedItem(0),
        uniformRowHeights(false), rootDecoration(true), itemsExpandable(true), sortingEnabled(false),
        expandsOnDoubleClick(true),allColumnsShowFocus(false), customIndent(false), m_current(0), m_spanning(false),
        m_header(nullptr), animationsEnabled(false), columnResizeTimerID(0), autoExpandDelay(-1), hoverBranch(-1),
        geometryRecursionBlock(false), hasRemovedItems(false),treePosition(0)
   {
   }

   ~QTreeViewPrivate()
   {
   }

   void initialize();

   int logicalIndexForTree() const;
   bool isTreePosition(int logicalIndex) const {
      return logicalIndex == logicalIndexForTree();
   }

   QItemViewPaintPairs draggablePaintPairs(const QModelIndexList &indexes, QRect *r) const override;
   void adjustViewOptionsForIndex(QStyleOptionViewItem *option, const QModelIndex &current) const override;

#ifndef QT_NO_ANIMATION
   struct AnimatedOperation : public QVariantAnimation {
      int item;
      QPixmap before;
      QPixmap after;
      QWidget *viewport;

      AnimatedOperation() : item(0) {
         setEasingCurve(QEasingCurve::InOutQuad);
      }

      int top() const {
         return startValue().toInt();
      }

      QRect rect() const {
         QRect rect = viewport->rect();
         rect.moveTop(top());
         return rect;
      }

      void updateCurrentValue(const QVariant &) override {
         viewport->update(rect());
      }

      void updateState(State state, State) override {
         if (state == Stopped) {
            before = after = QPixmap();
         }
      }
   } animatedOperation;

   void prepareAnimatedOperation(int item, QVariantAnimation::Direction d);
   void beginAnimatedOperation();
   void drawAnimatedOperation(QPainter *painter) const;
   QPixmap renderTreeToPixmapForAnimation(const QRect &rect) const;
   void _q_endAnimatedOperation();
#endif

   void expand(int item, bool emitSignal);
   void collapse(int item, bool emitSignal);

   void _q_columnsAboutToBeRemoved(const QModelIndex &, int, int) override;
   void _q_columnsRemoved(const QModelIndex &, int, int) override;
   void _q_modelAboutToBeReset();
   void _q_sortIndicatorChanged(int column, Qt::SortOrder order);
   void _q_modelDestroyed() override;

   void layout(int item, bool recusiveExpanding = false, bool afterIsUninitialized = false);

   int pageUp(int item) const;
   int pageDown(int item) const;

   int itemHeight(int item) const;
   int indentationForItem(int item) const;
   int coordinateForItem(int item) const;
   int itemAtCoordinate(int coordinate) const;

   int viewIndex(const QModelIndex &index) const;
   QModelIndex modelIndex(int i, int column = 0) const;

   void insertViewItems(int pos, int count, const QTreeViewItem &viewItem);
   void removeViewItems(int pos, int count);

#if 0
   bool checkViewItems() const;
#endif

   int firstVisibleItem(int *offset = nullptr) const;
   int lastVisibleItem(int firstVisual = -1, int offset = -1) const;
   int columnAt(int x) const;
   bool hasVisibleChildren( const QModelIndex &parent) const;

   bool expandOrCollapseItemAtPos(const QPoint &pos);

   void updateScrollBars();

   int itemDecorationAt(const QPoint &pos) const;
   QRect itemDecorationRect(const QModelIndex &index) const;

   QList<QPair<int, int>> columnRanges(const QModelIndex &topIndex, const QModelIndex &bottomIndex) const;
   void select(const QModelIndex &start, const QModelIndex &stop, QItemSelectionModel::SelectionFlags command);

   QPair<int, int> startAndEndColumns(const QRect &rect) const;

   void updateChildCount(const int parentItem, const int delta);

   void paintAlternatingRowColors(QPainter *painter, QStyleOptionViewItem *option, int y, int bottom) const;

   // logicalIndices: vector of currently visibly logical indices
   // itemPositions: vector of view item positions (beginning/middle/end/onlyone)
   void calcLogicalIndices(QVector<int> *logicalIndices, QVector<QStyleOptionViewItem::ViewItemPosition> *itemPositions,
         int left, int right) const;

   int widthHintForIndex(const QModelIndex &index, int hint, const QStyleOptionViewItem &option, int i) const;

   bool storeExpanded(const QPersistentModelIndex &idx) {
      if (expandedIndexes.contains(idx)) {
         return false;
      }

      expandedIndexes.insert(idx);
      return true;
   }

   bool isIndexExpanded(const QModelIndex &idx) const {
      // first check if the idx is a QPersistentModelIndex, because creating QPersistentModelIndex is slow
      return !(idx.flags() & Qt::ItemNeverHasChildren) && isPersistent(idx) && expandedIndexes.contains(idx);
   }

   bool isRowHidden(const QModelIndex &idx) const {
      if (hiddenIndexes.isEmpty()) {
         return false;
      }
      // first check if the idx is a QPersistentModelIndex, because creating QPersistentModelIndex is slow
      return isPersistent(idx) && hiddenIndexes.contains(idx);
   }

   bool isItemHiddenOrDisabled(int i) const {
      if (i < 0 || i >= viewItems.count()) {
         return false;
      }
      const QModelIndex index = viewItems.at(i).index;
      return isRowHidden(index) || !isIndexEnabled(index);
   }

   int above(int item) const {
      int i = item;
      while (isItemHiddenOrDisabled(--item)) {} return item < 0 ? i : item;
   }

   int below(int item) const {
      int i = item;
      while (isItemHiddenOrDisabled(++item)) {} return item >= viewItems.count() ? i : item;
   }

   void invalidateHeightCache(int item) const {
      viewItems[item].height = 0;
   }

   int accessibleTable2Index(const QModelIndex &index) const {
      return (viewIndex(index) + (m_header ? 1 : 0)) * model->columnCount() + index.column() + 1;
   }

   int indent;
   int defaultItemHeight;     // just a number, contentsHeight() / numItems

   mutable QVector<QTreeViewItem> viewItems;
   mutable int lastViewedItem;

   bool uniformRowHeights;    // used when all rows have the same height
   bool rootDecoration;
   bool itemsExpandable;
   bool sortingEnabled;
   bool expandsOnDoubleClick;
   bool allColumnsShowFocus;
   bool customIndent;

   int accessibleTree2Index(const QModelIndex &index) const;
   void updateIndentationFromStyle();

   // used for drawing
   mutable QPair<int, int> leftAndRight;
   mutable int m_current;
   mutable bool m_spanning;

   QHeaderView *m_header;

   // used when expanding and collapsing items
   QSet<QPersistentModelIndex> expandedIndexes;
   bool animationsEnabled;

   // used when hiding and showing items
   QSet<QPersistentModelIndex> hiddenIndexes;

   // used for spanning rows
   QVector<QPersistentModelIndex> spanningIndexes;

   // used for updating resized columns
   int columnResizeTimerID;
   QList<int> columnsToUpdate;

   // used for the automatic opening of nodes during DND
   int autoExpandDelay;
   QBasicTimer openTimer;

   // used for drawing hilighted expand/collapse indicators
   mutable int hoverBranch;

   // used for blocking recursion when calling setViewportMargins from updateGeometries
   bool geometryRecursionBlock;

   // If we should clean the set
   bool hasRemovedItems;
   int treePosition;

 private:
   Q_DECLARE_PUBLIC(QTreeView)
};

#endif // QT_NO_TREEVIEW

#endif // QTREEVIEW_P_H
