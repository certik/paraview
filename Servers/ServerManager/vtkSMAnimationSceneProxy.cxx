/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMAnimationSceneProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAnimationSceneProxy.h"

#include "vtkPVAnimationScene.h"
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVCacheSizeInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkCollection.h"

#include <vtksys/SystemTools.hxx>
#include <vtkstd/vector>

//----------------------------------------------------------------------------
class vtkSMAnimationSceneProxy::vtkInternals
{
public:
  typedef vtkstd::vector<vtkSmartPointer<vtkSMViewProxy> > 
    VectorOfViews;
  VectorOfViews ViewModules;
  vtkCollection* AnimationCues;

  vtkInternals()
    {
    this->AnimationCues = vtkCollection::New();
    }

  ~vtkInternals()
    {
    this->AnimationCues->Delete();
    this->AnimationCues = 0;
    }


  void StillRenderAllViews()
    {
    VectorOfViews::iterator iter = this->ViewModules.begin();
    for (; iter != this->ViewModules.end(); ++iter)
      {
      iter->GetPointer()->StillRender();
      }
    }

  void PassCacheTime(double cachetime)
    {
    VectorOfViews::iterator iter = this->ViewModules.begin();
    for (; iter != this->ViewModules.end(); ++iter)
      {
      (*iter)->SetCacheTime(cachetime);
      }
    }

  void PassUseCache(bool usecache)
    {
    VectorOfViews::iterator iter = this->ViewModules.begin();
    for (; iter != this->ViewModules.end(); ++iter)
      {
      (*iter)->SetUseCache(usecache);
      }
    }

  void DisableInteractionAllViews()
    {
    VectorOfViews::iterator iter = this->ViewModules.begin();
    for (; iter != this->ViewModules.end(); ++iter)
      {
      vtkSMRenderViewProxy* rm = vtkSMRenderViewProxy::SafeDownCast(
        iter->GetPointer());
      if (rm)
        {
        rm->GetInteractor()->Disable();
        }
      }
    }

  void EnableInteractionAllViews()
    {
    VectorOfViews::iterator iter = this->ViewModules.begin();
    for (; iter != this->ViewModules.end(); ++iter)
      {
      vtkSMRenderViewProxy* rm = vtkSMRenderViewProxy::SafeDownCast(
        iter->GetPointer());
      if (rm)
        {
        rm->GetInteractor()->Enable();
        }
      }
    }

};

class vtkSMAnimationSceneProxy::vtkPlayerObserver : public vtkCommand
{
public:
  static vtkPlayerObserver* New()
    {
    return new vtkPlayerObserver();
    }
  virtual void Execute(vtkObject*, unsigned long event, void* data)
    {
    if (this->Target)
      {
      if (event == vtkCommand::StartEvent)
        {
        this->Target->OnStartPlay(); 
        }
      else if (event == vtkCommand::EndEvent)
        {
        this->Target->OnEndPlay();
        }
      this->Target->InvokeEvent(event, data);
      }
    }

  void SetTarget(vtkSMAnimationSceneProxy* t)
    {
    this->Target = t;
    }

private:
  vtkPlayerObserver()
    {
    this->Target = 0;
    }
  vtkSMAnimationSceneProxy* Target;
};


vtkCxxRevisionMacro(vtkSMAnimationSceneProxy, "$Revision: 1.53 $");
vtkStandardNewMacro(vtkSMAnimationSceneProxy);
//----------------------------------------------------------------------------
vtkSMAnimationSceneProxy::vtkSMAnimationSceneProxy()
{
  this->OverrideStillRender = 0;
  this->Internals = new vtkInternals();
  this->CacheLimit = 100*1024; // 100 MBs.
  this->Caching = 0;
  this->AnimationPlayer = 0;
  this->PlayerObserver = vtkPlayerObserver::New();
  this->PlayerObserver->SetTarget(this);
  this->InTick = false;
}

