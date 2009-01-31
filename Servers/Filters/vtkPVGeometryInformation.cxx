/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVGeometryInformation.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGeometryInformation.h"

#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPVUpdateSuppressor.h"

vtkStandardNewMacro(vtkPVGeometryInformation);
vtkCxxRevisionMacro(vtkPVGeometryInformation, "$Revision: 1.3 $");

//----------------------------------------------------------------------------
vtkPVGeometryInformation::vtkPVGeometryInformation()
{
}

//----------------------------------------------------------------------------
vtkPVGeometryInformation::~vtkPVGeometryInformation()
{
}

//----------------------------------------------------------------------------
void vtkPVGeometryInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkPVGeometryInformation::CopyFromObject(vtkObject* object)
{
  vtkPVGeometryFilter* gf = vtkPVGeometryFilter::SafeDownCast(object);
  if (gf)
    {
    this->Superclass::CopyFromObject(gf->GetOutput());
    return;
    }

  vtkPVUpdateSuppressor* us = vtkPVUpdateSuppressor::SafeDownCast(object);
  if (us)
    {
    this->Superclass::CopyFromObject(us->GetOutput());
    return;
    }


  vtkErrorMacro("Cound not cast object to geometry filter.");
}

