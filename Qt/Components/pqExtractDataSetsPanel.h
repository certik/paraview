/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqExtractDataSetsPanel.h,v $

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
cxxPROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#ifndef _pqExtractDataSetsPanel_h
#define _pqExtractDataSetsPanel_h

#include "pqObjectPanel.h"
#include "pqComponentsExport.h"
#include "ui_pqExtractDataSetsPanel.h"

class pqTreeWidgetItemObject;
class QTreeWidget;
class QTreeWidgetItem;
class QListWidget;
class vtkPVArrayInformation;
class pqExtractDataSetsPanelInternals;

class PQCOMPONENTS_EXPORT pqExtractDataSetsPanel :
  public pqObjectPanel
{
  Q_OBJECT
public:
  /// constructor
  pqExtractDataSetsPanel(pqProxy* proxy, QWidget* p = NULL);
  /// destructor
  ~pqExtractDataSetsPanel();

signals:

protected slots:
  void datasetsItemChanged(QTreeWidgetItem* item);

  /// accept the changes made to the properties
  /// changes will be propogated down to the server manager
  virtual void accept();

  /// reset the changes made
  /// editor will query properties from the server manager
  virtual void reset();

protected:

  void updateMapState(QTreeWidgetItem* item);
  void updateGUI();

  pqExtractDataSetsPanelInternals* Internals;
  bool UpdateInProgress;
  Ui::ExtractDataSetsPanel* UI;

};

#endif

