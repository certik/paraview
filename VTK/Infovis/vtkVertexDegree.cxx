/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkVertexDegree.cxx,v $

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
#include "vtkVertexDegree.h"

#include "vtkAbstractGraph.h"
#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

vtkCxxRevisionMacro(vtkVertexDegree, "$Revision: 1.3 $");
vtkStandardNewMacro(vtkVertexDegree);

vtkVertexDegree::vtkVertexDegree()
{
  this->OutputArrayName = 0;
}

vtkVertexDegree::~vtkVertexDegree()
{
}


int vtkVertexDegree::RequestData(vtkInformation *vtkNotUsed(request),
                            vtkInformationVector **inputVector,
                            vtkInformationVector *outputVector)
{

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkAbstractGraph *input = vtkAbstractGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkAbstractGraph *output = vtkAbstractGraph::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
    
  // Do a shallow copy of the input to the output
  output->ShallowCopy(input);

  // Create the attribute array
  vtkIntArray* DegreeArray = vtkIntArray::New();
  if (this->OutputArrayName)
    {
    DegreeArray->SetName(this->OutputArrayName);
    }
  else
    {
    DegreeArray->SetName("VertexDegree");
    }
  DegreeArray->SetNumberOfTuples(output->GetNumberOfVertices());
  
  // Now loop through the verticies and set their degree in the array
  for(int i=0;i< DegreeArray->GetNumberOfTuples(); ++i)
    {
    DegreeArray->SetValue(i,output->GetDegree(i));
    
    double progress = static_cast<double>(i) / static_cast<double>(DegreeArray->GetNumberOfTuples());
    this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
    }
    
  // Add attribute array to the output
  output->GetVertexData()->AddArray(DegreeArray);
  DegreeArray->Delete();
    
  return 1;
} 

void vtkVertexDegree::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "OutputArrayName: " 
     << (this->OutputArrayName ? this->OutputArrayName : "(none)") << endl;
     
}
