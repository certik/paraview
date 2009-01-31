/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqSignalAdaptorSelectionTreeWidget.cxx,v $

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
#include "pqSignalAdaptorSelectionTreeWidget.h"

#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSmartPointer.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMProperty.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMStringListRangeDomain.h"

#include <QTreeWidget>
#include <QPointer>

#include "pqTreeWidgetItemObject.h"
#include "pqSMAdaptor.h"

//-----------------------------------------------------------------------------
class pqSignalAdaptorSelectionTreeWidget::pqInternal
{
public:
  QPointer<QTreeWidget> TreeWidget;
  vtkSmartPointer<vtkSMProperty> Property;
  vtkSmartPointer<vtkSMDomain> Domain;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
};

//-----------------------------------------------------------------------------
pqSignalAdaptorSelectionTreeWidget::pqSignalAdaptorSelectionTreeWidget(
  QTreeWidget* treeWidget, vtkSMProperty* prop) :QObject(treeWidget)
{
  this->Internal = new pqInternal();
  this->Internal->Property = prop;
  this->Internal->TreeWidget = treeWidget;
  this->Internal->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  
  // get domain
  vtkSMDomainIterator* iter = prop->NewDomainIterator();
  iter->Begin();
  while(!iter->IsAtEnd() && !this->Internal->Domain)
    {
    vtkSMDomain* d = iter->GetDomain();
    if(vtkSMEnumerationDomain::SafeDownCast(d) ||
       vtkSMStringListDomain::SafeDownCast(d) ||
       vtkSMStringListRangeDomain::SafeDownCast(d))
      {
      this->Internal->Domain = d;
      }
    iter->Next();
    }
  iter->Delete();
  

  if(this->Internal->Domain)
    {
    this->Internal->VTKConnect->Connect(
      this->Internal->Domain, vtkCommand::DomainModifiedEvent,
      this, SLOT(domainChanged()), 0, 0);

    this->domainChanged();
    }
}

//-----------------------------------------------------------------------------
pqSignalAdaptorSelectionTreeWidget::~pqSignalAdaptorSelectionTreeWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QList<QList<QVariant> > pqSignalAdaptorSelectionTreeWidget::values() const
{
  QList<QList<QVariant> > reply;

  int max = this->Internal->TreeWidget->topLevelItemCount();
  for (int cc=0; cc < max; cc++)
    {
    QTreeWidgetItem* item = this->Internal->TreeWidget->topLevelItem(cc);
    QList<QVariant> itemValue;
    itemValue.append(item->text(0));
    itemValue.append(item->checkState(0) == Qt::Checked ? 1 : 0);
    reply.append(itemValue);
    }

  return reply;
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorSelectionTreeWidget::setValues(
  const QList<QList<QVariant> >& new_values)
{
  if(new_values.size() != this->Internal->TreeWidget->topLevelItemCount())
    {
    qDebug("inconsistent count in selection list\n");
    }

  int max = this->Internal->TreeWidget->topLevelItemCount();
  if(new_values.size() < max)
    max = new_values.size();

  bool changed = false;
  for (int cc=0; cc < max; cc++)
    {
    QList<QVariant> nval = new_values[cc];
    QTreeWidgetItem* item = this->Internal->TreeWidget->topLevelItem(cc);
    if(item->text(0) != nval[0])
      {
      item->setText(0, nval[0].toString());
      changed = true;
      }
    Qt::CheckState newState = nval[1].toInt() == 0 ? 
      Qt::Unchecked : Qt::Checked;
    if(item->checkState(0) != newState)
      {
      item->setCheckState(0, newState);
      changed = true;
      }
    }
  
  if(changed)
    {
    emit this->valuesChanged();
    }
}

//-----------------------------------------------------------------------------
void pqSignalAdaptorSelectionTreeWidget::domainChanged()
{
  // when domain changes, we need to update the property with
  // new default values
  QList<QVariant> newDomain =
    pqSMAdaptor::getSelectionPropertyDomain(this->Internal->Property);
  QList<QList<QVariant> > oldValues = this->values();

  bool equal = true;
  if(oldValues.size() == newDomain.size())
    {
    for(int i=0; equal && i<oldValues.size(); i++)
      {
      if(oldValues[i][0] != newDomain[i])
        {
        equal = false;
        }
      }
    }
  else
    {
    equal = false;
    }

  if(equal)
    {
    return;
    }
  
  this->Internal->Property->ResetToDefault();
  
  QList<QList<QVariant> > newValues =
    pqSMAdaptor::getSelectionProperty(this->Internal->Property);

  // Now update the tree widget. We hide any elements no longer in the domain.
  this->Internal->TreeWidget->clear();

  foreach (QList<QVariant> newValue, newValues)
    {
    pqTreeWidgetItemObject* item = new pqTreeWidgetItemObject(
      this->Internal->TreeWidget, QStringList(newValue[0].toString()));
    item->setCheckState(0, newValue[1].toInt() ? Qt::Checked : Qt::Unchecked);
    QObject::connect(item, SIGNAL(checkedStateChanged(bool)),
      this, SIGNAL(valuesChanged()), Qt::QueuedConnection);
    }
}

