/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMProxyUnRegisterUndoElement.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyUnRegisterUndoElement.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMStateLoaderBase.h"


vtkStandardNewMacro(vtkSMProxyUnRegisterUndoElement);
vtkCxxRevisionMacro(vtkSMProxyUnRegisterUndoElement, "$Revision: 1.9 $");
//-----------------------------------------------------------------------------
vtkSMProxyUnRegisterUndoElement::vtkSMProxyUnRegisterUndoElement()
{
}

//-----------------------------------------------------------------------------
vtkSMProxyUnRegisterUndoElement::~vtkSMProxyUnRegisterUndoElement()
{
}

//-----------------------------------------------------------------------------
int vtkSMProxyUnRegisterUndoElement::Undo()
{
  if (!this->XMLElement)
    {
    vtkErrorMacro("No proxy state available to undo deletion.");
    return 0;
    }
  
  if (this->XMLElement->GetNumberOfNestedElements() < 1)
    {
    vtkErrorMacro("Invalid child elements. Cannot undo.");
    return 0;
    }

  vtkPVXMLElement* element = this->XMLElement;
  const char* group_name = element->GetAttribute("group_name");
  const char* proxy_name = element->GetAttribute("proxy_name");
  int id = 0;
  element->GetScalarAttribute("id",&id);
  if (!id)
    {
    vtkErrorMacro("Failed to locate proxy id.");
    return 0;
    }

  vtkSMStateLoaderBase* loader = this->GetStateLoader();
  if (!loader)
    {
    vtkErrorMacro("No loader set. Cannot Undo.");
    return 0;
    }

  vtkSMProxy* proxy = loader->NewProxyFromElement(
    this->XMLElement->FindNestedElementByName("Proxy"), id);

  if (!proxy)
    {
    vtkErrorMacro("Failed to locate the proxy to register.");
    return 0;
    }
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  pxm->RegisterProxy(group_name, proxy_name, proxy);
  // We note that the proxy is registered after its state has been 
  // loaded as a result when the vtkSMUndoStack updates the modified proxies,
  // this proxy is not going to be updated. Hence we explicitly mark it 
  // for update. 
  proxy->InvokeEvent(vtkCommand::PropertyModifiedEvent, 0);
  proxy->Delete();
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSMProxyUnRegisterUndoElement::Redo()
{
  if (!this->XMLElement)
    {
    vtkErrorMacro("No State present to redo.");
    return 0;
    }
  vtkPVXMLElement* element = this->XMLElement;
  const char* group_name = element->GetAttribute("group_name");
  const char* proxy_name = element->GetAttribute("proxy_name");
  int id = 0;
  element->GetScalarAttribute("id",&id);
  if (!id)
    {
    vtkErrorMacro("Failed to locate proxy id.");
    return 0;
    }

  vtkSMStateLoaderBase* loader = this->GetStateLoader();
  if (!loader)
    {
    vtkErrorMacro("No loader set. Cannot Redo.");
    return 0;
    }

  vtkSMProxy* proxy = loader->NewProxyFromElement(
    this->XMLElement->GetNestedElement(0), id);

  if (!proxy)
    {
    vtkErrorMacro("Failed to locate the proxy to register.");
    return 0;
    }

  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  pxm->UnRegisterProxy(group_name, proxy_name, proxy);
  proxy->Delete();
  
  // Unregistering may trigger deletion of the proxy.
  return 1;
}
//-----------------------------------------------------------------------------
bool vtkSMProxyUnRegisterUndoElement::CanLoadState(vtkPVXMLElement* elem)
{
  return (elem && elem->GetName() && 
    strcmp(elem->GetName(), "ProxyUnRegister") == 0);
}

//-----------------------------------------------------------------------------
void vtkSMProxyUnRegisterUndoElement::ProxyToUnRegister(const char* groupname,
  const char* proxyname, vtkSMProxy* proxy)
{
  if (!proxy)
    {
    vtkErrorMacro("Proxy cannot be NULL.");
    return;
    }
  
  vtkPVXMLElement* pdElement = vtkPVXMLElement::New();
  pdElement->SetName("ProxyUnRegister");
  pdElement->AddAttribute("group_name", groupname);
  pdElement->AddAttribute("proxy_name", proxyname);
  pdElement->AddAttribute("id", proxy->GetSelfIDAsString());
  
  proxy->SaveState(pdElement);
  
  this->SetXMLElement(pdElement);
  pdElement->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMProxyUnRegisterUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
