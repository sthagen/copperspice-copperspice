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

#include <qdirmodel.h>

#ifndef QT_NO_DIRMODEL

#include <qapplication.h>
#include <qdatetime.h>
#include <qdebug.h>
#include <qfile.h>
#include <qfilesystemmodel.h>
#include <qlocale.h>
#include <qmimedata.h>
#include <qobject.h>
#include <qpair.h>
#include <qstack.h>
#include <qstyle.h>
#include <qurl.h>
#include <qvector.h>

#include <qabstractitemmodel_p.h>

class QDirModelPrivate : public QAbstractItemModelPrivate
{
   Q_DECLARE_PUBLIC(QDirModel)

 public:
   struct QDirNode {
      QDirNode()
         : parent(nullptr), populated(false), stat(false)
      {
      }

      QDirNode *parent;
      QFileInfo info;
      QIcon icon; // cache the icon
      mutable QVector<QDirNode> children;
      mutable bool populated; // have we read the children
      mutable bool stat;
   };

   QDirModelPrivate()
      : resolveSymlinks(true), readOnly(true), lazyChildCount(false), allowAppendChild(true),
        iconProvider(&defaultProvider), shouldStat(true)
   { }

   void init();
   QDirNode *node(int row, QDirNode *parent) const;
   QVector<QDirNode> children(QDirNode *parent, bool stat) const;

   void _q_refresh();

   void savePersistentIndexes();
   void restorePersistentIndexes();

   QFileInfoList entryInfoList(const QString &path) const;
   QStringList entryList(const QString &path) const;

   QString name(const QModelIndex &index) const;
   QString size(const QModelIndex &index) const;
   QString type(const QModelIndex &index) const;
   QString time(const QModelIndex &index) const;

   void appendChild(QDirModelPrivate::QDirNode *parent, const QString &path) const;
   static QFileInfo resolvedInfo(QFileInfo info);

   inline QDirNode *node(const QModelIndex &index) const;
   inline void populate(QDirNode *parent) const;
   inline void clear(QDirNode *parent) const;

   void invalidate();

   mutable QDirNode root;
   bool resolveSymlinks;
   bool readOnly;
   bool lazyChildCount;
   bool allowAppendChild;

   QDir::Filters filters;
   QDir::SortFlags sort;
   QStringList nameFilters;

   QFileIconProvider *iconProvider;
   QFileIconProvider defaultProvider;

   struct SavedPersistent {
      QString path;
      int column;
      QPersistentModelIndexData *data;
      QPersistentModelIndex index;
   };
   QList<SavedPersistent> savedPersistent;
   QPersistentModelIndex toBeRefreshed;

   bool shouldStat; // use the "carefull not to stat directories" mode
};

void qt_setDirModelShouldNotStat(QDirModelPrivate *modelPrivate)
{
   modelPrivate->shouldStat = false;
}

QDirModelPrivate::QDirNode *QDirModelPrivate::node(const QModelIndex &index) const
{
   QDirModelPrivate::QDirNode *n =
      static_cast<QDirModelPrivate::QDirNode *>(index.internalPointer());
   Q_ASSERT(n);
   return n;
}

void QDirModelPrivate::populate(QDirNode *parent) const
{
   Q_ASSERT(parent);
   parent->children = children(parent, parent->stat);
   parent->populated = true;
}

void QDirModelPrivate::clear(QDirNode *parent) const
{
   Q_ASSERT(parent);
   parent->children.clear();
   parent->populated = false;
}

void QDirModelPrivate::invalidate()
{
   QStack<const QDirNode *> nodes;
   nodes.push(&root);
   while (!nodes.empty()) {
      const QDirNode *current = nodes.pop();
      current->stat = false;
      const QVector<QDirNode> children = current->children;
      for (int i = 0; i < children.count(); ++i) {
         nodes.push(&children.at(i));
      }
   }
}

