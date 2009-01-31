/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pq3DWidget.cxx,v $

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
#include "pq3DWidget.h"

// ParaView Server Manager includes.
#include "vtkMemberFunctionCommand.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMCompoundProxy.h"
#include "vtkSMInputProperty.h"

// Qt includes.
#include <QtDebug>
#include <QPointer>

// ParaView GUI includes.
#include "pqApplicationCore.h"
#include "pqImplicitPlaneWidget.h"
#include "pqLineSourceWidget.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqPointSourceWidget.h"
#include "pqProxy.h"
#include "pqRenderView.h"
#include "pqSMAdaptor.h"

class pq3DWidgetInternal
{
public:
  pq3DWidgetInternal() :
    IgnorePropertyChange(false),
    WidgetVisible(true),
    Selected(false)
  {
  }
    
  vtkSmartPointer<vtkSMProxy> ReferenceProxy;
  vtkSmartPointer<vtkSMNewWidgetRepresentationProxy> WidgetProxy;
  vtkSmartPointer<vtkCommand> ControlledPropertiesObserver;
  vtkSmartPointer<vtkPVXMLElement> Hints;

  QMap<vtkSmartPointer<vtkSMProperty>, vtkSmartPointer<vtkSMProperty> > PropertyMap;

  /// Used to avoid recursion when updating the controlled properties
  bool IgnorePropertyChange;
  /// Stores the visible/hidden state of the 3D widget (controlled by the user)
  bool WidgetVisible;
  /// Stores the selected/not selected state of the 3D widget (controlled by the owning panel)
  bool Selected;
};

//-----------------------------------------------------------------------------
pq3DWidget::pq3DWidget(vtkSMProxy* refProxy, vtkSMProxy* pxy, QWidget* _p) :
  pqProxyPanel(pxy, _p),
  Internal(new pq3DWidgetInternal())
{
  this->Internal->ReferenceProxy = refProxy;

  this->Internal->ControlledPropertiesObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, 
      &pq3DWidget::onControlledPropertyChanged));
  this->Internal->IgnorePropertyChange = false;

  this->setControlledProxy(pxy);
}

//-----------------------------------------------------------------------------
pq3DWidget::~pq3DWidget()
{
  this->setView(0);
  this->setControlledProxy(0);
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QList<pq3DWidget*> pq3DWidget::createWidgets(vtkSMProxy* refProxy, vtkSMProxy* pxy)
{
  QList<pq3DWidget*> widgets;

  vtkPVXMLElement* hints = pxy->GetHints();
  unsigned int max = hints->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < max; cc++)
    {
    vtkPVXMLElement* element = hints->GetNestedElement(cc);
    if (QString("PropertyGroup") == element->GetName())
      {
      QString widgetType = element->GetAttribute("type");
      pq3DWidget *widget = 0;
      if (widgetType == "Plane")
        {
        widget = new pqImplicitPlaneWidget(refProxy, pxy, 0);
        }
      else if (widgetType == "Handle")
        {
        widget = new pqHandleWidget(refProxy, pxy, 0);
        }
      else if (widgetType == "PointSource")
        {
        widget = new pqPointSourceWidget(refProxy, pxy, 0);
        }
      else if (widgetType == "LineSource")
        {
        widget = new pqLineSourceWidget(refProxy, pxy, 0);
        }
      else if (widgetType == "Line")
        {
        widget = new pqLineWidget(refProxy, pxy, 0);
        }

      if (widget)
        {
        widget->setHints(element);
        widgets.push_back(widget);
        }
      }
    }
  return widgets;
}

//-----------------------------------------------------------------------------
pqRenderView* pq3DWidget::renderView() const
{
  return qobject_cast<pqRenderView*>(this->view());
}

//-----------------------------------------------------------------------------
void pq3DWidget::setView(pqView* pqview)
{
  if (pqview == this->renderView())
    {
    this->Superclass::setView(pqview);
    return;
    }

  bool cur_visbility = this->widgetVisible();
  this->hideWidget();

  vtkSMRepresentationProxy* widget = this->getWidgetProxy();
  if (this->renderView() && widget)
    {
    // To add/remove the 3D widget display from the view module.
    // we don't use the property. This is so since the 3D widget add/remove 
    // should not get saved in state or undo-redo. 
    this->renderView()->getRenderViewProxy()->RemoveRepresentation(widget);
    }

  this->Superclass::setView(pqview);

  if (this->renderView() && widget)
    {
    // To add/remove the 3D widget display from the view module.
    // we don't use the property. This is so since the 3D widget add/remove 
    // should not get saved in state or undo-redo. 
    this->updateWidgetVisibility();
    this->renderView()->getRenderViewProxy()->AddRepresentation(widget);
    }

  if (cur_visbility)
    {
    this->showWidget();
    }
}

