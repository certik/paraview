/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMSimpleStrategy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSimpleStrategy.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMSimpleStrategy);
vtkCxxRevisionMacro(vtkSMSimpleStrategy, "$Revision: 1.11 $");
//----------------------------------------------------------------------------
vtkSMSimpleStrategy::vtkSMSimpleStrategy()
{
  this->LODDecimator = 0;
  this->UpdateSuppressor = 0;
  this->UpdateSuppressorLOD = 0;
  this->SetEnableLOD(true);
  this->SomethingCached = false;
  this->SomethingCachedLOD = false;
}

//----------------------------------------------------------------------------
vtkSMSimpleStrategy::~vtkSMSimpleStrategy()
{

}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::BeginCreateVTKObjects()
{
  this->Superclass::BeginCreateVTKObjects();

  this->LODDecimator = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("LODDecimator"));
  this->UpdateSuppressor = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("UpdateSuppressor"));
  this->UpdateSuppressorLOD = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("UpdateSuppressorLOD"));

  this->UpdateSuppressor->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);

  if (this->LODDecimator && this->UpdateSuppressorLOD)
    {
    this->LODDecimator->SetServers(vtkProcessModule::DATA_SERVER);
    this->UpdateSuppressorLOD->SetServers(vtkProcessModule::CLIENT_AND_SERVERS);
    }
  else
    {
    this->SetEnableLOD(false);
    }

}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::EndCreateVTKObjects()
{
  this->Superclass::EndCreateVTKObjects();

  // Update piece information on each of the update suppressors.
  this->UpdatePieceInformation(this->UpdateSuppressor);
  if (this->GetEnableLOD())
    {
    this->UpdatePieceInformation(this->UpdateSuppressorLOD);
    }

}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::UpdatePieceInformation(vtkSMSourceProxy* updateSuppressor)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;

  // Init UpdateSuppressor properties.
  // Seems like we can't use properties for this 
  // to work properly.
  stream
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetNumberOfLocalPartitions"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << updateSuppressor->GetID() << "SetUpdateNumberOfPieces"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  stream
    << vtkClientServerStream::Invoke
    << pm->GetProcessModuleID() << "GetPartitionId"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << updateSuppressor->GetID() << "SetUpdatePiece"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, updateSuppressor->GetServers(), stream);
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::CreatePipeline(vtkSMSourceProxy* input, int outputport)
{
  this->Connect(input, this->UpdateSuppressor, "Input", outputport);
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::CreateLODPipeline(vtkSMSourceProxy* input, int outputport)
{
  this->Connect(input, this->LODDecimator, "Input", outputport);
  this->Connect(this->LODDecimator, this->UpdateSuppressorLOD, "Input", 0);
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::GatherInformation(vtkPVDataInformation* info)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->GatherInformation(this->ConnectionID,
    this->UpdateSuppressor->GetServers(),
    info,
    this->UpdateSuppressor->GetID());
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::GatherLODInformation(vtkPVDataInformation* info)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->GatherInformation(this->ConnectionID,
    this->UpdateSuppressorLOD->GetServers(),
    info,
    this->UpdateSuppressorLOD->GetID());
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::UpdatePipeline()
{
  if (this->GetUseCache())
    {
    this->SomethingCached = true;
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->UpdateSuppressor->GetProperty("CacheUpdate"));
    dvp->SetElement(0, this->CacheTime);
    this->UpdateSuppressor->UpdateProperty("CacheUpdate", 1);
    }
  else
    {
    this->UpdateSuppressor->InvokeCommand("ForceUpdate");
    }
  this->Superclass::UpdatePipeline();
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::UpdateLODPipeline()
{
  if (this->GetUseCache())
    {
    this->SomethingCachedLOD = true;
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->UpdateSuppressorLOD->GetProperty("CacheUpdate"));
    dvp->SetElement(0, this->CacheTime);
    this->UpdateSuppressorLOD->UpdateProperty("CacheUpdate", 1);
    }
  else
    {
    this->UpdateSuppressorLOD->InvokeCommand("ForceUpdate");
    }

  this->Superclass::UpdateLODPipeline();
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::InvalidatePipeline()
{
  // Cache is cleaned up whenever something changes and caching is not currently
  // enabled.
  if (this->SomethingCached && !this->GetUseCache())
    {
    this->SomethingCached = false;
    this->UpdateSuppressor->InvokeCommand("RemoveAllCaches");
    }

  this->Superclass::InvalidatePipeline();
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::InvalidateLODPipeline()
{
  if (this->SomethingCachedLOD && !this->GetUseCache())
    {
    this->SomethingCachedLOD = false;
    this->UpdateSuppressorLOD->InvokeCommand("RemoveAllCaches");
    }

  this->Superclass::InvalidateLODPipeline();
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::SetLODResolution(int resolution)
{
  this->Superclass::SetLODResolution(resolution);

  if (this->LODDecimator)
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->LODDecimator->GetProperty("NumberOfDivisions"));
    if (ivp)
      {
      ivp->SetElement(0, this->LODResolution);
      ivp->SetElement(1, this->LODResolution);
      ivp->SetElement(2, this->LODResolution);
      this->LODDecimator->UpdateVTKObjects();
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMSimpleStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