QDirModel::QDirModel(const QStringList &nameFilters, QDir::Filters filters,
      QDir::SortFlags sort, QObject *parent)
   : QAbstractItemModel(*new QDirModelPrivate, parent)
{
   Q_D(QDirModel);

   // always start with QDir::drives()
   d->nameFilters = nameFilters.isEmpty() ? QStringList(QLatin1String("*")) : nameFilters;
   d->filters = filters;
   d->sort = sort;
   d->root.parent = nullptr;
   d->root.info = QFileInfo();
   d->clear(&d->root);
}

QDirModel::QDirModel(QObject *parent)
   : QAbstractItemModel(*new QDirModelPrivate, parent)
{
   Q_D(QDirModel);
   d->init();
}

QDirModel::QDirModel(QDirModelPrivate &dd, QObject *parent)
   : QAbstractItemModel(dd, parent)
{
   Q_D(QDirModel);
   d->init();
}

QDirModel::~QDirModel()
{
}

QModelIndex QDirModel::index(int row, int column, const QModelIndex &parent) const
{
   Q_D(const QDirModel);

   // rowCount does lazy population
   if (column < 0 || column >= columnCount(parent) || row < 0 || parent.column() > 0) {
      return QModelIndex();
   }

   // make sure the list of children is up to date
   QDirModelPrivate::QDirNode *p = (d->indexValid(parent) ? d->node(parent) : &d->root);
   Q_ASSERT(p);
   if (!p->populated) {
      d->populate(p);   // populate without stat'ing
   }

   if (row >= p->children.count()) {
      return QModelIndex();
   }

   // now get the pointer for the index
   QDirModelPrivate::QDirNode *n = d->node(row, d->indexValid(parent) ? p : nullptr);
   Q_ASSERT(n);

   return createIndex(row, column, n);
}

QModelIndex QDirModel::parent(const QModelIndex &child) const
{
   Q_D(const QDirModel);

   if (!d->indexValid(child)) {
      return QModelIndex();
   }
   QDirModelPrivate::QDirNode *node = d->node(child);
   QDirModelPrivate::QDirNode *par = (node ? node->parent : nullptr);

   if (par == nullptr) {
      // parent is the root node
      return QModelIndex();
   }

   // get the parent's row
   const QVector<QDirModelPrivate::QDirNode> children = par->parent ? par->parent->children : d->root.children;

   Q_ASSERT(children.count() > 0);
   int row = (par - & (children.at(0)));
   Q_ASSERT(row >= 0);

   return createIndex(row, 0, par);
}

int QDirModel::rowCount(const QModelIndex &parent) const
{
   Q_D(const QDirModel);

   if (parent.column() > 0) {
      return 0;
   }

   if (!parent.isValid()) {
      if (!d->root.populated) { // lazy population
         d->populate(&d->root);
      }
      return d->root.children.count();
   }

   if (parent.model() != this) {
      return 0;
   }

   QDirModelPrivate::QDirNode *p = d->node(parent);
   if (p->info.isDir() && !p->populated) { // lazy population
      d->populate(p);
   }

   return p->children.count();
}

int QDirModel::columnCount(const QModelIndex &parent) const
{
   if (parent.column() > 0) {
      return 0;
   }
   return 4;
}

QVariant QDirModel::data(const QModelIndex &index, int role) const
{
   Q_D(const QDirModel);

   if (! d->indexValid(index)) {
      return QVariant();
   }

   if (role == Qt::DisplayRole || role == Qt::EditRole) {
      switch (index.column()) {
         case 0:
            return d->name(index);

         case 1:
            return d->size(index);

         case 2:
            return d->type(index);

         case 3:
            return d->time(index);

         default:
            qWarning("QDirModel::data() Display value column %d is invalid", index.column());
            return QVariant();
      }
   }

   if (index.column() == 0) {
      if (role == FileIconRole) {
         return fileIcon(index);
      }
      if (role == FilePathRole) {
         return filePath(index);
      }
      if (role == FileNameRole) {
         return fileName(index);
      }
   }

   if (index.column() == 1 && Qt::TextAlignmentRole == role) {
      return Qt::AlignRight;
   }
   return QVariant();
}

