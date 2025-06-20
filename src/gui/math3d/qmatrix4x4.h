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

#ifndef QMATRIX4X4_H
#define QMATRIX4X4_H

#include <qgenericmatrix.h>
#include <qquaternion.h>
#include <qrect.h>
#include <qvector3d.h>
#include <qvector4d.h>

#ifndef QT_NO_MATRIX4X4

class QMatrix;
class QTransform;
class QVariant;

class Q_GUI_EXPORT QMatrix4x4
{
 public:
   QMatrix4x4() {
      setToIdentity();
   }

   explicit QMatrix4x4(const qreal *values);

   inline QMatrix4x4(qreal m11, qreal m12, qreal m13, qreal m14,
                     qreal m21, qreal m22, qreal m23, qreal m24,
                     qreal m31, qreal m32, qreal m33, qreal m34,
                     qreal m41, qreal m42, qreal m43, qreal m44);

   template <int N, int M>
   explicit QMatrix4x4(const QGenericMatrix<N, M, qreal> &matrix);

   QMatrix4x4(const qreal *values, int cols, int rows);
   QMatrix4x4(const QTransform &transform);
   QMatrix4x4(const QMatrix &matrix);

   inline const qreal &operator()(int row, int column) const;
   inline qreal &operator()(int row, int column);

#ifndef QT_NO_VECTOR4D
   inline QVector4D column(int index) const;
   inline void setColumn(int index, const QVector4D &value);

   inline QVector4D row(int index) const;
   inline void setRow(int index, const QVector4D &value);
#endif

   inline bool isAffine() const;
   inline bool isIdentity() const;
   inline void setToIdentity();

   inline void fill(qreal value);

   qreal determinant() const;
   QMatrix4x4 inverted(bool *invertible = nullptr) const;
   QMatrix4x4 transposed() const;
   QMatrix3x3 normalMatrix() const;

   inline QMatrix4x4 &operator+=(const QMatrix4x4 &other);
   inline QMatrix4x4 &operator-=(const QMatrix4x4 &other);
   inline QMatrix4x4 &operator*=(QMatrix4x4 other);
   inline QMatrix4x4 &operator*=(qreal factor);

   QMatrix4x4 &operator/=(qreal divisor);
   inline bool operator==(const QMatrix4x4 &other) const;
   inline bool operator!=(const QMatrix4x4 &other) const;

   friend QMatrix4x4 operator+(const QMatrix4x4 &m1, const QMatrix4x4 &m2);
   friend QMatrix4x4 operator-(const QMatrix4x4 &m1, const QMatrix4x4 &m2);
   friend QMatrix4x4 operator*(const QMatrix4x4 &m1, const QMatrix4x4 &m2);

#ifndef QT_NO_VECTOR3D
   friend QVector3D operator*(const QMatrix4x4 &matrix, const QVector3D &vector);
   friend QVector3D operator*(const QVector3D &vector, const QMatrix4x4 &matrix);
#endif

#ifndef QT_NO_VECTOR4D
   friend QVector4D operator*(const QVector4D &vector, const QMatrix4x4 &matrix);
   friend QVector4D operator*(const QMatrix4x4 &matrix, const QVector4D &vector);
#endif

   friend QPoint operator*(const QPoint &point, const QMatrix4x4 &matrix);
   friend QPointF operator*(const QPointF &point, const QMatrix4x4 &matrix);
   friend QMatrix4x4 operator-(const QMatrix4x4 &matrix);
   friend QPoint operator*(const QMatrix4x4 &matrix, const QPoint &point);
   friend QPointF operator*(const QMatrix4x4 &matrix, const QPointF &point);
   friend QMatrix4x4 operator*(qreal factor, const QMatrix4x4 &matrix);
   friend QMatrix4x4 operator*(const QMatrix4x4 &matrix, qreal factor);
   friend Q_GUI_EXPORT QMatrix4x4 operator/(const QMatrix4x4 &matrix, qreal divisor);

   friend inline bool qFuzzyCompare(const QMatrix4x4 &m1, const QMatrix4x4 &m2);

#ifndef QT_NO_VECTOR3D
   void scale(const QVector3D &vector);
   void translate(const QVector3D &vector);
   void rotate(qreal angle, const QVector3D &vector);
#endif

   void scale(qreal x, qreal y);
   void scale(qreal x, qreal y, qreal z);
   void scale(qreal factor);
   void translate(qreal x, qreal y);
   void translate(qreal x, qreal y, qreal z);
   void rotate(qreal angle, qreal x, qreal y, qreal z = 0.0f);

#ifndef QT_NO_QUATERNION
   void rotate(const QQuaternion &quaternion);
#endif

   void ortho(const QRect &rect);
   void ortho(const QRectF &rect);
   void ortho(qreal left, qreal right, qreal bottom, qreal top, qreal nearPlane, qreal farPlane);
   void frustum(qreal left, qreal right, qreal bottom, qreal top, qreal nearPlane, qreal farPlane);
   void perspective(qreal angle, qreal aspect, qreal nearPlane, qreal farPlane);

#ifndef QT_NO_VECTOR3D
   void lookAt(const QVector3D &eye, const QVector3D &center, const QVector3D &up);
#endif

   void viewport(const QRectF &rect);
   void viewport(qreal left, qreal bottom, qreal width, qreal height, qreal nearPlane = 0.0, qreal farPlane = 1.0);
   void flipCoordinates();

   void copyDataTo(qreal *values) const;

   QMatrix toAffine() const;
   QTransform toTransform() const;
   QTransform toTransform(qreal distanceToPlane) const;

   QPoint map(const QPoint &point) const;
   QPointF map(const QPointF &point) const;

#ifndef QT_NO_VECTOR3D
   QVector3D map(const QVector3D &point) const;
   QVector3D mapVector(const QVector3D &vector) const;
#endif

#ifndef QT_NO_VECTOR4D
   QVector4D map(const QVector4D &point) const;
#endif

   QRect mapRect(const QRect &rect) const;
   QRectF mapRect(const QRectF &rect) const;

   template <int N, int M>
   QGenericMatrix<N, M, qreal> toGenericMatrix() const;

   inline qreal *data();

   const qreal *data() const {
      return *m_matrix4;
   }

   const qreal *constData() const {
      return *m_matrix4;
   }

   void optimize();

   operator QVariant() const;

   friend Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QMatrix4x4 &m);

 private:
   enum MatrixType {
      Identity        = 0x0000,   // Identity matrix
      Translation     = 0x0001,   // Contains a simple translation
      Scale           = 0x0002,   // Contains a simple scale
      Rotation2D      = 0x0004,   // Contains a rotation about the Z axis
      Rotation        = 0x0008,   // Contains a simple rotation
      Perspective     = 0x0010,   // Last row is different from (0, 0, 0, 1)
      General         = 0x001f    // General matrix, unknown contents
   };

   qreal m_matrix4[4][4];         // Column-major order to match OpenGL.
   int flagBits;                  // Flag bits from the MatrixType enum

   // Construct without initializing identity matrix
   explicit QMatrix4x4(int) {
   }

   QMatrix4x4 orthonormalInverse() const;

   void projectedRotate(qreal angle, qreal x, qreal y, qreal z);

   friend class QGraphicsRotation;
};

