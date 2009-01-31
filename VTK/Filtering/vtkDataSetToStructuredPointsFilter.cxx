/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkDataSetToStructuredPointsFilter.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataSetToStructuredPointsFilter.h"

#include "vtkInformation.h"
#include "vtkStructuredPoints.h"

vtkCxxRevisionMacro(vtkDataSetToStructuredPointsFilter, "$Revision: 1.32 $");

//----------------------------------------------------------------------------
vtkDataSetToStructuredPointsFilter::vtkDataSetToStructuredPointsFilter()
{
  this->NumberOfRequiredInputs = 1;
  this->SetNumberOfInputPorts(1);
}

//----------------------------------------------------------------------------
vtkDataSetToStructuredPointsFilter::~vtkDataSetToStructuredPointsFilter()
{
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
void vtkDataSetToStructuredPointsFilter::SetInput(vtkDataSet *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
// Specify the input data or filter.
vtkDataSet *vtkDataSetToStructuredPointsFilter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkDataSet *)(this->Inputs[0]);
}

//----------------------------------------------------------------------------
// All the DataSetToStructuredPointsFilters require all their input.
void vtkDataSetToStructuredPointsFilter::ComputeInputUpdateExtents(
                                                         vtkDataObject *data)
{
  vtkStructuredPoints *output = (vtkStructuredPoints*)data;
  vtkDataSet *input = this->GetInput();
  int *ext;
  
  if (input == NULL)
    {
    return;
    }
  
  // Lets just check to see if the outputs UpdateExtent is valid.
  ext = output->GetUpdateExtent();
  if (ext[0] > ext[1] || ext[2] > ext[3] || ext[4] > ext[5])
    {
    return;
    }
  
  input->SetUpdateExtent(0, 1, 0);
  input->RequestExactExtentOn();
}

//----------------------------------------------------------------------------
int
vtkDataSetToStructuredPointsFilter
::FillInputPortInformation(int port, vtkInformation* info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void vtkDataSetToStructuredPointsFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
