/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMPQStateLoader.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPQStateLoader.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSmartPointer.h"
#include <vtkstd/list>
#include <vtkstd/algorithm>

vtkStandardNewMacro(vtkSMPQStateLoader);
vtkCxxRevisionMacro(vtkSMPQStateLoader, "$Revision: 1.20 $");

struct vtkSMPQStateLoaderInternals
{
  vtkstd::list<vtkSmartPointer<vtkSMRenderViewProxy> > PreferredRenderViews;
};


//-----------------------------------------------------------------------------
vtkSMPQStateLoader::vtkSMPQStateLoader()
{
  this->PQInternal = new vtkSMPQStateLoaderInternals;
  this->RenderViewXMLName = 0;
  this->SetRenderViewXMLName("RenderView");
}

//-----------------------------------------------------------------------------
vtkSMPQStateLoader::~vtkSMPQStateLoader()
{
  this->SetRenderViewXMLName(0);
  delete this->PQInternal;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMPQStateLoader::NewProxyInternal(
  const char* xml_group, const char* xml_name)
{
  // Check if the proxy requested is a render module.
  if (xml_group && xml_name && strcmp(xml_group, "views") == 0)
    {
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    vtkSMProxy* prototype = pxm->GetPrototypeProxy(xml_group, xml_name);
    if (prototype && prototype->IsA("vtkSMRenderViewProxy"))
      {
      if (!this->PQInternal->PreferredRenderViews.empty())
        {
        // Return a preferred render view if one exists.
        vtkSMRenderViewProxy *renMod = this->PQInternal->PreferredRenderViews.front();
        renMod->Register(this);
        this->PQInternal->PreferredRenderViews.pop_front();
        return renMod;
        }

      // Can't use exiting module (none present, or all present are have
      // already been used, hence we allocate a new one.
      return this->Superclass::NewProxyInternal(xml_group, 
        this->RenderViewXMLName);
      }
    }

  return this->Superclass::NewProxyInternal(xml_group, xml_name);
}

//---------------------------------------------------------------------------
void vtkSMPQStateLoader::AddPreferredRenderView(vtkSMRenderViewProxy *renderView)
{
  if(!renderView)
    { 
    vtkWarningMacro("Could not add preffered render module.");
    return;
    }
  // Make sure it is not part of the list yet
  vtkstd::list<vtkSmartPointer<vtkSMRenderViewProxy> >::iterator begin = 
    this->PQInternal->PreferredRenderViews.begin();
  vtkstd::list<vtkSmartPointer<vtkSMRenderViewProxy> >::iterator end = 
    this->PQInternal->PreferredRenderViews.end();
  if(find(begin,end,renderView) == end)
    {
    this->PQInternal->PreferredRenderViews.push_back(renderView);
    }
}

//---------------------------------------------------------------------------
void vtkSMPQStateLoader::RemovePreferredRenderView(vtkSMRenderViewProxy *renderView)
{
  this->PQInternal->PreferredRenderViews.remove(renderView);
}

//---------------------------------------------------------------------------
void vtkSMPQStateLoader::ClearPreferredRenderViews()
{
  this->PQInternal->PreferredRenderViews.clear();
}

//---------------------------------------------------------------------------
void vtkSMPQStateLoader::RegisterProxyInternal(const char* group,
  const char* name, vtkSMProxy* proxy)
{
  if (proxy->GetXMLGroup() 
    && strcmp(proxy->GetXMLGroup(), "views")==0)
    {
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    if (pxm->GetProxyName(group, proxy))
      {
      // render module is registered, don't re-register it.
      return;
      }
    }
  this->Superclass::RegisterProxyInternal(group, name, proxy);
}

//-----------------------------------------------------------------------------
void vtkSMPQStateLoader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RenderViewXMLName: " 
    << (this->RenderViewXMLName? this->RenderViewXMLName : "(none)")
    << endl;
}
