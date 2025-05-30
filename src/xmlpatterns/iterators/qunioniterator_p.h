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

#ifndef QUnionIterator_P_H
#define QUnionIterator_P_H

#include <qitem_p.h>

namespace QPatternist {

class UnionIterator : public Item::Iterator
{
 public:
   UnionIterator(const Item::Iterator::Ptr &it1, const Item::Iterator::Ptr &it2);

   Item next() override;
   Item current() const override;
   xsInteger position() const override;
   Item::Iterator::Ptr copy() const override;

 private:
   Item nextFromFirstOperand() {
      ++m_position;
      m_current = m_node1;
      m_node1 = m_it1->next();
      return m_current;
   }

   const Item::Iterator::Ptr   m_it1;
   const Item::Iterator::Ptr   m_it2;
   Item m_current;
   xsInteger m_position;
   Item m_node1;
   Item m_node2;
};

}

#endif
