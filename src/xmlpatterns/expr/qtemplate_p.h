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

#ifndef QTemplate_P_H
#define QTemplate_P_H

#include <qshareddata.h>
#include <qvector.h>

#include <qdynamiccontext_p.h>
#include <qexpression_p.h>
#include <qsourcelocationreflection_p.h>
#include <qtemplateinvoker_p.h>
#include <qvariabledeclaration_p.h>

namespace QPatternist {

class Template : public QSharedData, public SourceLocationReflection

{
 public:
   typedef QExplicitlySharedDataPointer<Template> Ptr;
   typedef QVector<Template::Ptr> Vector;

   Template(const ImportPrecedence ip, const SequenceType::Ptr &reqType)
      : importPrecedence(ip), m_reqType(reqType)
   { }

   Expression::Ptr body;

   const SourceLocationReflection *actualReflection() const override;

   const ImportPrecedence importPrecedence;

   VariableDeclaration::List templateParameters;

   DynamicContext::Ptr createContext(const TemplateInvoker *const invoker,
         const DynamicContext::Ptr &context, const bool isCallTemplate) const;

   void compileParameters(const StaticContext::Ptr &context);

   Expression::Properties properties() const;
   Expression::Properties dependencies() const;

   static void raiseXTSE0680(const ReportContext::Ptr &context, const QXmlName &name,
         const SourceLocationReflection *const reflection);

 private:
   DynamicContext::TemplateParameterHash parametersAsHash() const;
   const SequenceType::Ptr m_reqType;
};

}

#endif
