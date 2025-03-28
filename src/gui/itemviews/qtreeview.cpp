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

#include <qtreeview.h>
#include <qtreeview_p.h>

#ifndef QT_NO_TREEVIEW

#include <qapplication.h>
#include <qdebug.h>
#include <qevent.h>
#include <qheaderview.h>
#include <qitemdelegate.h>
#include <qmetamethod.h>
#include <qpainter.h>
#include <qpen.h>
#include <qscrollbar.h>
#include <qstack.h>
#include <qstyle.h>
#include <qstyleoption.h>

#ifndef QT_NO_ACCESSIBILITY
#include <qaccessible.h>
#endif

#include <qheaderview_p.h>

#include <algorithm>

QTreeView::QTreeView(QWidget *parent)
   : QAbstractItemView(*new QTreeViewPrivate, parent)
{
   Q_D(QTreeView);
   d->initialize();
}

QTreeView::QTreeView(QTreeViewPrivate &dd, QWidget *parent)
   : QAbstractItemView(dd, parent)
{
   Q_D(QTreeView);
   d->initialize();
}

QTreeView::~QTreeView()
{
}

void QTreeView::setModel(QAbstractItemModel *model)
{
   Q_D(QTreeView);

   if (model == d->model) {
      return;
   }

   if (d->model && d->model != QAbstractItemModelPrivate::staticEmptyModel()) {
      disconnect(d->model, &QAbstractItemModel::rowsRemoved,         this, &QTreeView::rowsRemoved);
      disconnect(d->model, &QAbstractItemModel::modelAboutToBeReset, this, &QTreeView::_q_modelAboutToBeReset);
   }

   if (d->selectionModel) {
      // support row editing
      disconnect(d->selectionModel.data(), &QItemSelectionModel::currentRowChanged, d->model, &QAbstractItemModel::submit);

      disconnect(d->model, &QAbstractItemModel::rowsRemoved,         this, &QTreeView::rowsRemoved);
      disconnect(d->model, &QAbstractItemModel::modelAboutToBeReset, this, &QTreeView::_q_modelAboutToBeReset);
   }

   d->viewItems.clear();
   d->expandedIndexes.clear();
   d->hiddenIndexes.clear();
   d->m_header->setModel(model);

   QAbstractItemView::setModel(model);

   // QAbstractItemView connects to a private slot
   disconnect(d->model, &QAbstractItemModel::rowsRemoved, this, &QAbstractItemView::_q_rowsRemoved);

   // do header layout after the tree
   disconnect(d->model, &QAbstractItemModel::layoutChanged, d->m_header, &QHeaderView::_q_layoutChanged);

   // QTreeView has a public slot for this
   connect(d->model, &QAbstractItemModel::rowsRemoved,         this, &QTreeView::rowsRemoved);
   connect(d->model, &QAbstractItemModel::modelAboutToBeReset, this, &QTreeView::_q_modelAboutToBeReset);

   if (d->sortingEnabled) {
      d->_q_sortIndicatorChanged(header()->sortIndicatorSection(), header()->sortIndicatorOrder());
   }
}

void QTreeView::setRootIndex(const QModelIndex &index)
{
   Q_D(QTreeView);

   d->m_header->setRootIndex(index);
   QAbstractItemView::setRootIndex(index);
}

void QTreeView::setSelectionModel(QItemSelectionModel *selectionModel)
{
   Q_D(QTreeView);
   Q_ASSERT(selectionModel);

   if (d->selectionModel) {
      // support row editing
      disconnect(d->selectionModel.data(), &QItemSelectionModel::currentRowChanged, d->model, &QAbstractItemModel::submit);
   }

   d->m_header->setSelectionModel(selectionModel);
   QAbstractItemView::setSelectionModel(selectionModel);

   if (d->selectionModel) {
      // support row editing
      connect(d->selectionModel.data(), &QItemSelectionModel::currentRowChanged, d->model, &QAbstractItemModel::submit);
   }
}

QHeaderView *QTreeView::header() const
{
   Q_D(const QTreeView);
   return d->m_header;
}

void QTreeView::setHeader(QHeaderView *header)
{
   Q_D(QTreeView);

   if (header == d->m_header || ! header) {
      return;
   }

   if (d->m_header && d->m_header->parent() == this) {
      delete d->m_header;
   }

   d->m_header = header;
   d->m_header->setParent(this);
   d->m_header->d_func()->setAllowUserMoveOfSection0(false);

   if (! d->m_header->model()) {
      d->m_header->setModel(d->model);

      if (d->selectionModel) {
         d->m_header->setSelectionModel(d->selectionModel);
      }
   }

   connect(d->m_header, &QHeaderView::sectionResized,             this, &QTreeView::columnResized);
   connect(d->m_header, &QHeaderView::sectionMoved,               this, &QTreeView::columnMoved);
   connect(d->m_header, &QHeaderView::sectionCountChanged,        this, &QTreeView::columnCountChanged);
   connect(d->m_header, &QHeaderView::sectionHandleDoubleClicked, this, &QTreeView::resizeColumnToContents);
   connect(d->m_header, &QHeaderView::geometriesChanged,          this, &QTreeView::updateGeometries);

   setSortingEnabled(d->sortingEnabled);
   d->updateGeometry();
}

int QTreeView::autoExpandDelay() const
{
   Q_D(const QTreeView);
   return d->autoExpandDelay;
}

void QTreeView::setAutoExpandDelay(int delay)
{
   Q_D(QTreeView);
   d->autoExpandDelay = delay;
}

int QTreeView::indentation() const
{
   Q_D(const QTreeView);
   return d->indent;
}

void QTreeView::setIndentation(int i)
{
   Q_D(QTreeView);
   if (!d->customIndent || (i != d->indent)) {
      d->indent = i;
      d->customIndent = true;
      d->viewport->update();
   }
}

void QTreeView::resetIndentation()
{
   Q_D(QTreeView);
   if (d->customIndent) {
      d->updateIndentationFromStyle();
      d->customIndent = false;
   }
}
bool QTreeView::rootIsDecorated() const
{
   Q_D(const QTreeView);
   return d->rootDecoration;
}

void QTreeView::setRootIsDecorated(bool show)
{
   Q_D(QTreeView);
   if (show != d->rootDecoration) {
      d->rootDecoration = show;
      d->viewport->update();
   }
}

bool QTreeView::uniformRowHeights() const
{
   Q_D(const QTreeView);
   return d->uniformRowHeights;
}

void QTreeView::setUniformRowHeights(bool uniform)
{
   Q_D(QTreeView);
   d->uniformRowHeights = uniform;
}

bool QTreeView::itemsExpandable() const
{
   Q_D(const QTreeView);
   return d->itemsExpandable;
}

void QTreeView::setItemsExpandable(bool enable)
{
   Q_D(QTreeView);
   d->itemsExpandable = enable;
}

bool QTreeView::expandsOnDoubleClick() const
{
   Q_D(const QTreeView);
   return d->expandsOnDoubleClick;
}

void QTreeView::setExpandsOnDoubleClick(bool enable)
{
   Q_D(QTreeView);
   d->expandsOnDoubleClick = enable;
}

int QTreeView::columnViewportPosition(int column) const
{
   Q_D(const QTreeView);
   return d->m_header->sectionViewportPosition(column);
}

int QTreeView::columnWidth(int column) const
{
   Q_D(const QTreeView);
   return d->m_header->sectionSize(column);
}

void QTreeView::setColumnWidth(int column, int width)
{
   Q_D(QTreeView);
   d->m_header->resizeSection(column, width);
}

int QTreeView::columnAt(int x) const
{
   Q_D(const QTreeView);
   return d->m_header->logicalIndexAt(x);
}

bool QTreeView::isColumnHidden(int column) const
{
   Q_D(const QTreeView);
   return d->m_header->isSectionHidden(column);
}

void QTreeView::setColumnHidden(int column, bool hide)
{
   Q_D(QTreeView);

   if (column < 0 || column >= d->m_header->count()) {
      return;
   }

   d->m_header->setSectionHidden(column, hide);
}

bool QTreeView::isHeaderHidden() const
{
   Q_D(const QTreeView);
   return d->m_header->isHidden();
}

void QTreeView::setHeaderHidden(bool hide)
{
   Q_D(QTreeView);
   d->m_header->setHidden(hide);
}

bool QTreeView::isRowHidden(int row, const QModelIndex &parent) const
{
   Q_D(const QTreeView);
   if (!d->model) {
      return false;
   }
   return d->isRowHidden(d->model->index(row, 0, parent));
}

void QTreeView::setRowHidden(int row, const QModelIndex &parent, bool hide)
{
   Q_D(QTreeView);
   if (!d->model) {
      return;
   }
   QModelIndex index = d->model->index(row, 0, parent);
   if (!index.isValid()) {
      return;
   }

   if (hide) {
      d->hiddenIndexes.insert(index);
   } else if (d->isPersistent(index)) { //if the index is not persistent, it cannot be in the set
      d->hiddenIndexes.remove(index);
   }

   d->doDelayedItemsLayout();
}

bool QTreeView::isFirstColumnSpanned(int row, const QModelIndex &parent) const
{
   Q_D(const QTreeView);
   if (d->spanningIndexes.isEmpty() || !d->model) {
      return false;
   }
   QModelIndex index = d->model->index(row, 0, parent);
   for (int i = 0; i < d->spanningIndexes.count(); ++i)
      if (d->spanningIndexes.at(i) == index) {
         return true;
      }
   return false;
}

void QTreeView::setFirstColumnSpanned(int row, const QModelIndex &parent, bool span)
{
   Q_D(QTreeView);
   if (!d->model) {
      return;
   }
   QModelIndex index = d->model->index(row, 0, parent);
   if (!index.isValid()) {
      return;
   }

   if (span) {
      QPersistentModelIndex persistent(index);
      if (!d->spanningIndexes.contains(persistent)) {
         d->spanningIndexes.append(persistent);
      }
   } else {
      QPersistentModelIndex persistent(index);
      int i = d->spanningIndexes.indexOf(persistent);
      if (i >= 0) {
         d->spanningIndexes.remove(i);
      }
   }

   d->executePostedLayout();
   int i = d->viewIndex(index);
   if (i >= 0) {
      d->viewItems[i].spanning = span;
   }

   d->viewport->update();
}

void QTreeView::dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
   Q_D(QTreeView);

   // if we are going to do a complete relayout anyway, there is no need to update
   if (d->delayedPendingLayout) {
      return;
   }

   // refresh the height cache here; we do not really lose anything by getting the size hint,
   // since QAbstractItemView::dataChanged() will get the visualRect for the items anyway

   bool sizeChanged = false;
   int topViewIndex = d->viewIndex(topLeft);

   if (topViewIndex == 0) {
      int newDefaultItemHeight = indexRowSizeHint(topLeft);
      sizeChanged = d->defaultItemHeight != newDefaultItemHeight;
      d->defaultItemHeight = newDefaultItemHeight;
   }

   if (topViewIndex != -1) {
      if (topLeft.row() == bottomRight.row()) {
         int oldHeight = d->itemHeight(topViewIndex);

         d->invalidateHeightCache(topViewIndex);
         sizeChanged |= (oldHeight != d->itemHeight(topViewIndex));

         if (topLeft.column() == 0) {
            d->viewItems[topViewIndex].hasChildren = d->hasVisibleChildren(topLeft);
         }

      } else {
         int bottomViewIndex = d->viewIndex(bottomRight);

         for (int i = topViewIndex; i <= bottomViewIndex; ++i) {
            int oldHeight = d->itemHeight(i);
            d->invalidateHeightCache(i);
            sizeChanged |= (oldHeight != d->itemHeight(i));

            if (topLeft.column() == 0) {
               d->viewItems[i].hasChildren = d->hasVisibleChildren(d->viewItems.at(i).index);
            }
         }
      }
   }

   if (sizeChanged) {
      d->updateScrollBars();
      d->viewport->update();
   }

   QAbstractItemView::dataChanged(topLeft, bottomRight, roles);
}

void QTreeView::hideColumn(int column)
{
   Q_D(QTreeView);
   d->m_header->hideSection(column);
}

void QTreeView::showColumn(int column)
{
   Q_D(QTreeView);
   d->m_header->showSection(column);
}

void QTreeView::expand(const QModelIndex &index)
{
   Q_D(QTreeView);

   if (!d->isIndexValid(index)) {
      return;
   }

   if (index.flags() & Qt::ItemNeverHasChildren) {
      return;
   }

   if (d->isIndexExpanded(index)) {
      return;
   }

   if (d->delayedPendingLayout) {
      //A complete relayout is going to be performed, just store the expanded index, no need to layout.
      if (d->storeExpanded(index)) {
         emit expanded(index);
      }
      return;
   }

   int i = d->viewIndex(index);

   if (i != -1) {
      // is visible
      d->expand(i, true);

      if (!d->isAnimating()) {
         updateGeometries();
         d->viewport->update();
      }

   } else if (d->storeExpanded(index)) {
      emit expanded(index);

   }
}

void QTreeView::collapse(const QModelIndex &index)
{
   Q_D(QTreeView);
   if (!d->isIndexValid(index)) {
      return;
   }

   if (!d->isIndexExpanded(index)) {
      return;
   }

   //if the current item is now invisible, the autoscroll will expand the tree to see it, so disable the autoscroll
   d->delayedAutoScroll.stop();

   if (d->delayedPendingLayout) {
      //A complete relayout is going to be performed, just un-store the expanded index, no need to layout.
      if (d->isPersistent(index) && d->expandedIndexes.remove(index)) {
         emit collapsed(index);
      }
      return;
   }
   int i = d->viewIndex(index);
   if (i != -1) { // is visible
      d->collapse(i, true);
      if (!d->isAnimating()) {
         updateGeometries();
         viewport()->update();
      }
   } else {
      if (d->isPersistent(index) && d->expandedIndexes.remove(index)) {
         emit collapsed(index);
      }
   }
}

bool QTreeView::isExpanded(const QModelIndex &index) const
{
   Q_D(const QTreeView);
   return d->isIndexExpanded(index);
}

void QTreeView::setExpanded(const QModelIndex &index, bool expanded)
{
   if (expanded) {
      this->expand(index);
   } else {
      this->collapse(index);
   }
}