bool QDirModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
   Q_D(QDirModel);

   if (!d->indexValid(index) || index.column() != 0
      || (flags(index) & Qt::ItemIsEditable) == 0 || role != Qt::EditRole) {
      return false;
   }

   QDirModelPrivate::QDirNode *node = d->node(index);
   QDir dir     = node->info.dir();
   QString name = value.toString();

   if (dir.rename(node->info.fileName(), name)) {
      node->info = QFileInfo(dir, name);
      QModelIndex sibling = index.sibling(index.row(), 3);
      emit dataChanged(index, sibling);

      d->toBeRefreshed = index.parent();
      QMetaObject::invokeMethod(this, "_q_refresh", Qt::QueuedConnection);

      return true;
   }

   return false;
}

QVariant QDirModel::headerData(int section, Qt::Orientation orientation, int role) const
{
   if (orientation == Qt::Horizontal) {
      if (role != Qt::DisplayRole) {
         return QVariant();
      }
      switch (section) {
         case 0:
            return tr("Name");

         case 1:
            return tr("Size");

         case 2:
            return
#ifdef Q_OS_DARWIN
               tr("Kind", "Match OS X Finder");
#else
               tr("Type", "All other platforms");
#endif

         // Windows   - Type
         // OS X      - Kind
         // Konqueror - File Type
         // Nautilus  - Type
         case 3:
            return tr("Date Modified");

         default:
            return QVariant();
      }
   }
   return QAbstractItemModel::headerData(section, orientation, role);
}

bool QDirModel::hasChildren(const QModelIndex &parent) const
{
   Q_D(const QDirModel);

   if (parent.column() > 0) {
      return false;
   }

   if (!parent.isValid()) { // the invalid index is the "My Computer" item
      return true;   // the drives
   }
   QDirModelPrivate::QDirNode *p = d->node(parent);
   Q_ASSERT(p);

   if (d->lazyChildCount) { // optimization that only checks for children if the node has been populated
      return p->info.isDir();
   }

   return p->info.isDir() && rowCount(parent) > 0;
}

Qt::ItemFlags QDirModel::flags(const QModelIndex &index) const
{
   Q_D(const QDirModel);

   Qt::ItemFlags flags = QAbstractItemModel::flags(index);
   if (!d->indexValid(index)) {
      return flags;
   }

   flags |= Qt::ItemIsDragEnabled;
   if (d->readOnly) {
      return flags;
   }

   QDirModelPrivate::QDirNode *node = d->node(index);
   if ((index.column() == 0) && node->info.isWritable()) {
      flags |= Qt::ItemIsEditable;
      if (fileInfo(index).isDir()) { // is directory and is editable
         flags |= Qt::ItemIsDropEnabled;
      }
   }

   return flags;
}

void QDirModel::sort(int column, Qt::SortOrder order)
{
   QDir::SortFlags sort = QDir::DirsFirst | QDir::IgnoreCase;
   if (order == Qt::DescendingOrder) {
      sort |= QDir::Reversed;
   }

   switch (column) {
      case 0:
         sort |= QDir::Name;
         break;

      case 1:
         sort |= QDir::Size;
         break;

      case 2:
         sort |= QDir::Type;
         break;

      case 3:
         sort |= QDir::Time;
         break;

      default:
         break;
   }

   setSorting(sort);
}

QStringList QDirModel::mimeTypes() const
{
   return QStringList(QLatin1String("text/uri-list"));
}

QMimeData *QDirModel::mimeData(const QModelIndexList &indexes) const
{
   QList<QUrl> urls;
   QList<QModelIndex>::const_iterator it = indexes.begin();

   for (; it != indexes.end(); ++it) {
      if ((*it).column() == 0) {
         urls << QUrl::fromLocalFile(filePath(*it));
      }
   }

   QMimeData *data = new QMimeData();
   data->setUrls(urls);

   return data;
}

