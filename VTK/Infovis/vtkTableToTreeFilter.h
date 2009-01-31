/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkTableToTreeFilter.h,v $

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
// .NAME vtkTableToTreeFilter - Filter that converts a vtkTable to a vtkTree
//
// .SECTION Description
//
// vtkTableToTreeFilter is a filter for converting a vtkTable data structure
// into a vtkTree datastructure.  Currently, this will convert the table into
// a star, with each row of the table as a child of a new root node.
// The columns of the table are passed as node fields of the tree.

#ifndef __vtkTableToTreeFilter_h
#define __vtkTableToTreeFilter_h

#include "vtkTreeAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkTableToTreeFilter : public vtkTreeAlgorithm
{
public:
  static vtkTableToTreeFilter* New();
  vtkTypeRevisionMacro(vtkTableToTreeFilter,vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkTableToTreeFilter();
  ~vtkTableToTreeFilter();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);
    
  int FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation* info);
  int FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation* info);

private:
  vtkTableToTreeFilter(const vtkTableToTreeFilter&); // Not implemented
  void operator=(const vtkTableToTreeFilter&);   // Not implemented
};

#endif

