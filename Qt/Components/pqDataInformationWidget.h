/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqDataInformationWidget.h,v $

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
#ifndef __pqDataInformationWidget_h
#define __pqDataInformationWidget_h

#include "pqComponentsExport.h"
#include <QWidget>

class pqDataInformationModel;
class QTableView;

// Widget for the DataInformation(or Statistics View).
// It creates the model and the view and connects them.
class PQCOMPONENTS_EXPORT pqDataInformationWidget : public QWidget
{
  Q_OBJECT
public:
  pqDataInformationWidget(QWidget* parent=NULL);
  virtual ~pqDataInformationWidget();

protected:
  // Filters events received by the View.
  virtual bool eventFilter(QObject* object, QEvent *event);

public slots:
  // Invoke this slot to force the information widget to update
  // data information. Typically called after Accept().
  void refreshData();

private slots:
  void showHeaderContextMenu(const QPoint&);
  void showBodyContextMenu(const QPoint&);

private:
  pqDataInformationModel* Model;
  QTableView* View;

};

#endif