bool QDirModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
   int, int, const QModelIndex &parent)
{
   Q_D(QDirModel);
   if (!d->indexValid(parent) || isReadOnly()) {
      return false;
   }

   bool success = true;
   QString to = filePath(parent) + QDir::separator();
   QModelIndex _parent = parent;

   QList<QUrl> urls = data->urls();
   QList<QUrl>::const_iterator it = urls.constBegin();

   switch (action) {
      case Qt::CopyAction:
         for (; it != urls.constEnd(); ++it) {
            QString path = (*it).toLocalFile();
            success = QFile::copy(path, to + QFileInfo(path).fileName()) && success;
         }
         break;

      case Qt::LinkAction:
         for (; it != urls.constEnd(); ++it) {
            QString path = (*it).toLocalFile();
            success = QFile::link(path, to + QFileInfo(path).fileName()) && success;
         }
         break;

      case Qt::MoveAction:
         for (; it != urls.constEnd(); ++it) {
            QString path = (*it).toLocalFile();

            if (QFile::copy(path, to + QFileInfo(path).fileName()) && QFile::remove(path)) {
               QModelIndex idx = index(QFileInfo(path).path());

               if (idx.isValid()) {
                  refresh(idx);
                  //the previous call to refresh may invalidate the _parent. so recreate a new QModelIndex
                  _parent = index(to);
               }

            } else {
               success = false;
            }
         }
         break;

      default:
         return false;
   }

   if (success) {
      refresh(_parent);
   }

   return success;
}

Qt::DropActions QDirModel::supportedDropActions() const
{
   return Qt::CopyAction | Qt::MoveAction; // FIXME: LinkAction is not supported yet
}

void QDirModel::setIconProvider(QFileIconProvider *provider)
{
   Q_D(QDirModel);
   d->iconProvider = provider;
}

QFileIconProvider *QDirModel::iconProvider() const
{
   Q_D(const QDirModel);
   return d->iconProvider;
}

void QDirModel::setNameFilters(const QStringList &filters)
{
   Q_D(QDirModel);
   d->nameFilters = filters;
   emit layoutAboutToBeChanged();
   if (d->shouldStat) {
      refresh(QModelIndex());
   } else {
      d->invalidate();
   }
   emit layoutChanged();
}

QStringList QDirModel::nameFilters() const
{
   Q_D(const QDirModel);
   return d->nameFilters;
}

void QDirModel::setFilter(QDir::Filters filters)
{
   Q_D(QDirModel);
   d->filters = filters;
   emit layoutAboutToBeChanged();
   if (d->shouldStat) {
      refresh(QModelIndex());
   } else {
      d->invalidate();
   }
   emit layoutChanged();
}

QDir::Filters QDirModel::filter() const
{
   Q_D(const QDirModel);
   return d->filters;
}

void QDirModel::setSorting(QDir::SortFlags sort)
{
   Q_D(QDirModel);
   d->sort = sort;
   emit layoutAboutToBeChanged();
   if (d->shouldStat) {
      refresh(QModelIndex());
   } else {
      d->invalidate();
   }
   emit layoutChanged();
}

QDir::SortFlags QDirModel::sorting() const
{
   Q_D(const QDirModel);
   return d->sort;
}

void QDirModel::setResolveSymlinks(bool enable)
{
   Q_D(QDirModel);
   d->resolveSymlinks = enable;
}

bool QDirModel::resolveSymlinks() const
{
   Q_D(const QDirModel);
   return d->resolveSymlinks;
}

void QDirModel::setReadOnly(bool enable)
{
   Q_D(QDirModel);
   d->readOnly = enable;
}

bool QDirModel::isReadOnly() const
{
   Q_D(const QDirModel);
   return d->readOnly;
}

void QDirModel::setLazyChildCount(bool enable)
{
   Q_D(QDirModel);
   d->lazyChildCount = enable;
}

bool QDirModel::lazyChildCount() const
{
   Q_D(const QDirModel);
   return d->lazyChildCount;
}

void QDirModel::refresh(const QModelIndex &parent)
{
   Q_D(QDirModel);

   QDirModelPrivate::QDirNode *n = d->indexValid(parent) ? d->node(parent) : &(d->root);

   int rows = n->children.count();
   if (rows == 0) {
      emit layoutAboutToBeChanged();
      n->stat = true; // make sure that next time we read all the info
      n->populated = false;
      emit layoutChanged();
      return;
   }

   emit layoutAboutToBeChanged();
   d->savePersistentIndexes();
   d->rowsAboutToBeRemoved(parent, 0, rows - 1);
   n->stat = true; // make sure that next time we read all the info
   d->clear(n);
   d->rowsRemoved(parent, 0, rows - 1);
   d->restorePersistentIndexes();
   emit layoutChanged();
}

