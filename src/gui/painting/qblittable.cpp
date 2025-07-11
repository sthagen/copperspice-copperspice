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

#include <qblittable_p.h>

#ifndef QT_NO_BLITTABLE

class QBlittablePrivate
{
 public:
   QBlittablePrivate(const QSize &size, QBlittable::Capabilities caps)
      : locked(false), m_caps(caps), m_size(size), cachedImg(nullptr)
   { }

   bool locked;

   QBlittable::Capabilities m_caps;
   QSize m_size;
   QImage *cachedImg;
};

QBlittable::QBlittable(const QSize &size, Capabilities caps)
   : d_ptr(new QBlittablePrivate(size, caps))
{
}

QBlittable::~QBlittable()
{
   delete d_ptr;
}

QBlittable::Capabilities QBlittable::capabilities() const
{
   Q_D(const QBlittable);
   return d->m_caps;
}

QSize QBlittable::size() const
{
   Q_D(const QBlittable);
   return d->m_size;
}

bool QBlittable::isLocked() const
{
   Q_D(const QBlittable);
   return d->locked;
}

QImage *QBlittable::lock()
{
   Q_D(QBlittable);

   if (! d->locked) {
      d->cachedImg = doLock();
      d->locked = true;
   }

   return d->cachedImg;
}

void QBlittable::unlock()
{
   Q_D(QBlittable);

   if (d->locked) {
      doUnlock();
      d->locked = false;
   }
}

#endif

