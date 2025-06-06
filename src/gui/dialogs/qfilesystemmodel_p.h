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

#ifndef QFILESYSTEMMODEL_P_H
#define QFILESYSTEMMODEL_P_H

#include <qfilesystemmodel.h>

#ifndef QT_NO_FILESYSTEMMODEL

#include <qabstractitemmodel.h>
#include <qdir.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qhash.h>
#include <qicon.h>
#include <qicon.h>
#include <qpair.h>
#include <qtimer.h>

#include <qabstractitemmodel_p.h>
#include <qfileinfogatherer_p.h>

class ExtendedInformation;
class QFileIconProvider;
class QFileSystemModelPrivate;

class QFileSystemModelPrivate : public QAbstractItemModelPrivate
{
   Q_DECLARE_PUBLIC(QFileSystemModel)

 public:
   static constexpr int NumColumns = 4;
   class QFileSystemNode
   {
      class FileNameCompare
      {
       public:
         bool operator()(const QString &a, const QString &b) const {

#ifdef Q_OS_WIN
            return a.compare(b, Qt::CaseInsensitive) < 0;
#else
            return a < b;
#endif
         }
      };

    public:
      explicit QFileSystemNode(const QString &filename = QString(), QFileSystemNode *p = nullptr)
         : fileName(filename), populatedChildren(false), isVisible(false), dirtyChildrenIndex(-1), parent(p), info(nullptr)
      {
      }

      ~QFileSystemNode() {
         for (auto i : children) {
            delete i;
         }

         delete info;
         info   = nullptr;
         parent = nullptr;
      }

      QString fileName;

#if defined(Q_OS_WIN)
      QString volumeName;
#endif

      qint64 size() const {
         if (info && ! info->isDir()) {
            return info->size();
         }
         return 0;
      }

      QString type() const {
         if (info) {
            return info->displayType;
         }
         return QString("");
      }

      QDateTime lastModified() const {
         if (info) {
            return info->lastModified();
         }
         return QDateTime();
      }

      QFileDevice::Permissions permissions() const {
         if (info) {
            return info->permissions();
         }

         return Qt::EmptyFlag;
      }

      bool isReadable() const {
         return ((permissions() & QFile::ReadUser) != 0);
      }

      bool isWritable() const {
         return ((permissions() & QFile::WriteUser) != 0);
      }

      bool isExecutable() const {
         return ((permissions() & QFile::ExeUser) != 0);
      }

      bool isDir() const {
         if (info) {
            return info->isDir();
         }

         if (children.count() > 0) {
            return true;
         }
         return false;
      }

      QFileInfo fileInfo() const {
         if (info) {
            return info->fileInfo();
         }
         return QFileInfo();
      }

      bool isFile() const {
         if (info) {
            return info->isFile();
         }
         return true;
      }

      bool isSystem() const {
         if (info) {
            return info->isSystem();
         }
         return true;
      }

      bool isHidden() const {
         if (info) {
            return info->isHidden();
         }
         return false;
      }

      bool isSymLink(bool ignoreNtfsSymLinks = false) const {
         return info && info->isSymLink(ignoreNtfsSymLinks);
      }

      bool caseSensitive() const {
         if (info) {
            return info->isCaseSensitive();
         }
         return false;
      }

      QIcon icon() const {
         if (info) {
            return info->icon;
         }
         return QIcon();
      }

      bool operator <(const QFileSystemNode &node) const {
         if (caseSensitive() || node.caseSensitive()) {
            return fileName < node.fileName;
         }
         return QString::compare(fileName, node.fileName, Qt::CaseInsensitive) < 0;
      }

      bool operator >(const QString &name) const {
         if (caseSensitive()) {
            return fileName > name;
         }
         return QString::compare(fileName, name, Qt::CaseInsensitive) > 0;
      }

      bool operator <(const QString &name) const {
         if (caseSensitive()) {
            return fileName < name;
         }
         return QString::compare(fileName, name, Qt::CaseInsensitive) < 0;
      }

      bool operator !=(const QExtendedInformation &fileInfo) const {
         return !operator==(fileInfo);
      }

      bool operator ==(const QString &name) const {
         if (caseSensitive()) {
            return fileName == name;
         }
         return QString::compare(fileName, name, Qt::CaseInsensitive) == 0;
      }

      bool operator ==(const QExtendedInformation &fileInfo) const {
         return info && (*info == fileInfo);
      }

      bool hasInformation() const {
         return info != nullptr;
      }

      void populate(const QExtendedInformation &fileInfo) {
         if (! info) {
            info = new QExtendedInformation(fileInfo.fileInfo());
         }
         (*info) = fileInfo;
      }

      // children shouldn't normally be accessed directly, use node()
      int visibleLocation(const QString &childName) {
         return visibleChildren.indexOf(childName);
      }

      void updateIcon(QFileIconProvider *iconProvider, const QString &path) {
         if (info) {
            info->icon = iconProvider->icon(QFileInfo(path));
         }

         for (auto i : children) {
            //On windows the root (My computer) has no path so we don't want to add a / for nothing (e.g. /C:/)

            if (! path.isEmpty()) {
               if (path.endsWith('/')) {
                  i->updateIcon(iconProvider, path + i->fileName);
               } else {
                  i->updateIcon(iconProvider, path + '/' + i->fileName);
               }

            } else {
               i->updateIcon(iconProvider, i->fileName);
            }
         }
      }

