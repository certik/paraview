/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkOutlineSource.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOutlineSource.h"

#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkOutlineSource, "$Revision: 1.35 $");
vtkStandardNewMacro(vtkOutlineSource);

//----------------------------------------------------------------------------
vtkOutlineSource::vtkOutlineSource()
{
  this->BoxType = VTK_BOX_TYPE_AXIS_ALIGNED;
  for (int i=0; i<3; i++)
    {
    this->Bounds[2*i] = -1.0;
    this->Bounds[2*i+1] = 1.0;
    }

  // Sensible initial values
  this->Corners[0] = 0.0;
  this->Corners[1] = 0.0;
  this->Corners[2] = 0.0;
  this->Corners[3] = 1.0;
  this->Corners[4] = 0.0;
  this->Corners[5] = 0.0;
  this->Corners[6] = 0.0;
  this->Corners[7] = 1.0;
  this->Corners[8] = 0.0;
  this->Corners[9] = 1.0;
  this->Corners[10] = 1.0;
  this->Corners[11] = 0.0;
  this->Corners[12] = 0.0;
  this->Corners[13] = 0.0;
  this->Corners[14] = 1.0;
  this->Corners[15] = 1.0;
  this->Corners[16] = 0.0;
  this->Corners[17] = 1.0;
  this->Corners[18] = 0.0;
  this->Corners[19] = 1.0;
  this->Corners[20] = 1.0;
  this->Corners[21] = 1.0;
  this->Corners[22] = 1.0;
  this->Corners[23] = 1.0;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
int vtkOutlineSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  double *bounds;
  double x[3];
  vtkIdType pts[2];
  vtkPoints *newPts;
  vtkCellArray *newLines;
  
  //
  // Initialize
  //
  bounds = this->Bounds;
  //
  // Allocate storage and create outline
  //
  newPts = vtkPoints::New();
  newPts->Allocate(8);
  newLines = vtkCellArray::New();
  newLines->Allocate(newLines->EstimateSize(12,2));

  if (this->BoxType==VTK_BOX_TYPE_AXIS_ALIGNED) 
    {
    x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[4];
    newPts->InsertPoint(0,x);
    x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[4];
    newPts->InsertPoint(1,x);
    x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[4];
    newPts->InsertPoint(2,x);
    x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[4];
    newPts->InsertPoint(3,x);
    x[0] = bounds[0]; x[1] = bounds[2]; x[2] = bounds[5];
    newPts->InsertPoint(4,x);
    x[0] = bounds[1]; x[1] = bounds[2]; x[2] = bounds[5];
    newPts->InsertPoint(5,x);
    x[0] = bounds[0]; x[1] = bounds[3]; x[2] = bounds[5];
    newPts->InsertPoint(6,x);
    x[0] = bounds[1]; x[1] = bounds[3]; x[2] = bounds[5];
    newPts->InsertPoint(7,x);
    }
  else //VTK_BOX_TYPE_ORIENTED
    {
    newPts->InsertPoint(0, &Corners[0]);
    newPts->InsertPoint(1, &Corners[3]);
    newPts->InsertPoint(2, &Corners[6]);
    newPts->InsertPoint(3, &Corners[9]);
    newPts->InsertPoint(4, &Corners[12]);
    newPts->InsertPoint(5, &Corners[15]);
    newPts->InsertPoint(6, &Corners[18]);
    newPts->InsertPoint(7, &Corners[21]);
    }

  pts[0] = 0; pts[1] = 1;
  newLines->InsertNextCell(2,pts);
  pts[0] = 2; pts[1] = 3;
  newLines->InsertNextCell(2,pts);
  pts[0] = 4; pts[1] = 5;
  newLines->InsertNextCell(2,pts);
  pts[0] = 6; pts[1] = 7;
  newLines->InsertNextCell(2,pts);
  pts[0] = 0; pts[1] = 2;
  newLines->InsertNextCell(2,pts);
  pts[0] = 1; pts[1] = 3;
  newLines->InsertNextCell(2,pts);
  pts[0] = 4; pts[1] = 6;
  newLines->InsertNextCell(2,pts);
  pts[0] = 5; pts[1] = 7;
  newLines->InsertNextCell(2,pts);
  pts[0] = 0; pts[1] = 4;
  newLines->InsertNextCell(2,pts);
  pts[0] = 1; pts[1] = 5;
  newLines->InsertNextCell(2,pts);
  pts[0] = 2; pts[1] = 6;
  newLines->InsertNextCell(2,pts);
  pts[0] = 3; pts[1] = 7;
  newLines->InsertNextCell(2,pts);

  // Update selves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetLines(newLines);
  newLines->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkOutlineSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Box Type: ";
  if ( this->BoxType == VTK_BOX_TYPE_AXIS_ALIGNED )
    {
    os << "Axis Aligned\n";
    os << indent << "Bounds: "
       << "(" << this->Bounds[0] << ", " << this->Bounds[1] << ") "
       << "(" << this->Bounds[2] << ", " << this->Bounds[3] << ") " 
       << "(" << this->Bounds[4] << ", " << this->Bounds[5] << ")\n";
    }
  else
    {
    os << "Corners: (\n";
    for (int i=0; i<8; i++)
      {
      os << "\t" << this->Corners[3*i] << ", "
         << this->Corners[3*i+1] << ", "
         << this->Corners[3*i+2] << "\n";
      }
    os << ")\n";
    }
}
