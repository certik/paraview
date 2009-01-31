/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkDataObjectToTable.cxx,v $

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

#include "vtkDataObjectToTable.h"

#include "vtkCellData.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTable.h"

vtkCxxRevisionMacro(vtkDataObjectToTable, "$Revision: 1.3 $");
vtkStandardNewMacro(vtkDataObjectToTable);
//---------------------------------------------------------------------------
vtkDataObjectToTable::vtkDataObjectToTable()
{
  this->FieldType = POINT_DATA;
}

//---------------------------------------------------------------------------
vtkDataObjectToTable::~vtkDataObjectToTable()
{
}

//---------------------------------------------------------------------------
int vtkDataObjectToTable::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}

//---------------------------------------------------------------------------
int vtkDataObjectToTable::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  // Get input data
  vtkInformation* inputInfo = inputVector[0]->GetInformationObject(0);
  vtkDataObject* input = inputInfo->Get(vtkDataObject::DATA_OBJECT());

  // Get output table
  vtkInformation* outputInfo = outputVector->GetInformationObject(0);
  vtkTable* output = vtkTable::SafeDownCast(
    outputInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkFieldData* data = vtkFieldData::New();
  
  switch(this->FieldType)
    {
    case FIELD_DATA:
      if(input->GetFieldData())
        {
        data->ShallowCopy(input->GetFieldData());
        }
      break;
    case POINT_DATA:
      if(vtkDataSet* const dataset = vtkDataSet::SafeDownCast(input))
        {
        if(dataset->GetPointData())
          {
          data->ShallowCopy(dataset->GetPointData());
          }
        }
      break;
    case CELL_DATA:
      if(vtkDataSet* const dataset = vtkDataSet::SafeDownCast(input))
        {
        if(dataset->GetCellData())
          {
          data->ShallowCopy(dataset->GetCellData());
          }
        }
      break;
    }
    
  output->SetFieldData(data);
  data->Delete();
  return 1;
}

//---------------------------------------------------------------------------
void vtkDataObjectToTable::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldType: " << this->FieldType << endl;
}
