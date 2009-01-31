/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMNewWidgetRepresentationProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMNewWidgetRepresentationProxy.h"

#include "vtkAbstractWidget.h"
#include "vtkClientServerInterpreter.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"
#include "vtkWidgetRepresentation.h"

#include <vtkstd/list>

vtkStandardNewMacro(vtkSMNewWidgetRepresentationProxy);
vtkCxxRevisionMacro(vtkSMNewWidgetRepresentationProxy, "$Revision: 1.5 $");

class vtkSMNewWidgetRepresentationObserver : public vtkCommand
{
public:
  static vtkSMNewWidgetRepresentationObserver *New() 
    { return new vtkSMNewWidgetRepresentationObserver; }
  virtual void Execute(vtkObject*, unsigned long event, void*)
    {
      if (this->Proxy)
        {
        this->Proxy->ExecuteEvent(event);
        }
    }
  vtkSMNewWidgetRepresentationObserver():Proxy(0) {}
  vtkSMNewWidgetRepresentationProxy* Proxy;
};

struct vtkSMNewWidgetRepresentationInternals
{
  typedef vtkstd::list<vtkSmartPointer<vtkSMLink> > LinksType;
  LinksType Links;
};

//----------------------------------------------------------------------------
vtkSMNewWidgetRepresentationProxy::vtkSMNewWidgetRepresentationProxy()
{
  this->RepresentationProxy = 0;
  this->WidgetProxy = 0;
  this->Widget = 0;
  this->Observer = vtkSMNewWidgetRepresentationObserver::New();
  this->Observer->Proxy = this;
  this->Internal = new vtkSMNewWidgetRepresentationInternals;
}

//----------------------------------------------------------------------------
vtkSMNewWidgetRepresentationProxy::~vtkSMNewWidgetRepresentationProxy()
{
  this->RepresentationProxy = 0;
  this->WidgetProxy = 0;
  this->Widget = 0;
  this->Observer->Proxy = 0;
  this->Observer->Delete();

  if (this->Internal)
    {
    delete this->Internal;
    }
}

//----------------------------------------------------------------------------
bool vtkSMNewWidgetRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (this->WidgetProxy)
    {
    vtkAbstractWidget* widget = vtkAbstractWidget::SafeDownCast(
      pm->GetObjectFromID(this->WidgetProxy->GetID()));
    if (widget)
      {
      widget->SetInteractor(renderView->GetInteractor());
      widget->SetCurrentRenderer(renderView->GetRenderer());
      }
    }

  if (this->RepresentationProxy)
    {
    vtkSMProxyProperty* rendererProp = 
      vtkSMProxyProperty::SafeDownCast(
        this->RepresentationProxy->GetProperty("Renderer"));

    if (rendererProp)
      {
      rendererProp->AddProxy(renderView->GetRendererProxy());
      this->RepresentationProxy->UpdateProperty("Renderer");
      }

    if(this->GetSubProxy("Prop"))
      {
      renderView->AddPropToRenderer(this->RepresentationProxy);
      }
    else if(this->GetSubProxy("Prop2D"))
      {
      renderView->AddPropToRenderer2D(this->RepresentationProxy);
      }
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMNewWidgetRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (this->WidgetProxy)
    {
    vtkAbstractWidget* widget = vtkAbstractWidget::SafeDownCast(
      pm->GetObjectFromID(this->WidgetProxy->GetID()));
    if (this->Widget)
      {
      widget->SetEnabled(0);
      widget->SetCurrentRenderer(0);
      widget->SetInteractor(0);
      }
    }

  if (this->RepresentationProxy)
    {
    vtkSMProxyProperty* rendererProp = 
      vtkSMProxyProperty::SafeDownCast(
        this->RepresentationProxy->GetProperty("Renderer"));
    if (rendererProp)
      {
      rendererProp->RemoveAllProxies();
      this->RepresentationProxy->UpdateProperty("Renderer");
      }

    if(this->GetSubProxy("Prop"))
      {
      renderView->RemovePropFromRenderer(this->RepresentationProxy);
      }
    else if(this->GetSubProxy("Prop2D"))
      {
      renderView->RemovePropFromRenderer2D(this->RepresentationProxy);
      }
    }

  return this->Superclass::RemoveFromView(view);
}