void QTreeView::setSortingEnabled(bool enable)
{
   Q_D(QTreeView);

   header()->setSortIndicatorShown(enable);
   header()->setSectionsClickable(enable);

   if (enable) {
      // sortByColumn has to be called before we connect or set the sortingEnabled flag
      // because otherwise it will not call sort on the model.
      sortByColumn(header()->sortIndicatorSection(), header()->sortIndicatorOrder());

      connect(header(),    &QHeaderView::sortIndicatorChanged, this, &QTreeView::_q_sortIndicatorChanged, Qt::UniqueConnection);

   } else {
      disconnect(header(), &QHeaderView::sortIndicatorChanged, this, &QTreeView::_q_sortIndicatorChanged);
   }

   d->sortingEnabled = enable;
}

bool QTreeView::isSortingEnabled() const
{
   Q_D(const QTreeView);
   return d->sortingEnabled;
}

void QTreeView::setAnimated(bool animate)
{
   Q_D(QTreeView);
   d->animationsEnabled = animate;
}

bool QTreeView::isAnimated() const
{
   Q_D(const QTreeView);
   return d->animationsEnabled;
}

void QTreeView::setAllColumnsShowFocus(bool enable)
{
   Q_D(QTreeView);
   if (d->allColumnsShowFocus == enable) {
      return;
   }
   d->allColumnsShowFocus = enable;
   d->viewport->update();
}

bool QTreeView::allColumnsShowFocus() const
{
   Q_D(const QTreeView);
   return d->allColumnsShowFocus;
}

void QTreeView::setWordWrap(bool on)
{
   Q_D(QTreeView);
   if (d->wrapItemText == on) {
      return;
   }
   d->wrapItemText = on;
   d->doDelayedItemsLayout();
}

bool QTreeView::wordWrap() const
{
   Q_D(const QTreeView);
   return d->wrapItemText;
}

void QTreeView::setTreePosition(int index)
{
   Q_D(QTreeView);
   d->treePosition = index;
   update();
}

int QTreeView::treePosition() const
{
    Q_D(const QTreeView);
    return d->treePosition;
}

void QTreeView::keyboardSearch(const QString &search)
{
   Q_D(QTreeView);

   if (! d->model->rowCount(d->root) || !d->model->columnCount(d->root)) {
      return;
   }

   QModelIndex start;
   if (currentIndex().isValid()) {
      start = currentIndex();
   } else {
      start = d->model->index(0, 0, d->root);
   }

   bool skipRow = false;
   bool keyboardTimeWasValid = d->keyboardInputTime.isValid();

   qint64 keyboardInputTimeElapsed = 0;

   if (keyboardTimeWasValid) {
      keyboardInputTimeElapsed = d->keyboardInputTime.restart();
   } else {
      d->keyboardInputTime.start();
   }

   if (search.isEmpty() || ! keyboardTimeWasValid
         || keyboardInputTimeElapsed > QApplication::keyboardInputInterval()) {
      d->keyboardInput = search;
      skipRow = currentIndex().isValid();    // if it is not valid, start at QModelIndex(0,0)

   } else {
      d->keyboardInput += search;
   }

   // special case for searches with same key like 'aaaaa'
   bool sameKey = false;
   if (d->keyboardInput.length() > 1) {
      int c = d->keyboardInput.count(d->keyboardInput.at(d->keyboardInput.length() - 1));
      sameKey = (c == d->keyboardInput.length());
      if (sameKey) {
         skipRow = true;
      }
   }

   // skip if we are searching for the same key or a new search started
   if (skipRow) {
      if (indexBelow(start).isValid()) {
         start = indexBelow(start);
      } else {
         start = d->model->index(0, start.column(), d->root);
      }
   }

   d->executePostedLayout();
   int startIndex = d->viewIndex(start);
   if (startIndex <= -1) {
      return;
   }

   int previousLevel = -1;
   int bestAbove = -1;
   int bestBelow = -1;
   QString searchString = sameKey ? QString(d->keyboardInput.at(0)) : d->keyboardInput;

   for (int i = 0; i < d->viewItems.count(); ++i) {
      if ((int)d->viewItems.at(i).level > previousLevel) {

         QModelIndex searchFrom = d->viewItems.at(i).index;

         if (start.column() > 0) {
            searchFrom = searchFrom.sibling(searchFrom.row(), start.column());
         }

         if (searchFrom.parent() == start.parent()) {
            searchFrom = start;
         }

         QModelIndexList match = d->model->match(searchFrom, Qt::DisplayRole, searchString);
         if (match.count()) {
            int hitIndex = d->viewIndex(match.at(0));
            if (hitIndex >= 0 && hitIndex < startIndex) {
               bestAbove = bestAbove == -1 ? hitIndex : qMin(hitIndex, bestAbove);
            } else if (hitIndex >= startIndex) {
               bestBelow = bestBelow == -1 ? hitIndex : qMin(hitIndex, bestBelow);
            }
         }
      }
      previousLevel = d->viewItems.at(i).level;
   }

   QModelIndex index;
   if (bestBelow > -1) {
      index = d->viewItems.at(bestBelow).index;
   } else if (bestAbove > -1) {
      index = d->viewItems.at(bestAbove).index;
   }

   if (start.column() > 0) {
      index = index.sibling(index.row(), start.column());
   }

   if (index.isValid()) {
      QItemSelectionModel::SelectionFlags flags = (d->selectionMode == SingleSelection
            ? QItemSelectionModel::SelectionFlags(
               QItemSelectionModel::ClearAndSelect
               | d->selectionBehaviorFlags())
            : QItemSelectionModel::SelectionFlags(
               QItemSelectionModel::NoUpdate));
      selectionModel()->setCurrentIndex(index, flags);
   }
}

QRect QTreeView::visualRect(const QModelIndex &index) const
{
   Q_D(const QTreeView);

   if (!d->isIndexValid(index) || isIndexHidden(index)) {
      return QRect();
   }

   d->executePostedLayout();

   int vi = d->viewIndex(index);
   if (vi < 0) {
      return QRect();
   }

   bool spanning = d->viewItems.at(vi).spanning;

   // if we have a spanning item, make the selection stretch from left to right
   int x = (spanning ? 0 : columnViewportPosition(index.column()));
   int w = (spanning ? d->m_header->length() : columnWidth(index.column()));

   // handle indentation
   if (d->isTreePosition(index.column())) {
      int i = d->indentationForItem(vi);
      w -= i;
      if (!isRightToLeft()) {
         x += i;
      }
   }

   int y = d->coordinateForItem(vi);
   int h = d->itemHeight(vi);

   return QRect(x, y, w, h);
}

void QTreeView::scrollTo(const QModelIndex &index, ScrollHint hint)
{
   Q_D(QTreeView);

   if (! d->isIndexValid(index)) {
      return;
   }

   d->executePostedLayout();
   d->updateScrollBars();

   // Expand all parents if the parent(s) of the node are not expanded.
   QModelIndex parent = index.parent();

   while (parent != d->root && parent.isValid() && state() == NoState && d->itemsExpandable) {
      if (!isExpanded(parent)) {
         expand(parent);
      }

      parent = d->model->parent(parent);
   }

   int item = d->viewIndex(index);
   if (item < 0) {
      return;
   }

   QRect area = d->viewport->rect();

   // vertical
   if (verticalScrollMode() == QAbstractItemView::ScrollPerItem) {
      int top = verticalScrollBar()->value();
      int bottom = top + verticalScrollBar()->pageStep();

      if (hint == EnsureVisible && item >= top && item < bottom) {
         // nothing to do

      } else if (hint == PositionAtTop || (hint == EnsureVisible && item < top)) {
         verticalScrollBar()->setValue(item);

      } else {
         // PositionAtBottom or PositionAtCenter
         const int currentItemHeight = d->itemHeight(item);

         int y = (hint == PositionAtCenter
               //we center on the current item with a preference to the top item (ie. -1)
               ? area.height() / 2 + currentItemHeight - 1
               //otherwise we simply take the whole space
               : area.height());

         if (y > currentItemHeight) {
            while (item >= 0) {
               y -= d->itemHeight(item);
               if (y < 0) { //there is no more space left
                  item++;
                  break;
               }
               item--;
            }
         }
         verticalScrollBar()->setValue(item);
      }

   } else {
      // ScrollPerPixel
      QRect rect(columnViewportPosition(index.column()),
         d->coordinateForItem(item), // ### slow for items outside the view
         columnWidth(index.column()),
         d->itemHeight(item));

      if (rect.isEmpty()) {
         // nothing to do

      } else if (hint == EnsureVisible && area.contains(rect)) {
         d->viewport->update(rect);
         // nothing to do

      } else {
         bool above = (hint == EnsureVisible && (rect.top() < area.top() || area.height() < rect.height()));
         bool below = (hint == EnsureVisible && rect.bottom() > area.bottom() && rect.height() < area.height());

         int verticalValue = verticalScrollBar()->value();
         if (hint == PositionAtTop || above) {
            verticalValue += rect.top();
         } else if (hint == PositionAtBottom || below) {
            verticalValue += rect.bottom() - area.height();
         } else if (hint == PositionAtCenter) {
            verticalValue += rect.top() - ((area.height() - rect.height()) / 2);
         }
         verticalScrollBar()->setValue(verticalValue);
      }
   }

   // horizontal
   int viewportWidth      = d->viewport->width();
   int horizontalOffset   = d->m_header->offset();
   int horizontalPosition = d->m_header->sectionPosition(index.column());
   int cellWidth = d->m_header->sectionSize(index.column());

   if (hint == PositionAtCenter) {
      horizontalScrollBar()->setValue(horizontalPosition - ((viewportWidth - cellWidth) / 2));

   } else {
      if (horizontalPosition - horizontalOffset < 0 || cellWidth > viewportWidth) {
         horizontalScrollBar()->setValue(horizontalPosition);
      } else if (horizontalPosition - horizontalOffset + cellWidth > viewportWidth) {
         horizontalScrollBar()->setValue(horizontalPosition - viewportWidth + cellWidth);
      }
   }
}

void QTreeView::timerEvent(QTimerEvent *event)
{
   Q_D(QTreeView);
   if (event->timerId() == d->columnResizeTimerID) {
      updateGeometries();
      killTimer(d->columnResizeTimerID);
      d->columnResizeTimerID = 0;
      QRect rect;
      int viewportHeight = d->viewport->height();
      int viewportWidth = d->viewport->width();
      for (int i = d->columnsToUpdate.size() - 1; i >= 0; --i) {
         int column = d->columnsToUpdate.at(i);
         int x = columnViewportPosition(column);
         if (isRightToLeft()) {
            rect |= QRect(0, 0, x + columnWidth(column), viewportHeight);
         } else {
            rect |= QRect(x, 0, viewportWidth - x, viewportHeight);
         }
      }
      d->viewport->update(rect.normalized());
      d->columnsToUpdate.clear();

   } else if (event->timerId() == d->openTimer.timerId()) {
      QPoint pos = d->viewport->mapFromGlobal(QCursor::pos());
      if (state() == QAbstractItemView::DraggingState
         && d->viewport->rect().contains(pos)) {
         QModelIndex index = indexAt(pos);
         setExpanded(index, !isExpanded(index));
      }
      d->openTimer.stop();
   }

   QAbstractItemView::timerEvent(event);
}

#ifndef QT_NO_DRAGANDDROP
void QTreeView::dragMoveEvent(QDragMoveEvent *event)
{
   Q_D(QTreeView);
   if (d->autoExpandDelay >= 0) {
      d->openTimer.start(d->autoExpandDelay, this);
   }
   QAbstractItemView::dragMoveEvent(event);
}
#endif

bool QTreeView::viewportEvent(QEvent *event)
{
   Q_D(QTreeView);

   switch (event->type()) {
      case QEvent::HoverEnter:
      case QEvent::HoverLeave:
      case QEvent::HoverMove: {
         QHoverEvent *he = static_cast<QHoverEvent *>(event);
         int oldBranch = d->hoverBranch;
         d->hoverBranch = d->itemDecorationAt(he->pos());

         QModelIndex newIndex = indexAt(he->pos());
         if (d->hover != newIndex || d->hoverBranch != oldBranch) {
            // Update the whole hovered over row. No need to update the old hovered
            // row, that is taken care in superclass hover handling.
            QRect rect = visualRect(newIndex);
            rect.setX(0);
            rect.setWidth(viewport()->width());
            viewport()->update(rect);
         }
         break;
      }

      default:
         break;
   }
   return QAbstractItemView::viewportEvent(event);
}

void QTreeView::paintEvent(QPaintEvent *event)
{
   Q_D(QTreeView);

   d->executePostedLayout();
   QPainter painter(viewport());

#ifndef QT_NO_ANIMATION
   if (d->isAnimating()) {
      drawTree(&painter, event->region() - d->animatedOperation.rect());
      d->drawAnimatedOperation(&painter);
   } else

#endif

   {
      drawTree(&painter, event->region());

#ifndef QT_NO_DRAGANDDROP
      d->paintDropIndicator(&painter);
#endif
   }
}

int QTreeViewPrivate::logicalIndexForTree() const
{
   int index = treePosition;
   if (index < 0) {
      index = m_header->logicalIndex(0);
   }
   return index;
}

void QTreeViewPrivate::paintAlternatingRowColors(QPainter *painter, QStyleOptionViewItem *option, int y, int bottom) const
{
   Q_Q(const QTreeView);

   if (! alternatingColors ||
      ! q->style()->styleHint(QStyle::SH_ItemView_PaintAlternatingRowColorsForEmptyArea, option, q)) {
      return;
   }

   int rowHeight = defaultItemHeight;

   if (rowHeight <= 0) {
      rowHeight = itemDelegate->sizeHint(*option, QModelIndex()).height();

      if (rowHeight <= 0) {
         return;
      }
   }

   while (y <= bottom) {
      option->rect.setRect(0, y, viewport->width(), rowHeight);

      if (m_current & 1) {
         option->features |= QStyleOptionViewItem::Alternate;
      } else {
         option->features &= ~QStyleOptionViewItem::Alternate;
      }

      ++m_current;

      q->style()->drawPrimitive(QStyle::PE_PanelItemViewRow, option, painter, q);
      y += rowHeight;
   }
}

