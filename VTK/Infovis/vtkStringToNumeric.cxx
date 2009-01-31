/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkStringToNumeric.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkStringToNumeric.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkDemandDrivenPipeline.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStringArray.h"
#include "vtkVariant.h"

vtkCxxRevisionMacro(vtkStringToNumeric, "$Revision: 1.2 $");
vtkStandardNewMacro(vtkStringToNumeric);

vtkStringToNumeric::vtkStringToNumeric()
{
  this->ConvertFieldData = true;
  this->ConvertPointData = true;
  this->ConvertCellData = true;
}

vtkStringToNumeric::~vtkStringToNumeric()
{
}

int vtkStringToNumeric::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  // Get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  
  // Get the input and output objects
  vtkDataObject* input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  output->ShallowCopy(input);
  
  if (this->ConvertFieldData)
    {
    this->ConvertArrays(output->GetFieldData());
    }
  vtkDataSet* outputDataSet = vtkDataSet::SafeDownCast(output);
  if (outputDataSet && this->ConvertPointData)
    {
    this->ConvertArrays(outputDataSet->GetPointData());
    }
  if (outputDataSet && this->ConvertCellData)
    {
    this->ConvertArrays(outputDataSet->GetCellData());
    }
  
  return 1;
}

void vtkStringToNumeric::ConvertArrays(vtkFieldData* fieldData)
{
  for (int arr = 0; arr < fieldData->GetNumberOfArrays(); arr++)
    {
    vtkStringArray* stringArray = vtkStringArray::SafeDownCast(
      fieldData->GetAbstractArray(arr));
    if (!stringArray)
      {
      continue;
      }
    
    vtkIdType numTuples = stringArray->GetNumberOfTuples();
    vtkIdType numComps = stringArray->GetNumberOfComponents();
  
    // Set up the output array
    vtkDoubleArray* doubleArray = vtkDoubleArray::New();
    doubleArray->SetNumberOfValues(numComps*numTuples);
    doubleArray->SetNumberOfComponents(numComps);
    doubleArray->SetName(stringArray->GetName());
  
    // Set up the output array
    vtkIntArray* intArray = vtkIntArray::New();
    intArray->SetNumberOfValues(numComps*numTuples);
    intArray->SetNumberOfComponents(numComps);
    intArray->SetName(stringArray->GetName());
  
    // Convert the strings to time point values
    bool allInteger = true;
    bool allNumeric = true;
    for (vtkIdType i = 0; i < numTuples*numComps; i++)
      {
      vtkStdString str = stringArray->GetValue(i);
      bool ok;
      if (allInteger)
        {
        if (str.length() == 0)
          {
          intArray->SetValue(i, 0);
          doubleArray->SetValue(i, 0.0);
          continue;
          }
        int intValue = vtkVariant(str).ToInt(&ok);
        if (ok)
          {
          double doubleValue = intValue;
          intArray->SetValue(i, intValue);
          doubleArray->SetValue(i, doubleValue);
          }
        else
          {
          allInteger = false;
          }
        }
      if (!allInteger)
        {
        if (str.length() == 0)
          {
          doubleArray->SetValue(i, 0.0);
          continue;
          }
        double doubleValue = vtkVariant(str).ToDouble(&ok);
        if (!ok)
          {
          allNumeric = false;
          break;
          }
        else
          {
          doubleArray->SetValue(i, doubleValue);
          }
        }
      }
    if (allNumeric)
      {
      // Calling AddArray will replace the old array since the names match.
      if (allInteger)
        {
        fieldData->AddArray(intArray);
        }
      else
        {
        fieldData->AddArray(doubleArray);
        }
      }
    intArray->Delete();
    doubleArray->Delete();
    }
}

//----------------------------------------------------------------------------
int vtkStringToNumeric::ProcessRequest(
  vtkInformation* request, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  // create the output
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(request, inputVector, outputVector);
    }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkStringToNumeric::RequestDataObject(
  vtkInformation*, 
  vtkInformationVector** inputVector , 
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  
  if (input)
    {
    // for each output
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* info = outputVector->GetInformationObject(i);
      vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());
    
      if (!output || !output->IsA(input->GetClassName())) 
        {
        vtkDataObject* newOutput = input->NewInstance();
        newOutput->SetPipelineInformation(info);
        newOutput->Delete();
        this->GetOutputPortInformation(0)->Set(
          vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
        }
      }
    return 1;
    }
  return 0;
}

void vtkStringToNumeric::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ConvertFieldData: " 
    << (this->ConvertFieldData ? "on" : "off") << endl;
  os << indent << "ConvertPointData: " 
    << (this->ConvertPointData ? "on" : "off") << endl;
  os << indent << "ConvertCellData: " 
    << (this->ConvertCellData ? "on" : "off") << endl;
}
