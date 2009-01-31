/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMCameraProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCameraProxy.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkCamera.h"

vtkStandardNewMacro(vtkSMCameraProxy);
vtkCxxRevisionMacro(vtkSMCameraProxy, "$Revision: 1.3 $");
//-----------------------------------------------------------------------------
vtkSMCameraProxy::vtkSMCameraProxy()
{
}

//-----------------------------------------------------------------------------
vtkSMCameraProxy::~vtkSMCameraProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMCameraProxy::UpdatePropertyInformation()
{
  if (this->InUpdateVTKObjects)
    {
    return;
    }

  vtkCamera* camera = vtkCamera::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetObjectFromID(this->GetID()));
  if (!camera)
    {
    this->Superclass::UpdatePropertyInformation();
    return;
    }

  vtkSMDoubleVectorProperty* dvp;
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("CameraPositionInfo"));
  dvp->SetElements(camera->GetPosition());

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("CameraFocalPointInfo"));
  dvp->SetElements(camera->GetFocalPoint());

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("CameraViewUpInfo"));
  dvp->SetElements(camera->GetViewUp());

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetProperty("CameraClippingRangeInfo"));
  dvp->SetElements(camera->GetClippingRange());
}

//-----------------------------------------------------------------------------
void vtkSMCameraProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
