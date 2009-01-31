/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMViewProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMViewProxy.h"

#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkTimerLog.h"

#include <vtkstd/vector>

//----------------------------------------------------------------------------
class vtkSMViewProxy::Command : public vtkCommand
{
public:
  static Command* New() {  return new Command(); }
  virtual void Execute(vtkObject *caller, unsigned long eventId,
                       void *callData)
    {
    if (this->Target)
      {
      this->Target->ProcessEvents(caller, eventId, callData);
      }
    }
  void SetTarget(vtkSMViewProxy* t)
    {
    this->Target = t;
    }
private:
  Command() { this->Target = 0; }
  vtkSMViewProxy* Target;
};

//----------------------------------------------------------------------------
class vtkSMViewProxy::vtkMultiViewInitializer : public vtkstd::vector<vtkSMViewProxy*>
{
public:
  void InitializeForMultiView(vtkSMViewProxy* other)
    {
    const char* xmlgroup = other->GetXMLGroup();
    const char* xmlname = other->GetXMLName();

    vtkMultiViewInitializer::iterator iter = this->begin();
    for (;iter != this->end(); ++iter)
      {
      if ( 
        (*iter)->GetConnectionID() == other->GetConnectionID() &&
        strcmp((*iter)->GetXMLGroup(), xmlgroup)==0 &&
        strcmp((*iter)->GetXMLName(), xmlname) == 0 &&
        (*iter)->IsA(other->GetClassName()))
        {
        other->InitializeForMultiView(*iter);
        break;
        }
      }
    }

  void Add(vtkSMViewProxy* view)
    {
    this->push_back(view);
    }

  void Remove(vtkSMViewProxy* view)
    {
    vtkMultiViewInitializer::iterator iter = this->begin();
    for (;iter != this->end(); ++iter)
      {
      if ((*iter) == view)
        {
        this->erase(iter);
        break;
        }
      }
    }
};

vtkSMViewProxy::vtkMultiViewInitializer* vtkSMViewProxy::MultiViewInitializer =0;

