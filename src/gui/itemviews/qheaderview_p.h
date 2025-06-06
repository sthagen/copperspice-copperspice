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

#ifndef QHEADERVIEW_P_H
#define QHEADERVIEW_P_H

#include <qabstractitemview_p.h>

#ifndef QT_NO_ITEMVIEWS

#include <qapplication.h>
#include <qbitarray.h>
#include <qlabel.h>

class QHeaderViewPrivate: public QAbstractItemViewPrivate
{
 public:
   enum StateVersion {
      VersionMarker = 0xff
   };

   enum State {
      NoState,
      ResizeSection,
      MoveSection,
      SelectSections,
      NoClear
   };

   QHeaderViewPrivate()
      : state(NoState), sortIndicatorOrder(Qt::DescendingOrder), sectionStartposRecalc(true),
        offset(0), sortIndicatorSection(0), lastPos(-1), firstPos(-1), originalSize(-1), m_headerViewSection(-1),
        target(-1), pressed(-1), hover(-1), length(0), resizeContentsPrecision(1000),
        sortIndicatorShown(false), preventCursorChangeInSetOffset(false),
        movableSections(false), clickableSections(false), highlightSelected(false),
        stretchLastSection(false), cascadingResizing(false), resizeRecursionBlock(false),
        allowUserMoveOfSection0(true), customDefaultSectionSize(false),
        stretchSections(0), contentsSections(0), minimumSectionSize(-1),
        maximumSectionSize(-1), lastSectionSize(0), sectionIndicatorOffset(0), sectionIndicator(nullptr),
        globalResizeMode(QHeaderView::Interactive)
   {
   }

   int lastVisibleVisualIndex() const;
   int sectionHandleAt(int position);
   void setupSectionIndicator(int section, int position);
   void updateSectionIndicator(int section, int position);
   void updateHiddenSections(int logicalFirst, int logicalLast);
   void resizeSections(QHeaderView::ResizeMode globalMode, bool useGlobalMode = false);
   void _q_sectionsRemoved(const QModelIndex &, int, int);
   void _q_layoutAboutToBeChanged();
   void _q_layoutChanged() override;

   bool isSectionSelected(int section) const;
   bool isFirstVisibleSection(int section) const;
   bool isLastVisibleSection(int section) const;

   bool rowIntersectsSelection(int row) const {
      return (selectionModel ? selectionModel->rowIntersectsSelection(row, root) : false);
   }

   bool columnIntersectsSelection(int column) const {
      return (selectionModel ? selectionModel->columnIntersectsSelection(column, root) : false);
   }

   bool sectionIntersectsSelection(int logical) const {
      return (orientation == Qt::Horizontal ? columnIntersectsSelection(logical) : rowIntersectsSelection(logical));
   }

   bool isRowSelected(int row) const {
      return (selectionModel ? selectionModel->isRowSelected(row, root) : false);
   }

   bool isColumnSelected(int column) const {
      return (selectionModel ? selectionModel->isColumnSelected(column, root) : false);
   }

   void prepareSectionSelected() {
      if (! selectionModel || !selectionModel->hasSelection()) {
         sectionSelected.clear();

      } else if (sectionSelected.count() != sectionCount() * 2) {
         sectionSelected.fill(false, sectionCount() * 2);

      } else {
         sectionSelected.fill(false);
      }
   }

   int sectionCount() const {
      return sectionItems.count();
   }

   bool reverse() const {
      return orientation == Qt::Horizontal && q_func()->isRightToLeft();
   }

   int logicalIndex(int visualIndex) const {
      return logicalIndices.isEmpty() ? visualIndex : logicalIndices.at(visualIndex);
   }

   int visualIndex(int logicalIndex) const {
      return visualIndices.isEmpty() ? logicalIndex : visualIndices.at(logicalIndex);
   }

