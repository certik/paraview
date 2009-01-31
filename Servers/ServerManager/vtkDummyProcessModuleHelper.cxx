/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkDummyProcessModuleHelper.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDummyProcessModuleHelper.h"

#include "vtkObjectFactory.h"
vtkCxxRevisionMacro(vtkDummyProcessModuleHelper, "$Revision: 1.1 $");
vtkStandardNewMacro(vtkDummyProcessModuleHelper);

//----------------------------------------------------------------------------
vtkDummyProcessModuleHelper::vtkDummyProcessModuleHelper()
{
}

//----------------------------------------------------------------------------
vtkDummyProcessModuleHelper::~vtkDummyProcessModuleHelper()
{
}

//----------------------------------------------------------------------------
void vtkDummyProcessModuleHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

