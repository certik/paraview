/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVArrowSource.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVArrowSource.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkPVArrowSource, "$Revision: 1.3 $");
vtkStandardNewMacro(vtkPVArrowSource);


void vtkPVArrowSource::ExecuteInformation()
{
  this->GetOutput()->SetMaximumNumberOfPieces(-1);
}


void vtkPVArrowSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
