/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqServerManagerObserver.cxx,v $

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

#include "pqServerManagerObserver.h"

#include "pqServer.h"
#include "pqXMLUtil.h"

#include "vtkObject.h"
#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMCompoundProxy.h"
#include "vtkSMInputProperty.h"
#include "vtkSMLookupTableProxy.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMViewProxy.h"
#include "vtkSMSourceProxy.h"

#include <QList>
#include <QMap>
#include <QtDebug>

//-----------------------------------------------------------------------------
class pqServerManagerObserverInternal
{
public:
  pqServerManagerObserverInternal()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    }
  /// Used to listen to proxy manager events.
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
};

//-----------------------------------------------------------------------------
pqServerManagerObserver::pqServerManagerObserver(QObject* p) : QObject(p)
{
  this->Internal = new pqServerManagerObserverInternal();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkSMProxyManager *proxyManager = vtkSMProxyManager::GetProxyManager();
  
  /// Listen to interesting events from the proxy manager and process module.
  this->Internal->VTKConnect->Connect(proxyManager, vtkCommand::RegisterEvent, this,
    SLOT(proxyRegistered(vtkObject*, unsigned long, void*, void*, vtkCommand*)),
    NULL, 1.0);

  this->Internal->VTKConnect->Connect(proxyManager, vtkCommand::UnRegisterEvent, this,
    SLOT(proxyUnRegistered(vtkObject*, unsigned long, void*, void*, vtkCommand*)),
    NULL, 1.0);

  this->Internal->VTKConnect->Connect(pm, vtkCommand::ConnectionCreatedEvent,
    this, SLOT(connectionCreated(vtkObject*, unsigned long, void*, void*)));
  this->Internal->VTKConnect->Connect(pm, vtkCommand::ConnectionClosedEvent,
    this, SLOT(connectionClosed(vtkObject*, unsigned long, void*, void*)));
}

//-----------------------------------------------------------------------------
pqServerManagerObserver::~pqServerManagerObserver()
{
  delete this->Internal;
}


//-----------------------------------------------------------------------------
void pqServerManagerObserver::proxyRegistered(vtkObject*, unsigned long, void*,
    void* callData, vtkCommand*)
{
  // Get the proxy information from the call data.
  vtkSMProxyManager::RegisteredProxyInformation *info =
    reinterpret_cast<vtkSMProxyManager::RegisteredProxyInformation *>(callData);
  if(!info || !this->Internal)
    {
    return;
    }



    if(info->IsCompoundProxyDefinition)
      {
      emit this->compoundProxyDefinitionRegistered(info->ProxyName);
      }
    else if (!info->IsLink && info->Proxy)
      {
      emit this->proxyRegistered(info->GroupName, info->ProxyName,
        info->Proxy);
      }
}

//-----------------------------------------------------------------------------
void pqServerManagerObserver::proxyUnRegistered(vtkObject*, unsigned long, void*,
    void* callData, vtkCommand*)
{
  // Get the proxy information from the call data.
  vtkSMProxyManager::RegisteredProxyInformation *info =
    reinterpret_cast<vtkSMProxyManager::RegisteredProxyInformation *>(callData);

  if(!info || !this->Internal)
    {
    return;
    }

    if(info->IsCompoundProxyDefinition)
      {
      emit this->compoundProxyDefinitionUnRegistered(info->ProxyName);
      }
    else

  if (!info->IsLink && info->Proxy)
    {
    emit this->proxyUnRegistered(info->GroupName, info->ProxyName,
      info->Proxy);
    }
}
//-----------------------------------------------------------------------------
void pqServerManagerObserver::connectionCreated(vtkObject*, unsigned long, void*, 
  void* callData)
{
  emit this->connectionCreated(*reinterpret_cast<vtkIdType*>(callData));
}

//-----------------------------------------------------------------------------
void pqServerManagerObserver::connectionClosed(vtkObject*, unsigned long, void*, 
  void* callData)
{
  emit this->connectionClosed(*reinterpret_cast<vtkIdType*>(callData));
}