   void setDefaultValues(Qt::Orientation o) {
      orientation = o;
      updateDefaultSectionSizeFromStyle();

      defaultAlignment = (o == Qt::Horizontal
            ? Qt::Alignment(Qt::AlignCenter) : Qt::AlignLeft | Qt::AlignVCenter);
   }

   bool isVisualIndexHidden(int visual) const {
      return sectionItems.at(visual).isHidden;
   }

   void setVisualIndexHidden(int visual, bool hidden) {
      sectionItems[visual].isHidden = hidden;
   }

   bool hasAutoResizeSections() const {
      return stretchSections || stretchLastSection || contentsSections;
   }

   QStyleOptionHeader getStyleOption() const;

   void invalidateCachedSizeHint() const {
      cachedSizeHint = QSize();
   }

   void initializeIndexMapping() const {
      if (visualIndices.count() != sectionCount() || logicalIndices.count() != sectionCount()) {
         visualIndices.resize(sectionCount());
         logicalIndices.resize(sectionCount());

         for (int s = 0; s < sectionCount(); ++s) {
            visualIndices[s] = s;
            logicalIndices[s] = s;
         }
      }
   }

   void clearCascadingSections() {
      firstCascadingSection = sectionItems.count();
      lastCascadingSection  = 0;
      cascadingSectionSize.clear();
   }

   void saveCascadingSectionSize(int visual, int size) {
      if (! cascadingSectionSize.contains(visual)) {
         cascadingSectionSize.insert(visual, size);
         firstCascadingSection = qMin(firstCascadingSection, visual);
         lastCascadingSection  = qMax(lastCascadingSection, visual);
      }
   }

   bool sectionIsCascadable(int visual) const {
      return headerSectionResizeMode(visual) == QHeaderView::Interactive;
   }

   int modelSectionCount() const {
      return (orientation == Qt::Horizontal
            ? model->columnCount(root) : model->rowCount(root));
   }

   bool modelIsEmpty() const {
      return (model->rowCount(root) == 0 || model->columnCount(root) == 0);
   }

   void doDelayedResizeSections() {
      if (!delayedResize.isActive()) {
         delayedResize.start(0, q_func());
      }
   }

   void executePostedResize() const {
      if (delayedResize.isActive() && state == NoState) {
         const_cast<QHeaderView *>(q_func())->resizeSections();
      }
   }

   void setAllowUserMoveOfSection0(bool b) {
      allowUserMoveOfSection0 = b;
   }

   void clear();
   void flipSortIndicator(int section);
   void cascadingResize(int visual, int newSize);



   // header section spans
   struct SectionItem {
      uint size       : 20;
      uint isHidden   : 1;
      uint resizeMode : 5;                    // (holding QHeaderView::ResizeMode)
      uint currentlyUnusedPadding : 6;

      union {                                 // used to save space and ensure good vector performance (on remove)
         mutable int calculated_startpos;     // primary member
         mutable int tmpLogIdx;               // When one of these 'tmp'-members has been used we call
         int tmpDataStreamSectionCount;       // recalcSectionStartPos() or set sectionStartposRecalc to true
      };                                      // to ensure that calculated_startpos will be calculated afterwards.

      SectionItem()
         : size(0), isHidden(0), resizeMode(QHeaderView::Interactive)
      { }

      SectionItem(int length, QHeaderView::ResizeMode mode)
         : size(length), isHidden(0), resizeMode(mode), calculated_startpos(-1)
      { }

      int sectionSize() const {
         return size;
      }

      int calculatedEndPos() const {
         return calculated_startpos + size;
      }

      void write(QDataStream &out) const {
         out << static_cast<int>(size);
         out << 1;
         out << (int)resizeMode;
      }

      void read(QDataStream &in) {
         int m;
         in >> m;
         size = m;
         in >> tmpDataStreamSectionCount;
         in >> m;
         resizeMode = m;
      }
   };

   QVector<SectionItem> sectionItems;