bool QTreeViewPrivate::expandOrCollapseItemAtPos(const QPoint &pos)
{
   Q_Q(QTreeView);

   // we want to handle mousePress in EditingState (persistent editors)
   if ((state != QAbstractItemView::NoState && state != QAbstractItemView::EditingState)
      || !viewport->rect().contains(pos)) {
      return true;
   }

   int i = itemDecorationAt(pos);

   if ((i != -1) && itemsExpandable && hasVisibleChildren(viewItems.at(i).index)) {
      if (viewItems.at(i).expanded) {
         collapse(i, true);
      } else {
         expand(i, true);
      }

      if (!isAnimating()) {
         q->updateGeometries();
         viewport->update();
      }

      return true;
   }
   return false;
}

void QTreeViewPrivate::_q_modelDestroyed()
{
   // clear that list because it contais QModelIndex to
   // the model currently being destroyed
   viewItems.clear();
   QAbstractItemViewPrivate::_q_modelDestroyed();
}

QItemViewPaintPairs QTreeViewPrivate::draggablePaintPairs(const QModelIndexList &indexes, QRect *r) const
{
   Q_ASSERT(r);
   Q_Q(const QTreeView);
   if (spanningIndexes.isEmpty()) {
      return QAbstractItemViewPrivate::draggablePaintPairs(indexes, r);
   }

   QModelIndexList list;
   for (const QModelIndex &idx : indexes) {
      if (idx.column() > 0 && q->isFirstColumnSpanned(idx.row(), idx.parent())) {
         continue;
      }
      list << idx;
   }
   return QAbstractItemViewPrivate::draggablePaintPairs(list, r);
}

void QTreeViewPrivate::adjustViewOptionsForIndex(QStyleOptionViewItem *option, const QModelIndex &current) const
{
   const int row = viewIndex(current); // get the index in viewItems[]

   option->state = option->state | (viewItems.at(row).expanded ? QStyle::State_Open : QStyle::State_None)
      | (viewItems.at(row).hasChildren ? QStyle::State_Children : QStyle::State_None)
      | (viewItems.at(row).hasMoreSiblings ? QStyle::State_Sibling : QStyle::State_None);

   option->showDecorationSelected = (selectionBehavior & QTreeView::SelectRows) || option->showDecorationSelected;

   // index = visual index of visible columns only. data = logical index.
   QVector<int> logicalIndices;

   // vector of left/middle/end for each logicalIndex, visible columns only
   QVector<QStyleOptionViewItem::ViewItemPosition>  viewItemPosList;

   const bool spanning = viewItems.at(row).spanning;

   const int left  = (spanning ? m_header->visualIndex(0) : 0);
   const int right = (spanning ? m_header->visualIndex(0) : m_header->count() - 1 );
   calcLogicalIndices(&logicalIndices, &viewItemPosList, left, right);

   const int visualIndex = logicalIndices.indexOf(current.column());
   option->viewItemPosition = viewItemPosList.at(visualIndex);
}

void QTreeView::drawTree(QPainter *painter, const QRegion &region) const
{
   Q_D(const QTreeView);
   const QVector<QTreeViewItem> viewItems = d->viewItems;

   QStyleOptionViewItem option = d->viewOptions();
   const QStyle::State state = option.state;
   d->m_current = 0;

   if (viewItems.count() == 0 || d->m_header->count() == 0 || ! d->itemDelegate) {
      d->paintAlternatingRowColors(painter, &option, 0, region.boundingRect().bottom() + 1);
      return;
   }

   int firstVisibleItemOffset = 0;
   const int firstVisibleItem = d->firstVisibleItem(&firstVisibleItemOffset);

   if (firstVisibleItem < 0) {
      d->paintAlternatingRowColors(painter, &option, 0, region.boundingRect().bottom() + 1);
      return;
   }

   const int viewportWidth = d->viewport->width();

   QPoint hoverPos = d->viewport->mapFromGlobal(QCursor::pos());
   d->hoverBranch = d->itemDecorationAt(hoverPos);

   QVector<QRect> rects = region.rects();
   QVector<int> drawn;
   bool multipleRects = (rects.size() > 1);

   for (int a = 0; a < rects.size(); ++a) {

      const QRect area = (multipleRects
            ? QRect(0, rects.at(a).y(), viewportWidth, rects.at(a).height()) : rects.at(a));

      d->leftAndRight = d->startAndEndColumns(area);

      int i = firstVisibleItem;          // the first item at the top of the viewport
      int y = firstVisibleItemOffset;    // we may only see part of the first item

      // start at the top of the viewport  and iterate down to the update area
      for (; i < viewItems.count(); ++i) {
         const int itemHeight = d->itemHeight(i);

         if (y + itemHeight > area.top()) {
            break;
         }

         y += itemHeight;
      }

      // paint the visible rows
      for (; i < viewItems.count() && y <= area.bottom(); ++i) {
         const int itemHeight = d->itemHeight(i);
         option.rect.setRect(0, y, viewportWidth, itemHeight);

         option.state = state | (viewItems.at(i).expanded ? QStyle::State_Open : QStyle::State_None)
            | (viewItems.at(i).hasChildren ? QStyle::State_Children : QStyle::State_None)
            | (viewItems.at(i).hasMoreSiblings ? QStyle::State_Sibling : QStyle::State_None);

         d->m_current = i;
         d->m_spanning = viewItems.at(i).spanning;

         if (! multipleRects || ! drawn.contains(i)) {

            drawRow(painter, option, viewItems.at(i).index);

            if (multipleRects) {
               // even if the rect only intersects the item,the entire item will be painted
               drawn.append(i);
            }
         }

         y += itemHeight;
      }

      if (y <= area.bottom()) {
         d->m_current = i;
         d->paintAlternatingRowColors(painter, &option, y, area.bottom());
      }
   }
}

/// ### move to QObject
static inline bool ancestorOf(QObject *widget, QObject *other)
{
   for (QObject *parent = other; parent != nullptr; parent = parent->parent()) {
      if (parent == widget) {
         return true;
      }
   }
   return false;
}

void QTreeViewPrivate::calcLogicalIndices(QVector<int> *logicalIndices,
   QVector<QStyleOptionViewItem::ViewItemPosition> *itemPositions, int left, int right) const
{
   const int columnCount = m_header->count();

   /* 'left' and 'right' are the left-most and right-most visible visual indices.
      Compute the first visible logical indices before and after the left and right.
      We will use these values to determine the QStyleOptionViewItem::viewItemPosition. */

   int logicalIndexBeforeLeft = -1, logicalIndexAfterRight = -1;

   for (int visualIndex = left - 1; visualIndex >= 0; --visualIndex) {
      int logicalIndex = m_header->logicalIndex(visualIndex);

      if (! m_header->isSectionHidden(logicalIndex)) {
         logicalIndexBeforeLeft = logicalIndex;
         break;
      }
   }

   for (int visualIndex = left; visualIndex < columnCount; ++visualIndex) {
      int logicalIndex = m_header->logicalIndex(visualIndex);

      if (! m_header->isSectionHidden(logicalIndex)) {
         if (visualIndex > right) {
            logicalIndexAfterRight = logicalIndex;
            break;
         }
         logicalIndices->append(logicalIndex);
      }
   }

   itemPositions->resize(logicalIndices->count());
   for (int currentLogicalSection = 0; currentLogicalSection < logicalIndices->count(); ++currentLogicalSection) {
      const int headerSection = logicalIndices->at(currentLogicalSection);

      // determine the viewItemPosition depending on the position of column 0
      int nextLogicalSection = currentLogicalSection + 1 >= logicalIndices->count()
         ? logicalIndexAfterRight
         : logicalIndices->at(currentLogicalSection + 1);

      int prevLogicalSection = currentLogicalSection - 1 < 0
         ? logicalIndexBeforeLeft
         : logicalIndices->at(currentLogicalSection - 1);

      QStyleOptionViewItem::ViewItemPosition pos;

      if (columnCount == 1 || (nextLogicalSection == 0 && prevLogicalSection == -1)
            || (headerSection == 0 && nextLogicalSection == -1) || m_spanning) {
         pos = QStyleOptionViewItem::OnlyOne;

      } else if (isTreePosition(headerSection) || (nextLogicalSection != 0 && prevLogicalSection == -1)) {
         pos = QStyleOptionViewItem::Beginning;

      } else if (nextLogicalSection == 0 || nextLogicalSection == -1) {
         pos = QStyleOptionViewItem::End;

      } else {
         pos = QStyleOptionViewItem::Middle;
         (*itemPositions)[currentLogicalSection] = pos;
      }
   }
}

int QTreeViewPrivate::widthHintForIndex(const QModelIndex &index, int hint, const QStyleOptionViewItem &option, int i) const
{
   QWidget *editor = editorForIndex(index).widget.data();

   if (editor && persistent.contains(editor)) {
      hint = qMax(hint, editor->sizeHint().width());
      int min = editor->minimumSize().width();
      int max = editor->maximumSize().width();
      hint = qBound(min, hint, max);
   }

   int xhint = delegateForIndex(index)->sizeHint(option, index).width();
   hint = qMax(hint, xhint + (isTreePosition(index.column()) ? indentationForItem(i) : 0));

   return hint;
}

void QTreeView::drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
   Q_D(const QTreeView);

   QStyleOptionViewItem opt = option;

   const QPoint offset = d->scrollDelayOffset;
   const int y = option.rect.y() + offset.y();

   const QModelIndex parent  = index.parent();
   const QHeaderView *header = d->m_header;

   const QModelIndex current = currentIndex();
   const QModelIndex hover   = d->hover;

   const bool reverse = isRightToLeft();
   const QStyle::State state = opt.state;
   const bool spanning = d->m_spanning;

   const int left  = (spanning ? header->visualIndex(0) : d->leftAndRight.first);
   const int right = (spanning ? header->visualIndex(0) : d->leftAndRight.second);

   const bool alternate = d->alternatingColors;
   const bool enabled = (state & QStyle::State_Enabled) != 0;
   const bool allColumnsShowFocus = d->allColumnsShowFocus;

   // when the row contains an index widget which has focus,
   // we want to paint the entire row as active

   bool indexWidgetHasFocus = false;

   if ((current.row() == index.row()) && !d->editorIndexHash.isEmpty()) {
      const int r = index.row();
      QWidget *fw = QApplication::focusWidget();

      for (int c = 0; c < header->count(); ++c) {
         QModelIndex idx = d->model->index(r, c, parent);

         if (QWidget *editor = indexWidget(idx)) {
            if (ancestorOf(editor, fw)) {
               indexWidgetHasFocus = true;
               break;
            }
         }
      }
   }

   const bool widgetHasFocus = hasFocus();
   bool currentRowHasFocus   = false;

   if (allColumnsShowFocus && widgetHasFocus && current.isValid()) {
      // check if the focus index is before or after the visible columns
      const int r = index.row();

      for (int c = 0; c < left && ! currentRowHasFocus; ++c) {
         QModelIndex idx = d->model->index(r, c, parent);
         currentRowHasFocus = (idx == current);
      }

      QModelIndex newParent = d->model->parent(index);

      for (int c = right; c < header->count() && ! currentRowHasFocus; ++c) {
         currentRowHasFocus = (d->model->index(r, c, newParent) == current);
      }
   }

   // special case: treeviews with multiple columns draw
   // the selections differently than with only one column
   opt.showDecorationSelected = (d->selectionBehavior & SelectRows) || option.showDecorationSelected;

   int width, height = option.rect.height();
   int position;

   QModelIndex modelIndex;

   const bool hoverRow = selectionBehavior() == QAbstractItemView::SelectRows
      && index.parent() == hover.parent() && index.row() == hover.row();

   QVector<int> logicalIndices;
   QVector<QStyleOptionViewItem::ViewItemPosition> viewItemPosList;

   // vector of left/middle/end for each logicalIndex
   d->calcLogicalIndices(&logicalIndices, &viewItemPosList, left, right);

   for (int currentLogicalSection = 0; currentLogicalSection < logicalIndices.count(); ++currentLogicalSection) {
      int headerSection = logicalIndices.at(currentLogicalSection);
      position = columnViewportPosition(headerSection) + offset.x();
      width    = header->sectionSize(headerSection);

      if (spanning) {
         int lastSection = header->logicalIndex(header->count() - 1);

         if (! reverse) {
            width = columnViewportPosition(lastSection) + header->sectionSize(lastSection) - position;

         } else {
            width += position - columnViewportPosition(lastSection);
            position = columnViewportPosition(lastSection);
         }
      }

      modelIndex = d->model->index(index.row(), headerSection, parent);
      if (! modelIndex.isValid()) {
         continue;
      }

      opt.state = state;
      opt.viewItemPosition = viewItemPosList.at(currentLogicalSection);

      // fake activeness when row editor has focus
      if (indexWidgetHasFocus) {
         opt.state |= QStyle::State_Active;
      }

      if (d->selectionModel->isSelected(modelIndex)) {
         opt.state |= QStyle::State_Selected;
      }

      if (widgetHasFocus && (current == modelIndex)) {
         if (allColumnsShowFocus) {
            currentRowHasFocus = true;
         } else {
            opt.state |= QStyle::State_HasFocus;
         }
      }

      if ((hoverRow || modelIndex == hover) && (option.showDecorationSelected || (d->hoverBranch == -1))) {
         opt.state |= QStyle::State_MouseOver;
      } else {
         opt.state &= ~QStyle::State_MouseOver;
      }

      if (enabled) {
         QPalette::ColorGroup cg;

         if ((d->model->flags(modelIndex) & Qt::ItemIsEnabled) == 0) {
            opt.state &= ~QStyle::State_Enabled;
            cg = QPalette::Disabled;

         } else if (opt.state & QStyle::State_Active) {
            cg = QPalette::Active;

         } else {
            cg = QPalette::Inactive;
         }

         opt.palette.setCurrentColorGroup(cg);
      }

      if (alternate) {
         if (d->m_current & 1) {
            opt.features |= QStyleOptionViewItem::Alternate;
         } else {
            opt.features &= ~QStyleOptionViewItem::Alternate;
         }
      }

      /* background of the branch (in selected state and alternate row color was provided by the view.
         For backward compatibility this is now delegated to the style using PE_PanelViewItemRow which
         does the appropriate fill */

      if (d->isTreePosition(headerSection)) {
         const int i = d->indentationForItem(d->m_current);

         QRect branches(reverse ? position + width - i : position, y, i, height);
         const bool setClipRect = branches.width() > width;

         if (setClipRect) {
            painter->save();
            painter->setClipRect(QRect(position, y, width, height));
         }

         // draw background for the branch (selection + alternate row)
         opt.rect = branches;
         style()->drawPrimitive(QStyle::PE_PanelItemViewRow, &opt, painter, this);

         // draw background of the item (only alternate row). rest of the background
         // is provided by the delegate
         QStyle::State oldState = opt.state;
         opt.state &= ~QStyle::State_Selected;
         opt.rect.setRect(reverse ? position : i + position, y, width - i, height);
         style()->drawPrimitive(QStyle::PE_PanelItemViewRow, &opt, painter, this);
         opt.state = oldState;

         if (d->indent != 0) {
            drawBranches(painter, branches, index);
         }

         if (setClipRect) {
            painter->restore();
         }

      } else {
         QStyle::State oldState = opt.state;
         opt.state &= ~QStyle::State_Selected;
         opt.rect.setRect(position, y, width, height);

         style()->drawPrimitive(QStyle::PE_PanelItemViewRow, &opt, painter, this);
         opt.state = oldState;
      }

      d->delegateForIndex(modelIndex)->paint(painter, opt, modelIndex);
   }

   if (currentRowHasFocus) {
      QStyleOptionFocusRect o;
      o.QStyleOption::operator=(option);
      o.state |= QStyle::State_KeyboardFocusChange;

      QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled) ? QPalette::Normal : QPalette::Disabled;

      o.backgroundColor = option.palette.color(cg, d->selectionModel->isSelected(index)
            ? QPalette::Highlight : QPalette::Background);

      int x = 0;
      if (! option.showDecorationSelected) {
         x = header->sectionPosition(0) + d->indentationForItem(d->m_current);
      }

      QRect focusRect(x - header->offset(), y, header->length() - x, height);
      o.rect = style()->visualRect(layoutDirection(), d->viewport->rect(), focusRect);
      style()->drawPrimitive(QStyle::PE_FrameFocusRect, &o, painter);

      // if we show focus on all columns and the first section is moved,
      // we have to split the focus rect into two rects

      if (allColumnsShowFocus && !option.showDecorationSelected
            && header->sectionsMoved() && (header->visualIndex(0) != 0)) {
         QRect sectionRect(0, y, header->sectionPosition(0), height);
         o.rect = style()->visualRect(layoutDirection(), d->viewport->rect(), sectionRect);
         style()->drawPrimitive(QStyle::PE_FrameFocusRect, &o, painter);
      }
   }
}

