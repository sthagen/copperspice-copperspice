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

#ifndef QABSTRACTPRINTDIALOG_P_H
#define QABSTRACTPRINTDIALOG_P_H

#include <qdialog_p.h>

#ifndef QT_NO_PRINTDIALOG

#include <qabstractprintdialog.h>

#ifndef QT_NO_PRINTER

class QPrinter;

class QPrinterPrivate;

class QAbstractPrintDialogPrivate : public QDialogPrivate
{
   Q_DECLARE_PUBLIC(QAbstractPrintDialog)

 public:
   QAbstractPrintDialogPrivate()
      : printer(nullptr), m_printDialog(nullptr), ownsPrinter(false)
      , options(QAbstractPrintDialog::PrintToFile | QAbstractPrintDialog::PrintPageRange |
                QAbstractPrintDialog::PrintCollateCopies | QAbstractPrintDialog::PrintShowPageSize),
        m_minPage(0), m_maxPage(INT_MAX)
   {  }

   virtual void setTabs(const QList<QWidget *> &) {
   }

   QPrinter *printer;
   QPrinterPrivate *m_printDialog;

   bool ownsPrinter;
   QPointer<QObject> receiverToDisconnectOnClose;
   QString memberToDisconnectOnClose;

   QAbstractPrintDialog::PrintDialogOptions options;

   void setPrinter(QPrinter *newPrinter);

   int m_minPage;
   int m_maxPage;
};

#endif //QT_NO_PRINTER


#endif // QT_NO_PRINTDIALOG

#endif