inline QMatrix4x4::QMatrix4x4(qreal m11, qreal m12, qreal m13, qreal m14,
      qreal m21, qreal m22, qreal m23, qreal m24,
      qreal m31, qreal m32, qreal m33, qreal m34,
      qreal m41, qreal m42, qreal m43, qreal m44)
{
   m_matrix4[0][0] = m11;
   m_matrix4[0][1] = m21;
   m_matrix4[0][2] = m31;
   m_matrix4[0][3] = m41;
   m_matrix4[1][0] = m12;
   m_matrix4[1][1] = m22;
   m_matrix4[1][2] = m32;
   m_matrix4[1][3] = m42;
   m_matrix4[2][0] = m13;
   m_matrix4[2][1] = m23;
   m_matrix4[2][2] = m33;
   m_matrix4[2][3] = m43;
   m_matrix4[3][0] = m14;
   m_matrix4[3][1] = m24;
   m_matrix4[3][2] = m34;
   m_matrix4[3][3] = m44;

   flagBits = General;
}

template <int N, int M>
inline QMatrix4x4::QMatrix4x4(const QGenericMatrix<N, M, qreal> &matrix)
{
   const qreal *values = matrix.constData();

   for (int matrixCol = 0; matrixCol < 4; ++matrixCol) {
      for (int matrixRow = 0; matrixRow < 4; ++matrixRow) {
         if (matrixCol < N && matrixRow < M) {
            m_matrix4[matrixCol][matrixRow] = values[matrixCol * M + matrixRow];

         } else if (matrixCol == matrixRow) {
            m_matrix4[matrixCol][matrixRow] = 1.0f;

         } else {
            m_matrix4[matrixCol][matrixRow] = 0.0f;
         }
      }
   }

   flagBits = General;
}

template <int N, int M>
QGenericMatrix<N, M, qreal> QMatrix4x4::toGenericMatrix() const
{
   QGenericMatrix<N, M, qreal> result;
   qreal *values = result.data();

   for (int matrixCol = 0; matrixCol < N; ++matrixCol) {
      for (int matrixRow = 0; matrixRow < M; ++matrixRow) {
         if (matrixCol < 4 && matrixRow < 4) {
            values[matrixCol * M + matrixRow] = m_matrix4[matrixCol][matrixRow];

         } else if (matrixCol == matrixRow) {
            values[matrixCol * M + matrixRow] = 1.0f;

         } else {
            values[matrixCol * M + matrixRow] = 0.0f;
         }
      }
   }
   return result;
}

inline const qreal &QMatrix4x4::operator()(int row, int column) const
{
   Q_ASSERT(row >= 0 && row < 4 && column >= 0 && column < 4);
   return m_matrix4[column][row];
}

inline qreal &QMatrix4x4::operator()(int row, int column)
{
   Q_ASSERT(row >= 0 && row < 4 && column >= 0 && column < 4);
   flagBits = General;

   return m_matrix4[column][row];
}

#ifndef QT_NO_VECTOR4D
inline QVector4D QMatrix4x4::column(int index) const
{
   Q_ASSERT(index >= 0 && index < 4);
   return QVector4D(m_matrix4[index][0], m_matrix4[index][1], m_matrix4[index][2], m_matrix4[index][3]);
}

inline void QMatrix4x4::setColumn(int index, const QVector4D &value)
{
   Q_ASSERT(index >= 0 && index < 4);

   m_matrix4[index][0] = value.x();
   m_matrix4[index][1] = value.y();
   m_matrix4[index][2] = value.z();
   m_matrix4[index][3] = value.w();

   flagBits = General;
}

inline QVector4D QMatrix4x4::row(int index) const
{
   Q_ASSERT(index >= 0 && index < 4);
   return QVector4D(m_matrix4[0][index], m_matrix4[1][index], m_matrix4[2][index], m_matrix4[3][index]);
}

inline void QMatrix4x4::setRow(int index, const QVector4D &value)
{
   Q_ASSERT(index >= 0 && index < 4);

   m_matrix4[0][index] = value.x();
   m_matrix4[1][index] = value.y();
   m_matrix4[2][index] = value.z();
   m_matrix4[3][index] = value.w();

   flagBits = General;
}
#endif

Q_GUI_EXPORT QMatrix4x4 operator/(const QMatrix4x4 &matrix, qreal divisor);

inline bool QMatrix4x4::isAffine() const
{
    return m_matrix4[0][3] == 0.0f && m_matrix4[1][3] == 0.0f && m_matrix4[2][3] == 0.0f && m_matrix4[3][3] == 1.0f;
}

inline bool QMatrix4x4::isIdentity() const
{
   if (flagBits == Identity) {
      return true;
   }

   if (m_matrix4[0][0] != 1.0f || m_matrix4[0][1] != 0.0f || m_matrix4[0][2] != 0.0f) {
      return false;
   }

   if (m_matrix4[0][3] != 0.0f || m_matrix4[1][0] != 0.0f || m_matrix4[1][1] != 1.0f) {
      return false;
   }

   if (m_matrix4[1][2] != 0.0f || m_matrix4[1][3] != 0.0f || m_matrix4[2][0] != 0.0f) {
      return false;
   }

   if (m_matrix4[2][1] != 0.0f || m_matrix4[2][2] != 1.0f || m_matrix4[2][3] != 0.0f) {
      return false;
   }

   if (m_matrix4[3][0] != 0.0f || m_matrix4[3][1] != 0.0f || m_matrix4[3][2] != 0.0f) {
      return false;
   }

   return (m_matrix4[3][3] == 1.0f);
}

inline void QMatrix4x4::setToIdentity()
{
   m_matrix4[0][0] = 1.0f;
   m_matrix4[0][1] = 0.0f;
   m_matrix4[0][2] = 0.0f;
   m_matrix4[0][3] = 0.0f;
   m_matrix4[1][0] = 0.0f;
   m_matrix4[1][1] = 1.0f;
   m_matrix4[1][2] = 0.0f;
   m_matrix4[1][3] = 0.0f;
   m_matrix4[2][0] = 0.0f;
   m_matrix4[2][1] = 0.0f;
   m_matrix4[2][2] = 1.0f;
   m_matrix4[2][3] = 0.0f;
   m_matrix4[3][0] = 0.0f;
   m_matrix4[3][1] = 0.0f;
   m_matrix4[3][2] = 0.0f;
   m_matrix4[3][3] = 1.0f;

   flagBits = Identity;
}

