/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMProxyManagerReviver.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyManagerReviver.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkSMProxyManagerReviver, "$Revision: 1.1 $");
//-----------------------------------------------------------------------------
vtkSMProxyManagerReviver::vtkSMProxyManagerReviver()
{
}

//-----------------------------------------------------------------------------
vtkSMProxyManagerReviver::~vtkSMProxyManagerReviver()
{
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void vtkSMProxyManagerReviver::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