void QTreeView::drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const
{
   Q_D(const QTreeView);

   const bool reverse = isRightToLeft();
   const int indent = d->indent;
   const int outer  = d->rootDecoration ? 0 : 1;
   const int item   = d->m_current;

   const QTreeViewItem &viewItem = d->viewItems.at(item);
   int level = viewItem.level;

   QRect primitive(reverse ? rect.left() : rect.right() + 1, rect.top(), indent, rect.height());

   QModelIndex parent   = index.parent();
   QModelIndex current  = parent;
   QModelIndex ancestor = current.parent();

   QStyleOptionViewItem opt = viewOptions();
   QStyle::State extraFlags = QStyle::State_None;

   if (isEnabled()) {
      extraFlags |= QStyle::State_Enabled;
   }
   if (window()->isActiveWindow()) {
      extraFlags |= QStyle::State_Active;
   }
   QPoint oldBO = painter->brushOrigin();
   if (verticalScrollMode() == QAbstractItemView::ScrollPerPixel) {
      painter->setBrushOrigin(QPoint(0, verticalOffset()));
   }

   if (d->alternatingColors) {
      if (d->m_current & 1) {
         opt.features |= QStyleOptionViewItem::Alternate;
      } else {
         opt.features &= ~QStyleOptionViewItem::Alternate;
      }
   }

   // When hovering over a row, pass State_Hover for painting the branch
   // indicators if it has the decoration (aka branch) selected.
   bool hoverRow = selectionBehavior() == QAbstractItemView::SelectRows && opt.showDecorationSelected
         && index.parent() == d->hover.parent() && index.row() == d->hover.row();

   if (d->selectionModel->isSelected(index)) {
      extraFlags |= QStyle::State_Selected;
   }

   if (level >= outer) {
      // start with the innermost branch
      primitive.moveLeft(reverse ? primitive.left() : primitive.left() - indent);
      opt.rect = primitive;

      const bool expanded = viewItem.expanded;
      const bool children = viewItem.hasChildren;
      bool moreSiblings = viewItem.hasMoreSiblings;

      opt.state = QStyle::State_Item | extraFlags
         | (moreSiblings ? QStyle::State_Sibling : QStyle::State_None)
         | (children ? QStyle::State_Children : QStyle::State_None)
         | (expanded ? QStyle::State_Open : QStyle::State_None);

      if (hoverRow || item == d->hoverBranch) {
         opt.state |= QStyle::State_MouseOver;

      } else {
         opt.state &= ~QStyle::State_MouseOver;
      }
      style()->drawPrimitive(QStyle::PE_IndicatorBranch, &opt, painter, this);
   }

   // then go out level by level
   for (--level; level >= outer; --level) {
      // we have already drawn the innermost branch
      primitive.moveLeft(reverse ? primitive.left() + indent : primitive.left() - indent);
      opt.rect = primitive;
      opt.state = extraFlags;
      bool moreSiblings = false;

      if (d->hiddenIndexes.isEmpty()) {
         moreSiblings = (d->model->rowCount(ancestor) - 1 > current.row());

      } else {
         int successor = item + viewItem.total + 1;

         while (successor < d->viewItems.size() && d->viewItems.at(successor).level >= uint(level)) {

            const QTreeViewItem &successorItem = d->viewItems.at(successor);
            if (successorItem.level == uint(level)) {
               moreSiblings = true;
               break;
            }
            successor += successorItem.total + 1;
         }
      }
      if (moreSiblings) {
         opt.state |= QStyle::State_Sibling;
      }

      if (hoverRow || item == d->hoverBranch) {
         opt.state |= QStyle::State_MouseOver;
      } else {
         opt.state &= ~QStyle::State_MouseOver;
      }

      style()->drawPrimitive(QStyle::PE_IndicatorBranch, &opt, painter, this);
      current = ancestor;
      ancestor = current.parent();
   }
   painter->setBrushOrigin(oldBO);
}

void QTreeView::mousePressEvent(QMouseEvent *event)
{
   Q_D(QTreeView);
   bool handled = false;

   if (style()->styleHint(QStyle::SH_ListViewExpand_SelectMouseType, nullptr, this) == QEvent::MouseButtonPress) {
      handled = d->expandOrCollapseItemAtPos(event->pos());
   }

   if (! handled && d->itemDecorationAt(event->pos()) == -1)  {
      QAbstractItemView::mousePressEvent(event);
   }
}

void QTreeView::mouseReleaseEvent(QMouseEvent *event)
{
   Q_D(QTreeView);

   if (d->itemDecorationAt(event->pos()) == -1) {
      QAbstractItemView::mouseReleaseEvent(event);

   } else {
      if (state() == QAbstractItemView::DragSelectingState) {
         setState(QAbstractItemView::NoState);
      }

      if (style()->styleHint(QStyle::SH_ListViewExpand_SelectMouseType, nullptr, this) == QEvent::MouseButtonRelease) {
         d->expandOrCollapseItemAtPos(event->pos());
      }
   }
}

void QTreeView::mouseDoubleClickEvent(QMouseEvent *event)
{
   Q_D(QTreeView);

   if (state() != NoState || !d->viewport->rect().contains(event->pos())) {
      return;
   }

   int i = d->itemDecorationAt(event->pos());

   if (i == -1) {
      i = d->itemAtCoordinate(event->y());

      if (i == -1) {
         return;   // user clicked outside the items
      }

      const QPersistentModelIndex firstColumnIndex = d->viewItems.at(i).index;
      const QPersistentModelIndex persistent = indexAt(event->pos());

      if (d->pressedIndex != persistent) {
         mousePressEvent(event);
         return;
      }

      // signal handlers may change the model
      emit doubleClicked(persistent);

      if (! persistent.isValid()) {
         return;
      }

      if (edit(persistent, DoubleClicked, event) || state() != NoState) {
         return;   // the double click triggered editing
      }

      if (! style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, nullptr, this)) {
         emit activated(persistent);
      }

      d->executePostedLayout(); // we need to make sure viewItems is updated

      if (d->itemsExpandable && d->expandsOnDoubleClick && d->hasVisibleChildren(persistent)) {

         if (! ((i < d->viewItems.count()) && (d->viewItems.at(i).index == firstColumnIndex))) {
            // find the new index of the item
            for (i = 0; i < d->viewItems.count(); ++i) {
               if (d->viewItems.at(i).index == firstColumnIndex) {
                  break;
               }
            }

            if (i == d->viewItems.count()) {
               return;
            }
         }

         if (d->viewItems.at(i).expanded) {
            d->collapse(i, true);

         } else {
            d->expand(i, true);
         }

         updateGeometries();
         viewport()->update();
      }
   }
}

void QTreeView::mouseMoveEvent(QMouseEvent *event)
{
   Q_D(QTreeView);

   // what about expanding/collapsing state ?
   if (d->itemDecorationAt(event->pos()) == -1) {
      QAbstractItemView::mouseMoveEvent(event);
   }
}

void QTreeView::keyPressEvent(QKeyEvent *event)
{
   Q_D(QTreeView);

   QModelIndex current = currentIndex();

   // this is the management of the expansion

   if (d->isIndexValid(current) && d->model && d->itemsExpandable) {
      switch (event->key()) {

         case Qt::Key_Asterisk: {
            QStack<QModelIndex> parents;
            parents.push(current);

            while (! parents.isEmpty()) {
               QModelIndex parent = parents.pop();

               for (int row = 0; row < d->model->rowCount(parent); ++row) {
                  QModelIndex child = d->model->index(row, 0, parent);
                  if (!d->isIndexValid(child)) {
                     break;
                  }

                  parents.push(child);
                  expand(child);
               }
            }

            expand(current);
            break;
         }

         case Qt::Key_Plus:
            expand(current);
            break;

         case Qt::Key_Minus:
            collapse(current);
            break;
      }
   }

   QAbstractItemView::keyPressEvent(event);
}

QModelIndex QTreeView::indexAt(const QPoint &point) const
{
   Q_D(const QTreeView);
   d->executePostedLayout();

   int visualIndex = d->itemAtCoordinate(point.y());
   QModelIndex idx = d->modelIndex(visualIndex);
   if (!idx.isValid()) {
      return QModelIndex();
   }

   if (d->viewItems.at(visualIndex).spanning) {
      return idx;
   }

   int column = d->columnAt(point.x());
   if (column == idx.column()) {
      return idx;
   }
   if (column < 0) {
      return QModelIndex();
   }
   return idx.sibling(idx.row(), column);
}

QModelIndex QTreeView::indexAbove(const QModelIndex &index) const
{
   Q_D(const QTreeView);

   if (!d->isIndexValid(index)) {
      return QModelIndex();
   }

   d->executePostedLayout();
   int i = d->viewIndex(index);
   if (--i < 0) {
      return QModelIndex();
   }

   const QModelIndex firstColumnIndex = d->viewItems.at(i).index;
   return firstColumnIndex.sibling(firstColumnIndex.row(), index.column());
}

QModelIndex QTreeView::indexBelow(const QModelIndex &index) const
{
   Q_D(const QTreeView);
   if (!d->isIndexValid(index)) {
      return QModelIndex();
   }

   d->executePostedLayout();
   int i = d->viewIndex(index);

   if (++i >= d->viewItems.count()) {
      return QModelIndex();
   }

   const QModelIndex firstColumnIndex = d->viewItems.at(i).index;

   return firstColumnIndex.sibling(firstColumnIndex.row(), index.column());
}

void QTreeView::doItemsLayout()
{
   Q_D(QTreeView);

   if (!d->customIndent) {
      // TODO: move to event()
      // QAbstractItemView calls this method in case of a style change,
      // so update the indentation here if it wasn't set manually.
      d->updateIndentationFromStyle();
   }

   if (d->hasRemovedItems) {
      //clean the QSet that may contains old (and this invalid) indexes
      d->hasRemovedItems = false;
      QSet<QPersistentModelIndex>::iterator it = d->expandedIndexes.begin();

      while (it != d->expandedIndexes.constEnd()) {
         if (! it->isValid()) {
            it = d->expandedIndexes.erase(it);

         } else {
            ++it;
         }
      }

      it = d->hiddenIndexes.begin();
      while (it != d->hiddenIndexes.constEnd()) {
         if (!it->isValid()) {
            it = d->hiddenIndexes.erase(it);
         } else {
            ++it;
         }
      }
   }

   // prepare for a new layout
   d->viewItems.clear();

   QModelIndex parent = d->root;

   if (d->model->hasChildren(parent)) {
      d->layout(-1);
   }

   QAbstractItemView::doItemsLayout();
   d->m_header->doItemsLayout();
}

void QTreeView::reset()
{
   Q_D(QTreeView);

   d->expandedIndexes.clear();
   d->hiddenIndexes.clear();
   d->spanningIndexes.clear();
   d->viewItems.clear();

   QAbstractItemView::reset();
}

int QTreeView::horizontalOffset() const
{
   Q_D(const QTreeView);
   return d->m_header->offset();
}

int QTreeView::verticalOffset() const
{
   Q_D(const QTreeView);

   if (d->verticalScrollMode == QAbstractItemView::ScrollPerItem) {
      if (d->uniformRowHeights) {
         return verticalScrollBar()->value() * d->defaultItemHeight;
      }

      // If we are scrolling per item and have non-uniform row heights,
      // finding the vertical offset in pixels is going to be relatively slow.

      // ### find a faster way to do this
      d->executePostedLayout();

      int offset = 0;

      for (int i = 0; i < d->viewItems.count(); ++i) {
         if (i == verticalScrollBar()->value()) {
            return offset;
         }
         offset += d->itemHeight(i);
      }

      return 0;
   }

   // scroll per pixel
   return verticalScrollBar()->value();
}