QModelIndex QDirModel::index(const QString &path, int column) const
{
   Q_D(const QDirModel);

   if (path.isEmpty() || path == QCoreApplication::translate("QFileDialog", "My Computer")) {
      return QModelIndex();
   }

   QString absolutePath = QDir(path).absolutePath();

#if defined(Q_OS_WIN)
   absolutePath = absolutePath.toLower();

   // On Windows, "filename......." and "filename" are equivalent
   if (absolutePath.endsWith(QLatin1Char('.'))) {
      int i;
      for (i = absolutePath.count() - 1; i >= 0; --i) {
         if (absolutePath.at(i) != '.') {
            break;
         }
      }
      absolutePath = absolutePath.left(i + 1);
   }
#endif

   QStringList pathElements = absolutePath.split(QLatin1Char('/'), QStringParser::SkipEmptyParts);
   if ((pathElements.isEmpty() || !QFileInfo(path).exists())

#if !defined(Q_OS_WIN)
      && path != "/"
#endif
   ) {
      return QModelIndex();
   }

   QModelIndex idx; // start with "My Computer"
   if (!d->root.populated) { // make sure the root is populated
      d->populate(&d->root);
   }

#if defined(Q_OS_WIN)
   if (absolutePath.startsWith(QLatin1String("//"))) { // UNC path
      QString host = pathElements.first();
      int r = 0;
      for (; r < d->root.children.count(); ++r)
         if (d->root.children.at(r).info.fileName() == host) {
            break;
         }
      bool childAppended = false;
      if (r >= d->root.children.count() && d->allowAppendChild) {
         d->appendChild(&d->root, QLatin1String("//") + host);
         childAppended = true;
      }
      idx = index(r, 0, QModelIndex());
      pathElements.pop_front();
      if (childAppended) {
         emit const_cast<QDirModel *>(this)->layoutChanged();
      }
   } else
#endif

#if defined(Q_OS_WIN)
      if (pathElements.at(0).endsWith(QLatin1Char(':'))) {
         pathElements[0] += QLatin1Char('/');
      }
#else
      // add the "/" item, since it is a valid path element on unix
      pathElements.prepend(QLatin1String("/"));
#endif

   for (int i = 0; i < pathElements.count(); ++i) {
      Q_ASSERT(!pathElements.at(i).isEmpty());
      QString element = pathElements.at(i);
      QDirModelPrivate::QDirNode *parent = (idx.isValid() ? d->node(idx) : &d->root);

      Q_ASSERT(parent);
      if (!parent->populated) {
         d->populate(parent);
      }

      // search for the element in the child nodes first
      int row = -1;
      for (int j = parent->children.count() - 1; j >= 0; --j) {
         const QFileInfo &fi = parent->children.at(j).info;
         QString childFileName;
         childFileName = idx.isValid() ? fi.fileName() : fi.absoluteFilePath();
#if defined(Q_OS_WIN)
         childFileName = childFileName.toLower();
#endif
         if (childFileName == element) {
            if (i == pathElements.count() - 1) {
               parent->children[j].stat = true;
            }
            row = j;
            break;
         }
      }

      // we couldn't find the path element, we create a new node since we _know_ that the path is valid
      if (row == -1) {
         QString newPath = parent->info.absoluteFilePath() + QLatin1Char('/') + element;

         if (!d->allowAppendChild || !QFileInfo(newPath).isDir()) {
            return QModelIndex();
         }
         d->appendChild(parent, newPath);
         row = parent->children.count() - 1;
         if (i == pathElements.count() - 1) { // always stat children of  the last element
            parent->children[row].stat = true;
         }
         emit const_cast<QDirModel *>(this)->layoutChanged();
      }

      Q_ASSERT(row >= 0);
      idx = createIndex(row, 0, static_cast<void *>(&parent->children[row]));
      Q_ASSERT(idx.isValid());
   }

   if (column != 0) {
      return idx.sibling(idx.row(), column);
   }
   return idx;
}

