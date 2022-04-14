/***********************************************************************
*
* Copyright (c) 2012-2022 Barbara Geller
* Copyright (c) 2012-2022 Ansel Sermersheim
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

#ifndef QPOINT_H
#define QPOINT_H

#include <qnamespace.h>

class QDataStream;
class QDebug;

class Q_CORE_EXPORT QPoint
{
 public:
   QPoint();
   QPoint(int xPos, int yPos);

   inline bool isNull() const;

   inline int x() const;
   inline int y() const;
   inline void setX(int xPos);
   inline void setY(int yPos);

   int manhattanLength() const;

   inline int &rx();
   inline int &ry();

   inline QPoint &operator+=(const QPoint &p);
   inline QPoint &operator-=(const QPoint &p);

   inline QPoint &operator*=(float c);
   inline QPoint &operator*=(double c);
   inline QPoint &operator*=(int c);

   inline QPoint &operator/=(qreal c);

 private:
   int xp;
   int yp;

   friend inline bool operator==(const QPoint &, const QPoint &);
   friend inline bool operator!=(const QPoint &, const QPoint &);
   friend inline const QPoint operator+(const QPoint &, const QPoint &);
   friend inline const QPoint operator-(const QPoint &, const QPoint &);
   friend inline const QPoint operator*(const QPoint &, float);
   friend inline const QPoint operator*(float, const QPoint &);
   friend inline const QPoint operator*(const QPoint &, double);
   friend inline const QPoint operator*(double, const QPoint &);
   friend inline const QPoint operator*(const QPoint &, int);
   friend inline const QPoint operator*(int, const QPoint &);
   friend inline const QPoint operator-(const QPoint &);
   friend inline const QPoint operator/(const QPoint &, qreal);
   friend class QTransform;
};

Q_CORE_EXPORT QDataStream &operator<<(QDataStream &stream, const QPoint &point);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &stream, QPoint &point);

inline QPoint::QPoint()
{
   xp = 0;
   yp = 0;
}

inline QPoint::QPoint(int xPos, int yPos)
{
   xp = xPos;
   yp = yPos;
}

inline bool QPoint::isNull() const
{
   return xp == 0 && yp == 0;
}

inline int QPoint::x() const
{
   return xp;
}

inline int QPoint::y() const
{
   return yp;
}

inline void QPoint::setX(int xPos)
{
   xp = xPos;
}

inline void QPoint::setY(int yPos)
{
   yp = yPos;
}

inline int &QPoint::rx()
{
   return xp;
}

inline int &QPoint::ry()
{
   return yp;
}

inline QPoint &QPoint::operator+=(const QPoint &p)
{
   xp += p.xp;
   yp += p.yp;
   return *this;
}

inline QPoint &QPoint::operator-=(const QPoint &p)
{
   xp -= p.xp;
   yp -= p.yp;
   return *this;
}

inline QPoint &QPoint::operator*=(float c)
{
   xp = qRound(xp * c);
   yp = qRound(yp * c);
   return *this;
}

inline QPoint &QPoint::operator*=(double c)
{
   xp = qRound(xp * c);
   yp = qRound(yp * c);
   return *this;
}

inline QPoint &QPoint::operator*=(int c)
{
   xp = xp * c;
   yp = yp * c;
   return *this;
}

inline bool operator==(const QPoint &p1, const QPoint &p2)
{
   return p1.xp == p2.xp && p1.yp == p2.yp;
}

inline bool operator!=(const QPoint &p1, const QPoint &p2)
{
   return p1.xp != p2.xp || p1.yp != p2.yp;
}

inline const QPoint operator+(const QPoint &p1, const QPoint &p2)
{
   return QPoint(p1.xp + p2.xp, p1.yp + p2.yp);
}

inline const QPoint operator-(const QPoint &p1, const QPoint &p2)
{
   return QPoint(p1.xp - p2.xp, p1.yp - p2.yp);
}

inline const QPoint operator*(const QPoint &p, float c)
{
   return QPoint(qRound(p.xp * c), qRound(p.yp * c));
}

inline const QPoint operator*(const QPoint &p, double c)
{
   return QPoint(qRound(p.xp * c), qRound(p.yp * c));
}

inline const QPoint operator*(const QPoint &p, int c)
{
   return QPoint(p.xp * c, p.yp * c);
}

inline const QPoint operator*(float c, const QPoint &p)
{
   return QPoint(qRound(p.xp * c), qRound(p.yp * c));
}

inline const QPoint operator*(double c, const QPoint &p)
{
   return QPoint(qRound(p.xp * c), qRound(p.yp * c));
}

inline const QPoint operator*(int c, const QPoint &p)
{
   return QPoint(p.xp * c, p.yp * c);
}

inline const QPoint operator-(const QPoint &p)
{
   return QPoint(-p.xp, -p.yp);
}

inline QPoint &QPoint::operator/=(qreal c)
{
   xp = qRound(xp / c);
   yp = qRound(yp / c);
   return *this;
}

inline const QPoint operator/(const QPoint &p, qreal c)
{
   return QPoint(qRound(p.xp / c), qRound(p.yp / c));
}

Q_CORE_EXPORT QDebug operator<<(QDebug, const QPoint &);

class Q_CORE_EXPORT QPointF
{
 public:
   QPointF();
   QPointF(const QPoint &p);
   QPointF(qreal xPos, qreal yPos);

   qreal manhattanLength() const;

   inline bool isNull() const;

   inline qreal x() const;
   inline qreal y() const;
   inline void setX(qreal xPos);
   inline void setY(qreal yPos);

   inline qreal &rx();
   inline qreal &ry();

   inline QPointF &operator+=(const QPointF &p);
   inline QPointF &operator-=(const QPointF &p);
   inline QPointF &operator*=(qreal c);
   inline QPointF &operator/=(qreal c);

   inline QPoint toPoint() const;

 private:
   qreal xp;
   qreal yp;

   friend inline bool operator==(const QPointF &, const QPointF &);
   friend inline bool operator!=(const QPointF &, const QPointF &);
   friend inline const QPointF operator+(const QPointF &, const QPointF &);
   friend inline const QPointF operator-(const QPointF &, const QPointF &);
   friend inline const QPointF operator*(qreal, const QPointF &);
   friend inline const QPointF operator*(const QPointF &, qreal);
   friend inline const QPointF operator-(const QPointF &);
   friend inline const QPointF operator/(const QPointF &, qreal);

   friend class QMatrix;
   friend class QTransform;
};

Q_CORE_EXPORT QDataStream &operator<<(QDataStream &stream, const QPointF &pointF);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &stream, QPointF &pointF);

inline QPointF::QPointF()
   : xp(0), yp(0)
{ }

inline QPointF::QPointF(qreal xPos, qreal yPos)
   : xp(xPos), yp(yPos)
{ }

inline QPointF::QPointF(const QPoint &p)
   : xp(p.x()), yp(p.y())
{ }

inline bool QPointF::isNull() const
{
   return qIsNull(xp) && qIsNull(yp);
}

inline qreal QPointF::x() const
{
   return xp;
}

inline qreal QPointF::y() const
{
   return yp;
}

inline void QPointF::setX(qreal xPos)
{
   xp = xPos;
}

inline void QPointF::setY(qreal yPos)
{
   yp = yPos;
}

inline qreal &QPointF::rx()
{
   return xp;
}

inline qreal &QPointF::ry()
{
   return yp;
}

inline QPointF &QPointF::operator+=(const QPointF &p)
{
   xp += p.xp;
   yp += p.yp;
   return *this;
}

inline QPointF &QPointF::operator-=(const QPointF &p)
{
   xp -= p.xp;
   yp -= p.yp;
   return *this;
}

inline QPointF &QPointF::operator*=(qreal c)
{
   xp *= c;
   yp *= c;
   return *this;
}

inline bool operator==(const QPointF &p1, const QPointF &p2)
{
   return qFuzzyIsNull(p1.xp - p2.xp) && qFuzzyIsNull(p1.yp - p2.yp);
}

inline bool operator!=(const QPointF &p1, const QPointF &p2)
{
   return !qFuzzyIsNull(p1.xp - p2.xp) || !qFuzzyIsNull(p1.yp - p2.yp);
}

inline const QPointF operator+(const QPointF &p1, const QPointF &p2)
{
   return QPointF(p1.xp + p2.xp, p1.yp + p2.yp);
}

inline const QPointF operator-(const QPointF &p1, const QPointF &p2)
{
   return QPointF(p1.xp - p2.xp, p1.yp - p2.yp);
}

inline const QPointF operator*(const QPointF &p, qreal c)
{
   return QPointF(p.xp * c, p.yp * c);
}

inline const QPointF operator*(qreal c, const QPointF &p)
{
   return QPointF(p.xp * c, p.yp * c);
}

inline const QPointF operator-(const QPointF &p)
{
   return QPointF(-p.xp, -p.yp);
}

inline QPointF &QPointF::operator/=(qreal c)
{
   xp /= c;
   yp /= c;
   return *this;
}

inline const QPointF operator/(const QPointF &p, qreal c)
{
   return QPointF(p.xp / c, p.yp / c);
}

inline QPoint QPointF::toPoint() const
{
   return QPoint(qRound(xp), qRound(yp));
}

Q_CORE_EXPORT QDebug operator<<(QDebug d, const QPointF &p);


#endif // QPOINT_H