QModelIndex QTreeView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
   Q_D(QTreeView);
   (void) modifiers;

   d->executePostedLayout();

   QModelIndex current = currentIndex();
   if (! current.isValid()) {
      int i = d->below(-1);
      int c = 0;

      while (c < d->m_header->count() && d->m_header->isSectionHidden(d->m_header->logicalIndex(c)))  {
         ++c;
      }

      if (i < d->viewItems.count() && c < d->m_header->count()) {
         return d->modelIndex(i, d->m_header->logicalIndex(c));
      }

      return QModelIndex();
   }

   int vi = -1;

   if (vi < 0) {
      vi = qMax(0, d->viewIndex(current));
   }

   if (isRightToLeft()) {
      if (cursorAction == MoveRight) {
         cursorAction = MoveLeft;
      } else if (cursorAction == MoveLeft) {
         cursorAction = MoveRight;
      }
   }
   switch (cursorAction) {
      case MoveNext:
      case MoveDown:

#ifdef QT_KEYPAD_NAVIGATION
         if (vi == d->viewItems.count() - 1 && QApplication::keypadNavigationEnabled()) {
            return d->model->index(0, current.column(), d->root);
         }
#endif
         return d->modelIndex(d->below(vi), current.column());

      case MovePrevious:
      case MoveUp:

#ifdef QT_KEYPAD_NAVIGATION
         if (vi == 0 && QApplication::keypadNavigationEnabled()) {
            return d->modelIndex(d->viewItems.count() - 1, current.column());
         }
#endif
         return d->modelIndex(d->above(vi), current.column());

      case MoveLeft: {
         QScrollBar *sb = horizontalScrollBar();

         if (vi < d->viewItems.count() && d->viewItems.at(vi).expanded && d->itemsExpandable && sb->value() == sb->minimum()) {
            d->collapse(vi, true);
            d->moveCursorUpdatedView = true;

         } else {
            bool descend = style()->styleHint(QStyle::SH_ItemView_ArrowKeysNavigateIntoChildren, nullptr, this);

            if (descend) {
               QModelIndex par = current.parent();

               if (par.isValid() && par != rootIndex()) {
                  return par;
               } else {
                  descend = false;
               }
            }

            if (!descend) {
               if (d->selectionBehavior == SelectItems || d->selectionBehavior == SelectColumns) {
                  int visualColumn = d->m_header->visualIndex(current.column()) - 1;

                  while (visualColumn >= 0 && isColumnHidden(d->m_header->logicalIndex(visualColumn))) {
                     visualColumn--;
                  }

                  int newColumn = d->m_header->logicalIndex(visualColumn);
                  QModelIndex next = current.sibling(current.row(), newColumn);

                  if (next.isValid()) {
                     return next;
                  }
               }

               int oldValue = sb->value();
               sb->setValue(sb->value() - sb->singleStep());
               if (oldValue != sb->value()) {
                  d->moveCursorUpdatedView = true;
               }
            }

         }

         updateGeometries();
         viewport()->update();
         break;
      }

      case MoveRight:
         if (vi < d->viewItems.count() && !d->viewItems.at(vi).expanded && d->itemsExpandable
               && d->hasVisibleChildren(d->viewItems.at(vi).index)) {
            d->expand(vi, true);
            d->moveCursorUpdatedView = true;

         } else {
            bool descend = style()->styleHint(QStyle::SH_ItemView_ArrowKeysNavigateIntoChildren, nullptr, this);

            if (descend) {
               QModelIndex idx = d->modelIndex(d->below(vi));

               if (idx.parent() == current) {
                  return idx;
               } else {
                  descend = false;
               }
            }

            if (!descend) {
               if (d->selectionBehavior == SelectItems || d->selectionBehavior == SelectColumns) {
                  int visualColumn = d->m_header->visualIndex(current.column()) + 1;

                  while (visualColumn < d->model->columnCount(current.parent()) && isColumnHidden(d->m_header->logicalIndex(visualColumn))) {
                     visualColumn++;
                  }

                  const int newColumn    = d->m_header->logicalIndex(visualColumn);
                  const QModelIndex next = current.sibling(current.row(), newColumn);

                  if (next.isValid()) {
                     return next;
                  }
               }

               //last restort: we change the scrollbar value
               QScrollBar *sb = horizontalScrollBar();
               int oldValue = sb->value();
               sb->setValue(sb->value() + sb->singleStep());

               if (oldValue != sb->value()) {
                  d->moveCursorUpdatedView = true;
               }
            }
         }
         updateGeometries();
         viewport()->update();
         break;

      case MovePageUp:
         return d->modelIndex(d->pageUp(vi), current.column());

      case MovePageDown:
         return d->modelIndex(d->pageDown(vi), current.column());

      case MoveHome:
         return d->model->index(0, current.column(), d->root);

      case MoveEnd:
         return d->modelIndex(d->viewItems.count() - 1, current.column());
   }

   return current;
}

void QTreeView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command)
{
   Q_D(QTreeView);

   if (!selectionModel() || rect.isNull()) {
      return;
   }

   d->executePostedLayout();
   QPoint tl(isRightToLeft() ? qMax(rect.left(), rect.right())
      : qMin(rect.left(), rect.right()), qMin(rect.top(), rect.bottom()));

   QPoint br(isRightToLeft() ? qMin(rect.left(), rect.right()) :
      qMax(rect.left(), rect.right()), qMax(rect.top(), rect.bottom()));

   QModelIndex topLeft = indexAt(tl);
   QModelIndex bottomRight = indexAt(br);

   if (!topLeft.isValid() && !bottomRight.isValid()) {
      if (command & QItemSelectionModel::Clear) {
         selectionModel()->clear();
      }
      return;
   }

   if (!topLeft.isValid() && !d->viewItems.isEmpty()) {
      topLeft = d->viewItems.first().index;
   }

   if (! bottomRight.isValid() && ! d->viewItems.isEmpty()) {
      const int column = d->m_header->logicalIndex(d->m_header->count() - 1);
      const QModelIndex index = d->viewItems.last().index;
      bottomRight = index.sibling(index.row(), column);
   }

   if (!d ->isIndexEnabled(topLeft) || ! d->isIndexEnabled(bottomRight)) {
      return;
   }

   d->select(topLeft, bottomRight, command);
}

QRegion QTreeView::visualRegionForSelection(const QItemSelection &selection) const
{
   Q_D(const QTreeView);
   if (selection.isEmpty()) {
      return QRegion();
   }

   QRegion selectionRegion;
   const QRect &viewportRect = d->viewport->rect();
   for (int i = 0; i < selection.count(); ++i) {
      QItemSelectionRange range = selection.at(i);

      if (!range.isValid()) {
         continue;
      }

      QModelIndex parent = range.parent();
      QModelIndex leftIndex = range.topLeft();
      int columnCount = d->model->columnCount(parent);

      while (leftIndex.isValid() && isIndexHidden(leftIndex)) {
         if (leftIndex.column() + 1 < columnCount) {
            leftIndex = d->model->index(leftIndex.row(), leftIndex.column() + 1, parent);
         } else {
            leftIndex = QModelIndex();
         }
      }
      if (!leftIndex.isValid()) {
         continue;
      }

      const QRect leftRect = visualRect(leftIndex);
      int top = leftRect.top();
      QModelIndex rightIndex = range.bottomRight();

      while (rightIndex.isValid() && isIndexHidden(rightIndex)) {
         if (rightIndex.column() - 1 >= 0) {
            rightIndex = d->model->index(rightIndex.row(), rightIndex.column() - 1, parent);
         } else {
            rightIndex = QModelIndex();
         }
      }

      if (!rightIndex.isValid()) {
         continue;
      }

      const QRect rightRect = visualRect(rightIndex);
      int bottom = rightRect.bottom();

      if (top > bottom) {
         qSwap<int>(top, bottom);
      }

      int height = bottom - top + 1;

      if (d->m_header->sectionsMoved()) {
         for (int c = range.left(); c <= range.right(); ++c) {
            const QRect rangeRect(columnViewportPosition(c), top, columnWidth(c), height);

            if (viewportRect.intersects(rangeRect)) {
               selectionRegion += rangeRect;
            }
         }

      } else {
         QRect combined = leftRect | rightRect;
         combined.setX(columnViewportPosition(isRightToLeft() ? range.right() : range.left()));
         if (viewportRect.intersects(combined)) {
            selectionRegion += combined;
         }
      }
   }
   return selectionRegion;
}

QModelIndexList QTreeView::selectedIndexes() const
{
   QModelIndexList viewSelected;
   QModelIndexList modelSelected;

   if (selectionModel()) {
      modelSelected = selectionModel()->selectedIndexes();
   }

   for (int i = 0; i < modelSelected.count(); ++i) {
      // check that neither the parents nor the index is hidden before we add
      QModelIndex index = modelSelected.at(i);

      while (index.isValid() && !isIndexHidden(index)) {
         index = index.parent();
      }
      if (index.isValid()) {
         continue;
      }
      viewSelected.append(modelSelected.at(i));
   }

   return viewSelected;
}

void QTreeView::scrollContentsBy(int dx, int dy)
{
   Q_D(QTreeView);

   d->delayedAutoScroll.stop(); // auto scroll was canceled by the user scrolling

   dx = isRightToLeft() ? -dx : dx;
   if (dx) {
      int oldOffset = d->m_header->offset();

      d->m_header->d_func()->setScrollOffset(horizontalScrollBar(), horizontalScrollMode());

      if (horizontalScrollMode() == QAbstractItemView::ScrollPerItem) {
         int newOffset = d->m_header->offset();
         dx = isRightToLeft() ? newOffset - oldOffset : oldOffset - newOffset;
      }
   }

   const int itemHeight = d->defaultItemHeight <= 0 ? sizeHintForRow(0) : d->defaultItemHeight;
   if (d->viewItems.isEmpty() || itemHeight == 0) {
      return;
   }

   // guestimate the number of items in the viewport
   int viewCount = d->viewport->height() / itemHeight;
   int maxDeltaY = qMin(d->viewItems.count(), viewCount);

   // no need to do a lot of work if we are going to redraw the whole thing anyway
   if (qAbs(dy) > qAbs(maxDeltaY) && d->editorIndexHash.isEmpty()) {
      verticalScrollBar()->update();
      d->viewport->update();
      return;
   }

   if (dy && verticalScrollMode() == QAbstractItemView::ScrollPerItem) {
      int currentScrollbarValue = verticalScrollBar()->value();
      int previousScrollbarValue = currentScrollbarValue + dy; // -(-dy)
      int currentViewIndex = currentScrollbarValue; // the first visible item
      int previousViewIndex = previousScrollbarValue;
      const QVector<QTreeViewItem> viewItems = d->viewItems;
      dy = 0;
      if (previousViewIndex < currentViewIndex) { // scrolling down
         for (int i = previousViewIndex; i < currentViewIndex; ++i) {
            if (i < d->viewItems.count()) {
               dy -= d->itemHeight(i);
            }
         }
      } else if (previousViewIndex > currentViewIndex) { // scrolling up
         for (int i = previousViewIndex - 1; i >= currentViewIndex; --i) {
            if (i < d->viewItems.count()) {
               dy += d->itemHeight(i);
            }
         }
      }
   }

   d->scrollContentsBy(dx, dy);
}

void QTreeView::columnMoved()
{
   Q_D(QTreeView);
   updateEditorGeometries();
   d->viewport->update();
}

void QTreeView::reexpand()
{
   // do nothing
}

void QTreeView::rowsInserted(const QModelIndex &parent, int start, int end)
{
   Q_D(QTreeView);

   // if we are going to do a complete relayout anyway, there is no need to update
   if (d->delayedPendingLayout) {
      QAbstractItemView::rowsInserted(parent, start, end);
      return;
   }

   //don't add a hierarchy on a column != 0
   if (parent.column() != 0 && parent.isValid()) {
      QAbstractItemView::rowsInserted(parent, start, end);
      return;
   }

   const int parentRowCount = d->model->rowCount(parent);
   const int delta = end - start + 1;
   if (parent != d->root && !d->isIndexExpanded(parent) && parentRowCount > delta) {
      QAbstractItemView::rowsInserted(parent, start, end);
      return;
   }

   const int parentItem = d->viewIndex(parent);
   if (((parentItem != -1) && d->viewItems.at(parentItem).expanded)
      || (parent == d->root)) {
      d->doDelayedItemsLayout();
   } else if (parentItem != -1 && parentRowCount == delta) {
      // the parent just went from 0 children to more. update to re-paint the decoration
      d->viewItems[parentItem].hasChildren = true;
      viewport()->update();
   }
   QAbstractItemView::rowsInserted(parent, start, end);
}

void QTreeView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
   Q_D(QTreeView);
   QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
   d->viewItems.clear();
}

void QTreeView::rowsRemoved(const QModelIndex &parent, int start, int end)
{
   Q_D(QTreeView);

   d->viewItems.clear();
   d->doDelayedItemsLayout();
   d->hasRemovedItems = true;
   d->_q_rowsRemoved(parent, start, end);
}

void QTreeView::columnCountChanged(int oldCount, int newCount)
{
   Q_D(QTreeView);
   if (oldCount == 0 && newCount > 0) {
      //if the first column has just been added we need to relayout.
      d->doDelayedItemsLayout();
   }

   if (isVisible()) {
      updateGeometries();
   }
   viewport()->update();
}

void QTreeView::resizeColumnToContents(int column)
{
   Q_D(QTreeView);

   d->executePostedLayout();

   if (column < 0 || column >= d->m_header->count()) {
      return;
   }

   int contents = sizeHintForColumn(column);
   int header = d->m_header->isHidden() ? 0 : d->m_header->sectionSizeHint(column);
   d->m_header->resizeSection(column, qMax(contents, header));
}

void QTreeView::sortByColumn(int column)
{
   Q_D(QTreeView);
   sortByColumn(column, d->m_header->sortIndicatorOrder());
}

void QTreeView::sortByColumn(int column, Qt::SortOrder order)
{
   Q_D(QTreeView);

   // If sorting is enabled  will emit a signal connected to _q_sortIndicatorChanged, which then actually sorts
   d->m_header->setSortIndicator(column, order);

   // If sorting is not enabled, force to sort now
   if (! d->sortingEnabled) {
      d->model->sort(column, order);
   }
}

