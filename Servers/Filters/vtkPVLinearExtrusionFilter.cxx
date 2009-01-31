/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVLinearExtrusionFilter.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVLinearExtrusionFilter.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPVLinearExtrusionFilter, "$Revision: 1.1 $");
vtkStandardNewMacro(vtkPVLinearExtrusionFilter);

vtkPVLinearExtrusionFilter::vtkPVLinearExtrusionFilter()
{
  this->ExtrusionType = VTK_VECTOR_EXTRUSION;
}

void vtkPVLinearExtrusionFilter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