inline void QMatrix4x4::fill(qreal value)
{
   m_matrix4[0][0] = value;
   m_matrix4[0][1] = value;
   m_matrix4[0][2] = value;
   m_matrix4[0][3] = value;
   m_matrix4[1][0] = value;
   m_matrix4[1][1] = value;
   m_matrix4[1][2] = value;
   m_matrix4[1][3] = value;
   m_matrix4[2][0] = value;
   m_matrix4[2][1] = value;
   m_matrix4[2][2] = value;
   m_matrix4[2][3] = value;
   m_matrix4[3][0] = value;
   m_matrix4[3][1] = value;
   m_matrix4[3][2] = value;
   m_matrix4[3][3] = value;

   flagBits = General;
}

inline QMatrix4x4 &QMatrix4x4::operator+=(const QMatrix4x4 &other)
{
   m_matrix4[0][0] += other.m_matrix4[0][0];
   m_matrix4[0][1] += other.m_matrix4[0][1];
   m_matrix4[0][2] += other.m_matrix4[0][2];
   m_matrix4[0][3] += other.m_matrix4[0][3];
   m_matrix4[1][0] += other.m_matrix4[1][0];
   m_matrix4[1][1] += other.m_matrix4[1][1];
   m_matrix4[1][2] += other.m_matrix4[1][2];
   m_matrix4[1][3] += other.m_matrix4[1][3];
   m_matrix4[2][0] += other.m_matrix4[2][0];
   m_matrix4[2][1] += other.m_matrix4[2][1];
   m_matrix4[2][2] += other.m_matrix4[2][2];
   m_matrix4[2][3] += other.m_matrix4[2][3];
   m_matrix4[3][0] += other.m_matrix4[3][0];
   m_matrix4[3][1] += other.m_matrix4[3][1];
   m_matrix4[3][2] += other.m_matrix4[3][2];
   m_matrix4[3][3] += other.m_matrix4[3][3];

   flagBits = General;

   return *this;
}

inline QMatrix4x4 &QMatrix4x4::operator-=(const QMatrix4x4 &other)
{
   m_matrix4[0][0] -= other.m_matrix4[0][0];
   m_matrix4[0][1] -= other.m_matrix4[0][1];
   m_matrix4[0][2] -= other.m_matrix4[0][2];
   m_matrix4[0][3] -= other.m_matrix4[0][3];
   m_matrix4[1][0] -= other.m_matrix4[1][0];
   m_matrix4[1][1] -= other.m_matrix4[1][1];
   m_matrix4[1][2] -= other.m_matrix4[1][2];
   m_matrix4[1][3] -= other.m_matrix4[1][3];
   m_matrix4[2][0] -= other.m_matrix4[2][0];
   m_matrix4[2][1] -= other.m_matrix4[2][1];
   m_matrix4[2][2] -= other.m_matrix4[2][2];
   m_matrix4[2][3] -= other.m_matrix4[2][3];
   m_matrix4[3][0] -= other.m_matrix4[3][0];
   m_matrix4[3][1] -= other.m_matrix4[3][1];
   m_matrix4[3][2] -= other.m_matrix4[3][2];
   m_matrix4[3][3] -= other.m_matrix4[3][3];

   flagBits = General;

   return *this;
}

inline QMatrix4x4 &QMatrix4x4::operator*=(QMatrix4x4 other)
{
    flagBits |= other.flagBits;

    if (flagBits < Rotation2D) {
        m_matrix4[3][0] += m_matrix4[0][0] * other.m_matrix4[3][0];
        m_matrix4[3][1] += m_matrix4[1][1] * other.m_matrix4[3][1];
        m_matrix4[3][2] += m_matrix4[2][2] * other.m_matrix4[3][2];

        m_matrix4[0][0] *= other.m_matrix4[0][0];
        m_matrix4[1][1] *= other.m_matrix4[1][1];
        m_matrix4[2][2] *= other.m_matrix4[2][2];

        return *this;
    }

    qreal m0;
    qreal m1;
    qreal m2;

    m0 = m_matrix4[0][0] * other.m_matrix4[0][0]
          + m_matrix4[1][0] * other.m_matrix4[0][1]
          + m_matrix4[2][0] * other.m_matrix4[0][2]
          + m_matrix4[3][0] * other.m_matrix4[0][3];

    m1 = m_matrix4[0][0] * other.m_matrix4[1][0] + m_matrix4[1][0] * other.m_matrix4[1][1]
          + m_matrix4[2][0] * other.m_matrix4[1][2]
          + m_matrix4[3][0] * other.m_matrix4[1][3];

    m2 = m_matrix4[0][0] * other.m_matrix4[2][0]
          + m_matrix4[1][0] * other.m_matrix4[2][1]
          + m_matrix4[2][0] * other.m_matrix4[2][2]
          + m_matrix4[3][0] * other.m_matrix4[2][3];

    m_matrix4[3][0] = m_matrix4[0][0] * other.m_matrix4[3][0]
          + m_matrix4[1][0] * other.m_matrix4[3][1]
          + m_matrix4[2][0] * other.m_matrix4[3][2]
          + m_matrix4[3][0] * other.m_matrix4[3][3];

    m_matrix4[0][0] = m0;
    m_matrix4[1][0] = m1;
    m_matrix4[2][0] = m2;

    m0 = m_matrix4[0][1] * other.m_matrix4[0][0]
          + m_matrix4[1][1] * other.m_matrix4[0][1]
          + m_matrix4[2][1] * other.m_matrix4[0][2]
          + m_matrix4[3][1] * other.m_matrix4[0][3];

    m1 = m_matrix4[0][1] * other.m_matrix4[1][0]
          + m_matrix4[1][1] * other.m_matrix4[1][1]
          + m_matrix4[2][1] * other.m_matrix4[1][2]
          + m_matrix4[3][1] * other.m_matrix4[1][3];

    m2 = m_matrix4[0][1] * other.m_matrix4[2][0]
          + m_matrix4[1][1] * other.m_matrix4[2][1]
          + m_matrix4[2][1] * other.m_matrix4[2][2]
          + m_matrix4[3][1] * other.m_matrix4[2][3];

    m_matrix4[3][1] = m_matrix4[0][1] * other.m_matrix4[3][0]
          + m_matrix4[1][1] * other.m_matrix4[3][1]
          + m_matrix4[2][1] * other.m_matrix4[3][2]
          + m_matrix4[3][1] * other.m_matrix4[3][3];

    m_matrix4[0][1] = m0;
    m_matrix4[1][1] = m1;
    m_matrix4[2][1] = m2;

    m0 = m_matrix4[0][2] * other.m_matrix4[0][0]
          + m_matrix4[1][2] * other.m_matrix4[0][1]
          + m_matrix4[2][2] * other.m_matrix4[0][2]
          + m_matrix4[3][2] * other.m_matrix4[0][3];

    m1 = m_matrix4[0][2] * other.m_matrix4[1][0]
          + m_matrix4[1][2] * other.m_matrix4[1][1]
          + m_matrix4[2][2] * other.m_matrix4[1][2]
          + m_matrix4[3][2] * other.m_matrix4[1][3];

    m2 = m_matrix4[0][2] * other.m_matrix4[2][0]
          + m_matrix4[1][2] * other.m_matrix4[2][1]
          + m_matrix4[2][2] * other.m_matrix4[2][2]
          + m_matrix4[3][2] * other.m_matrix4[2][3];

    m_matrix4[3][2] = m_matrix4[0][2] * other.m_matrix4[3][0]
          + m_matrix4[1][2] * other.m_matrix4[3][1]
          + m_matrix4[2][2] * other.m_matrix4[3][2]
          + m_matrix4[3][2] * other.m_matrix4[3][3];

    m_matrix4[0][2] = m0;
    m_matrix4[1][2] = m1;
    m_matrix4[2][2] = m2;

    m0 = m_matrix4[0][3] * other.m_matrix4[0][0]
          + m_matrix4[1][3] * other.m_matrix4[0][1]
          + m_matrix4[2][3] * other.m_matrix4[0][2]
          + m_matrix4[3][3] * other.m_matrix4[0][3];

    m1 = m_matrix4[0][3] * other.m_matrix4[1][0]
          + m_matrix4[1][3] * other.m_matrix4[1][1]
          + m_matrix4[2][3] * other.m_matrix4[1][2]
          + m_matrix4[3][3] * other.m_matrix4[1][3];

    m2 = m_matrix4[0][3] * other.m_matrix4[2][0]
          + m_matrix4[1][3] * other.m_matrix4[2][1]
          + m_matrix4[2][3] * other.m_matrix4[2][2]
          + m_matrix4[3][3] * other.m_matrix4[2][3];

    m_matrix4[3][3] = m_matrix4[0][3] * other.m_matrix4[3][0]
          + m_matrix4[1][3] * other.m_matrix4[3][1]
          + m_matrix4[2][3] * other.m_matrix4[3][2]
          + m_matrix4[3][3] * other.m_matrix4[3][3];

    m_matrix4[0][3] = m0;
    m_matrix4[1][3] = m1;
    m_matrix4[2][3] = m2;

    return *this;
}

