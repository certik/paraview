/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMUniformGridVolumeRepresentationProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUniformGridVolumeRepresentationProxy.h"

#include "vtkAbstractMapper.h"
#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProp3D.h"
#include "vtkSelection.h"
#include "vtkSelectionSerializer.h"
#include "vtkSmartPointer.h"
#include "vtkSMCompoundProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationStrategy.h"
#include "vtkSMSelectionHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMUniformGridVolumeRepresentationProxy);
vtkCxxRevisionMacro(vtkSMUniformGridVolumeRepresentationProxy, "$Revision: 1.11 $");
//----------------------------------------------------------------------------
vtkSMUniformGridVolumeRepresentationProxy::vtkSMUniformGridVolumeRepresentationProxy()
{
  this->VolumeFixedPointRayCastMapper = 0;
  this->VolumeActor = 0;
  this->VolumeProperty = 0;
  this->ClientMapper = 0;
}

//----------------------------------------------------------------------------
vtkSMUniformGridVolumeRepresentationProxy::~vtkSMUniformGridVolumeRepresentationProxy()
{
  this->VolumeFixedPointRayCastMapper = 0;
  this->VolumeActor = 0;
  this->VolumeProperty = 0;
}

//----------------------------------------------------------------------------
bool vtkSMUniformGridVolumeRepresentationProxy::InitializeStrategy(vtkSMViewProxy* view)
{
  vtkSmartPointer<vtkSMRepresentationStrategy> strategy;
  strategy.TakeReference(view->NewStrategy(VTK_UNIFORM_GRID));
  if (!strategy.GetPointer())
    {
    vtkErrorMacro("View could not provide a strategy to use. "
      << "Cannot be rendered in this view of type " << view->GetClassName());
    return false;
    }



  // Now initialize the data pipelines involving this strategy.
  // Since representations are not added to views unless their input is set, we
  // can assume that the objects for this proxy have been created.
  // (Look at vtkSMDataRepresentationProxy::AddToView()).

  // This representation interprets LOD to mean client-side data. Hence, LOD
  // pipeline is enabled only when client-server are separate processes. 
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm->IsRemote(this->ConnectionID))
    {
    strategy->SetEnableLOD(false);
    }

  this->Connect(this->GetInputProxy(), strategy, "Input", this->OutputPort);
  this->Connect(strategy->GetOutput(), this->VolumeFixedPointRayCastMapper);
  this->Connect(strategy->GetLODOutput(), this->ClientMapper);

  // Creates the strategy objects.
  strategy->UpdateVTKObjects();
  this->AddStrategy(strategy);

  return this->Superclass::InitializeStrategy(view);
}

//----------------------------------------------------------------------------
bool vtkSMUniformGridVolumeRepresentationProxy::BeginCreateVTKObjects()
{
  if (!this->Superclass::BeginCreateVTKObjects())
    {
    return false;
    }

  // Set server flags correctly on all subproxies.
  this->VolumeFixedPointRayCastMapper = this->GetSubProxy(
    "VolumeFixedPointRayCastMapper");
  this->VolumeActor = this->GetSubProxy("Prop3D");
  this->VolumeProperty = this->GetSubProxy("VolumeProperty");
  this->ClientMapper = this->GetSubProxy("LODMapper");

  this->VolumeFixedPointRayCastMapper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->VolumeActor->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->VolumeProperty->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->ClientMapper->SetServers(
    vtkProcessModule::CLIENT);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMUniformGridVolumeRepresentationProxy::EndCreateVTKObjects()
{
  this->Connect(this->VolumeFixedPointRayCastMapper, this->VolumeActor, "Mapper");
  this->Connect(this->VolumeProperty, this->VolumeActor, "Property");

  // This representation interprets LOD to mean client-side data. Hence, LOD
  // pipeline is enabled only when client-server are separate processes. 
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (pm->IsRemote(this->ConnectionID))
    {
    this->VolumeActor->UpdateVTKObjects();

    vtkClientServerStream stream;
    stream  << vtkClientServerStream::Invoke
            << this->VolumeActor->GetID()
            << "SetEnableLOD" << 1
            << vtkClientServerStream::End;
    stream  << vtkClientServerStream::Invoke
            << this->VolumeActor->GetID()
            << "SetLODMapper"
            << this->ClientMapper->GetID()
            << vtkClientServerStream::End;
    vtkProcessModule::GetProcessModule()->SendStream(
      this->ConnectionID, vtkProcessModule::CLIENT, stream);   
    }

  return this->Superclass::EndCreateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMUniformGridVolumeRepresentationProxy::SetColorArrayName(
  const char* name)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    this->VolumeFixedPointRayCastMapper->GetProperty("SelectScalarArray"));

  if (name && name[0])
    {
    svp->SetElement(0, name);
    }
  else
    {
    svp->SetElement(0, "");
    }

  this->VolumeFixedPointRayCastMapper->UpdateVTKObjects();;
}