//-----------------------------------------------------------------------------
void pq3DWidget::render()
{
  if (this->renderView())
    {
    this->renderView()->render();
    }
}

//-----------------------------------------------------------------------------
void pq3DWidget::onControlledPropertyChanged()
{
  if (this->Internal->IgnorePropertyChange)
    {
    return;
    }

  // Synchronize the 3D and Qt widgets with the controlled properties
  this->reset();
}

//-----------------------------------------------------------------------------
void pq3DWidget::setWidgetProxy(vtkSMNewWidgetRepresentationProxy* pxy)
{
 vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();

 if (this->renderView() && widget)
    {
    // To add/remove the 3D widget display from the view module.
    // we don't use the property. This is so since the 3D widget add/remove 
    // should not get saved in state or undo-redo. 
    this->renderView()->getRenderViewProxy()->RemoveRepresentation(widget);
    this->renderView()->render();
    }
  this->Internal->WidgetProxy = pxy;

  if (this->renderView() && pxy)
    {
    // To add/remove the 3D widget display from the view module.
    // we don't use the property. This is so since the 3D widget add/remove 
    // should not get saved in state or undo-redo. 
    this->updateWidgetVisibility();
    this->renderView()->getRenderViewProxy()->AddRepresentation(widget);
    this->renderView()->render();
    }
}