bool QDirModel::isDir(const QModelIndex &index) const
{
   Q_D(const QDirModel);
   Q_ASSERT(d->indexValid(index));
   QDirModelPrivate::QDirNode *node = d->node(index);
   return node->info.isDir();
}

QModelIndex QDirModel::mkdir(const QModelIndex &parent, const QString &name)
{
   Q_D(QDirModel);
   if (!d->indexValid(parent) || isReadOnly()) {
      return QModelIndex();
   }

   QDirModelPrivate::QDirNode *p = d->node(parent);
   QString path = p->info.absoluteFilePath();

   // For indexOf() method to work the new directory has to be a direct child of the parent directory.

   QDir newDir(name);
   QDir dir(path);
   if (newDir.isRelative()) {
      newDir = QDir(path + QLatin1Char('/') + name);
   }
   QString childName = newDir.dirName(); // Get the singular name of the directory
   newDir.cdUp();

   if (newDir.absolutePath() != dir.absolutePath() || !dir.mkdir(name)) {
      return QModelIndex();   // nothing happened
   }

   refresh(parent);

   QStringList entryList = d->entryList(path);
   int r = entryList.indexOf(childName);
   QModelIndex i = index(r, 0, parent); // return an invalid index

   return i;
}

bool QDirModel::rmdir(const QModelIndex &index)
{
   Q_D(QDirModel);

   if (!d->indexValid(index) || isReadOnly()) {
      return false;
   }

   QDirModelPrivate::QDirNode *n = d_func()->node(index);
   if (! n->info.isDir()) {
      qWarning("QDirModel::rmdir() Node is not a directory");
      return false;
   }

   QModelIndex par = parent(index);
   QDirModelPrivate::QDirNode *p = d_func()->node(par);
   QDir dir     = p->info.dir(); // parent dir
   QString path = n->info.absoluteFilePath();

   if (!dir.rmdir(path)) {
      return false;
   }

   refresh(par);

   return true;
}

bool QDirModel::remove(const QModelIndex &index)
{
   Q_D(QDirModel);
   if (!d->indexValid(index) || isReadOnly()) {
      return false;
   }

   QDirModelPrivate::QDirNode *n = d_func()->node(index);
   if (n->info.isDir()) {
      return false;
   }

   QModelIndex par = parent(index);
   QDirModelPrivate::QDirNode *p = d_func()->node(par);
   QDir dir = p->info.dir(); // parent dir
   QString path = n->info.absoluteFilePath();
   if (!dir.remove(path)) {
      return false;
   }

   refresh(par);

   return true;
}

QString QDirModel::filePath(const QModelIndex &index) const
{
   Q_D(const QDirModel);
   if (d->indexValid(index)) {
      QFileInfo fi = fileInfo(index);
      if (d->resolveSymlinks && fi.isSymLink()) {
         fi = d->resolvedInfo(fi);
      }
      return QDir::cleanPath(fi.absoluteFilePath());
   }
   return QString(); // root path
}

QString QDirModel::fileName(const QModelIndex &index) const
{
   Q_D(const QDirModel);
   if (!d->indexValid(index)) {
      return QString();
   }
   QFileInfo info = fileInfo(index);
   if (info.isRoot()) {
      return info.absoluteFilePath();
   }
   if (d->resolveSymlinks && info.isSymLink()) {
      info = d->resolvedInfo(info);
   }
   return info.fileName();
}

QIcon QDirModel::fileIcon(const QModelIndex &index) const
{
   Q_D(const QDirModel);
   if (!d->indexValid(index)) {
      return d->iconProvider->icon(QFileIconProvider::Computer);
   }
   QDirModelPrivate::QDirNode *node = d->node(index);
   if (node->icon.isNull()) {
      node->icon = d->iconProvider->icon(node->info);
   }
   return node->icon;
}

QFileInfo QDirModel::fileInfo(const QModelIndex &index) const
{
   Q_D(const QDirModel);
   Q_ASSERT(d->indexValid(index));

   QDirModelPrivate::QDirNode *node = d->node(index);
   return node->info;
}