inline QMatrix4x4 &QMatrix4x4::operator*=(qreal factor)
{
   m_matrix4[0][0] *= factor;
   m_matrix4[0][1] *= factor;
   m_matrix4[0][2] *= factor;
   m_matrix4[0][3] *= factor;
   m_matrix4[1][0] *= factor;
   m_matrix4[1][1] *= factor;
   m_matrix4[1][2] *= factor;
   m_matrix4[1][3] *= factor;
   m_matrix4[2][0] *= factor;
   m_matrix4[2][1] *= factor;
   m_matrix4[2][2] *= factor;
   m_matrix4[2][3] *= factor;
   m_matrix4[3][0] *= factor;
   m_matrix4[3][1] *= factor;
   m_matrix4[3][2] *= factor;
   m_matrix4[3][3] *= factor;

   flagBits = General;

   return *this;
}

inline bool QMatrix4x4::operator==(const QMatrix4x4 &other) const
{
   return m_matrix4[0][0] == other.m_matrix4[0][0] &&
          m_matrix4[0][1] == other.m_matrix4[0][1] &&
          m_matrix4[0][2] == other.m_matrix4[0][2] &&
          m_matrix4[0][3] == other.m_matrix4[0][3] &&
          m_matrix4[1][0] == other.m_matrix4[1][0] &&
          m_matrix4[1][1] == other.m_matrix4[1][1] &&
          m_matrix4[1][2] == other.m_matrix4[1][2] &&
          m_matrix4[1][3] == other.m_matrix4[1][3] &&
          m_matrix4[2][0] == other.m_matrix4[2][0] &&
          m_matrix4[2][1] == other.m_matrix4[2][1] &&
          m_matrix4[2][2] == other.m_matrix4[2][2] &&
          m_matrix4[2][3] == other.m_matrix4[2][3] &&
          m_matrix4[3][0] == other.m_matrix4[3][0] &&
          m_matrix4[3][1] == other.m_matrix4[3][1] &&
          m_matrix4[3][2] == other.m_matrix4[3][2] &&
          m_matrix4[3][3] == other.m_matrix4[3][3];
}

inline bool QMatrix4x4::operator!=(const QMatrix4x4 &other) const
{
   return m_matrix4[0][0] != other.m_matrix4[0][0] ||
          m_matrix4[0][1] != other.m_matrix4[0][1] ||
          m_matrix4[0][2] != other.m_matrix4[0][2] ||
          m_matrix4[0][3] != other.m_matrix4[0][3] ||
          m_matrix4[1][0] != other.m_matrix4[1][0] ||
          m_matrix4[1][1] != other.m_matrix4[1][1] ||
          m_matrix4[1][2] != other.m_matrix4[1][2] ||
          m_matrix4[1][3] != other.m_matrix4[1][3] ||
          m_matrix4[2][0] != other.m_matrix4[2][0] ||
          m_matrix4[2][1] != other.m_matrix4[2][1] ||
          m_matrix4[2][2] != other.m_matrix4[2][2] ||
          m_matrix4[2][3] != other.m_matrix4[2][3] ||
          m_matrix4[3][0] != other.m_matrix4[3][0] ||
          m_matrix4[3][1] != other.m_matrix4[3][1] ||
          m_matrix4[3][2] != other.m_matrix4[3][2] ||
          m_matrix4[3][3] != other.m_matrix4[3][3];
}

inline QMatrix4x4 operator+(const QMatrix4x4 &m1, const QMatrix4x4 &m2)
{
   QMatrix4x4 m(1);

   m.m_matrix4[0][0] = m1.m_matrix4[0][0] + m2.m_matrix4[0][0];
   m.m_matrix4[0][1] = m1.m_matrix4[0][1] + m2.m_matrix4[0][1];
   m.m_matrix4[0][2] = m1.m_matrix4[0][2] + m2.m_matrix4[0][2];
   m.m_matrix4[0][3] = m1.m_matrix4[0][3] + m2.m_matrix4[0][3];
   m.m_matrix4[1][0] = m1.m_matrix4[1][0] + m2.m_matrix4[1][0];
   m.m_matrix4[1][1] = m1.m_matrix4[1][1] + m2.m_matrix4[1][1];
   m.m_matrix4[1][2] = m1.m_matrix4[1][2] + m2.m_matrix4[1][2];
   m.m_matrix4[1][3] = m1.m_matrix4[1][3] + m2.m_matrix4[1][3];
   m.m_matrix4[2][0] = m1.m_matrix4[2][0] + m2.m_matrix4[2][0];
   m.m_matrix4[2][1] = m1.m_matrix4[2][1] + m2.m_matrix4[2][1];
   m.m_matrix4[2][2] = m1.m_matrix4[2][2] + m2.m_matrix4[2][2];
   m.m_matrix4[2][3] = m1.m_matrix4[2][3] + m2.m_matrix4[2][3];
   m.m_matrix4[3][0] = m1.m_matrix4[3][0] + m2.m_matrix4[3][0];
   m.m_matrix4[3][1] = m1.m_matrix4[3][1] + m2.m_matrix4[3][1];
   m.m_matrix4[3][2] = m1.m_matrix4[3][2] + m2.m_matrix4[3][2];
   m.m_matrix4[3][3] = m1.m_matrix4[3][3] + m2.m_matrix4[3][3];

   m.flagBits = QMatrix4x4::General;

   return m;
}

