/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPruneTreeFilter.cxx,v $

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

#include "vtkPruneTreeFilter.h"
#include "vtkTree.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkStringArray.h"

vtkCxxRevisionMacro(vtkPruneTreeFilter, "$Revision: 1.3 $");
vtkStandardNewMacro(vtkPruneTreeFilter);


vtkPruneTreeFilter::vtkPruneTreeFilter()
{
  this->ParentVertex = 0;
}

vtkPruneTreeFilter::~vtkPruneTreeFilter()
{
}

void vtkPruneTreeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Parent: " << this->ParentVertex << endl;
}

int vtkPruneTreeFilter::RequestData(
  vtkInformation*, 
  vtkInformationVector** inputVector, 
  vtkInformationVector* outputVector)
{
  // Store the XML hieredgehy into a vtkTree
  vtkTree* inputTree = vtkTree::GetData(inputVector[0]);
  vtkTree* outputTree = vtkTree::GetData(outputVector);

  if (this->ParentVertex < 0 || this->ParentVertex >= inputTree->GetNumberOfVertices())
    {
    vtkErrorMacro("Parent vertex must be part of the tree " << this->ParentVertex 
      << " >= " << inputTree->GetNumberOfVertices());
    return 0;
    }

  outputTree->DeepCopy(inputTree);

  // Now, prune the tree
  outputTree->RemoveVertexAndDescendants(this->ParentVertex);

  return 1;
}