   void createSectionItems(int start, int end, int size, QHeaderView::ResizeMode mode);
   void removeSectionsFromSectionItems(int start, int end);
   void resizeSectionItem(int visualIndex, int oldSize, int newSize);
   void setDefaultSectionSize(int size);
   void updateDefaultSectionSizeFromStyle();
   void recalcSectionStartPos() const;           // not really const

   int headerLength() const {
      // for debugging
      int len = 0;

      for (int i = 0; i < sectionItems.count(); ++i) {
         len += sectionItems.at(i).size;
      }
      return len;
   }

   QBitArray sectionsHiddenToBitVector() const {
      QBitArray sectionHidden;

      if (! hiddenSectionSize.isEmpty()) {
         sectionHidden.resize(sectionItems.size());

         for (int u = 0; u < sectionItems.size(); ++u) {
            sectionHidden[u] = sectionItems.at(u).isHidden;
         }
      }
      return sectionHidden;
   }

   void setHiddenSectionsFromBitVector(const QBitArray &sectionHidden) {
      SectionItem *sectionData = sectionItems.data();
      for (int i = 0; i < sectionHidden.count(); ++i) {
         sectionData[i].isHidden = sectionHidden.at(i);
      }
   }

   int headerSectionSize(int visual) const;
   int headerSectionPosition(int visual) const;
   int headerVisualIndexAt(int position) const;

   // resize mode
   void setHeaderSectionResizeMode(int visual, QHeaderView::ResizeMode mode);
   QHeaderView::ResizeMode headerSectionResizeMode(int visual) const;
   void setGlobalHeaderResizeMode(QHeaderView::ResizeMode mode);

   // other
   int viewSectionSizeHint(int logical) const;
   int adjustedVisualIndex(int visualIndex) const;
   void setScrollOffset(const QScrollBar *scrollBar, QAbstractItemView::ScrollMode scrollMode);

   void write(QDataStream &out) const;
   bool read(QDataStream &in);

   State state;

   Qt::Orientation orientation;
   Qt::SortOrder sortIndicatorOrder;
   Qt::Alignment defaultAlignment;

   mutable QVector<int> visualIndices;           // visualIndex = visualIndices.at(logicalIndex)
   mutable QVector<int> logicalIndices;          // logicalIndex = row or column in the model
   mutable QBitArray sectionSelected;            // from logical index to bit

   mutable QHash<int, int> hiddenSectionSize;    // from logical index to section size
   mutable QHash<int, int> cascadingSectionSize; // from visual index to section size
   mutable QSize cachedSizeHint;
   mutable QBasicTimer delayedResize;

   mutable bool sectionStartposRecalc;

   int offset;
   int sortIndicatorSection;
   int firstCascadingSection;
   int lastCascadingSection;
   int lastPos;
   int firstPos;
   int originalSize;
   int m_headerViewSection;                 // used for resizing and moving sections
   int target;
   int pressed;
   int hover;
   int length;
   int resizeContentsPrecision;

   bool sortIndicatorShown;
   bool preventCursorChangeInSetOffset;
   bool movableSections;
   bool clickableSections;
   bool highlightSelected;
   bool stretchLastSection;
   bool cascadingResizing;
   bool resizeRecursionBlock;
   bool allowUserMoveOfSection0;            // false for QTreeView and true for QTableView
   bool customDefaultSectionSize;

   int stretchSections;
   int contentsSections;
   int defaultSectionSize;
   int minimumSectionSize;
   int maximumSectionSize;
   int lastSectionSize;
   int sectionIndicatorOffset;

   QLabel *sectionIndicator;
   QHeaderView::ResizeMode globalResizeMode;
   QList<QPersistentModelIndex> persistentHiddenSections;

 private:
   Q_DECLARE_PUBLIC(QHeaderView)
};

#endif // QT_NO_ITEMVIEWS

#endif // QHEADERVIEW_P_H