void QDirModelPrivate::init()
{
   filters = QDir::AllEntries | QDir::NoDotAndDotDot;
   sort    = QDir::Name;
   nameFilters << "*";

   root.parent = nullptr;
   root.info = QFileInfo();
   clear(&root);

   roleNames.insertMulti(QDirModel::FileIconRole, "fileIcon");
   roleNames.insert(QDirModel::FilePathRole,      "filePath");
   roleNames.insert(QDirModel::FileNameRole,      "fileName");
}

QDirModelPrivate::QDirNode *QDirModelPrivate::node(int row, QDirNode *parent) const
{
   if (row < 0) {
      return nullptr;
   }

   bool isDir = !parent || parent->info.isDir();
   QDirNode *p = (parent ? parent : &root);

   if (isDir && !p->populated) {
      populate(p);   // will also resolve symlinks
   }

   if (row >= p->children.count()) {
      qWarning("QDirNode::node() Specified row does not exist");
      return nullptr;
   }

   return const_cast<QDirNode *>(&p->children.at(row));
}

QVector<QDirModelPrivate::QDirNode> QDirModelPrivate::children(QDirNode *parent, bool stat) const
{
   Q_ASSERT(parent);
   QFileInfoList infoList;

   if (parent == &root) {
      parent = nullptr;
      infoList = QDir::drives();

   } else if (parent->info.isDir()) {
      //resolve directory links only if requested.
      if (parent->info.isSymLink() && resolveSymlinks) {
         QString link = parent->info.symLinkTarget();
         if (link.size() > 1 && link.at(link.size() - 1) == QDir::separator()) {
            link.chop(1);
         }
         if (stat) {
            infoList = entryInfoList(link);
         } else {
            infoList = QDir(link).entryInfoList(nameFilters, QDir::AllEntries | QDir::System);
         }
      } else {
         if (stat) {
            infoList = entryInfoList(parent->info.absoluteFilePath());
         } else {
            infoList = QDir(parent->info.absoluteFilePath()).entryInfoList(nameFilters, QDir::AllEntries | QDir::System);
         }
      }
   }

   QVector<QDirNode> nodes(infoList.count());
   for (int i = 0; i < infoList.count(); ++i) {
      QDirNode &node = nodes[i];
      node.parent = parent;
      node.info = infoList.at(i);
      node.populated = false;
      node.stat = shouldStat;
   }

   return nodes;
}

void QDirModelPrivate::_q_refresh()
{
   Q_Q(QDirModel);
   q->refresh(toBeRefreshed);
   toBeRefreshed = QModelIndex();
}

void QDirModelPrivate::savePersistentIndexes()
{
   Q_Q(QDirModel);
   savedPersistent.clear();

   for (QPersistentModelIndexData *data : persistent.m_indexes) {
      SavedPersistent saved;

      QModelIndex index = data->index;
      saved.path = q->filePath(index);
      saved.column = index.column();
      saved.data = data;
      saved.index = index;
      savedPersistent.append(saved);
   }
}

void QDirModelPrivate::restorePersistentIndexes()
{
   Q_Q(QDirModel);
   bool allow = allowAppendChild;
   allowAppendChild = false;

   for (int i = 0; i < savedPersistent.count(); ++i) {
      QPersistentModelIndexData *data = savedPersistent.at(i).data;
      QString path = savedPersistent.at(i).path;
      int column   = savedPersistent.at(i).column;

      QModelIndex idx = q->index(path, column);
      if (idx != data->index || data->model == nullptr) {
         //data->model may be equal to 0 if the model is getting destroyed
         persistent.m_indexes.remove(data->index);

         data->index = idx;
         data->model = q;

         if (idx.isValid()) {
            persistent.m_indexes.insert(idx, data);
         }
      }
   }
   savedPersistent.clear();
   allowAppendChild = allow;
}

QFileInfoList QDirModelPrivate::entryInfoList(const QString &path) const
{
   const QDir dir(path);
   return dir.entryInfoList(nameFilters, filters, sort);
}

