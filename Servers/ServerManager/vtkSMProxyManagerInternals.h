/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMProxyManagerInternals.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkSMProxyManagerInternals_h
#define __vtkSMProxyManagerInternals_h

#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMLink.h"
#include "vtkSMProxy.h"

#include <vtkstd/map>
#include <vtkstd/set>
#include <vtkstd/vector>
#include "vtkStdString.h"

// Sub-classed to avoid symbol length explosion.
class vtkSMProxyManagerElementMapType:
  public vtkstd::map<vtkStdString, vtkSmartPointer<vtkPVXMLElement> > {};

class vtkSMProxyManagerProxyInfo : public vtkObjectBase
{
public:
  vtkSmartPointer<vtkSMProxy> Proxy;
  unsigned long ModifiedObserverTag;
  unsigned long UpdateObserverTag;
  unsigned long UpdateInformationObserverTag;

  static vtkSMProxyManagerProxyInfo* New()
    {
    return new vtkSMProxyManagerProxyInfo();
    }

  // Description:
  // this needs to be overridden otherwise vtkDebugLeaks warnings are objects
  // are destroyed.
  void UnRegister()
    {
    int refcount = this->GetReferenceCount()-1;
    this->SetReferenceCount(refcount);
    if (refcount <= 0)
      {
#ifdef VTK_DEBUG_LEAKS
      vtkDebugLeaks::DestructClass("vtkSMProxyManagerProxyInfo");
#endif
      delete this;
      }
    }
  virtual void UnRegister(vtkObjectBase *)
    { this->UnRegister(); }

private:
  vtkSMProxyManagerProxyInfo()
    {
    this->ModifiedObserverTag = 0;
    this->UpdateObserverTag = 0;
    this->UpdateInformationObserverTag = 0;
    }
  ~vtkSMProxyManagerProxyInfo()
    {
    // Remove observers.
    if (this->ModifiedObserverTag && this->Proxy.GetPointer())
      {
      this->Proxy.GetPointer()->RemoveObserver(this->ModifiedObserverTag);
      this->ModifiedObserverTag = 0;
      }
    if (this->UpdateObserverTag && this->Proxy.GetPointer())
      {
      this->Proxy.GetPointer()->RemoveObserver(this->UpdateObserverTag);
      this->UpdateObserverTag = 0;
      }
    if (this->UpdateInformationObserverTag && this->Proxy.GetPointer())
      {
      this->Proxy.GetPointer()->RemoveObserver(
        this->UpdateInformationObserverTag);
      this->UpdateInformationObserverTag = 0;
      }
    }
};

//-----------------------------------------------------------------------------
class vtkSMProxyManagerProxyListType :
  public vtkstd::vector<vtkSmartPointer<vtkSMProxyManagerProxyInfo> >
{
public:
  // Returns if the proxy exists in  this vector.
  bool Contains(vtkSMProxy* proxy) 
    {
    vtkSMProxyManagerProxyListType::iterator iter =
      this->begin();
    for (; iter != this->end(); ++iter)
      {
      if (iter->GetPointer()->Proxy == proxy)
        {
        return true;
        }
      }
    return false;
    }
  vtkSMProxyManagerProxyListType::iterator Find(vtkSMProxy* proxy)
    {
    vtkSMProxyManagerProxyListType::iterator iter =
      this->begin();
    for (; iter != this->end(); ++iter)
      {
      if (iter->GetPointer()->Proxy.GetPointer() == proxy)
        {
        return iter;
        }
      }
    return this->end();
    }
};

//-----------------------------------------------------------------------------
class vtkSMProxyManagerProxyMapType:
  public vtkstd::map<vtkStdString, vtkSMProxyManagerProxyListType> {};

//-----------------------------------------------------------------------------
struct vtkSMProxyManagerInternals
{
  // This data structure stores the XML elements (prototypes) from which
  // proxies and properties are instantiated and initialized.
  typedef 
  vtkstd::map<vtkStdString, vtkSMProxyManagerElementMapType> GroupMapType;
  GroupMapType GroupMap;

  // This data structure stores actual proxy instances grouped in
  // collections.
  typedef 
  vtkstd::map<vtkStdString, vtkSMProxyManagerProxyMapType> ProxyGroupType;
  ProxyGroupType RegisteredProxyMap;

  // This data structure stores a set of proxies that have been modified.
  typedef vtkstd::set<vtkSMProxy*> SetOfProxies;
  SetOfProxies ModifiedProxies;

  typedef vtkstd::map<vtkStdString, vtkSmartPointer<vtkPVXMLElement> >
     DefinitionType;
  DefinitionType CompoundProxyDefinitions;

  // Data structure to save registered links.
  typedef vtkstd::map<vtkStdString, vtkSmartPointer<vtkSMLink> >
    LinkType;
  LinkType RegisteredLinkMap;
};

#endif

