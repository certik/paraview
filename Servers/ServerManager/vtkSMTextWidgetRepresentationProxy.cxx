/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMTextWidgetRepresentationProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTextWidgetRepresentationProxy.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMViewProxy.h"

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkSMTextWidgetRepresentationProxy);
vtkCxxRevisionMacro(vtkSMTextWidgetRepresentationProxy, "$Revision: 1.1 $");

//----------------------------------------------------------------------------
vtkSMTextWidgetRepresentationProxy::vtkSMTextWidgetRepresentationProxy()
{
  this->TextActorProxy = 0;
  this->TextPropertyProxy = 0;
  this->ViewProxy = 0;
  this->Visibility = 0;
}

//----------------------------------------------------------------------------
vtkSMTextWidgetRepresentationProxy::~vtkSMTextWidgetRepresentationProxy()
{
  this->TextActorProxy = 0;
  this->TextPropertyProxy = 0;
  this->ViewProxy = 0;
}

//----------------------------------------------------------------------------
bool vtkSMTextWidgetRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  if(!this->Superclass::AddToView(view))
    {
    return false;
    }
  
  this->ViewProxy = view;
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMTextWidgetRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  if(!this->Superclass::RemoveFromView(view))
    {
    return false;
    }
  
  this->ViewProxy = 0;
  return true;
}

//----------------------------------------------------------------------------
void vtkSMTextWidgetRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->TextActorProxy = this->GetSubProxy("Prop2DActor");
  if (!this->TextActorProxy)
    {
    vtkErrorMacro("Failed to find subproxy Prop2DActor.");
    return;
    }
  this->TextPropertyProxy = this->GetSubProxy("Prop2DProperty");
  if (!this->TextPropertyProxy)
    {
    vtkErrorMacro("Failed to find subproxy Prop2DProperty.");
    return;
    }

  this->TextActorProxy->SetServers(
    vtkProcessModule::RENDER_SERVER | vtkProcessModule::CLIENT);
  this->TextPropertyProxy->SetServers(
    vtkProcessModule::RENDER_SERVER | vtkProcessModule::CLIENT);

  this->Superclass::CreateVTKObjects();

  if (!this->RepresentationProxy)
    {
    vtkErrorMacro("Failed to find subproxy Prop2D.");
    return;
    }

  vtkSMProxyProperty* tppp = vtkSMProxyProperty::SafeDownCast(
    this->TextActorProxy->GetProperty("TextProperty"));
  if (!tppp)
    {
    vtkErrorMacro("Failed to find property TextProperty on TextActorProxy.");
    return;
    }
  if(!tppp->AddProxy(this->TextPropertyProxy))
    {
    return;
    }

  vtkSMProxyProperty* tapp = vtkSMProxyProperty::SafeDownCast(
    this->RepresentationProxy->GetProperty("TextActor"));
  if (!tapp)
    {
    vtkErrorMacro("Failed to find property TextActor on TextRepresentationProxy.");
    return;
    }
  if(!tapp->AddProxy(this->TextActorProxy))
    {
    return;
    }
}

//----------------------------------------------------------------------------
void vtkSMTextWidgetRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Visibility: " << this->Visibility << endl;

}
