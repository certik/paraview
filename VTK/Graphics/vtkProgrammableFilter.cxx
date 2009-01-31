/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkProgrammableFilter.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProgrammableFilter.h"
#include "vtkPolyData.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkRectilinearGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkProgrammableFilter, "$Revision: 1.23 $");
vtkStandardNewMacro(vtkProgrammableFilter);

// Construct programmable filter with empty execute method.
vtkProgrammableFilter::vtkProgrammableFilter()
{
  this->ExecuteMethod = NULL;
  this->ExecuteMethodArg = NULL;
  this->ExecuteMethodArgDelete = NULL;
}

vtkProgrammableFilter::~vtkProgrammableFilter()
{
  // delete the current arg if there is one and a delete meth
  if ((this->ExecuteMethodArg)&&(this->ExecuteMethodArgDelete))
    {
    (*this->ExecuteMethodArgDelete)(this->ExecuteMethodArg);
    }
}


// Get the input as a concrete type. This method is typically used by the
// writer of the filter function to get the input as a particular type (i.e.,
// it essentially does type casting). It is the users responsibility to know
// the correct type of the input data.
vtkPolyData *vtkProgrammableFilter::GetPolyDataInput()
{
  return (vtkPolyData *)this->GetInput();
}

// Get the input as a concrete type.
vtkStructuredPoints *vtkProgrammableFilter::GetStructuredPointsInput()
{
  return (vtkStructuredPoints *)this->GetInput();
}

// Get the input as a concrete type.
vtkStructuredGrid *vtkProgrammableFilter::GetStructuredGridInput()
{
  return (vtkStructuredGrid *)this->GetInput();
}

// Get the input as a concrete type.
vtkUnstructuredGrid *vtkProgrammableFilter::GetUnstructuredGridInput()
{
  return (vtkUnstructuredGrid *)this->GetInput();
}

// Get the input as a concrete type.
vtkRectilinearGrid *vtkProgrammableFilter::GetRectilinearGridInput()
{
  return (vtkRectilinearGrid *)this->GetInput();
}

// Specify the function to use to operate on the point attribute data. Note
// that the function takes a single (void *) argument.
void vtkProgrammableFilter::SetExecuteMethod(void (*f)(void *), void *arg)
{
  if ( f != this->ExecuteMethod || arg != this->ExecuteMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->ExecuteMethodArg)&&(this->ExecuteMethodArgDelete))
      {
      (*this->ExecuteMethodArgDelete)(this->ExecuteMethodArg);
      }
    this->ExecuteMethod = f;
    this->ExecuteMethodArg = arg;
    this->Modified();
    }
}

// Set the arg delete method. This is used to free user memory.
void vtkProgrammableFilter::SetExecuteMethodArgDelete(void (*f)(void *))
{
  if ( f != this->ExecuteMethodArgDelete)
    {
    this->ExecuteMethodArgDelete = f;
    this->Modified();
    }
}


int vtkProgrammableFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = 0;
  if (inputVector[0]->GetNumberOfInformationObjects() > 0)
    {
    inInfo = inputVector[0]->GetInformationObject(0);
    }
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkDataSet *input = 0;
  if (inInfo)
    {
    input = vtkDataSet::SafeDownCast(
      inInfo->Get(vtkDataObject::DATA_OBJECT()));
    }
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<<"Executing programmable filter");

  // First, copy the input to the output as a starting point
  if (input && input->GetDataObjectType() == output->GetDataObjectType())
    {
    output->CopyStructure( input );
    }

  // Now invoke the procedure, if specified.
  if ( this->ExecuteMethod != NULL )
    {
    (*this->ExecuteMethod)(this->ExecuteMethodArg);
    }

  return 1;
}

