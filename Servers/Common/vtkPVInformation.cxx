/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVInformation.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVInformation.h"

vtkCxxRevisionMacro(vtkPVInformation, "$Revision: 1.2 $");

//----------------------------------------------------------------------------
vtkPVInformation::vtkPVInformation()
{
  this->RootOnly = 0;
}

//----------------------------------------------------------------------------
vtkPVInformation::~vtkPVInformation()
{
}

//----------------------------------------------------------------------------
void vtkPVInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "RootOnly: " << this->RootOnly << endl;
}

//----------------------------------------------------------------------------
void vtkPVInformation::CopyFromObject(vtkObject*)
{
  vtkErrorMacro("CopyFromObject not implemented.");
}

//----------------------------------------------------------------------------
void vtkPVInformation::AddInformation(vtkPVInformation*)
{
  vtkErrorMacro("AddInformation not implemented.");
}

//----------------------------------------------------------------------------
void vtkPVInformation::CopyFromStream(const vtkClientServerStream*)
{
  vtkErrorMacro("CopyFromStream not implemented.");
}