inline QMatrix4x4 operator-(const QMatrix4x4 &m1, const QMatrix4x4 &m2)
{
   QMatrix4x4 m(1);

   m.m_matrix4[0][0] = m1.m_matrix4[0][0] - m2.m_matrix4[0][0];
   m.m_matrix4[0][1] = m1.m_matrix4[0][1] - m2.m_matrix4[0][1];
   m.m_matrix4[0][2] = m1.m_matrix4[0][2] - m2.m_matrix4[0][2];
   m.m_matrix4[0][3] = m1.m_matrix4[0][3] - m2.m_matrix4[0][3];
   m.m_matrix4[1][0] = m1.m_matrix4[1][0] - m2.m_matrix4[1][0];
   m.m_matrix4[1][1] = m1.m_matrix4[1][1] - m2.m_matrix4[1][1];
   m.m_matrix4[1][2] = m1.m_matrix4[1][2] - m2.m_matrix4[1][2];
   m.m_matrix4[1][3] = m1.m_matrix4[1][3] - m2.m_matrix4[1][3];
   m.m_matrix4[2][0] = m1.m_matrix4[2][0] - m2.m_matrix4[2][0];
   m.m_matrix4[2][1] = m1.m_matrix4[2][1] - m2.m_matrix4[2][1];
   m.m_matrix4[2][2] = m1.m_matrix4[2][2] - m2.m_matrix4[2][2];
   m.m_matrix4[2][3] = m1.m_matrix4[2][3] - m2.m_matrix4[2][3];
   m.m_matrix4[3][0] = m1.m_matrix4[3][0] - m2.m_matrix4[3][0];
   m.m_matrix4[3][1] = m1.m_matrix4[3][1] - m2.m_matrix4[3][1];
   m.m_matrix4[3][2] = m1.m_matrix4[3][2] - m2.m_matrix4[3][2];
   m.m_matrix4[3][3] = m1.m_matrix4[3][3] - m2.m_matrix4[3][3];

   m.flagBits = QMatrix4x4::General;

   return m;
}

inline QMatrix4x4 operator*(const QMatrix4x4 &m1, const QMatrix4x4 &m2)
{
    int flagBits = m1.flagBits | m2.flagBits;

    if (flagBits < QMatrix4x4::Rotation2D) {
        QMatrix4x4 m = m1;

        m.m_matrix4[3][0] += m.m_matrix4[0][0] * m2.m_matrix4[3][0];
        m.m_matrix4[3][1] += m.m_matrix4[1][1] * m2.m_matrix4[3][1];
        m.m_matrix4[3][2] += m.m_matrix4[2][2] * m2.m_matrix4[3][2];

        m.m_matrix4[0][0] *= m2.m_matrix4[0][0];
        m.m_matrix4[1][1] *= m2.m_matrix4[1][1];
        m.m_matrix4[2][2] *= m2.m_matrix4[2][2];

        m.flagBits = flagBits;

        return m;
   }

   QMatrix4x4 m(1);

   m.m_matrix4[0][0] = m1.m_matrix4[0][0] * m2.m_matrix4[0][0] +
         m1.m_matrix4[1][0] * m2.m_matrix4[0][1] +
         m1.m_matrix4[2][0] * m2.m_matrix4[0][2] +
         m1.m_matrix4[3][0] * m2.m_matrix4[0][3];

   m.m_matrix4[0][1] = m1.m_matrix4[0][1] * m2.m_matrix4[0][0] +
         m1.m_matrix4[1][1] * m2.m_matrix4[0][1] +
         m1.m_matrix4[2][1] * m2.m_matrix4[0][2] +
         m1.m_matrix4[3][1] * m2.m_matrix4[0][3];

   m.m_matrix4[0][2] = m1.m_matrix4[0][2] * m2.m_matrix4[0][0] +
         m1.m_matrix4[1][2] * m2.m_matrix4[0][1] +
         m1.m_matrix4[2][2] * m2.m_matrix4[0][2] +
         m1.m_matrix4[3][2] * m2.m_matrix4[0][3];

   m.m_matrix4[0][3] = m1.m_matrix4[0][3] * m2.m_matrix4[0][0] +
         m1.m_matrix4[1][3] * m2.m_matrix4[0][1] +
         m1.m_matrix4[2][3] * m2.m_matrix4[0][2] +
         m1.m_matrix4[3][3] * m2.m_matrix4[0][3];

   m.m_matrix4[1][0] = m1.m_matrix4[0][0] * m2.m_matrix4[1][0] +
         m1.m_matrix4[1][0] * m2.m_matrix4[1][1] +
         m1.m_matrix4[2][0] * m2.m_matrix4[1][2] +
         m1.m_matrix4[3][0] * m2.m_matrix4[1][3];

   m.m_matrix4[1][1] = m1.m_matrix4[0][1] * m2.m_matrix4[1][0] +
         m1.m_matrix4[1][1] * m2.m_matrix4[1][1] +
         m1.m_matrix4[2][1] * m2.m_matrix4[1][2] +
         m1.m_matrix4[3][1] * m2.m_matrix4[1][3];

   m.m_matrix4[1][2] = m1.m_matrix4[0][2] * m2.m_matrix4[1][0] +
         m1.m_matrix4[1][2] * m2.m_matrix4[1][1] +
         m1.m_matrix4[2][2] * m2.m_matrix4[1][2] +
         m1.m_matrix4[3][2] * m2.m_matrix4[1][3];

   m.m_matrix4[1][3] = m1.m_matrix4[0][3] * m2.m_matrix4[1][0] +
         m1.m_matrix4[1][3] * m2.m_matrix4[1][1] +
         m1.m_matrix4[2][3] * m2.m_matrix4[1][2] +
         m1.m_matrix4[3][3] * m2.m_matrix4[1][3];

   m.m_matrix4[2][0] = m1.m_matrix4[0][0] * m2.m_matrix4[2][0] +
         m1.m_matrix4[1][0] * m2.m_matrix4[2][1] +
         m1.m_matrix4[2][0] * m2.m_matrix4[2][2] +
         m1.m_matrix4[3][0] * m2.m_matrix4[2][3];

   m.m_matrix4[2][1] = m1.m_matrix4[0][1] * m2.m_matrix4[2][0] +
         m1.m_matrix4[1][1] * m2.m_matrix4[2][1] +
         m1.m_matrix4[2][1] * m2.m_matrix4[2][2] +
         m1.m_matrix4[3][1] * m2.m_matrix4[2][3];

   m.m_matrix4[2][2] = m1.m_matrix4[0][2] * m2.m_matrix4[2][0] +
         m1.m_matrix4[1][2] * m2.m_matrix4[2][1] +
         m1.m_matrix4[2][2] * m2.m_matrix4[2][2] +
         m1.m_matrix4[3][2] * m2.m_matrix4[2][3];

   m.m_matrix4[2][3] = m1.m_matrix4[0][3] * m2.m_matrix4[2][0] +
         m1.m_matrix4[1][3] * m2.m_matrix4[2][1] +
         m1.m_matrix4[2][3] * m2.m_matrix4[2][2] +
         m1.m_matrix4[3][3] * m2.m_matrix4[2][3];

   m.m_matrix4[3][0] = m1.m_matrix4[0][0] * m2.m_matrix4[3][0] +
         m1.m_matrix4[1][0] * m2.m_matrix4[3][1] +
         m1.m_matrix4[2][0] * m2.m_matrix4[3][2] +
         m1.m_matrix4[3][0] * m2.m_matrix4[3][3];

   m.m_matrix4[3][1] = m1.m_matrix4[0][1] * m2.m_matrix4[3][0] +
         m1.m_matrix4[1][1] * m2.m_matrix4[3][1] +
         m1.m_matrix4[2][1] * m2.m_matrix4[3][2] +
         m1.m_matrix4[3][1] * m2.m_matrix4[3][3];

   m.m_matrix4[3][2] = m1.m_matrix4[0][2] * m2.m_matrix4[3][0] +
         m1.m_matrix4[1][2] * m2.m_matrix4[3][1] +
         m1.m_matrix4[2][2] * m2.m_matrix4[3][2] +
         m1.m_matrix4[3][2] * m2.m_matrix4[3][3];

   m.m_matrix4[3][3] = m1.m_matrix4[0][3] * m2.m_matrix4[3][0] +
         m1.m_matrix4[1][3] * m2.m_matrix4[3][1] +
         m1.m_matrix4[2][3] * m2.m_matrix4[3][2] +
         m1.m_matrix4[3][3] * m2.m_matrix4[3][3];

   m.flagBits = flagBits;

   return m;
}