void QTreeView::selectAll()
{
   Q_D(QTreeView);

   if (! selectionModel()) {
      return;
   }

   SelectionMode mode = d->selectionMode;
   d->executePostedLayout(); //make sure we lay out the items

   if (mode != SingleSelection && mode != NoSelection && !d->viewItems.isEmpty()) {
      const QModelIndex &idx = d->viewItems.last().index;
      QModelIndex lastItemIndex = idx.sibling(idx.row(), d->model->columnCount(idx.parent()) - 1);

      d->select(d->viewItems.first().index, lastItemIndex,
            QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
   }
}

QSize QTreeView::viewportSizeHint() const
{
   Q_D(const QTreeView);
   d->executePostedLayout(); // Make sure that viewItems are up to date.

   if (d->viewItems.size() == 0) {
      return QAbstractItemView::viewportSizeHint();
   }

   // Get rect for last item
   const QRect deepestRect = visualRect(d->viewItems.last().index);

   if (!deepestRect.isValid()) {
      return QAbstractItemView::viewportSizeHint();
   }

   QSize result = QSize(d->m_header->length(), deepestRect.bottom() + 1);

   // add size for header
   result += QSize(0, d->m_header->isVisible() ? d->m_header->height() : 0);

   // add size for scrollbars
   result += QSize(verticalScrollBar()->isVisible() ? verticalScrollBar()->width() : 0,
         horizontalScrollBar()->isVisible() ? horizontalScrollBar()->height() : 0);

   return result;
}

void QTreeView::expandAll()
{
   Q_D(QTreeView);

   d->viewItems.clear();
   d->interruptDelayedItemsLayout();
   d->layout(-1, true);
   updateGeometries();
   d->viewport->update();
}

void QTreeView::collapseAll()
{
   Q_D(QTreeView);
   QSet<QPersistentModelIndex> old_expandedIndexes;
   old_expandedIndexes = d->expandedIndexes;
   d->expandedIndexes.clear();

   if (! signalsBlocked() && isSignalConnected(QMetaMethod::fromSignal(&QTreeView::collapsed))) {
      QSet<QPersistentModelIndex>::const_iterator i = old_expandedIndexes.constBegin();

      for (; i != old_expandedIndexes.constEnd(); ++i) {
         const QPersistentModelIndex &mi = (*i);

         if (mi.isValid() && !(mi.flags() & Qt::ItemNeverHasChildren)) {
            emit collapsed(mi);
         }
      }
   }
   doItemsLayout();
}

void QTreeView::expandToDepth(int depth)
{
   Q_D(QTreeView);

   d->viewItems.clear();
   QSet<QPersistentModelIndex> old_expandedIndexes;
   old_expandedIndexes = d->expandedIndexes;

   d->expandedIndexes.clear();
   d->interruptDelayedItemsLayout();
   d->layout(-1);

   for (int i = 0; i < d->viewItems.count(); ++i) {
      if (d->viewItems.at(i).level <= (uint)depth) {
         d->viewItems[i].expanded = true;
         d->layout(i);
         d->storeExpanded(d->viewItems.at(i).index);
      }
   }

   bool someSignalEnabled = isSignalConnected(QMetaMethod::fromSignal(&QTreeView::collapsed));
   someSignalEnabled |= isSignalConnected(QMetaMethod::fromSignal(&QTreeView::expanded));

   if (! signalsBlocked() && someSignalEnabled) {
      // emit signals
      QSet<QPersistentModelIndex> collapsedIndexes = old_expandedIndexes - d->expandedIndexes;
      QSet<QPersistentModelIndex>::const_iterator i = collapsedIndexes.constBegin();

      for (; i != collapsedIndexes.constEnd(); ++i) {
         const QPersistentModelIndex &mi = (*i);
         if (mi.isValid() && !(mi.flags() & Qt::ItemNeverHasChildren)) {
            emit collapsed(mi);
         }
      }

      QSet<QPersistentModelIndex> expandedIndexs = d->expandedIndexes - old_expandedIndexes;
      i = expandedIndexs.constBegin();
      for (; i != expandedIndexs.constEnd(); ++i) {
         const QPersistentModelIndex &mi = (*i);
         if (mi.isValid() && !(mi.flags() & Qt::ItemNeverHasChildren)) {
            emit expanded(mi);
         }
      }
   }

   updateGeometries();
   d->viewport->update();
}

void QTreeView::columnResized(int column, int, int)
{
   Q_D(QTreeView);

   d->columnsToUpdate.append(column);
   if (d->columnResizeTimerID == 0) {
      d->columnResizeTimerID = startTimer(0);
   }
}

void QTreeView::updateGeometries()
{
   Q_D(QTreeView);

   if (d->m_header) {
      if (d->geometryRecursionBlock) {
         return;
      }

      d->geometryRecursionBlock = true;
      int height = 0;

      if (! d->m_header->isHidden()) {
         height = qMax(d->m_header->minimumHeight(), d->m_header->sizeHint().height());
         height = qMin(height, d->m_header->maximumHeight());
      }

      setViewportMargins(0, height, 0, 0);
      QRect vg = d->viewport->geometry();
      QRect geometryRect(vg.left(), vg.top() - height, vg.width(), height);
      d->m_header->setGeometry(geometryRect);

      // d->header->setOffset(horizontalScrollBar()->value()); // ### bug ???
      QMetaObject::invokeMethod(d->m_header, "updateGeometries");
      d->updateScrollBars();
      d->geometryRecursionBlock = false;
   }

   QAbstractItemView::updateGeometries();
}

int QTreeView::sizeHintForColumn(int column) const
{
   Q_D(const QTreeView);

   d->executePostedLayout();
   if (d->viewItems.isEmpty()) {
      return -1;
   }

   ensurePolished();
   int w = 0;

   QStyleOptionViewItem option = d->viewOptions();
   const QVector<QTreeViewItem> viewItems = d->viewItems;

   const int maximumProcessRows = d->m_header->resizeContentsPrecision(); // To avoid this to take forever.

   int offset = 0;
   int start  = d->firstVisibleItem(&offset);
   int end    = d->lastVisibleItem(start, offset);

   if (start < 0 || end < 0 || end == viewItems.size() - 1) {
      end = viewItems.size() - 1;
      if (maximumProcessRows < 0) {
         start = 0;
      } else if (maximumProcessRows == 0) {
         start = qMax(0, end - 1);
         int remainingHeight = viewport()->height();
         while (start > 0 && remainingHeight > 0) {
            remainingHeight -= d->itemHeight(start);
            --start;
         }
      } else {
         start = qMax(0, end - maximumProcessRows);
      }
   }

   int rowsProcessed = 0;

   for (int i = start; i <= end; ++i) {
      if (viewItems.at(i).spanning) {
         continue;   // we have no good size hint
      }
      QModelIndex index = viewItems.at(i).index;
      index = index.sibling(index.row(), column);
      w = d->widthHintForIndex(index, w, option, i);
      ++rowsProcessed;
      if (rowsProcessed == maximumProcessRows) {
         break;
      }
   }

   --end;
   int actualBottom = viewItems.size() - 1;

   if (maximumProcessRows == 0) {
      rowsProcessed = 0;   // skip the while loop
   }

   while (rowsProcessed != maximumProcessRows && (start > 0 || end < actualBottom)) {
      int idx  = -1;

      if ((rowsProcessed % 2 && start > 0) || end == actualBottom) {
         while (start > 0) {
            --start;
            if (viewItems.at(start).spanning) {
               continue;
            }
            idx = start;
            break;
         }
      } else {
         while (end < actualBottom) {
            ++end;
            if (viewItems.at(end).spanning) {
               continue;
            }
            idx = end;
            break;
         }
      }
      if (idx < 0) {
         continue;
      }

      QModelIndex index = viewItems.at(idx).index;
      index = index.sibling(index.row(), column);
      w = d->widthHintForIndex(index, w, option, idx);
      ++rowsProcessed;
   }
   return w;
}

int QTreeView::indexRowSizeHint(const QModelIndex &index) const
{
   Q_D(const QTreeView);

   if (! d->isIndexValid(index) || ! d->itemDelegate) {
      return 0;
   }

   int start    = -1;
   int end      = -1;
   int indexRow = index.row();
   int count    = d->m_header->count();

   bool emptyHeader = (count == 0);
   QModelIndex parent = index.parent();

   if (count && isVisible()) {
      // If the sections have moved, we end up checking too many or too few
      start = d->m_header->visualIndexAt(0);

   } else {
      // If the header has not been laid out yet, we use the model directly
      count = d->model->columnCount(parent);
   }

   if (isRightToLeft()) {
      start = (start == -1 ? count - 1 : start);
      end = 0;

   } else {
      start = (start == -1 ? 0 : start);
      end = count - 1;
   }

   if (end < start) {
      qSwap(end, start);
   }

   int height = -1;
   QStyleOptionViewItem option = d->viewOptions();

   // If we want word wrapping in the items, we need to go through all the
   // columns and set the width of the column

   // Hack to speed up the function
   option.rect.setWidth(-1);


   for (int column = start; column <= end; ++column) {

      int logicalColumn = emptyHeader ? column : d->m_header->logicalIndex(column);
      if (d->m_header->isSectionHidden(logicalColumn)) {
         continue;
      }

      QModelIndex idx = d->model->index(indexRow, logicalColumn, parent);

      if (idx.isValid()) {
         QWidget *editor = d->editorForIndex(idx).widget.data();

         if (editor && d->persistent.contains(editor)) {
            height  = qMax(height, editor->sizeHint().height());
            int min = editor->minimumSize().height();
            int max = editor->maximumSize().height();
            height  = qBound(min, height, max);
         }

         int hint = d->delegateForIndex(idx)->sizeHint(option, idx).height();
         height   = qMax(height, hint);
      }
   }

   return height;
}

int QTreeView::rowHeight(const QModelIndex &index) const
{
   Q_D(const QTreeView);

   d->executePostedLayout();
   int i = d->viewIndex(index);

   if (i == -1) {
      return 0;
   }
   return d->itemHeight(i);
}

void QTreeView::horizontalScrollbarAction(int action)
{
   QAbstractItemView::horizontalScrollbarAction(action);
}

bool QTreeView::isIndexHidden(const QModelIndex &index) const
{
   return (isColumnHidden(index.column()) || isRowHidden(index.row(), index.parent()));
}

void QTreeViewPrivate::initialize()
{
   Q_Q(QTreeView);

   updateIndentationFromStyle();
   updateStyledFrameWidths();

   q->setSelectionBehavior(QAbstractItemView::SelectRows);
   q->setSelectionMode(QAbstractItemView::SingleSelection);
   q->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
   q->setAttribute(Qt::WA_MacShowFocusRect);

   QHeaderView *header = new QHeaderView(Qt::Horizontal, q);
   header->setSectionsMovable(true);
   header->setStretchLastSection(true);
   header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
   q->setHeader(header);

#ifndef QT_NO_ANIMATION
   animationsEnabled = q->style()->styleHint(QStyle::SH_Widget_Animate, nullptr, q);
   QObject::connect(&animatedOperation, &QVariantAnimation::finished, q, &QTreeView::_q_endAnimatedOperation);
#endif

}

void QTreeViewPrivate::expand(int item, bool emitSignal)
{
   Q_Q(QTreeView);

   if (item == -1 || viewItems.at(item).expanded) {
      return;
   }

   const QModelIndex index = viewItems.at(item).index;
   if (index.flags() & Qt::ItemNeverHasChildren) {
      return;
   }

#ifndef QT_NO_ANIMATION
   if (emitSignal && animationsEnabled) {
      prepareAnimatedOperation(item, QVariantAnimation::Forward);
   }
#endif

   // if already animating, stateBeforeAnimation is set to the correct value
   if (state != QAbstractItemView::AnimatingState) {
      stateBeforeAnimation = state;
   }

   q->setState(QAbstractItemView::ExpandingState);
   storeExpanded(index);
   viewItems[item].expanded = true;
   layout(item);
   q->setState(stateBeforeAnimation);

   if (model->canFetchMore(index)) {
      model->fetchMore(index);
   }

   if (emitSignal) {
      emit q->expanded(index);

#ifndef QT_NO_ANIMATION
      if (animationsEnabled) {
         beginAnimatedOperation();
      }
#endif

   }
}

void QTreeViewPrivate::insertViewItems(int pos, int count, const QTreeViewItem &viewItem)
{
   viewItems.insert(pos, count, viewItem);

   QTreeViewItem *items = viewItems.data();

   for (int i = pos + count; i < viewItems.count(); i++) {
      if (items[i].parentItem >= pos) {
         items[i].parentItem += count;
      }
   }
}

void QTreeViewPrivate::removeViewItems(int pos, int count)
{
   viewItems.remove(pos, count);

   QTreeViewItem *items = viewItems.data();

   for (int i = pos; i < viewItems.count(); i++) {
      if (items[i].parentItem >= pos) {
         items[i].parentItem -= count;
      }
   }
}

void QTreeViewPrivate::collapse(int item, bool emitSignal)
{
   Q_Q(QTreeView);

   if (item == -1 || expandedIndexes.isEmpty()) {
      return;
   }

   // if the current item is now invisible, the autoscroll will expand the tree to see it, so disable the autoscroll
   delayedAutoScroll.stop();

   int total = viewItems.at(item).total;
   const QModelIndex &modelIndex = viewItems.at(item).index;

   if (!isPersistent(modelIndex)) {
      return;   // if the index is not persistent, no chances it is expanded
   }

   QSet<QPersistentModelIndex>::iterator it = expandedIndexes.find(modelIndex);
   if (it == expandedIndexes.end() || viewItems.at(item).expanded == false) {
      return;   // nothing to do
   }

#ifndef QT_NO_ANIMATION
   if (emitSignal && animationsEnabled) {
      prepareAnimatedOperation(item, QVariantAnimation::Backward);
   }
#endif

   //if already animating, stateBeforeAnimation is set to the correct value
   if (state != QAbstractItemView::AnimatingState) {
      stateBeforeAnimation = state;
   }

   q->setState(QAbstractItemView::CollapsingState);
   expandedIndexes.erase(it);
   viewItems[item].expanded = false;
   int index = item;

   while (index > -1) {
      viewItems[index].total -= total;
      index = viewItems[index].parentItem;
   }

   removeViewItems(item + 1, total); // collapse
   q->setState(stateBeforeAnimation);

   if (emitSignal) {
      emit q->collapsed(modelIndex);

#ifndef QT_NO_ANIMATION
      if (animationsEnabled) {
         beginAnimatedOperation();
      }
#endif
   }
}

#ifndef QT_NO_ANIMATION
void QTreeViewPrivate::prepareAnimatedOperation(int item, QVariantAnimation::Direction direction)
{
   animatedOperation.item = item;
   animatedOperation.viewport = viewport;
   animatedOperation.setDirection(direction);

   int top = coordinateForItem(item) + itemHeight(item);
   QRect rect = viewport->rect();
   rect.setTop(top);

   if (direction == QVariantAnimation::Backward) {
      const int limit = rect.height() * 2;
      int h = 0;
      int c = item + viewItems.at(item).total + 1;

      for (int i = item + 1; i < c && h < limit; ++i) {
         h += itemHeight(i);
      }

      rect.setHeight(h);
      animatedOperation.setEndValue(top + h);
   }

   animatedOperation.setStartValue(top);
   animatedOperation.before = renderTreeToPixmapForAnimation(rect);
}

void QTreeViewPrivate::beginAnimatedOperation()
{
   Q_Q(QTreeView);

   QRect rect = viewport->rect();
   rect.setTop(animatedOperation.top());

   if (animatedOperation.direction() == QVariantAnimation::Forward) {
      const int limit = rect.height() * 2;
      int h = 0;
      int c = animatedOperation.item + viewItems.at(animatedOperation.item).total + 1;

      for (int i = animatedOperation.item + 1; i < c && h < limit; ++i) {
         h += itemHeight(i);
      }

      rect.setHeight(h);
      animatedOperation.setEndValue(animatedOperation.top() + h);
   }

   if (! rect.isEmpty()) {
      animatedOperation.after = renderTreeToPixmapForAnimation(rect);

      q->setState(QAbstractItemView::AnimatingState);
      animatedOperation.start(); //let's start the animation
   }
}

void QTreeViewPrivate::drawAnimatedOperation(QPainter *painter) const
{
   const int start   = animatedOperation.startValue().toInt(),
             end     = animatedOperation.endValue().toInt(),
             current = animatedOperation.currentValue().toInt();

   bool collapsing = animatedOperation.direction() == QVariantAnimation::Backward;
   const QPixmap top = collapsing ? animatedOperation.before : animatedOperation.after;
   painter->drawPixmap(0, start, top, 0, end - current - 1, top.width(), top.height());

   const QPixmap bottom = collapsing ? animatedOperation.after : animatedOperation.before;
   painter->drawPixmap(0, current, bottom);
}

QPixmap QTreeViewPrivate::renderTreeToPixmapForAnimation(const QRect &rect) const
{
   Q_Q(const QTreeView);
   QPixmap pixmap(rect.size() * q->devicePixelRatio());
   pixmap.setDevicePixelRatio(q->devicePixelRatio());

   if (rect.size().isEmpty()) {
      return pixmap;
   }

   pixmap.fill(Qt::transparent); //the base might not be opaque, and we don't want uninitialized pixels.

   QPainter painter(&pixmap);
   painter.fillRect(QRect(QPoint(0, 0), rect.size()), q->palette().base());
   painter.translate(0, -rect.top());

   q->drawTree(&painter, QRegion(rect));

   painter.end();

   // render the editors
   QStyleOptionViewItem option = viewOptions();

   for (QEditorIndexHash::const_iterator it = editorIndexHash.constBegin(); it != editorIndexHash.constEnd(); ++it) {
      QWidget *editor = it.key();
      const QModelIndex &index = it.value();
      option.rect = q->visualRect(index);

      if (option.rect.isValid()) {

         if (QAbstractItemDelegate *delegate = delegateForIndex(index)) {
            delegate->updateEditorGeometry(editor, option, index);
         }

         const QPoint pos = editor->pos();
         if (rect.contains(pos)) {
            editor->render(&pixmap, pos - rect.topLeft());
            //the animation uses pixmap to display the treeview's content
            //the editor is rendered on this pixmap and thus can (should) be hidden
            editor->hide();
         }
      }
   }

   return pixmap;
}

void QTreeViewPrivate::_q_endAnimatedOperation()
{
   Q_Q(QTreeView);

   q->setState(stateBeforeAnimation);
   q->updateGeometries();
   viewport->update();
}
#endif //QT_NO_ANIMATION

void QTreeViewPrivate::_q_modelAboutToBeReset()
{
   viewItems.clear();
}

void QTreeViewPrivate::_q_columnsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
   if (start <= 0 && 0 <= end) {
      viewItems.clear();
   }
   QAbstractItemViewPrivate::_q_columnsAboutToBeRemoved(parent, start, end);
}