//-----------------------------------------------------------------------------
vtkSMNewWidgetRepresentationProxy* pq3DWidget::getWidgetProxy() const
{
  return this->Internal->WidgetProxy;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pq3DWidget::getControlledProxy() const
{
  return this->proxy();
}

//-----------------------------------------------------------------------------
vtkSMProxy* pq3DWidget::getReferenceProxy() const
{
  return this->Internal->ReferenceProxy;
}

//-----------------------------------------------------------------------------
void pq3DWidget::setControlledProxy(vtkSMProxy* /*pxy*/)
{
  foreach(vtkSMProperty* controlledProperty, this->Internal->PropertyMap)
    {
    controlledProperty->RemoveObserver(
      this->Internal->ControlledPropertiesObserver);
    }
  this->Internal->PropertyMap.clear();
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* pq3DWidget::getHints() const
{
  return this->Internal->Hints;
}

//-----------------------------------------------------------------------------
void pq3DWidget::setHints(vtkPVXMLElement* hints)
{
  this->Internal->Hints = hints;
  if (!hints)
    {
    return;
    }

  if (!this->proxy())
    {
    qDebug() << "pq3DWidget::setHints must be called only after the controlled "
      << "proxy has been set.";
    return;
    }
  if (QString("PropertyGroup") != hints->GetName())
    {
    qDebug() << "Argument to setHints must be a <PropertyGroup /> element.";
    return;
    }

  vtkSMProxy* pxy = this->proxy();
  unsigned int max = hints->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < max; cc++)
    {
    unsigned int max_props = hints->GetNumberOfNestedElements();
    for (unsigned int i=0; i < max_props; i++)
      {
      vtkPVXMLElement* propElem = hints->GetNestedElement(i);
      this->setControlledProperty(propElem->GetAttribute("function"),
        pxy->GetProperty(propElem->GetAttribute("name")));
      }
    }
}

//-----------------------------------------------------------------------------
void pq3DWidget::setControlledProperty(const char* function,
  vtkSMProperty* controlled_property)
{
  this->Internal->PropertyMap.insert(
    this->Internal->WidgetProxy->GetProperty(function),
    controlled_property);

  controlled_property->AddObserver(vtkCommand::ModifiedEvent,
    this->Internal->ControlledPropertiesObserver);
}

//-----------------------------------------------------------------------------
void pq3DWidget::setControlledProperty(vtkSMProperty* widget_property, 
  vtkSMProperty* controlled_property)
{
  this->Internal->PropertyMap.insert(
    widget_property,
    controlled_property);
    
  controlled_property->AddObserver(vtkCommand::ModifiedEvent,
    this->Internal->ControlledPropertiesObserver);
}

//-----------------------------------------------------------------------------
void pq3DWidget::accept()
{
  this->Internal->IgnorePropertyChange = true;
  QMap<vtkSmartPointer<vtkSMProperty>, 
    vtkSmartPointer<vtkSMProperty> >::const_iterator iter;
  for (iter = this->Internal->PropertyMap.constBegin() ;
    iter != this->Internal->PropertyMap.constEnd(); 
    ++iter)
    {
    iter.value()->Copy(iter.key());
    }
  if (this->proxy())
    {
    this->proxy()->UpdateVTKObjects();
    }
  this->Internal->IgnorePropertyChange = false;
}

//-----------------------------------------------------------------------------
void pq3DWidget::reset()
{
  // We don't want to fire any widget modified events while resetting the 
  // 3D widget, hence we block all signals. Otherwise, on reset, we fire a
  // widget modified event, which makes the accept button enabled again.
  this->blockSignals(true);
  QMap<vtkSmartPointer<vtkSMProperty>, 
    vtkSmartPointer<vtkSMProperty> >::const_iterator iter;
  for (iter = this->Internal->PropertyMap.constBegin() ;
    iter != this->Internal->PropertyMap.constEnd(); 
    ++iter)
    {
    iter.key()->Copy(iter.value());
    iter.key()->Modified();
    }

  if (this->Internal->WidgetProxy)
    {
    this->Internal->WidgetProxy->UpdateVTKObjects();
    this->Internal->WidgetProxy->UpdatePropertyInformation();
    pqApplicationCore::instance()->render();
    }
  this->blockSignals(false);
}

//-----------------------------------------------------------------------------
bool pq3DWidget::widgetVisible() const
{
  return this->Internal->WidgetVisible;
}

//-----------------------------------------------------------------------------
void pq3DWidget::select()
{
  if(true != this->Internal->Selected)
    {
    this->Internal->Selected = true;
    this->updateWidgetVisibility();
    }
}

//-----------------------------------------------------------------------------
void pq3DWidget::deselect()
{
  if(false != this->Internal->Selected)
    {
    this->Internal->Selected = false;
    this->updateWidgetVisibility();
    }
}

//-----------------------------------------------------------------------------
void pq3DWidget::setWidgetVisible(bool visible)
{
  if(visible != this->Internal->WidgetVisible)
    {
    this->Internal->WidgetVisible = visible;
    this->updateWidgetVisibility();
    
    emit this->widgetVisibilityChanged(visible);
    }
}

//-----------------------------------------------------------------------------
void pq3DWidget::showWidget()
{
  this->setWidgetVisible(true);
}

//-----------------------------------------------------------------------------
void pq3DWidget::hideWidget()
{
  this->setWidgetVisible(false);
}

//-----------------------------------------------------------------------------
int pq3DWidget::getReferenceInputBounds(double bounds[6]) const
{
  if (!this->getReferenceProxy())
    {
    return 0;
    }
  
  vtkSMSourceProxy* input = NULL;
  vtkSMInputProperty* ivp = vtkSMInputProperty::SafeDownCast(
    this->getReferenceProxy()->GetProperty("Input"));
  if(ivp && ivp->GetNumberOfProxies())
    {
    vtkSMProxy* pxy = ivp->GetProxy(0);
    input = vtkSMSourceProxy::SafeDownCast(pxy);
    vtkSMCompoundProxy* cp;
    cp = vtkSMCompoundProxy::SafeDownCast(pxy);
    if(cp)
      {
      input = vtkSMSourceProxy::SafeDownCast(cp->GetConsumableProxy());
      }
    }

  if(input)
    {
    input->GetDataInformation()->GetBounds(bounds);
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
void pq3DWidget::updateWidgetVisibility()
{
  const bool widget_visible = this->Internal->Selected
    && this->Internal->WidgetVisible;
    
  const bool widget_enabled = widget_visible;
  
  if(this->Internal->WidgetProxy && this->renderView())
    {
    if(vtkSMIntVectorProperty* const visibility =
      vtkSMIntVectorProperty::SafeDownCast(
        this->Internal->WidgetProxy->GetProperty("Visibility")))
      {
      visibility->SetElement(0, widget_visible);
      }

    if(vtkSMIntVectorProperty* const enabled =
      vtkSMIntVectorProperty::SafeDownCast(
        this->Internal->WidgetProxy->GetProperty("Enabled")))
      {
      enabled->SetElement(0, widget_enabled);
      }

    this->Internal->WidgetProxy->UpdateVTKObjects();

    // We don't need to explicitly call a render here since the 
    // vtkAbstractWidget render the views when enable/visibily 
    // state changes.
    //pqApplicationCore::instance()->render();
    }
}