#ifndef QT_NO_VECTOR3D

inline QVector3D operator*(const QVector3D &vector, const QMatrix4x4 &matrix)
{
   qreal x, y, z, w;

   x = vector.x() * matrix.m_matrix4[0][0] +
       vector.y() * matrix.m_matrix4[0][1] +
       vector.z() * matrix.m_matrix4[0][2] +
       matrix.m_matrix4[0][3];

   y = vector.x() * matrix.m_matrix4[1][0] +
       vector.y() * matrix.m_matrix4[1][1] +
       vector.z() * matrix.m_matrix4[1][2] +
       matrix.m_matrix4[1][3];

   z = vector.x() * matrix.m_matrix4[2][0] +
       vector.y() * matrix.m_matrix4[2][1] +
       vector.z() * matrix.m_matrix4[2][2] +
       matrix.m_matrix4[2][3];

   w = vector.x() * matrix.m_matrix4[3][0] +
       vector.y() * matrix.m_matrix4[3][1] +
       vector.z() * matrix.m_matrix4[3][2] +
       matrix.m_matrix4[3][3];

   if (w == 1.0f) {
      return QVector3D(x, y, z);
   } else {
      return QVector3D(x / w, y / w, z / w);
   }
}

inline QVector3D operator*(const QMatrix4x4 &matrix, const QVector3D &vector)
{
   qreal x, y, z, w;
   if (matrix.flagBits == QMatrix4x4::Identity) {
      return vector;

    } else if (matrix.flagBits < QMatrix4x4::Rotation2D) {
      // Translation | Scale
      return QVector3D(vector.x() * matrix.m_matrix4[0][0] + matrix.m_matrix4[3][0],
            vector.y() * matrix.m_matrix4[1][1] + matrix.m_matrix4[3][1],
            vector.z() * matrix.m_matrix4[2][2] + matrix.m_matrix4[3][2]);

    } else if (matrix.flagBits < QMatrix4x4::Rotation) {
       return QVector3D(vector.x() * matrix.m_matrix4[0][0] + vector.y() * matrix.m_matrix4[1][0] + matrix.m_matrix4[3][0],
            vector.x() * matrix.m_matrix4[0][1] + vector.y() * matrix.m_matrix4[1][1] + matrix.m_matrix4[3][1],
            vector.z() * matrix.m_matrix4[2][2] + matrix.m_matrix4[3][2]);

   } else {
      x = vector.x() * matrix.m_matrix4[0][0] +
          vector.y() * matrix.m_matrix4[1][0] +
          vector.z() * matrix.m_matrix4[2][0] +
          matrix.m_matrix4[3][0];

      y = vector.x() * matrix.m_matrix4[0][1] +
          vector.y() * matrix.m_matrix4[1][1] +
          vector.z() * matrix.m_matrix4[2][1] +
          matrix.m_matrix4[3][1];

      z = vector.x() * matrix.m_matrix4[0][2] +
          vector.y() * matrix.m_matrix4[1][2] +
          vector.z() * matrix.m_matrix4[2][2] +
          matrix.m_matrix4[3][2];

      w = vector.x() * matrix.m_matrix4[0][3] +
          vector.y() * matrix.m_matrix4[1][3] +
          vector.z() * matrix.m_matrix4[2][3] +
          matrix.m_matrix4[3][3];

      if (w == 1.0f) {
         return QVector3D(x, y, z);
      } else {
         return QVector3D(x / w, y / w, z / w);
      }
   }
}

#endif

#ifndef QT_NO_VECTOR4D

inline QVector4D operator*(const QVector4D &vector, const QMatrix4x4 &matrix)
{
   qreal x, y, z, w;

   x = vector.x() * matrix.m_matrix4[0][0] +
       vector.y() * matrix.m_matrix4[0][1] +
       vector.z() * matrix.m_matrix4[0][2] +
       vector.w() * matrix.m_matrix4[0][3];

   y = vector.x() * matrix.m_matrix4[1][0] +
       vector.y() * matrix.m_matrix4[1][1] +
       vector.z() * matrix.m_matrix4[1][2] +
       vector.w() * matrix.m_matrix4[1][3];

   z = vector.x() * matrix.m_matrix4[2][0] +
       vector.y() * matrix.m_matrix4[2][1] +
       vector.z() * matrix.m_matrix4[2][2] +
       vector.w() * matrix.m_matrix4[2][3];

   w = vector.x() * matrix.m_matrix4[3][0] +
       vector.y() * matrix.m_matrix4[3][1] +
       vector.z() * matrix.m_matrix4[3][2] +
       vector.w() * matrix.m_matrix4[3][3];

   return QVector4D(x, y, z, w);
}

