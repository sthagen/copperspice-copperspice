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

#ifndef QT_NO_PRINTDIALOG

#include <qapplication.h>
#include <qmessagebox.h>
#include <qprintdialog.h>
#include <qwidget.h>

#include <qabstractprintdialog_p.h>
#include <qapplication_p.h>
#include <qprintengine_win_p.h>
#include <qprinter_p.h>

#if !defined(PD_NOCURRENTPAGE)
#define PD_NOCURRENTPAGE    0x00800000
#define PD_RESULT_PRINT     1
#define PD_RESULT_APPLY     2
#define START_PAGE_GENERAL  0XFFFFFFFF
#endif

extern void qt_win_eatMouseMove();

class QPrintDialogPrivate : public QAbstractPrintDialogPrivate
{
   Q_DECLARE_PUBLIC(QPrintDialog)

 public:
   QPrintDialogPrivate()
      : engine(nullptr), ep(nullptr)
   {
   }

   int openWindowsPrintDialogModally();

   QWin32PrintEngine *engine;
   QWin32PrintEnginePrivate *ep;
};

static void qt_win_setup_PRINTDLGEX(PRINTDLGEX *pd, QWidget *parent,
   QPrintDialog *pdlg, QPrintDialogPrivate *d, HGLOBAL *tempDevNames)
{
   DEVMODE *devMode = d->ep->devMode;

   if (devMode) {
      int size = sizeof(DEVMODE) + devMode->dmDriverExtra;
      pd->hDevMode = GlobalAlloc(GHND, size);
      {
         void *dest = GlobalLock(pd->hDevMode);
         memcpy(dest, devMode, size);
         GlobalUnlock(pd->hDevMode);
      }

   } else {
      pd->hDevMode = NULL;
   }
   pd->hDevNames  = tempDevNames;

   pd->Flags = PD_RETURNDC;
   pd->Flags |= PD_USEDEVMODECOPIESANDCOLLATE;

   if (!pdlg->isOptionEnabled(QPrintDialog::PrintSelection)) {
      pd->Flags |= PD_NOSELECTION;
   }
   if (pdlg->isOptionEnabled(QPrintDialog::PrintPageRange)) {
      pd->nMinPage = pdlg->minPage();
      pd->nMaxPage = pdlg->maxPage();
   }

   if (!pdlg->isOptionEnabled(QPrintDialog::PrintToFile)) {
      pd->Flags |= PD_DISABLEPRINTTOFILE;
   }

   if (pdlg->isOptionEnabled(QPrintDialog::PrintSelection) && pdlg->printRange() == QPrintDialog::Selection) {
      pd->Flags |= PD_SELECTION;
   } else if (pdlg->isOptionEnabled(QPrintDialog::PrintPageRange) && pdlg->printRange() == QPrintDialog::PageRange) {
      pd->Flags |= PD_PAGENUMS;
   } else if (pdlg->isOptionEnabled(QPrintDialog::PrintCurrentPage) && pdlg->printRange() == QPrintDialog::CurrentPage) {
      pd->Flags |= PD_CURRENTPAGE;
   } else {
      pd->Flags |= PD_ALLPAGES;
   }

   // As stated by MSDN, to enable collate option when minpage==maxpage==0
   // set the PD_NOPAGENUMS flag
   if (pd->nMinPage == 0 && pd->nMaxPage == 0) {
      pd->Flags |= PD_NOPAGENUMS;
   }

   // Disable Current Page option if not required as default is Enabled
   if (!pdlg->isOptionEnabled(QPrintDialog::PrintCurrentPage)) {
      pd->Flags |= PD_NOCURRENTPAGE;
   }

   // Default to showing the General tab first
   pd->nStartPage = START_PAGE_GENERAL;

   // We don't support more than one page range in the QPrinter API yet.
   pd->nPageRanges = 1;
   pd->nMaxPageRanges = 1;

   if (d->ep->printToFile) {
      pd->Flags |= PD_PRINTTOFILE;
   }
   Q_ASSERT(parent);

   QWindow *parentWindow = parent->windowHandle();

   pd->hwndOwner = parentWindow ? (HWND)QGuiApplication::platformNativeInterface()->nativeResourceForWindow("handle", parentWindow) : nullptr;
   pd->lpPageRanges[0].nFromPage = qMax(pdlg->fromPage(), pdlg->minPage());
   pd->lpPageRanges[0].nToPage   = (pdlg->toPage() > 0) ? qMin(pdlg->toPage(), pdlg->maxPage()) : 1;
   pd->nCopies = d->printer->copyCount();
}