QStringList QDirModelPrivate::entryList(const QString &path) const
{
   const QDir dir(path);
   return dir.entryList(nameFilters, filters, sort);
}

QString QDirModelPrivate::name(const QModelIndex &index) const
{
   const QDirNode *n = node(index);
   const QFileInfo info = n->info;
   if (info.isRoot()) {
      QString name = info.absoluteFilePath();
#if defined(Q_OS_WIN)
      if (name.startsWith(QLatin1Char('/'))) { // UNC host
         return info.fileName();
      }

      if (name.endsWith(QLatin1Char('/'))) {
         name.chop(1);
      }
#endif
      return name;
   }
   return info.fileName();
}

QString QDirModelPrivate::size(const QModelIndex &index) const
{
   const QDirNode *n = node(index);
   if (n->info.isDir()) {
#ifdef Q_OS_DARWIN
      return QLatin1String("--");
#else
      return QLatin1String("");
#endif
      // Windows   - ""
      // OS X      - "--"
      // Konqueror - "4 KB"
      // Nautilus  - "9 items" (the number of children)
   }

   // According to the Si standard KB is 1000 bytes, KiB is 1024
   // but on windows sizes are calulated by dividing by 1024 so we do what they do.
   const quint64 kb = 1024;
   const quint64 mb = 1024 * kb;
   const quint64 gb = 1024 * mb;
   const quint64 tb = 1024 * gb;
   quint64 bytes = n->info.size();
   if (bytes >= tb) {
      return QFileSystemModel::tr("%1 TB").formatArg(QLocale().toString(qreal(bytes) / tb, 'f', 3));
   }
   if (bytes >= gb) {
      return QFileSystemModel::tr("%1 GB").formatArg(QLocale().toString(qreal(bytes) / gb, 'f', 2));
   }
   if (bytes >= mb) {
      return QFileSystemModel::tr("%1 MB").formatArg(QLocale().toString(qreal(bytes) / mb, 'f', 1));
   }
   if (bytes >= kb) {
      return QFileSystemModel::tr("%1 KB").formatArg(QLocale().toString(bytes / kb));
   }
   return QFileSystemModel::tr("%1 byte(s)").formatArg(QLocale().toString(bytes));
}

QString QDirModelPrivate::type(const QModelIndex &index) const
{
   return iconProvider->type(node(index)->info);
}

QString QDirModelPrivate::time(const QModelIndex &index) const
{
   return node(index)->info.lastModified().toString(Qt::LocalDate);
}

void QDirModelPrivate::appendChild(QDirModelPrivate::QDirNode *parent, const QString &path) const
{
   QDirModelPrivate::QDirNode node;
   node.populated = false;
   node.stat = shouldStat;
   node.parent = (parent == &root ? nullptr : parent);
   node.info = QFileInfo(path);
   node.info.setCaching(true);

   // The following append(node) may reallocate the vector, thus
   // we need to update the pointers to the childnodes parent.
   QDirModelPrivate *that = const_cast<QDirModelPrivate *>(this);
   that->savePersistentIndexes();
   parent->children.append(node);
   for (int i = 0; i < parent->children.count(); ++i) {
      QDirNode *childNode = &parent->children[i];
      for (int j = 0; j < childNode->children.count(); ++j) {
         childNode->children[j].parent = childNode;
      }
   }
   that->restorePersistentIndexes();
}

QFileInfo QDirModelPrivate::resolvedInfo(QFileInfo info)
{
#ifdef Q_OS_WIN
   // On windows, we cannot create a shortcut to a shortcut.
   return QFileInfo(info.symLinkTarget());

#else
   QStringList paths;

   do {
      QFileInfo link(info.symLinkTarget());

      if (link.isRelative()) {
         info.setFile(info.absolutePath(), link.filePath());
      } else {
         info = link;
      }

      if (paths.contains(info.absoluteFilePath())) {
         return QFileInfo();
      }

      paths.append(info.absoluteFilePath());
   } while (info.isSymLink());

   return info;
#endif
}

void QDirModel::_q_refresh()
{
   Q_D(QDirModel);
   d->_q_refresh();
}

#endif // QT_NO_DIRMODEL
