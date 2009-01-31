/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVNumberOfOutputsInformation.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVNumberOfOutputsInformation.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerStream.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkObjectFactory.h"
#include "vtkSource.h"

vtkStandardNewMacro(vtkPVNumberOfOutputsInformation);
vtkCxxRevisionMacro(vtkPVNumberOfOutputsInformation, "$Revision: 1.6 $");

//----------------------------------------------------------------------------
vtkPVNumberOfOutputsInformation::vtkPVNumberOfOutputsInformation()
{
  this->RootOnly = 1;
  this->NumberOfOutputs = 0;
}

//----------------------------------------------------------------------------
vtkPVNumberOfOutputsInformation::~vtkPVNumberOfOutputsInformation()
{
}

//----------------------------------------------------------------------------
void vtkPVNumberOfOutputsInformation::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfOutputs: " << this->NumberOfOutputs << "\n";
}

//----------------------------------------------------------------------------
void vtkPVNumberOfOutputsInformation::CopyFromObject(vtkObject* obj)
{
  this->NumberOfOutputs = 0;

  vtkAlgorithm* algorithm = vtkAlgorithm::SafeDownCast(obj);
  if(!algorithm)
    {
    vtkErrorMacro("Could not downcast vtkAlgorithm.");
    return;
    }
  vtkDemandDrivenPipeline* pipeline = 
    vtkDemandDrivenPipeline::SafeDownCast(algorithm->GetExecutive());
  if (pipeline)
    {
    //pipeline->UpdateDataObject();
    }
  vtkSource* source = vtkSource::SafeDownCast(obj);
  if (source)
    {
    this->NumberOfOutputs = source->GetNumberOfOutputs();
    }
  else
    {
    this->NumberOfOutputs = algorithm->GetNumberOfOutputPorts();
    }
}

//----------------------------------------------------------------------------
void vtkPVNumberOfOutputsInformation::AddInformation(vtkPVInformation* info)
{
  if (vtkPVNumberOfOutputsInformation::SafeDownCast(info))
    {
    this->NumberOfOutputs = vtkPVNumberOfOutputsInformation::SafeDownCast(info)
      ->GetNumberOfOutputs();
    }
}

//----------------------------------------------------------------------------
void
vtkPVNumberOfOutputsInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply << this->NumberOfOutputs
       << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void
vtkPVNumberOfOutputsInformation
::CopyFromStream(const vtkClientServerStream* css)
{
  css->GetArgument(0, 0, &this->NumberOfOutputs);
}