void QTreeViewPrivate::_q_columnsRemoved(const QModelIndex &parent, int start, int end)
{
   if (start <= 0 && 0 <= end) {
      doDelayedItemsLayout();
   }
   QAbstractItemViewPrivate::_q_columnsRemoved(parent, start, end);
}

void QTreeViewPrivate::layout(int i, bool recursiveExpanding, bool afterIsUninitialized)
{
   Q_Q(QTreeView);

   QModelIndex current;
   QModelIndex parent = (i < 0) ? (QModelIndex)root : modelIndex(i);

   if (i >= 0 && ! parent.isValid()) {

      // modelIndex() should never return something invalid for the real items
      // can happen if columncount has been set to 0. To avoid infinite loop we stop here
      return;
   }

   int count = 0;
   if (model->hasChildren(parent)) {

      if (model->canFetchMore(parent)) {
         model->fetchMore(parent);
      }

      count = model->rowCount(parent);
   }

   bool expanding = true;
   if (i == -1) {
      if (uniformRowHeights) {
         QModelIndex index = model->index(0, 0, parent);
         defaultItemHeight = q->indexRowSizeHint(index);
      }

      viewItems.resize(count);
      afterIsUninitialized = true;

   } else if (viewItems[i].total != (uint)count) {
      if (!afterIsUninitialized) {
         insertViewItems(i + 1, count, QTreeViewItem());   // expand

      } else if (count > 0) {
         viewItems.resize(viewItems.count() + count);
      }

   } else {
      expanding = false;
   }

   int first    = i + 1;
   int level    = (i >= 0 ? viewItems.at(i).level + 1 : 0);
   int hidden   = 0;
   int last     = 0;
   int children = 0;

   QTreeViewItem *item = nullptr;

   for (int j = first; j < first + count; ++j) {
      current = model->index(j - first, 0, parent);

      if (isRowHidden(current)) {
         ++hidden;
         last = j - hidden + children;

      } else {
         last = j - hidden + children;
         if (item) {
            item->hasMoreSiblings = true;
         }

         item = &viewItems[last];
         item->index      = current;
         item->parentItem = i;
         item->level      = level;
         item->height     = 0;
         item->spanning   = q->isFirstColumnSpanned(current.row(), parent);
         item->expanded   = false;
         item->total      = 0;
         item->hasMoreSiblings = false;

         if ((recursiveExpanding && !(current.flags() & Qt::ItemNeverHasChildren)) || isIndexExpanded(current)) {

            if (recursiveExpanding && storeExpanded(current) && !q->signalsBlocked()) {
               emit q->expanded(current);
            }

            item->expanded = true;

            layout(last, recursiveExpanding, afterIsUninitialized);
            item = &viewItems[last];
            children += item->total;
            item->hasChildren = item->total > 0;
            last = j - hidden + children;

         } else {
            item->hasChildren = hasVisibleChildren(current);
         }
      }
   }

   // remove hidden items
   if (hidden > 0) {
      if (! afterIsUninitialized) {
         removeViewItems(last + 1, hidden);

      } else {
         viewItems.resize(viewItems.size() - hidden);
      }
   }

   if (! expanding) {
      return;   // nothing changed
   }

   while (i > -1) {
      viewItems[i].total += count - hidden;
      i = viewItems[i].parentItem;
   }
}

int QTreeViewPrivate::pageUp(int i) const
{
   int index = itemAtCoordinate(coordinateForItem(i) - viewport->height());
   while (isItemHiddenOrDisabled(index)) {
      index--;
   }
   return index == -1 ? 0 : index;
}

int QTreeViewPrivate::pageDown(int i) const
{
   int index = itemAtCoordinate(coordinateForItem(i) + viewport->height());
   while (isItemHiddenOrDisabled(index)) {
      index++;
   }
   return index == -1 ? viewItems.count() - 1 : index;
}

int QTreeViewPrivate::indentationForItem(int item) const
{
   if (item < 0 || item >= viewItems.count()) {
      return 0;
   }

   int level = viewItems.at(item).level;
   if (rootDecoration) {
      ++level;
   }

   return level * indent;
}

int QTreeViewPrivate::itemHeight(int item) const
{
   if (uniformRowHeights) {
      return defaultItemHeight;
   }

   if (viewItems.isEmpty()) {
      return 0;
   }

   const QModelIndex &index = viewItems.at(item).index;
   if (! index.isValid()) {
      return 0;
   }

   int height = viewItems.at(item).height;

   if (height <= 0) {
      height = q_func()->indexRowSizeHint(index);
      viewItems[item].height = height;
   }

   return qMax(height, 0);
}

int QTreeViewPrivate::coordinateForItem(int item) const
{
   if (verticalScrollMode == QAbstractItemView::ScrollPerPixel) {
      if (uniformRowHeights) {
         return (item * defaultItemHeight) - vbar->value();
      }

      // ### optimize (spans or caching)
      int y = 0;
      for (int i = 0; i < viewItems.count(); ++i) {
         if (i == item) {
            return y - vbar->value();
         }
         y += itemHeight(i);
      }

   } else { // ScrollPerItem
      int topViewItemIndex = vbar->value();
      if (uniformRowHeights) {
         return defaultItemHeight * (item - topViewItemIndex);
      }
      if (item >= topViewItemIndex) {
         // search in the visible area first and continue down
         // ### slow if the item is not visible
         int viewItemCoordinate = 0;
         int viewItemIndex = topViewItemIndex;

         while (viewItemIndex < viewItems.count()) {
            if (viewItemIndex == item) {
               return viewItemCoordinate;
            }
            viewItemCoordinate += itemHeight(viewItemIndex);
            ++viewItemIndex;
         }
         // below the last item in the view
         Q_ASSERT(false);
         return viewItemCoordinate;

      } else {
         // search the area above the viewport (used for editor widgets)
         int viewItemCoordinate = 0;
         for (int viewItemIndex = topViewItemIndex; viewItemIndex > 0; --viewItemIndex) {
            if (viewItemIndex == item) {
               return viewItemCoordinate;
            }
            viewItemCoordinate -= itemHeight(viewItemIndex - 1);
         }
         return viewItemCoordinate;
      }
   }
   return 0;
}

int QTreeViewPrivate::itemAtCoordinate(int coordinate) const
{
   const int itemCount = viewItems.count();
   if (itemCount == 0) {
      return -1;
   }

   if (uniformRowHeights && defaultItemHeight <= 0) {
      return -1;
   }

   if (verticalScrollMode == QAbstractItemView::ScrollPerPixel) {
      if (uniformRowHeights) {
         const int viewItemIndex = (coordinate + vbar->value()) / defaultItemHeight;
         return ((viewItemIndex >= itemCount || viewItemIndex < 0) ? -1 : viewItemIndex);
      }

      // ### optimize
      int viewItemCoordinate = 0;
      const int contentsCoordinate = coordinate + vbar->value();
      for (int viewItemIndex = 0; viewItemIndex < viewItems.count(); ++viewItemIndex) {
         viewItemCoordinate += itemHeight(viewItemIndex);
         if (viewItemCoordinate >= contentsCoordinate) {
            return (viewItemIndex >= itemCount ? -1 : viewItemIndex);
         }
      }

   } else { // ScrollPerItem
      int topViewItemIndex = vbar->value();
      if (uniformRowHeights) {
         if (coordinate < 0) {
            coordinate -= defaultItemHeight - 1;
         }
         const int viewItemIndex = topViewItemIndex + (coordinate / defaultItemHeight);
         return ((viewItemIndex >= itemCount || viewItemIndex < 0) ? -1 : viewItemIndex);
      }
      if (coordinate >= 0) {
         // the coordinate is in or below the viewport
         int viewItemCoordinate = 0;
         for (int viewItemIndex = topViewItemIndex; viewItemIndex < viewItems.count(); ++viewItemIndex) {
            viewItemCoordinate += itemHeight(viewItemIndex);
            if (viewItemCoordinate > coordinate) {
               return (viewItemIndex >= itemCount ? -1 : viewItemIndex);
            }
         }
      } else {
         // the coordinate is above the viewport
         int viewItemCoordinate = 0;
         for (int viewItemIndex = topViewItemIndex; viewItemIndex >= 0; --viewItemIndex) {
            if (viewItemCoordinate <= coordinate) {
               return (viewItemIndex >= itemCount ? -1 : viewItemIndex);
            }
            viewItemCoordinate -= itemHeight(viewItemIndex);
         }
      }
   }

   return -1;
}

int QTreeViewPrivate::viewIndex(const QModelIndex &tmpIndex) const
{
   if (! tmpIndex.isValid() || viewItems.isEmpty()) {
      return -1;
   }

   const int totalCount    = viewItems.count();
   const QModelIndex index = tmpIndex.sibling(tmpIndex.row(), 0);
   const int row           = index.row();

   const qint64 internalId = index.internalId();

   // start nearest to the lastViewedItem
   int localCount = qMin(lastViewedItem - 1, totalCount - lastViewedItem);
   for (int i = 0; i < localCount; ++i) {
      const QModelIndex &idx1 = viewItems.at(lastViewedItem + i).index;

      if (idx1.row() == row && idx1.internalId() == internalId) {
         lastViewedItem = lastViewedItem + i;
         return lastViewedItem;
      }

      const QModelIndex &idx2 = viewItems.at(lastViewedItem - i - 1).index;

      if (idx2.row() == row && idx2.internalId() == internalId) {
         lastViewedItem = lastViewedItem - i - 1;
         return lastViewedItem;
      }
   }

   for (int j = qMax(0, lastViewedItem + localCount); j < totalCount; ++j) {
      const QModelIndex &idx = viewItems.at(j).index;
      if (idx.row() == row && idx.internalId() == internalId) {
         lastViewedItem = j;
         return j;
      }
   }
   for (int j = qMin(totalCount, lastViewedItem - localCount) - 1; j >= 0; --j) {
      const QModelIndex &idx = viewItems.at(j).index;
      if (idx.row() == row && idx.internalId() == internalId) {
         lastViewedItem = j;
         return j;
      }
   }

   // nothing found
   return -1;
}

