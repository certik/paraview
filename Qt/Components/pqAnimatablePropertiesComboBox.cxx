/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqAnimatablePropertiesComboBox.cxx,v $

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

========================================================================*/
#include "pqAnimatablePropertiesComboBox.h"

// Server Manager Includes.
#include "vtkEventQtSlotConnect.h"
#include "vtkSmartPointer.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMVectorProperty.h"

// Qt Includes.
#include <QtDebug>

// ParaView Includes.
#include "pqSMAdaptor.h"

//-----------------------------------------------------------------------------
class pqAnimatablePropertiesComboBox::pqInternal 
{
public:
  pqInternal()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    }

  vtkSmartPointer<vtkSMProxy> Source;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;

  struct PropertyInfo
    {
    vtkSmartPointer<vtkSMProxy> Proxy;
    QString Name;
    int Index;
    };
};

Q_DECLARE_METATYPE(pqAnimatablePropertiesComboBox::pqInternal::PropertyInfo);

//-----------------------------------------------------------------------------
pqAnimatablePropertiesComboBox::pqAnimatablePropertiesComboBox(QWidget* _parent)
  :Superclass(_parent)
{
  this->Internal = new pqInternal();
  this->UseBlankEntry = false;
}

//-----------------------------------------------------------------------------
pqAnimatablePropertiesComboBox::~pqAnimatablePropertiesComboBox()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqAnimatablePropertiesComboBox::setSource(vtkSMProxy* proxy)
{
  if (this->Internal->Source == proxy)
    {
    return;
    }

  this->Internal->VTKConnect->Disconnect();
  this->setEnabled(proxy != 0);
  this->Internal->Source = proxy;
  this->buildPropertyList();
}

//-----------------------------------------------------------------------------
void pqAnimatablePropertiesComboBox::setSourceWithoutProperties(vtkSMProxy* proxy)
{
  if (this->Internal->Source == proxy)
    {
    return;
    }

  this->Internal->VTKConnect->Disconnect();
  this->setEnabled(proxy != 0);
  this->Internal->Source = proxy;
  this->clear();
}

//-----------------------------------------------------------------------------
void pqAnimatablePropertiesComboBox::buildPropertyList()
{
  this->clear();
  if (!this->Internal->Source.GetPointer())
    {
    return;
    }
  if (this->UseBlankEntry)
    {
    this->addSMPropertyInternal("<select>", 0, QString(), -1);
    }
  this->buildPropertyListInternal(this->Internal->Source, QString());
}

//-----------------------------------------------------------------------------
void pqAnimatablePropertiesComboBox::buildPropertyListInternal(vtkSMProxy* proxy, 
  const QString& labelPrefix)
{
  vtkSmartPointer<vtkSMPropertyIterator> iter;
  iter.TakeReference(proxy->NewPropertyIterator());
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMVectorProperty* smproperty = 
      vtkSMVectorProperty::SafeDownCast(iter->GetProperty());
    if (!smproperty || !smproperty->GetAnimateable() || 
      smproperty->GetInformationOnly())
      {
      continue;
      }
    unsigned int num_elems = smproperty->GetNumberOfElements();
    if (smproperty->GetRepeatCommand())
      {
      num_elems = 1;
      }
    for (unsigned int cc=0; cc < num_elems; cc++)
      {
      int index = smproperty->GetRepeatCommand()? -1 : static_cast<int>(cc);
      QString label = labelPrefix.isEmpty()? "" : labelPrefix + " - ";
      label += iter->GetProperty()->GetXMLLabel();
      label = (num_elems>1) ? label + " (" + QString::number(cc) + ")" : label;

      this->addSMPropertyInternal(label, proxy, iter->GetKey(), index);
      }
    }

  // Now add properties of all proxies in properties that have
  // proxy lists.
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProxyProperty* smproperty = 
      vtkSMProxyProperty::SafeDownCast(iter->GetProperty());
    if (smproperty && 
      pqSMAdaptor::getPropertyType(smproperty) == pqSMAdaptor::PROXYSELECTION)
      {
      vtkSMProxy* child_proxy = pqSMAdaptor::getProxyProperty(smproperty);
      if(child_proxy)
        {
        QString newPrefix = labelPrefix.isEmpty()? "" : labelPrefix + ":";
        newPrefix += smproperty->GetXMLLabel();
        this->buildPropertyListInternal(child_proxy, newPrefix);

        // if this property's value changes, we'll have to rebuild
        // the property names menu.
        this->Internal->VTKConnect->Connect(smproperty, vtkCommand::ModifiedEvent,
          this, SLOT(buildPropertyList()), 0, 0, Qt::QueuedConnection);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqAnimatablePropertiesComboBox::addSMProperty(const QString& label, 
  const QString& propertyname, int index)
{
  if (!this->Internal->Source)
    {
    qCritical() << "Source must be set before adding properties.";
    return;
    }

  this->addSMPropertyInternal(label, this->Internal->Source, propertyname, index);
}

//-----------------------------------------------------------------------------
void pqAnimatablePropertiesComboBox::addSMPropertyInternal(
  const QString& label, vtkSMProxy* proxy, const QString& propertyname, 
  int index)
{
  pqInternal::PropertyInfo info;
  info.Proxy = proxy;
  info.Name = propertyname;
  info.Index = index;
  this->addItem(label, QVariant::fromValue(info));
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqAnimatablePropertiesComboBox::getCurrentProxy() const
{
  int index = this->currentIndex();
  if (index != -1)
    {
    QVariant _data = this->itemData(index);
    pqInternal::PropertyInfo info = _data.value<pqInternal::PropertyInfo>();
    return info.Proxy;
    }
  return 0;
}

//-----------------------------------------------------------------------------
QString pqAnimatablePropertiesComboBox::getCurrentPropertyName() const
{
  int index = this->currentIndex();
  if (index != -1)
    {
    QVariant _data = this->itemData(index);
    pqInternal::PropertyInfo info = _data.value<pqInternal::PropertyInfo>();
    return info.Name;
    }
  return QString();
}


//-----------------------------------------------------------------------------
int pqAnimatablePropertiesComboBox::getCurrentIndex() const
{
  int index = this->currentIndex();
  if (index != -1)
    {
    QVariant _data = this->itemData(index);
    pqInternal::PropertyInfo info = _data.value<pqInternal::PropertyInfo>();
    return info.Index;
    }
  return 0;
}

