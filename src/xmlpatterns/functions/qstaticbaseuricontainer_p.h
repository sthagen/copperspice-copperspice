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

#ifndef QStaticBaseUriContainer_P_H
#define QStaticBaseUriContainer_P_H

#include <qurl.h>

#include <qfunctioncall_p.h>

namespace QPatternist {

class StaticBaseUriContainer : public FunctionCall
{
 protected:
   StaticBaseUriContainer()
   { }

   void prepareStaticBaseURI(const StaticContext::Ptr &context) {
      m_staticBaseURI = context->baseURI();
   }

   const QUrl &staticBaseURI() const {
      return m_staticBaseURI;
   }

   Expression::Ptr typeCheck(const StaticContext::Ptr &context, const SequenceType::Ptr &reqType) override {
      prepareStaticBaseURI(context);
      return FunctionCall::typeCheck(context, reqType);
   }

 private:
   StaticBaseUriContainer(const StaticBaseUriContainer &) = delete;
   StaticBaseUriContainer &operator=(const StaticBaseUriContainer &) = delete;

   QUrl m_staticBaseURI;
};

}

#endif
