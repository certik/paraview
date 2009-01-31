/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPruneTreeFilter.h,v $

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
// .NAME vtkPruneTreeFilter - prune a subtree out of a vtkTree
//
// .SECTION Description
// Removes a subtree rooted at a particular vertex in a vtkTree.
//

#ifndef __vtkPruneTreeFilter_h
#define __vtkPruneTreeFilter_h

#include "vtkTreeAlgorithm.h"

class vtkTree;
class vtkPVXMLElement;

class VTK_INFOVIS_EXPORT vtkPruneTreeFilter : public vtkTreeAlgorithm
{
public:
  static vtkPruneTreeFilter* New();
  vtkTypeRevisionMacro(vtkPruneTreeFilter,vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the parent vertex of the subtree to remove.
  vtkGetMacro(ParentVertex, vtkIdType);
  vtkSetMacro(ParentVertex, vtkIdType);

protected:
  vtkPruneTreeFilter();
  ~vtkPruneTreeFilter();

  vtkIdType ParentVertex;

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

private:
  vtkPruneTreeFilter(const vtkPruneTreeFilter&); // Not implemented
  void operator=(const vtkPruneTreeFilter&);   // Not implemented
};

#endif

