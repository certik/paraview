/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVServerObject.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVServerObject.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVServerObject);
vtkCxxRevisionMacro(vtkPVServerObject, "$Revision: 1.2 $");
vtkCxxSetObjectMacro(vtkPVServerObject, ProcessModule, vtkProcessModule);

//----------------------------------------------------------------------------
vtkPVServerObject::vtkPVServerObject()
{
  this->ProcessModule = 0;
}

//----------------------------------------------------------------------------
vtkPVServerObject::~vtkPVServerObject()
{
  this->SetProcessModule(0);
}

//----------------------------------------------------------------------------
void vtkPVServerObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent); 
  if(this->ProcessModule)
    {
    os << indent << "ProcessModule: " << *this->ProcessModule << endl;
    }
  else
    {
    os << indent << "ProcessModule: NULL" << endl;
    }
}