QModelIndex QTreeViewPrivate::modelIndex(int i, int column) const
{
   if (i < 0 || i >= viewItems.count()) {
      return QModelIndex();
   }

   QModelIndex ret = viewItems.at(i).index;
   if (column) {
      ret = ret.sibling(ret.row(), column);
   }
   return ret;
}

int QTreeViewPrivate::firstVisibleItem(int *offset) const
{
   const int value = vbar->value();
   if (verticalScrollMode == QAbstractItemView::ScrollPerItem) {
      if (offset) {
         *offset = 0;
      }
      return (value < 0 || value >= viewItems.count()) ? -1 : value;
   }
   // ScrollMode == ScrollPerPixel
   if (uniformRowHeights) {
      if (!defaultItemHeight) {
         return -1;
      }

      if (offset) {
         *offset = -(value % defaultItemHeight);
      }
      return value / defaultItemHeight;
   }

   int y = 0; // ### optimize (use spans ?)
   for (int i = 0; i < viewItems.count(); ++i) {
      y += itemHeight(i); // the height value is cached
      if (y > value) {
         if (offset) {
            *offset = y - value - itemHeight(i);
         }
         return i;
      }
   }
   return -1;
}

int QTreeViewPrivate::lastVisibleItem(int firstVisual, int offset) const
{
   if (firstVisual < 0 || offset < 0) {
      firstVisual = firstVisibleItem(&offset);
      if (firstVisual < 0) {
         return -1;
      }
   }
   int y = - offset;
   int value = viewport->height();

   for (int i = firstVisual; i < viewItems.count(); ++i) {
      y += itemHeight(i); // the height value is cached
      if (y > value) {
         return i;
      }
   }
   return viewItems.size() - 1;
}

int QTreeViewPrivate::columnAt(int x) const
{
   return m_header->logicalIndexAt(x);
}

void QTreeViewPrivate::updateScrollBars()
{
   Q_Q(QTreeView);

   QSize viewportSize = viewport->size();

   if (!viewportSize.isValid()) {
      viewportSize = QSize(0, 0);
   }

   executePostedLayout();

   if (viewItems.isEmpty()) {
      q->doItemsLayout();
   }

   int itemsInViewport = 0;
   if (uniformRowHeights) {
      if (defaultItemHeight <= 0) {
         itemsInViewport = viewItems.count();
      } else {
         itemsInViewport = viewportSize.height() / defaultItemHeight;
      }

   } else {
      const int itemsCount = viewItems.count();
      const int viewportHeight = viewportSize.height();
      for (int height = 0, item = itemsCount - 1; item >= 0; --item) {
         height += itemHeight(item);
         if (height > viewportHeight) {
            break;
         }
         ++itemsInViewport;
      }
   }

   if (verticalScrollMode == QAbstractItemView::ScrollPerItem) {
      if (!viewItems.isEmpty()) {
         itemsInViewport = qMax(1, itemsInViewport);
      }

      vbar->setRange(0, viewItems.count() - itemsInViewport);
      vbar->setPageStep(itemsInViewport);
      vbar->setSingleStep(1);

   } else {
      // scroll per pixel
      int contentsHeight = 0;

      if (uniformRowHeights) {
         contentsHeight = defaultItemHeight * viewItems.count();
      } else {
         // ### optimize (spans or caching)

         for (int i = 0; i < viewItems.count(); ++i) {
            contentsHeight += itemHeight(i);
         }
      }

      vbar->setRange(0, contentsHeight - viewportSize.height());
      vbar->setPageStep(viewportSize.height());
      vbar->setSingleStep(qMax(viewportSize.height() / (itemsInViewport + 1), 2));
   }

   const int columnCount   = m_header->count();
   const int viewportWidth = viewportSize.width();

   int columnsInViewport = 0;

   for (int width = 0, column = columnCount - 1; column >= 0; --column) {
      int logical = m_header->logicalIndex(column);
      width += m_header->sectionSize(logical);

      if (width > viewportWidth) {
         break;
      }

      ++columnsInViewport;
   }

   if (columnCount > 0) {
      columnsInViewport = qMax(1, columnsInViewport);
   }

   if (horizontalScrollMode == QAbstractItemView::ScrollPerItem) {
      hbar->setRange(0, columnCount - columnsInViewport);
      hbar->setPageStep(columnsInViewport);
      hbar->setSingleStep(1);

   } else {
      // scroll per pixel
      const int horizontalLength = m_header->length();
      const QSize maxSize = q->maximumViewportSize();

      if (maxSize.width() >= horizontalLength && vbar->maximum() <= 0) {
         viewportSize = maxSize;
      }

      hbar->setPageStep(viewportSize.width());
      hbar->setRange(0, qMax(horizontalLength - viewportSize.width(), 0));
      hbar->setSingleStep(qMax(viewportSize.width() / (columnsInViewport + 1), 2));
   }
}

int QTreeViewPrivate::itemDecorationAt(const QPoint &pos) const
{
   executePostedLayout();
   int x = pos.x();
   int column = m_header->logicalIndexAt(x);

   if (! isTreePosition(column)) {
      return -1;   // no logical index at x
   }

   int viewItemIndex = itemAtCoordinate(pos.y());
   QRect returning = itemDecorationRect(modelIndex(viewItemIndex));

   if (! returning.contains(pos)) {
      return -1;
   }

   return viewItemIndex;
}

QRect QTreeViewPrivate::itemDecorationRect(const QModelIndex &index) const
{
   Q_Q(const QTreeView);

   if (!rootDecoration && index.parent() == root) {
      return QRect();   // no decoration at root
   }

   int viewItemIndex = viewIndex(index);
   if (viewItemIndex < 0 || !hasVisibleChildren(viewItems.at(viewItemIndex).index)) {
      return QRect();
   }

   int itemIndentation = indentationForItem(viewItemIndex);
   int position = m_header->sectionViewportPosition(logicalIndexForTree());
   int size = m_header->sectionSize(logicalIndexForTree());

   QRect rect;
   if (q->isRightToLeft())
      rect = QRect(position + size - itemIndentation, coordinateForItem(viewItemIndex),
            indent, itemHeight(viewItemIndex));
   else
      rect = QRect(position + itemIndentation - indent, coordinateForItem(viewItemIndex),
            indent, itemHeight(viewItemIndex));

   QStyleOption opt;
   opt.initFrom(q);
   opt.rect = rect;
   return q->style()->subElementRect(QStyle::SE_TreeViewDisclosureItem, &opt, q);
}

QList<QPair<int, int>> QTreeViewPrivate::columnRanges(const QModelIndex &topIndex,
      const QModelIndex &bottomIndex) const
{
   const int topVisual    = m_header->visualIndex(topIndex.column());
   const int bottomVisual = m_header->visualIndex(bottomIndex.column());

   const int start = qMin(topVisual, bottomVisual);
   const int end   = qMax(topVisual, bottomVisual);

   QList<int> logicalIndexes;

   // iterate over the visual indexes to get the logical indexes
   for (int c = start; c <= end; c++) {
      const int logical = m_header->logicalIndex(c);

      if (! m_header->isSectionHidden(logical)) {
         logicalIndexes << logical;
      }
   }

   // sort the list
   std::sort(logicalIndexes.begin(), logicalIndexes.end());

   QList<QPair<int, int>> ret;
   QPair<int, int> current;
   current.first  = -2;          // -1 is not enough because -1+1 = 0
   current.second = -2;

   for (int i = 0; i < logicalIndexes.count(); ++i) {
      const int logicalColumn = logicalIndexes.at(i);

      if (current.second + 1 != logicalColumn) {
         if (current.first != -2) {
            //let's save the current one
            ret += current;
         }

         // start a new one
         current.first = current.second = logicalColumn;

      } else {
         current.second++;
      }
   }

   // get the last range
   if (current.first != -2) {
      ret += current;
   }

   return ret;
}

void QTreeViewPrivate::select(const QModelIndex &topIndex, const QModelIndex &bottomIndex,
   QItemSelectionModel::SelectionFlags command)
{
   Q_Q(QTreeView);

   QItemSelection selection;
   const int top    = viewIndex(topIndex);
   const int bottom = viewIndex(bottomIndex);

   const QList< QPair<int, int>> colRanges = columnRanges(topIndex, bottomIndex);
   QList< QPair<int, int>>::const_iterator it;

   for (it = colRanges.begin(); it != colRanges.end(); ++it) {
      const int left = (*it).first, right = (*it).second;

      QModelIndex previous;
      QItemSelectionRange currentRange;
      QStack<QItemSelectionRange> rangeStack;

      for (int i = top; i <= bottom; ++i) {
         QModelIndex index = modelIndex(i);
         QModelIndex parent = index.parent();
         QModelIndex previousParent = previous.parent();

         if (previous.isValid() && parent == previousParent) {
            // same parent

            if (qAbs(previous.row() - index.row()) > 1) {
               // a hole (hidden index inside a range) has been detected
               if (currentRange.isValid()) {
                  selection.append(currentRange);
               }

               // start a new range
               currentRange = QItemSelectionRange(index.sibling(index.row(), left), index.sibling(index.row(), right));

            } else {
               QModelIndex tl = model->index(currentRange.top(), currentRange.left(), currentRange.parent());

               currentRange = QItemSelectionRange(tl, index.sibling(index.row(), right));
            }

         } else if (previous.isValid() && parent == model->index(previous.row(), 0, previousParent)) {
            // item is child of previous
            rangeStack.push(currentRange);
            currentRange = QItemSelectionRange(index.sibling(index.row(), left), index.sibling(index.row(), right));

         } else {
            if (currentRange.isValid()) {
               selection.append(currentRange);
            }

            if (rangeStack.isEmpty()) {
               currentRange = QItemSelectionRange(index.sibling(index.row(), left), index.sibling(index.row(), right));

            } else {
               currentRange = rangeStack.pop();
               index = currentRange.bottomRight(); //let's resume the range
               --i;       // process again the current item
            }
         }
         previous = index;
      }

      if (currentRange.isValid()) {
         selection.append(currentRange);
      }

      for (int i = 0; i < rangeStack.count(); ++i) {
         selection.append(rangeStack.at(i));
      }
   }

   q->selectionModel()->select(selection, command);
}

QPair<int, int> QTreeViewPrivate::startAndEndColumns(const QRect &rect) const
{
   Q_Q(const QTreeView);

   int start = m_header->visualIndexAt(rect.left());
   int end   = m_header->visualIndexAt(rect.right());

   if (q->isRightToLeft()) {
      start = (start == -1 ? m_header->count() - 1 : start);
      end = (end == -1 ? 0 : end);

   } else {
      start = (start == -1 ? 0 : start);
      end = (end == -1 ? m_header->count() - 1 : end);
   }

   return qMakePair<int, int>(qMin(start, end), qMax(start, end));
}

bool QTreeViewPrivate::hasVisibleChildren(const QModelIndex &parent) const
{
   Q_Q(const QTreeView);

   if (parent.flags() & Qt::ItemNeverHasChildren) {
      return false;
   }

   if (model->hasChildren(parent)) {
      if (hiddenIndexes.isEmpty()) {
         return true;
      }

      if (q->isIndexHidden(parent)) {
         return false;
      }

      int rowCount = model->rowCount(parent);
      for (int i = 0; i < rowCount; ++i) {
         if (!q->isRowHidden(i, parent)) {
            return true;
         }
      }

      if (rowCount == 0) {
         return true;
      }
   }
   return false;
}

void QTreeViewPrivate::_q_sortIndicatorChanged(int column, Qt::SortOrder order)
{
   model->sort(column, order);
}

int QTreeViewPrivate::accessibleTree2Index(const QModelIndex &index) const
{
   Q_Q(const QTreeView);
   return (q->visualIndex(index) + (q->header() ? 1 : 0)) * index.model()->columnCount() + index.column();
}

void QTreeViewPrivate::updateIndentationFromStyle()
{
   Q_Q(const QTreeView);
   indent = q->style()->pixelMetric(QStyle::PM_TreeViewIndentation, nullptr, q);
}

void QTreeView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
   QAbstractItemView::currentChanged(current, previous);

   if (allColumnsShowFocus()) {
      if (previous.isValid()) {
         QRect previousRect = visualRect(previous);
         previousRect.setX(0);
         previousRect.setWidth(viewport()->width());
         viewport()->update(previousRect);
      }

      if (current.isValid()) {
         QRect currentRect = visualRect(current);
         currentRect.setX(0);
         currentRect.setWidth(viewport()->width());
         viewport()->update(currentRect);
      }
   }

#ifndef QT_NO_ACCESSIBILITY
   if (QAccessible::isActive() && current.isValid()) {
      Q_D(QTreeView);

      QAccessibleEvent event(this, QAccessible::Focus);
      event.setChild(d->accessibleTree2Index(current));
      QAccessible::updateAccessibility(&event);
   }

#endif
}

void QTreeView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
   QAbstractItemView::selectionChanged(selected, deselected);

#ifndef QT_NO_ACCESSIBILITY
   if (QAccessible::isActive()) {
      Q_D(QTreeView);

      // ### does not work properly for selection ranges.
      QModelIndex sel = selected.indexes().value(0);
      if (sel.isValid()) {
         int entry = d->accessibleTree2Index(sel);
         Q_ASSERT(entry >= 0);
         QAccessibleEvent event(this, QAccessible::SelectionAdd);
         event.setChild(entry);
         QAccessible::updateAccessibility(&event);
      }
      QModelIndex desel = deselected.indexes().value(0);
      if (desel.isValid()) {
         int entry = d->accessibleTree2Index(desel);
         Q_ASSERT(entry >= 0);
         QAccessibleEvent event(this, QAccessible::SelectionRemove);
         event.setChild(entry);
         QAccessible::updateAccessibility(&event);
      }
   }
#endif
}

int QTreeView::visualIndex(const QModelIndex &index) const
{
   Q_D(const QTreeView);
   d->executePostedLayout();
   return d->viewIndex(index);
}

#ifndef QT_NO_ANIMATION
void QTreeView::_q_endAnimatedOperation()
{
   Q_D(QTreeView);
   d->_q_endAnimatedOperation();
}
#endif

void QTreeView::_q_modelAboutToBeReset()
{
   Q_D(QTreeView);
   d->_q_modelAboutToBeReset();
}

void QTreeView::_q_sortIndicatorChanged(int column, Qt::SortOrder order)
{
   Q_D(QTreeView);
   d->_q_sortIndicatorChanged(column, order);
}

#endif // QT_NO_TREEVIEW