//----------------------------------------------------------------------------
vtkSMAnimationSceneProxy::~vtkSMAnimationSceneProxy()
{
  if (this->AnimationPlayer)
    {
    this->AnimationPlayer->RemoveObserver(this->PlayerObserver);
    }
  this->PlayerObserver->SetTarget(0);
  this->PlayerObserver->Delete();
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::AddCueProxy(vtkSMAnimationCueProxy* cueProxy)
{
  if (cueProxy && !this->Internals->AnimationCues->IsItemPresent(cueProxy))
    {
    this->CreateVTKObjects();
    cueProxy->CreateVTKObjects();
    vtkPVAnimationScene::SafeDownCast(this->AnimationCue)->AddCue(
      cueProxy->AnimationCue);
    this->Internals->AnimationCues->AddItem(cueProxy);
    }
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::RemoveCueProxy(vtkSMAnimationCueProxy* cueProxy)
{
  if (cueProxy && this->Internals->AnimationCues->IsItemPresent(cueProxy))
    {
    vtkPVAnimationScene::SafeDownCast(this->AnimationCue)->RemoveCue(
      cueProxy->AnimationCue);
    this->Internals->AnimationCues->RemoveItem(cueProxy);
    }
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::AddViewModule(vtkSMViewProxy* view)
{
  vtkInternals::VectorOfViews::iterator iter = 
    this->Internals->ViewModules.begin();
  for (; iter != this->Internals->ViewModules.end(); ++iter)
    {
    if (iter->GetPointer() == view)
      {
      // already added.
      return;
      }
    }
  this->Internals->ViewModules.push_back(view);
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::RemoveViewModule(
  vtkSMViewProxy* view)
{
  vtkInternals::VectorOfViews::iterator iter = 
    this->Internals->ViewModules.begin();
  for (; iter != this->Internals->ViewModules.end(); ++iter)
    {
    if (iter->GetPointer() == view)
      {
      this->Internals->ViewModules.erase(iter);
      break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::RemoveAllViewModules()
{
  this->Internals->ViewModules.clear();
}

//----------------------------------------------------------------------------
unsigned int vtkSMAnimationSceneProxy::GetNumberOfViewModules()
{
  return this->Internals->ViewModules.size();
}

//----------------------------------------------------------------------------
vtkSMViewProxy* vtkSMAnimationSceneProxy::GetViewModule(
  unsigned int cc)
{
  if (cc < this->Internals->ViewModules.size())
    {
    return this->Internals->ViewModules[cc];
    }
  return 0;
}


//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->AnimationPlayer = this->GetSubProxy("AnimationPlayer");
  if (!this->AnimationPlayer)
    {
    vtkErrorMacro("Missing animation player subproxy");
    return;
    }

  this->Superclass::CreateVTKObjects();

  if (!this->ObjectsCreated)
    {
    return;
    }

  // Fire events from the player.
  this->AnimationPlayer->AddObserver(vtkCommand::StartEvent, this->PlayerObserver);
  this->AnimationPlayer->AddObserver(vtkCommand::EndEvent, this->PlayerObserver);
  this->AnimationPlayer->AddObserver(vtkCommand::ProgressEvent, this->PlayerObserver);

  // Set the animation scene pointer on the player.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << this->AnimationPlayer->GetID()
          << "SetAnimationScene"
          << this->GetID()
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers, stream);
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::StartCueInternal(void* info)
{
  if (!this->OverrideStillRender)
    {
    this->Internals->StillRenderAllViews();
    }
  this->Superclass::StartCueInternal(info);
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::TickInternal(void* info)
{
  this->InTick = true;
  this->CacheUpdate(info);

  if (!this->OverrideStillRender)
    {
    // Render All Views.
    this->Internals->StillRenderAllViews();
    }

  this->Superclass::TickInternal(info);
  this->InTick = false;
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::EndCueInternal(void* info)
{
  this->CacheUpdate(info);
  if (!this->OverrideStillRender)
    {
    this->Internals->StillRenderAllViews();
    }
  this->Superclass::EndCueInternal(info);

}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::OnStartPlay()
{
  this->Internals->DisableInteractionAllViews();
  this->Internals->PassUseCache(this->GetCaching()>0);
}

//-----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::OnEndPlay()
{
  this->Internals->PassUseCache(false);
  this->Internals->EnableInteractionAllViews();
}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneProxy::CheckCacheSizeWithinLimit()
{
  vtkSmartPointer<vtkPVCacheSizeInformation> info = 
    vtkSmartPointer<vtkPVCacheSizeInformation>::New();
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->GatherInformation(this->ConnectionID,
    vtkProcessModule::RENDER_SERVER, info, pm->GetProcessModuleID());

  vtkSmartPointer<vtkPVCacheSizeInformation> clientinfo = 
    vtkSmartPointer<vtkPVCacheSizeInformation>::New();
  clientinfo->CopyFromObject(pm);
  clientinfo->AddInformation(info);

  return (clientinfo->GetCacheSize() < static_cast<unsigned long>(this->CacheLimit));
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::CacheUpdate(void* info)
{
  if (!this->GetCaching())
    {
    return;
    }

  // Check if the cache has overgrown the limit.
  int cachefull = this->CheckCacheSizeWithinLimit()? 0 : 1;

  // Update cache full status on the cache size keeper so that all update
  // suppressors i.e. cache maintainer will be made aware.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream; 
  stream  << vtkClientServerStream::Invoke
          << pm->GetProcessModuleID()
          << "GetCacheSizeKeeper"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << vtkClientServerStream::LastResult
          << "SetCacheFull"
          << cachefull
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, 
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER,
    stream);

  vtkAnimationCue::AnimationCueInfo *cueInfo = reinterpret_cast<
    vtkAnimationCue::AnimationCueInfo*>(info);

  double cachetime = cueInfo->AnimationTime;
  this->Internals->PassCacheTime(cachetime);
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::SetAnimationTime(double time)
{
  if (this->InTick)
    {
    return;
    }

  vtkPVAnimationScene* scene = 
    vtkPVAnimationScene::SafeDownCast(this->AnimationCue);
  if (scene)
    {
    this->Internals->PassUseCache(this->GetCaching()>0);
    scene->SetSceneTime(time);
    this->Internals->PassUseCache(false);
    }
}

//----------------------------------------------------------------------------
double vtkSMAnimationSceneProxy::GetAnimationTime()
{
  vtkPVAnimationScene* scene = 
    vtkPVAnimationScene::SafeDownCast(this->AnimationCue);
  if (scene)
    {
    return scene->GetSceneTime();
    }
  return 0.0;
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "OverrideStillRender: " << this->OverrideStillRender << endl;
  os << indent << "CacheLimit: " << this->CacheLimit << endl;
}