//-----------------------------------------------------------------------------
void vtkSMNewWidgetRepresentationProxy::SetEnabled(int enable)
{
  if (this->WidgetProxy)
    {
    vtkSMIntVectorProperty* enabled = vtkSMIntVectorProperty::SafeDownCast(
      this->WidgetProxy->GetProperty("Enabled"));
    enabled->SetElements1(enable);
    this->WidgetProxy->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void vtkSMNewWidgetRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->RepresentationProxy = this->GetSubProxy("Prop");
  if (!this->RepresentationProxy)
    {
    this->RepresentationProxy = this->GetSubProxy("Prop2D");
    }
  if (!this->RepresentationProxy)
    {
    vtkErrorMacro(
      "A representation proxy must be defined as a Prop (or Prop2D) sub-proxy");
    return;
    }
  this->RepresentationProxy->SetServers(
    vtkProcessModule::RENDER_SERVER | vtkProcessModule::CLIENT);

  this->WidgetProxy = this->GetSubProxy("Widget");
  if (!this->WidgetProxy)
    {
    vtkErrorMacro("A widget proxy must be defined as a Widget sub-proxy");
    return;
    }
  this->WidgetProxy->SetServers(vtkProcessModule::CLIENT);

  this->Superclass::CreateVTKObjects();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Representation"));
  if (pp)
    {
    pp->AddProxy(this->RepresentationProxy);
    }
  this->WidgetProxy->UpdateVTKObjects();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  this->Widget = vtkAbstractWidget::SafeDownCast(
    pm->GetObjectFromID(this->WidgetProxy->GetID()));
  if (this->Widget)
    {
    this->Widget->AddObserver(
      vtkCommand::StartInteractionEvent, this->Observer);
    this->Widget->AddObserver(
      vtkCommand::EndInteractionEvent, this->Observer);
    this->Widget->AddObserver(
      vtkCommand::InteractionEvent, this->Observer);
    }

  // Since links copy values from input to output,
  // we need to make sure that input properties i.e. the info
  // properties are not empty.
  this->UpdatePropertyInformation();

  vtkSMPropertyIterator* piter = this->NewPropertyIterator();
  for(piter->Begin(); !piter->IsAtEnd(); piter->Next())
    {
    vtkSMProperty* prop = piter->GetProperty();
    vtkSMProperty* info = prop->GetInformationProperty();
    if (info)
      {
      vtkSMPropertyLink* link = vtkSMPropertyLink::New();
      link->AddLinkedProperty(this, 
                              piter->GetKey(), 
                              vtkSMLink::OUTPUT);
      link->AddLinkedProperty(this, 
                              this->GetPropertyName(info),
                              vtkSMLink::INPUT);
      this->Internal->Links.push_back(link);
      link->Delete();
      }
    }
  piter->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMNewWidgetRepresentationProxy::ExecuteEvent(unsigned long event)
{
  this->InvokeEvent(event);

  if (event == vtkCommand::StartInteractionEvent)
    {
    vtkPVGenericRenderWindowInteractor* inter =
      vtkPVGenericRenderWindowInteractor::SafeDownCast(
        this->Widget->GetInteractor());
    if (inter)
      {
      inter->InteractiveRenderEnabledOn();
      }
    vtkSMProperty* startInt = 
      this->RepresentationProxy->GetProperty("OnStartInteraction");
    if (startInt)
      {
      startInt->Modified();
      this->RepresentationProxy->UpdateProperty("OnStartInteraction");
      }
    }
  else if (event == vtkCommand::InteractionEvent)
    {
    this->RepresentationProxy->UpdatePropertyInformation();
    this->UpdateVTKObjects();

    vtkSMProperty* interaction = 
      this->RepresentationProxy->GetProperty("OnInteraction");
    if (interaction)
      {
      interaction->Modified();
      this->RepresentationProxy->UpdateProperty("OnInteraction");
      }
    }
  else if (event == vtkCommand::EndInteractionEvent)
    {
    vtkPVGenericRenderWindowInteractor* inter =
      vtkPVGenericRenderWindowInteractor::SafeDownCast(
        this->Widget->GetInteractor());
    if (inter)
      {
      inter->InteractiveRenderEnabledOff();
      }
    vtkSMProperty* sizeHandles = 
      this->RepresentationProxy->GetProperty("SizeHandles");
    if (sizeHandles)
      {
      sizeHandles->Modified();
      this->RepresentationProxy->UpdateProperty("SizeHandles");
      }
    vtkSMProperty* endInt = 
      this->RepresentationProxy->GetProperty("OnEndInteraction");
    if (endInt)
      {
      endInt->Modified();
      this->RepresentationProxy->UpdateProperty("OnEndInteraction");
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMNewWidgetRepresentationProxy::UnRegister(vtkObjectBase* obj)
{
  if ( this->GetSelfIDInternal().ID != 0 )
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    // If the object is not being deleted by the interpreter and it
    // has a reference count of 2 (SelfID and the reference that is
    // being released), delete the internals so that the links
    // release their references to the proxy
    if ( pm && obj != pm->GetInterpreter() && this->Internal )
      {
      int size = this->Internal->Links.size();
      if (size > 0 && this->ReferenceCount == 2 + 2*size)
        {
        vtkSMNewWidgetRepresentationInternals* aInternal = this->Internal;
        this->Internal = 0;
        delete aInternal;
        aInternal = 0;
        }
      }
    }

  this->Superclass::UnRegister(obj);
}

//----------------------------------------------------------------------------
void vtkSMNewWidgetRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


