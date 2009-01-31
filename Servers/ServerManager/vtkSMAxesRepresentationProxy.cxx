/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMAxesRepresentationProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAxesRepresentationProxy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMViewProxy.h"

vtkStandardNewMacro(vtkSMAxesRepresentationProxy);
vtkCxxRevisionMacro(vtkSMAxesRepresentationProxy, "$Revision: 1.2 $");

//----------------------------------------------------------------------------
vtkSMAxesRepresentationProxy::vtkSMAxesRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSMAxesRepresentationProxy::~vtkSMAxesRepresentationProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMAxesRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->SetServers(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->Superclass::CreateVTKObjects();
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();

  vtkClientServerStream str;
  vtkClientServerID id = this->GetID();
  str << vtkClientServerStream::Invoke << id 
      << "SymmetricOn" << vtkClientServerStream::End;
  str << vtkClientServerStream::Invoke << id
      << "ComputeNormalsOff" << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers,str,0);

  // Setup the pipeline.
  vtkSMProxy* mapper = this->GetSubProxy("Mapper");
  vtkSMProxy* actor = this->GetSubProxy("Prop");

  if (!mapper)
    {
    vtkErrorMacro("Subproxy Mapper must be defined.");
    return;
    }
  
  if (!actor)
    {
    vtkErrorMacro("Subproxy Actor must be defined.");
    return;
    }
 
  str << vtkClientServerStream::Invoke << this->GetID() 
      << "GetOutput" << vtkClientServerStream::End;
  str << vtkClientServerStream::Invoke << mapper->GetID()
      << "SetInput" 
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers,str,0);

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    actor->GetProperty("Mapper"));
  pp->RemoveAllProxies();
  pp->AddProxy(mapper);

  this->UpdateVTKObjects();

}

//----------------------------------------------------------------------------
bool vtkSMAxesRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  vtkSMProxy* p = this->GetSubProxy("Prop");
  if (p)
    {
    renderView->AddPropToRenderer(p);
    }
  p = this->GetSubProxy("Prop2D");
  if (p)
    {
    renderView->AddPropToRenderer2D(p);
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMAxesRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMRenderViewProxy* renderView = vtkSMRenderViewProxy::SafeDownCast(view);
  if (!renderView)
    {
    vtkErrorMacro("View must be a vtkSMRenderViewProxy.");
    return false;
    }

  vtkSMProxy* p = this->GetSubProxy("Prop");
  if (p)
    {
    renderView->RemovePropFromRenderer(p);
    }
  p = this->GetSubProxy("Prop2D");
  if (p)
    {
    renderView->RemovePropFromRenderer2D(p);
    }

  return true;
}

//----------------------------------------------------------------------------
void vtkSMAxesRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


