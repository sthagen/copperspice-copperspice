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

#ifndef QAtomicMathematicians_P_H
#define QAtomicMathematicians_P_H

#include <qatomicmathematician_p.h>
#include <qsourcelocationreflection_p.h>

namespace QPatternist {

class DecimalMathematician : public AtomicMathematician, public DelegatingSourceLocationReflection
{
 public:
   DecimalMathematician(const SourceLocationReflection *const r)
      : DelegatingSourceLocationReflection(r)
   { }

   Item calculate(const Item &o1, const Operator op, const Item &o2,
         const QExplicitlySharedDataPointer<DynamicContext> &context) const override;
};

class IntegerMathematician : public AtomicMathematician, public DelegatingSourceLocationReflection
{
 public:
   IntegerMathematician(const SourceLocationReflection *const r)
      : DelegatingSourceLocationReflection(r)
   { }

   Item calculate(const Item &o1, const Operator op, const Item &o2,
         const QExplicitlySharedDataPointer<DynamicContext> &context) const override;
};

class DurationNumericMathematician : public AtomicMathematician, public DelegatingSourceLocationReflection
{
 public:
   DurationNumericMathematician(const SourceLocationReflection *const r)
      : DelegatingSourceLocationReflection(r)
   { }

   Item calculate(const Item &o1, const Operator op, const Item &o2,
      const QExplicitlySharedDataPointer<DynamicContext> &context) const override;
};

class DurationDurationDivisor : public AtomicMathematician
{
 public:
   Item calculate(const Item &o1, const Operator op, const Item &o2,
         const QExplicitlySharedDataPointer<DynamicContext> &context) const override;
};


class DurationDurationMathematician : public AtomicMathematician
{
 public:
   Item calculate(const Item &o1, const Operator op, const Item &o2,
         const QExplicitlySharedDataPointer<DynamicContext> &context) const override;
};

class OperandSwitcherMathematician : public AtomicMathematician
{
 public:
   OperandSwitcherMathematician(const AtomicMathematician::Ptr &mathematician);

   Item calculate(const Item &o1, const Operator op, const Item &o2,
         const QExplicitlySharedDataPointer<DynamicContext> &context) const override;

 private:
   const AtomicMathematician::Ptr m_mather;
};

class DateTimeDurationMathematician : public AtomicMathematician, public DelegatingSourceLocationReflection
{
 public:

   DateTimeDurationMathematician(const SourceLocationReflection *const r)
      : DelegatingSourceLocationReflection(r)
   { }

   Item calculate(const Item &o1, const Operator op, const Item &o2,
         const QExplicitlySharedDataPointer<DynamicContext> &context) const override;
};

class AbstractDateTimeMathematician : public AtomicMathematician
{
 public:
   Item calculate(const Item &o1, const Operator op, const Item &o2,
         const QExplicitlySharedDataPointer<DynamicContext> &context) const override;
};

}

#endif
