/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkHierarchicalDataGroupFilter.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalDataGroupFilter.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkHierarchicalDataGroupFilter, "$Revision: 1.2 $");
vtkStandardNewMacro(vtkHierarchicalDataGroupFilter);

vtkHierarchicalDataGroupFilter::vtkHierarchicalDataGroupFilter()
{
}

vtkHierarchicalDataGroupFilter::~vtkHierarchicalDataGroupFilter()
{
}

void vtkHierarchicalDataGroupFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
