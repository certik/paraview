/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMRepresentationProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMRepresentationProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkInformation.h"

vtkCxxRevisionMacro(vtkSMRepresentationProxy, "$Revision: 1.8 $");
vtkCxxSetObjectMacro(vtkSMRepresentationProxy, ViewInformation, vtkInformation);
//----------------------------------------------------------------------------
vtkSMRepresentationProxy::vtkSMRepresentationProxy()
{
  this->ViewInformation = 0;
  this->ViewUpdateTime = 0;
  this->ViewUpdateTimeInitialized = false;
}

//----------------------------------------------------------------------------
vtkSMRepresentationProxy::~vtkSMRepresentationProxy()
{
  this->SetViewInformation(0);
}

//----------------------------------------------------------------------------
void vtkSMRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  if (!this->BeginCreateVTKObjects())
    {
    // BeginCreateVTKObjects() requested an abortion of VTK object creation.
    return;
    }

  this->Superclass::CreateVTKObjects();
  this->EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
bool vtkSMRepresentationProxy::GetVisibility()
{
  if (!this->ObjectsCreated)
    {
    return false;
    }

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetProperty("Visibility"));
  if (ivp && ivp->GetNumberOfElements()== 1 && ivp->GetElement(0))
    {
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
void vtkSMRepresentationProxy::Connect(vtkSMProxy* producer,
  vtkSMProxy* consumer, const char* propertyname/*="Input"*/,
  int outputport/*=0*/)
{
  if (!propertyname)
    {
    vtkErrorMacro("propertyname cannot be NULL.");
    return;
    }

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    consumer->GetProperty(propertyname));
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(pp);
  if (!pp)
    {
    vtkErrorMacro("Failed to locate property " << propertyname
      << " on the consumer " << consumer->GetXMLName());
    return;
    }

  if (ip)
    {
    ip->RemoveAllProxies();
    ip->AddInputConnection(producer, outputport);
    }
  else
    {
    pp->RemoveAllProxies();
    pp->AddProxy(producer);
    }
  consumer->UpdateProperty(propertyname);
}

//----------------------------------------------------------------------------
void vtkSMRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ViewUpdateTime: " << this->ViewUpdateTime << endl;
  os << indent << "ViewInformation: " << this->ViewInformation << endl;
}