//----------------------------------------------------------------------------
void vtkSMUniformGridVolumeRepresentationProxy::SetColorAttributeType(
  int type)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->VolumeFixedPointRayCastMapper->GetProperty("ScalarMode"));
  switch (type)
    {
  case POINT_DATA:
    ivp->SetElement(0, VTK_SCALAR_MODE_USE_POINT_FIELD_DATA); 
    break;

  case CELL_DATA:
    ivp->SetElement(0, VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
    break;

  case FIELD_DATA:
    ivp->SetElement(0, VTK_SCALAR_MODE_USE_FIELD_DATA);
    break;

  default:
    ivp->SetElement(0,  VTK_SCALAR_MODE_DEFAULT);
    }

  this->VolumeFixedPointRayCastMapper->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
bool vtkSMUniformGridVolumeRepresentationProxy::HasVisibleProp3D(vtkProp3D* prop)
{
  if(!prop)
  {
    return false;
  }

  if(this->Superclass::HasVisibleProp3D(prop))
  {
    return true;
  }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  if (this->GetVisibility() && 
    pm->GetIDFromObject(prop) == this->VolumeActor->GetID())
  {
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMUniformGridVolumeRepresentationProxy::ConvertSelection(
  vtkSelection* userSel)
{
  if (!this->GetVisibility())
    {
    return 0;
    }

  vtkSmartPointer<vtkSelection> mySelection = 
    vtkSmartPointer<vtkSelection>::New();
  mySelection->GetProperties()->Copy(userSel->GetProperties(), 0);

  unsigned int numChildren = userSel->GetNumberOfChildren();
  for (unsigned int cc=0; cc < numChildren; cc++)
    {
    vtkSelection* child = userSel->GetChild(cc);
    vtkInformation* properties = child->GetProperties();
    // If there is no PROP_ID or PROP key set, we assume the selection
    // is valid on all representations
    bool hasProp = true;
    if (properties->Has(vtkSelection::PROP_ID()))
      {
      hasProp = false;
      vtkClientServerID propId;

      propId.ID = static_cast<vtkTypeUInt32>(properties->Get(
        vtkSelection::PROP_ID()));
      if (propId == this->VolumeActor->GetID())
        {
        hasProp = true;
        }
      }
    else if(properties->Has(vtkSelection::PROP()))
      {
      hasProp = false;
      vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
      if (properties->Get(vtkSelection::PROP()) == 
        pm->GetObjectFromID(this->VolumeActor->GetID()))
        {
        hasProp = true;
        }
      }
    if(hasProp)
      {
      vtkSelection* myChild = vtkSelection::New();
      myChild->ShallowCopy(child);
      mySelection->AddChild(myChild);
      myChild->Delete();
      }
    }

  if (mySelection->GetNumberOfChildren() == 0)
    {
    return 0;
    }

  // Create a selection source for the selection.
  vtkSMProxy* selectionSource = 
    vtkSMSelectionHelper::NewSelectionSourceFromSelection(
      this->ConnectionID, mySelection);
  
  return selectionSource;
}

//----------------------------------------------------------------------------
void vtkSMUniformGridVolumeRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