inline QVector4D operator*(const QMatrix4x4 &matrix, const QVector4D &vector)
{
   qreal x, y, z, w;

   x = vector.x() * matrix.m_matrix4[0][0] +
       vector.y() * matrix.m_matrix4[1][0] +
       vector.z() * matrix.m_matrix4[2][0] +
       vector.w() * matrix.m_matrix4[3][0];

   y = vector.x() * matrix.m_matrix4[0][1] +
       vector.y() * matrix.m_matrix4[1][1] +
       vector.z() * matrix.m_matrix4[2][1] +
       vector.w() * matrix.m_matrix4[3][1];

   z = vector.x() * matrix.m_matrix4[0][2] +
       vector.y() * matrix.m_matrix4[1][2] +
       vector.z() * matrix.m_matrix4[2][2] +
       vector.w() * matrix.m_matrix4[3][2];

   w = vector.x() * matrix.m_matrix4[0][3] +
       vector.y() * matrix.m_matrix4[1][3] +
       vector.z() * matrix.m_matrix4[2][3] +
       vector.w() * matrix.m_matrix4[3][3];

   return QVector4D(x, y, z, w);
}

#endif

inline QPoint operator*(const QPoint &point, const QMatrix4x4 &matrix)
{
   qreal xin, yin;
   qreal x, y, w;

   xin = point.x();
   yin = point.y();

   x = xin * matrix.m_matrix4[0][0] +
       yin * matrix.m_matrix4[0][1] +
       matrix.m_matrix4[0][3];

   y = xin * matrix.m_matrix4[1][0] +
       yin * matrix.m_matrix4[1][1] +
       matrix.m_matrix4[1][3];

   w = xin * matrix.m_matrix4[3][0] +
       yin * matrix.m_matrix4[3][1] +
       matrix.m_matrix4[3][3];

   if (w == 1.0f) {
      return QPoint(qRound(x), qRound(y));
   } else {
      return QPoint(qRound(x / w), qRound(y / w));
   }
}

inline QPointF operator*(const QPointF &point, const QMatrix4x4 &matrix)
{
   qreal xin, yin;
   qreal x, y, w;

   xin = point.x();
   yin = point.y();

   x = xin * matrix.m_matrix4[0][0] +
       yin * matrix.m_matrix4[0][1] +
       matrix.m_matrix4[0][3];

   y = xin * matrix.m_matrix4[1][0] +
       yin * matrix.m_matrix4[1][1] +
       matrix.m_matrix4[1][3];

   w = xin * matrix.m_matrix4[3][0] +
       yin * matrix.m_matrix4[3][1] +
       matrix.m_matrix4[3][3];

   if (w == 1.0f) {
      return QPointF(qreal(x), qreal(y));
   } else {
      return QPointF(qreal(x / w), qreal(y / w));
   }
}

inline QPoint operator*(const QMatrix4x4 &matrix, const QPoint &point)
{
   qreal xin, yin;
   qreal x, y, w;

   xin = point.x();
   yin = point.y();

   if (matrix.flagBits == QMatrix4x4::Identity) {
      return point;

   } else if (matrix.flagBits < QMatrix4x4::Rotation2D) {
      // Translation | Scale
      return QPoint(qRound(xin * matrix.m_matrix4[0][0] + matrix.m_matrix4[3][0]),
            qRound(yin * matrix.m_matrix4[1][1] + matrix.m_matrix4[3][1]));

   } else if (matrix.flagBits < QMatrix4x4::Perspective) {
      return QPoint(qRound(xin * matrix.m_matrix4[0][0] + yin * matrix.m_matrix4[1][0] + matrix.m_matrix4[3][0]),
            qRound(xin * matrix.m_matrix4[0][1] + yin * matrix.m_matrix4[1][1] + matrix.m_matrix4[3][1]));

    } else {
        x = xin * matrix.m_matrix4[0][0] +
            yin * matrix.m_matrix4[1][0] +
            matrix.m_matrix4[3][0];

        y = xin * matrix.m_matrix4[0][1] +
            yin * matrix.m_matrix4[1][1] +
            matrix.m_matrix4[3][1];

        w = xin * matrix.m_matrix4[0][3] +
            yin * matrix.m_matrix4[1][3] +
            matrix.m_matrix4[3][3];

        if (w == 1.0f) {
           return QPoint(qRound(x), qRound(y));
        } else {
           return QPoint(qRound(x / w), qRound(y / w));
        }
    }
}

inline QPointF operator*(const QMatrix4x4 &matrix, const QPointF &point)
{
   qreal xin, yin;
   qreal x, y, w;

   xin = point.x();
   yin = point.y();

   if (matrix.flagBits == QMatrix4x4::Identity) {
      return point;

   } else if (matrix.flagBits < QMatrix4x4::Rotation2D) {
      // Translation | Scale
      return QPointF(xin * matrix.m_matrix4[0][0] + matrix.m_matrix4[3][0],
            yin * matrix.m_matrix4[1][1] + matrix.m_matrix4[3][1]);

   } else if (matrix.flagBits < QMatrix4x4::Perspective) {
      return QPointF(xin * matrix.m_matrix4[0][0] + yin * matrix.m_matrix4[1][0] + matrix.m_matrix4[3][0],
            xin * matrix.m_matrix4[0][1] + yin * matrix.m_matrix4[1][1] + matrix.m_matrix4[3][1]);

   } else {
      x = xin * matrix.m_matrix4[0][0] +
          yin * matrix.m_matrix4[1][0] +
          matrix.m_matrix4[3][0];

      y = xin * matrix.m_matrix4[0][1] +
          yin * matrix.m_matrix4[1][1] +
          matrix.m_matrix4[3][1];

      w = xin * matrix.m_matrix4[0][3] +
          yin * matrix.m_matrix4[1][3] +
          matrix.m_matrix4[3][3];

      if (w == 1.0f) {
         return QPointF(qreal(x), qreal(y));
      } else {
         return QPointF(qreal(x / w), qreal(y / w));
      }
   }
}

inline QMatrix4x4 operator-(const QMatrix4x4 &matrix)
{
   QMatrix4x4 m(1);

   m.m_matrix4[0][0] = -matrix.m_matrix4[0][0];
   m.m_matrix4[0][1] = -matrix.m_matrix4[0][1];
   m.m_matrix4[0][2] = -matrix.m_matrix4[0][2];
   m.m_matrix4[0][3] = -matrix.m_matrix4[0][3];
   m.m_matrix4[1][0] = -matrix.m_matrix4[1][0];
   m.m_matrix4[1][1] = -matrix.m_matrix4[1][1];
   m.m_matrix4[1][2] = -matrix.m_matrix4[1][2];
   m.m_matrix4[1][3] = -matrix.m_matrix4[1][3];
   m.m_matrix4[2][0] = -matrix.m_matrix4[2][0];
   m.m_matrix4[2][1] = -matrix.m_matrix4[2][1];
   m.m_matrix4[2][2] = -matrix.m_matrix4[2][2];
   m.m_matrix4[2][3] = -matrix.m_matrix4[2][3];
   m.m_matrix4[3][0] = -matrix.m_matrix4[3][0];
   m.m_matrix4[3][1] = -matrix.m_matrix4[3][1];
   m.m_matrix4[3][2] = -matrix.m_matrix4[3][2];
   m.m_matrix4[3][3] = -matrix.m_matrix4[3][3];

   m.flagBits = QMatrix4x4::General;

   return m;
}