//----------------------------------------------------------------------------
vtkSMViewProxy::vtkMultiViewInitializer* vtkSMViewProxy::GetMultiViewInitializer()
{
  if (!vtkSMViewProxy::MultiViewInitializer)
    {
    vtkSMViewProxy::MultiViewInitializer = new vtkSMViewProxy::vtkMultiViewInitializer();
    }

  return vtkSMViewProxy::MultiViewInitializer;
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::CleanMultiViewInitializer()
{
  if (vtkSMViewProxy::MultiViewInitializer &&
    vtkSMViewProxy::MultiViewInitializer->size() == 0)
    {
    delete vtkSMViewProxy::MultiViewInitializer;
    vtkSMViewProxy::MultiViewInitializer = 0;
    }
}

vtkStandardNewMacro(vtkSMViewProxy);
vtkCxxRevisionMacro(vtkSMViewProxy, "$Revision: 1.17 $");

vtkInformationKeyMacro(vtkSMViewProxy, USE_CACHE, Integer);
vtkInformationKeyMacro(vtkSMViewProxy, CACHE_TIME, Double);

//----------------------------------------------------------------------------
vtkSMViewProxy::vtkSMViewProxy()
{
  this->Representations = vtkCollection::New();
  this->Observer = vtkSMViewProxy::Command::New();
  this->Observer->SetTarget(this);
  this->Information = vtkInformation::New();

  this->GUISize[0] = this->GUISize[1] = 300;
  this->ViewPosition[0] = this->ViewPosition[1] = 0;

  this->DisplayedDataSize = 0;
  this->DisplayedDataSizeValid = false;

  this->FullResDataSize = 0;
  this->FullResDataSizeValid = false;

  this->ForceRepresentationUpdate = false;

  this->DefaultRepresentationName = 0;
  this->ViewUpdateTime = 0;
  this->ViewUpdateTimeInitialized = false;

  this->UseCache = false;
  this->CacheTime = 0.0;

  this->Information->Set(USE_CACHE(), this->UseCache);
  this->Information->Set(CACHE_TIME(), this->CacheTime);

  this->InRender = false;
}

//----------------------------------------------------------------------------
vtkSMViewProxy::~vtkSMViewProxy()
{
  vtkSMViewProxy::GetMultiViewInitializer()->Remove(this);
  vtkSMViewProxy::CleanMultiViewInitializer();

  this->Observer->SetTarget(0);
  this->Observer->Delete();

  this->RemoveAllRepresentations();
  this->Representations->Delete();

  this->SetDefaultRepresentationName(0);

  this->Information->Clear();
  this->Information->Delete();
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::SetViewUpdateTime(double time)
{
  if (!this->ViewUpdateTimeInitialized || this->ViewUpdateTime != time)
    {
    this->ViewUpdateTimeInitialized = true;
    this->ViewUpdateTime = time;

    // Send the view update time to all representations.
    vtkSmartPointer<vtkCollectionIterator> iter;
    iter.TakeReference(this->Representations->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); 
      iter->GoToNextItem())
      {
      vtkSMRepresentationProxy* repr = 
        vtkSMRepresentationProxy::SafeDownCast(iter->GetCurrentObject());
      if (repr)
        {
        repr->SetViewUpdateTime(time);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  vtkSMViewProxy::GetMultiViewInitializer()->InitializeForMultiView(this);

  if (!this->BeginCreateVTKObjects())
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  this->EndCreateVTKObjects();

  vtkSMViewProxy::GetMultiViewInitializer()->Add(this);
}

//----------------------------------------------------------------------------
vtkCommand* vtkSMViewProxy::GetObserver()
{
  return this->Observer;
}

//----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMViewProxy::CreateDefaultRepresentation(
  vtkSMProxy* vtkNotUsed(proxy), int vtkNotUsed(opport))
{
  if (this->DefaultRepresentationName)
    {
    vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
    vtkSmartPointer<vtkSMProxy> p;
    p.TakeReference(pxm->NewProxy("representations", this->DefaultRepresentationName));
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(p);
    if (repr)
      {
      repr->Register(this);
      return repr;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::AddRepresentation(vtkSMRepresentationProxy* repr)
{
  if (repr && !this->Representations->IsItemPresent(repr))
    {
    if (repr->AddToView(this))
      {
      this->AddRepresentationInternal(repr);
      }
    else
      {
      vtkErrorMacro(<< repr->GetClassName() << " cannot be added to view "
        << "of type " << this->GetClassName());
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::AddRepresentationInternal(vtkSMRepresentationProxy* repr)
{
  this->InvalidateDataSizes();

  // Pass the view information to the representation.
  repr->SetViewInformation(this->Information);

  this->Representations->AddItem(repr);

  // If representation is modified, we invalidate the data information.
  repr->AddObserver(vtkCommand::ModifiedEvent, this->Observer);

  // If representation pipeline is updated, we invalidate the data info.
  repr->AddObserver(vtkCommand::EndEvent, this->Observer);

  // repr->AddObserver(vtkCommand::SelectionChanged, this->Observer);

  // Pass the view update time to the representation.
  if (this->ViewUpdateTimeInitialized)
    {
    repr->SetViewUpdateTime(this->ViewUpdateTime);
    }
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::RemoveRepresentation(vtkSMRepresentationProxy* repr)
{
  if (repr)
    {
    repr->RemoveFromView(this);
    this->RemoveRepresentationInternal(repr);
    }
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::RemoveRepresentationInternal(vtkSMRepresentationProxy* repr)
{
  this->InvalidateDataSizes();

  repr->SetViewInformation(0);

  repr->RemoveObserver(this->Observer);
  this->Representations->RemoveItem(repr);
  // Don't access repr after removing, it may be already deleted.
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::RemoveAllRepresentations()
{
  while (this->Representations->GetNumberOfItems() > 0)
    {
    this->RemoveRepresentation(
      vtkSMRepresentationProxy::SafeDownCast(
        this->Representations->GetItemAsObject(0)));
    }
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::BeginStillRender()
{
  int interactive = 0;
  this->InvokeEvent(vtkCommand::StartEvent, &interactive);

  vtkTimerLog::MarkStartEvent("Still Render");
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::EndStillRender()
{
  vtkTimerLog::MarkEndEvent("Still Render");

  int interactive = 0;
  this->InvokeEvent(vtkCommand::EndEvent, &interactive);
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::BeginInteractiveRender()
{
  int interactive = 1;
  this->InvokeEvent(vtkCommand::StartEvent, &interactive);

  vtkTimerLog::MarkStartEvent("Interactive Render");
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::EndInteractiveRender()
{
  vtkTimerLog::MarkEndEvent("Interactive Render");

  int interactive = 1;
  this->InvokeEvent(vtkCommand::EndEvent, &interactive);
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::StillRender()
{
  if (this->InRender)
    {
    return;
    }

  this->InRender = true;
  // Ensure that all representation pipelines are updated.
  this->UpdateAllRepresentations();

  this->ForceRepresentationUpdate = false;
  this->BeginStillRender();
  if (this->ForceRepresentationUpdate)
    {
    // BeginStillRender modified the representations, we need another update.
    this->UpdateAllRepresentations();
    }
  this->PerformRender();
  this->EndStillRender();
  this->InRender = false;
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::InteractiveRender()
{
  if (this->InRender)
    {
    return;
    }

  this->InRender = true;
  // Ensure that all representation pipelines are updated.
  this->UpdateAllRepresentations();

  this->ForceRepresentationUpdate = false;
  this->BeginInteractiveRender();
  if (this->ForceRepresentationUpdate)
    {
    // BeginInteractiveRender modified the representations, we need another 
    // update.
    this->UpdateAllRepresentations();
    }
  this->PerformRender();
  this->EndInteractiveRender();
  this->InRender = false;
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::UpdateAllRepresentations()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkSmartPointer<vtkCollectionIterator> iter;
  iter.TakeReference(this->Representations->NewIterator());

  // We are calling SendPrepareProgress-SendCleanupPendingProgress
  // only if any representation is indeed going to update. 
  // I wonder of this condition is required and we can;t always
  // called these methods. But since there's minimal effort in calling 
  // these conditionally, I am letting it be (erring on the safer side).
  bool enable_progress = false;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMRepresentationProxy* repr = 
      vtkSMRepresentationProxy::SafeDownCast(iter->GetCurrentObject());
    if (!repr->GetVisibility())
      {
      // Invisible representations are not updated.
      continue;
      }

    if (!enable_progress && repr->UpdateRequired())
      {
      // If a representation required an update, than it implies that the
      // update will result in progress events. We don't to ignore those
      // progress events, hence we enable progress handling.
      pm->SendPrepareProgress(this->ConnectionID,
        vtkProcessModule::CLIENT | vtkProcessModule::DATA_SERVER);
      enable_progress = true;
      }

    repr->Update(this);
    }

  if (enable_progress)
    {
    pm->SendCleanupPendingProgress(this->ConnectionID);
    }
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::ProcessEvents(vtkObject* caller, unsigned long eventId, 
    void* callData)
{
  (void)caller;
  (void)eventId;
  (void)callData;

  if (vtkSMRepresentationProxy::SafeDownCast(caller) && 
    ( eventId == vtkCommand::ModifiedEvent  ||
      eventId == vtkCommand::EndEvent))
    {
    this->InvalidateDataSizes();
    }
}

//-----------------------------------------------------------------------------
void vtkSMViewProxy::Connect(vtkSMProxy* producer, vtkSMProxy* consumer,
    const char* propertyname/*="Input"*/)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    consumer->GetProperty(propertyname));
  if (!pp)
    {
    vtkErrorMacro("Failed to locate property " << propertyname
      << " on " << consumer->GetXMLName());
    return;
    }

  pp->AddProxy(producer);
  consumer->UpdateProperty(propertyname);
}

//----------------------------------------------------------------------------
vtkSMRepresentationStrategy* vtkSMViewProxy::NewStrategy(int dataType)
{
  vtkSMRepresentationStrategy* strategy = 
    this->NewStrategyInternal(dataType);

  if (strategy)
    {
    strategy->SetConnectionID(this->ConnectionID);

    // Pass the view information to the strategy.
    strategy->SetViewInformation(this->Information);
    }

  return strategy;
}

//----------------------------------------------------------------------------
unsigned long vtkSMViewProxy::GetVisibleDisplayedDataSize()
{
  if (!this->DisplayedDataSizeValid)
    {
    this->DisplayedDataSizeValid = true;
    this->DisplayedDataSize = 0;

    vtkSmartPointer<vtkCollectionIterator> iter;
    iter.TakeReference(this->Representations->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      vtkSMRepresentationProxy* repr = 
        vtkSMRepresentationProxy::SafeDownCast(iter->GetCurrentObject());
      if (!repr->GetVisibility())
        {
        // Skip invisible representations.
        continue;
        }

      vtkPVDataInformation* info = repr->GetDisplayedDataInformation();
      if (info)
        {
        this->DisplayedDataSize += info->GetMemorySize();
        }
      }
    }

  return this->DisplayedDataSize;
}

//----------------------------------------------------------------------------
unsigned long vtkSMViewProxy::GetVisibileFullResDataSize()
{
  if (!this->FullResDataSizeValid)
    {
    this->FullResDataSize = 0;
    this->FullResDataSizeValid = true;

    vtkSmartPointer<vtkCollectionIterator> iter;
    iter.TakeReference(this->Representations->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      vtkSMRepresentationProxy* repr = 
        vtkSMRepresentationProxy::SafeDownCast(iter->GetCurrentObject());
      if (!repr->GetVisibility())
        {
        // Skip invisible representations.
        continue;
        }

      vtkPVDataInformation* info = repr->GetFullResDataInformation();
      if (info)
        {
        this->FullResDataSize += info->GetMemorySize();
        }
      }
    }

  return this->FullResDataSize;
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::InvalidateDataSizes()
{
  this->FullResDataSizeValid = false;
  this->DisplayedDataSizeValid = false;
}

//----------------------------------------------------------------------------
int vtkSMViewProxy::ReadXMLAttributes(
  vtkSMProxyManager* pm, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(pm, element))
    {
    return 0;
    }

  const char* repr_name = element->GetAttribute("representation_name");
  if (repr_name)
    {
    this->SetDefaultRepresentationName(repr_name);
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMViewProxy::SetCacheTime(double time)
{
  this->Information->Set(CACHE_TIME(), time);
  this->CacheTime = time;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSMViewProxy::SetUseCache(int usecache)
{
  this->Information->Set(USE_CACHE(), usecache);
  this->UseCache = usecache;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "GUISize: " 
    << this->GUISize[0] << ", " << this->GUISize[1] << endl;
  os << indent << "ViewPosition: " 
    << this->ViewPosition[0] << ", " << this->ViewPosition[1] << endl;
  os << indent << "ViewUpdateTime: " << this->ViewUpdateTime << endl;
  os << indent << "UseCache: " << this->UseCache << endl;
  os << indent << "CacheTime: " << this->CacheTime << endl;
}


