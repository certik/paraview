/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVCreateProcessModule.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCreateProcessModule.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkToolkits.h" // For 

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkPVCreateProcessModule, "$Revision: 1.3 $");

//----------------------------------------------------------------------------
vtkProcessModule* vtkPVCreateProcessModule::CreateProcessModule(vtkPVOptions* op)
{
  vtkProcessModule *pm;
  pm = vtkProcessModule::New();
  pm->SetOptions(op);
  vtkProcessModule::SetProcessModule(pm);
  return pm;
}

//----------------------------------------------------------------------------
void vtkPVCreateProcessModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