static void qt_win_read_back_PRINTDLGEX(PRINTDLGEX *pd, QPrintDialog *pdlg, QPrintDialogPrivate *d)
{
   if (pd->Flags & PD_SELECTION) {
      pdlg->setPrintRange(QPrintDialog::Selection);
      pdlg->setFromTo(0, 0);

   } else if (pd->Flags & PD_PAGENUMS) {
      pdlg->setPrintRange(QPrintDialog::PageRange);
      pdlg->setFromTo(pd->lpPageRanges[0].nFromPage, pd->lpPageRanges[0].nToPage);

   } else if (pd->Flags & PD_CURRENTPAGE) {
      pdlg->setPrintRange(QPrintDialog::CurrentPage);
      pdlg->setFromTo(0, 0);

   } else { // PD_ALLPAGES
      pdlg->setPrintRange(QPrintDialog::AllPages);
      pdlg->setFromTo(0, 0);
   }

   d->ep->printToFile = (pd->Flags & PD_PRINTTOFILE) != 0;

   d->engine->setGlobalDevMode(pd->hDevNames, pd->hDevMode);


   if (d->ep->printToFile && d->ep->fileName.isEmpty()) {
      d->ep->fileName = "FILE:";

   } else if (!d->ep->printToFile && d->ep->fileName == QLatin1String("FILE:")) {
      d->ep->fileName.clear();
   }
}

static bool warnIfNotNative(QPrinter *printer)
{
   if (printer->outputFormat() != QPrinter::NativeFormat) {
      qWarning("QPrintDialog::warnIfNotNative() Unable to open a Windows Print Dialog, native format was not configured");
      return false;
   }

   return true;
}

QPrintDialog::QPrintDialog(QPrinter *printer, QWidget *parent)
   : QAbstractPrintDialog( *(new QPrintDialogPrivate), printer, parent)
{
   Q_D(QPrintDialog);

   if ( ! warnIfNotNative(d->printer)) {
      return;
   }

   d->engine = static_cast<QWin32PrintEngine *>(d->printer->printEngine());


   d->ep = static_cast<QWin32PrintEngine *>(d->printer->printEngine())->d_func();
   setAttribute(Qt::WA_DontShowOnScreen);
}

QPrintDialog::QPrintDialog(QWidget *parent)
   : QAbstractPrintDialog( *(new QPrintDialogPrivate), nullptr, parent)
{
   Q_D(QPrintDialog);

   if (! warnIfNotNative(d->printer)) {
      return;
   }

   d->engine = static_cast<QWin32PrintEngine *>(d->printer->printEngine());
   d->ep = static_cast<QWin32PrintEngine *>(d->printer->printEngine())->d_func();
   setAttribute(Qt::WA_DontShowOnScreen);
}

QPrintDialog::~QPrintDialog()
{
}

int QPrintDialog::exec()
{
   if (! warnIfNotNative(printer())) {
      return 0;
   }

   Q_D(QPrintDialog);
   return d->openWindowsPrintDialogModally();
}

int QPrintDialogPrivate::openWindowsPrintDialogModally()
{
   Q_Q(QPrintDialog);

   QWidget *parent = q->parentWidget();

   if (parent) {
      parent = parent->window();
   } else {
      parent = QApplication::activeWindow();
   }

   // If there is no window, fall back to the print dialog itself
   if (parent == nullptr) {
      parent = q;
   }

   q->QDialog::setVisible(true);

   HGLOBAL *tempDevNames = engine->createGlobalDevNames();

   bool done;
   bool result;
   bool doPrinting;

   PRINTPAGERANGE pageRange;
   PRINTDLGEX pd;
   memset(&pd, 0, sizeof(PRINTDLGEX));
   pd.lStructSize = sizeof(PRINTDLGEX);
   pd.lpPageRanges = &pageRange;
   qt_win_setup_PRINTDLGEX(&pd, parent, q, this, tempDevNames);

   do {
      done = true;
      doPrinting = false;
      result = (PrintDlgEx(&pd) == S_OK);
      if (result && (pd.dwResultAction == PD_RESULT_PRINT
            || pd.dwResultAction == PD_RESULT_APPLY)) {
         doPrinting = (pd.dwResultAction == PD_RESULT_PRINT);
         if ((pd.Flags & PD_PAGENUMS)
            && (pd.lpPageRanges[0].nFromPage > pd.lpPageRanges[0].nToPage)) {
            pd.lpPageRanges[0].nFromPage = 1;
            pd.lpPageRanges[0].nToPage = 1;
            done = false;
         }

         if (pd.hDC == nullptr) {
            result = false;
         }
      }

      if (!done) {
         QMessageBox::warning(nullptr, QPrintDialog::tr("Print"),
            QPrintDialog::tr("The 'From' value cannot be greater than the 'To' value."),
            QPrintDialog::tr("OK"));
      }
   } while (!done);

   q->QDialog::setVisible(false);


   // write values back...
   if (result && (pd.dwResultAction == PD_RESULT_PRINT
         || pd.dwResultAction == PD_RESULT_APPLY)) {
      qt_win_read_back_PRINTDLGEX(&pd, q, this);

      // update printer validity
      printer->d_func()->validPrinter = !printer->printerName().isEmpty();
   }

   // Cleanup...
   GlobalFree(tempDevNames);

   q->done(result && doPrinting);

   return result && doPrinting;
}

void QPrintDialog::setVisible(bool visible)
{
   Q_D(QPrintDialog);

   // its always modal, so we can not hide a native print dialog
   if (! visible) {
      return;
   }

   if (! warnIfNotNative(d->printer)) {
      return;
   }

   (void)d->openWindowsPrintDialogModally();
   return;
}

#endif
