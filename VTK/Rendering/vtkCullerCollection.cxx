/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkCullerCollection.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCullerCollection.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkCullerCollection, "$Revision: 1.8 $");
vtkStandardNewMacro(vtkCullerCollection);

//----------------------------------------------------------------------------
void vtkCullerCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