      void retranslateStrings(QFileIconProvider *iconProvider, const QString &path) {
         if (info) {
            info->displayType = iconProvider->type(QFileInfo(path));
         }

         for (auto i : children) {

            //On windows the root (My computer) has no path so we don't want to add a / for nothing (e.g. /C:/)
            if (! path.isEmpty()) {
               if (path.endsWith('/')) {
                  i->retranslateStrings(iconProvider, path + i->fileName);
               } else {
                  i->retranslateStrings(iconProvider, path + '/' + i->fileName);
               }
            } else {
               i->retranslateStrings(iconProvider, i->fileName);
            }
         }
      }

      bool populatedChildren;
      bool isVisible;

      QMap<QString, QFileSystemNode *, FileNameCompare> children;

      QList<QString> visibleChildren;
      int dirtyChildrenIndex;
      QFileSystemNode *parent;

      QExtendedInformation *info;
   };

   QFileSystemModelPrivate() :
      forceSort(true),
      sortColumn(0),
      sortOrder(Qt::AscendingOrder),
      readOnly(true),
      setRootPath(false),
      filters(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::AllDirs),
      nameFilterDisables(true), // false on windows, true on mac and unix
      disableRecursiveSort(false) {
      delayedSortTimer.setSingleShot(true);
   }

   void init();
   bool isHiddenByFilter(QFileSystemNode *indexNode, const QModelIndex &index) const {
      return (indexNode != &root && !index.isValid());
   }

   QFileSystemNode *node(const QModelIndex &index) const;
   QFileSystemNode *node(const QString &path, bool fetch = true) const;

   QModelIndex index(const QString &path, int column = 0) {
      return index(node(path), column);
   }

   QModelIndex index(const QFileSystemNode *node, int column = 0) const;
   bool filtersAcceptsNode(const QFileSystemNode *node) const;
   bool passNameFilters(const QFileSystemNode *node) const;
   void removeNode(QFileSystemNode *parentNode, const QString &name);
   QFileSystemNode *addNode(QFileSystemNode *parentNode, const QString &fileName, const QFileInfo &info);
   void addVisibleFiles(QFileSystemNode *parentNode, const QStringList &newFiles);
   void removeVisibleFile(QFileSystemNode *parentNode, int visibleLocation);
   void sortChildren(int column, const QModelIndex &parent);

   int translateVisibleLocation(QFileSystemNode *parent, int row) const {
      if (sortOrder != Qt::AscendingOrder) {
         if (parent->dirtyChildrenIndex == -1) {
            return parent->visibleChildren.count() - row - 1;
         }

         if (row < parent->dirtyChildrenIndex) {
            return parent->dirtyChildrenIndex - row - 1;
         }
      }

      return row;
   }

   static QString myComputer() {
      // ### TODO We should query the system to find out what the string should be
      // XP == "My Computer",
      // Vista == "Computer",
      // OS X == "Computer" (sometime user generated) "Benjamin's PowerBook G4"

#ifdef Q_OS_WIN
      return QFileSystemModel::tr("My Computer");
#else
      return QFileSystemModel::tr("Computer");
#endif

   }

   void delayedSort() {
      if (!delayedSortTimer.isActive()) {
         delayedSortTimer.start(0);
      }
   }

   static bool caseInsensitiveLessThan(const QString &s1, const QString &s2) {
      return QString::compare(s1, s2, Qt::CaseInsensitive) < 0;
   }

   static bool nodeCaseInsensitiveLessThan(const QFileSystemModelPrivate::QFileSystemNode &s1,
      const QFileSystemModelPrivate::QFileSystemNode &s2) {
      return QString::compare(s1.fileName, s2.fileName, Qt::CaseInsensitive) < 0;
   }

   QIcon icon(const QModelIndex &index) const;
   QString name(const QModelIndex &index) const;
   QString displayName(const QModelIndex &index) const;
   QString filePath(const QModelIndex &index) const;
   QString size(const QModelIndex &index) const;
   static QString size(qint64 bytes);
   QString type(const QModelIndex &index) const;
   QString time(const QModelIndex &index) const;

   void _q_directoryChanged(const QString &directory, const QStringList &list);
   void _q_performDelayedSort();
   void _q_fileSystemChanged(const QString &path, const QVector<QPair<QString, QFileInfo>> &);
   void _q_resolvedName(const QString &fileName, const QString &resolvedName);

   static int naturalCompare(const QString &s1, const QString &s2, Qt::CaseSensitivity cs);

   QDir rootDir;

#ifndef QT_NO_FILESYSTEMWATCHER
   QFileInfoGatherer fileInfoGatherer;
#endif

   QTimer delayedSortTimer;
   bool forceSort;
   int sortColumn;
   Qt::SortOrder sortOrder;
   bool readOnly;
   bool setRootPath;
   QDir::Filters filters;
   QHash<const QFileSystemNode *, bool> bypassFilters;
   bool nameFilterDisables;

   //This flag is an optimization for the QFileDialog
   //It enable a sort which is not recursive, it means
   //we sort only what we see.
   bool disableRecursiveSort;


   QList<QRegularExpression> nameFilters;


   QHash<QString, QString> resolvedSymLinks;

   QFileSystemNode root;

   QBasicTimer fetchingTimer;
   struct Fetching {
      QString dir;
      QString file;
      const QFileSystemNode *node;
   };
   QList<Fetching> toFetch;

};
#endif // QT_NO_FILESYSTEMMODEL



#endif

