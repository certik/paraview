/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqProgressWidget.h,v $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#ifndef __pqProgressWidget_h
#define __pqProgressWidget_h


#include <QWidget>
#include "QtWidgetsExport.h"

class pqProgressBar;
class QToolButton;

class QTWIDGETS_EXPORT pqProgressWidget : public QWidget
{
  Q_OBJECT
public:
  pqProgressWidget(QWidget* parent=0);
  virtual ~pqProgressWidget();

  QToolButton* getAbortButton() const
    {
    return this->AbortButton;
    }
public slots:
  /// Set the progress.
  void setProgress(const QString& message, int value);

  /// Enabled/Disable the progress. This is different from 
  /// enabling/disabling the widget itself. This shows/hides
  /// the progress part of the widget.
  void enableProgress(bool enabled);

  /// Enable/Disable the abort button.
  void enableAbort(bool enabled);

signals:
  void abortPressed();

protected:
  pqProgressBar* ProgressBar;
  QToolButton* AbortButton;

private:
  pqProgressWidget(const pqProgressWidget&); // Not implemented.
  void operator=(const pqProgressWidget&); // Not implemented.
};

#endif