inline QMatrix4x4 operator*(qreal factor, const QMatrix4x4 &matrix)
{
   QMatrix4x4 m(1);

   m.m_matrix4[0][0] = matrix.m_matrix4[0][0] * factor;
   m.m_matrix4[0][1] = matrix.m_matrix4[0][1] * factor;
   m.m_matrix4[0][2] = matrix.m_matrix4[0][2] * factor;
   m.m_matrix4[0][3] = matrix.m_matrix4[0][3] * factor;
   m.m_matrix4[1][0] = matrix.m_matrix4[1][0] * factor;
   m.m_matrix4[1][1] = matrix.m_matrix4[1][1] * factor;
   m.m_matrix4[1][2] = matrix.m_matrix4[1][2] * factor;
   m.m_matrix4[1][3] = matrix.m_matrix4[1][3] * factor;
   m.m_matrix4[2][0] = matrix.m_matrix4[2][0] * factor;
   m.m_matrix4[2][1] = matrix.m_matrix4[2][1] * factor;
   m.m_matrix4[2][2] = matrix.m_matrix4[2][2] * factor;
   m.m_matrix4[2][3] = matrix.m_matrix4[2][3] * factor;
   m.m_matrix4[3][0] = matrix.m_matrix4[3][0] * factor;
   m.m_matrix4[3][1] = matrix.m_matrix4[3][1] * factor;
   m.m_matrix4[3][2] = matrix.m_matrix4[3][2] * factor;
   m.m_matrix4[3][3] = matrix.m_matrix4[3][3] * factor;

   m.flagBits = QMatrix4x4::General;

   return m;
}

inline QMatrix4x4 operator*(const QMatrix4x4 &matrix, qreal factor)
{
   QMatrix4x4 m(1);

   m.m_matrix4[0][0] = matrix.m_matrix4[0][0] * factor;
   m.m_matrix4[0][1] = matrix.m_matrix4[0][1] * factor;
   m.m_matrix4[0][2] = matrix.m_matrix4[0][2] * factor;
   m.m_matrix4[0][3] = matrix.m_matrix4[0][3] * factor;
   m.m_matrix4[1][0] = matrix.m_matrix4[1][0] * factor;
   m.m_matrix4[1][1] = matrix.m_matrix4[1][1] * factor;
   m.m_matrix4[1][2] = matrix.m_matrix4[1][2] * factor;
   m.m_matrix4[1][3] = matrix.m_matrix4[1][3] * factor;
   m.m_matrix4[2][0] = matrix.m_matrix4[2][0] * factor;
   m.m_matrix4[2][1] = matrix.m_matrix4[2][1] * factor;
   m.m_matrix4[2][2] = matrix.m_matrix4[2][2] * factor;
   m.m_matrix4[2][3] = matrix.m_matrix4[2][3] * factor;
   m.m_matrix4[3][0] = matrix.m_matrix4[3][0] * factor;
   m.m_matrix4[3][1] = matrix.m_matrix4[3][1] * factor;
   m.m_matrix4[3][2] = matrix.m_matrix4[3][2] * factor;
   m.m_matrix4[3][3] = matrix.m_matrix4[3][3] * factor;

   m.flagBits = QMatrix4x4::General;

   return m;
}

inline bool qFuzzyCompare(const QMatrix4x4 &m1, const QMatrix4x4 &m2)
{
   return qFuzzyCompare(m1.m_matrix4[0][0], m2.m_matrix4[0][0]) &&
          qFuzzyCompare(m1.m_matrix4[0][1], m2.m_matrix4[0][1]) &&
          qFuzzyCompare(m1.m_matrix4[0][2], m2.m_matrix4[0][2]) &&
          qFuzzyCompare(m1.m_matrix4[0][3], m2.m_matrix4[0][3]) &&
          qFuzzyCompare(m1.m_matrix4[1][0], m2.m_matrix4[1][0]) &&
          qFuzzyCompare(m1.m_matrix4[1][1], m2.m_matrix4[1][1]) &&
          qFuzzyCompare(m1.m_matrix4[1][2], m2.m_matrix4[1][2]) &&
          qFuzzyCompare(m1.m_matrix4[1][3], m2.m_matrix4[1][3]) &&
          qFuzzyCompare(m1.m_matrix4[2][0], m2.m_matrix4[2][0]) &&
          qFuzzyCompare(m1.m_matrix4[2][1], m2.m_matrix4[2][1]) &&
          qFuzzyCompare(m1.m_matrix4[2][2], m2.m_matrix4[2][2]) &&
          qFuzzyCompare(m1.m_matrix4[2][3], m2.m_matrix4[2][3]) &&
          qFuzzyCompare(m1.m_matrix4[3][0], m2.m_matrix4[3][0]) &&
          qFuzzyCompare(m1.m_matrix4[3][1], m2.m_matrix4[3][1]) &&
          qFuzzyCompare(m1.m_matrix4[3][2], m2.m_matrix4[3][2]) &&
          qFuzzyCompare(m1.m_matrix4[3][3], m2.m_matrix4[3][3]);
}

inline QPoint QMatrix4x4::map(const QPoint &point) const
{
   return *this * point;
}

inline QPointF QMatrix4x4::map(const QPointF &point) const
{
   return *this * point;
}

#ifndef QT_NO_VECTOR3D

inline QVector3D QMatrix4x4::map(const QVector3D &point) const
{
   return *this * point;
}

inline QVector3D QMatrix4x4::mapVector(const QVector3D &vector) const
{
   if (flagBits < Scale) {
      return vector;

   } else if (flagBits < Rotation2D) {
      return QVector3D(vector.x() * m_matrix4[0][0],
            vector.y() * m_matrix4[1][1],
            vector.z() * m_matrix4[2][2]);

   } else {
      return QVector3D(vector.x() * m_matrix4[0][0] +
            vector.y() * m_matrix4[1][0] +
            vector.z() * m_matrix4[2][0],
            vector.x() * m_matrix4[0][1] +
            vector.y() * m_matrix4[1][1] +
            vector.z() * m_matrix4[2][1],
            vector.x() * m_matrix4[0][2] +
            vector.y() * m_matrix4[1][2] +
            vector.z() * m_matrix4[2][2]);
   }
}

#endif

#ifndef QT_NO_VECTOR4D

inline QVector4D QMatrix4x4::map(const QVector4D &point) const
{
   return *this * point;
}

#endif

inline qreal *QMatrix4x4::data()
{
   // assume the caller will modify the matrix elements, so we flip it over to "General" mode
   flagBits = General;

   return *m_matrix4;
}

inline void QMatrix4x4::viewport(const QRectF &rect)
{
    viewport(rect.x(), rect.y(), rect.width(), rect.height());
}

Q_GUI_EXPORT QDebug operator<<(QDebug dbg, const QMatrix4x4 &m);

Q_GUI_EXPORT QDataStream &operator<<(QDataStream &, const QMatrix4x4 &);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &, QMatrix4x4 &);

#endif

#endif
