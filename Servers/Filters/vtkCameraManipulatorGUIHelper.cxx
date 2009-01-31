/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkCameraManipulatorGUIHelper.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCameraManipulatorGUIHelper.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkCameraManipulatorGUIHelper, "$Revision: 1.1 $");
//-----------------------------------------------------------------------------
vtkCameraManipulatorGUIHelper::vtkCameraManipulatorGUIHelper()
{
}

//-----------------------------------------------------------------------------
vtkCameraManipulatorGUIHelper::~vtkCameraManipulatorGUIHelper()
{
 
}

//-----------------------------------------------------------------------------
void vtkCameraManipulatorGUIHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
