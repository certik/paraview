/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkRepresentationPainter.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRepresentationPainter.h"

#include "vtkGraphicsFactory.h"
#include "vtkObjectFactory.h"

vtkInstantiatorNewMacro(vtkRepresentationPainter);
vtkCxxRevisionMacro(vtkRepresentationPainter, "$Revision: 1.2 $");
//-----------------------------------------------------------------------------
vtkRepresentationPainter::vtkRepresentationPainter()
{
}

//-----------------------------------------------------------------------------
vtkRepresentationPainter::~vtkRepresentationPainter()
{
}

//-----------------------------------------------------------------------------
vtkRepresentationPainter* vtkRepresentationPainter::New()
{
  vtkObject* o = vtkGraphicsFactory::CreateInstance("vtkRepresentationPainter");
  return (vtkRepresentationPainter*)o;
}

//-----------------------------------------------------------------------------
void vtkRepresentationPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
