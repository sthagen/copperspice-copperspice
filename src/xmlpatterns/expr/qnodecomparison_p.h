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

#ifndef QNodeComparison_P_H
#define QNodeComparison_P_H

#include <qpaircontainer_p.h>

namespace QPatternist {

class NodeComparison : public PairContainer
{
 public:
   NodeComparison(const Expression::Ptr &operand1, const QXmlNodeModelIndex::DocumentOrder op, const Expression::Ptr &operand2);

   Item evaluateSingleton(const DynamicContext::Ptr &) const override;
   bool evaluateEBV(const DynamicContext::Ptr &) const override;

   SequenceType::List expectedOperandTypes() const override;

   virtual QXmlNodeModelIndex::DocumentOrder operatorID() const;

   Expression::Ptr compress(const StaticContext::Ptr &context) override;

   SequenceType::Ptr staticType() const override;

   static QString displayName(const QXmlNodeModelIndex::DocumentOrder op);

   ExpressionVisitorResult::Ptr accept(const ExpressionVisitor::Ptr &visitor) const override;

 private:
   enum Result {
      Empty,
      True,
      False
   };

   inline Result evaluate(const DynamicContext::Ptr &context) const;

   const QXmlNodeModelIndex::DocumentOrder m_op;
};

}

#endif
