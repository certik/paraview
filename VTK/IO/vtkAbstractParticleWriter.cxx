/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkAbstractParticleWriter.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAbstractParticleWriter.h"

vtkCxxRevisionMacro(vtkAbstractParticleWriter, "$Revision: 1.2 $");

// Construct with no start and end write methods or arguments.
vtkAbstractParticleWriter::vtkAbstractParticleWriter()
{
  this->TimeStep  = 0;
  this->TimeValue = 0.0;
  this->FileName  = NULL;
}

vtkAbstractParticleWriter::~vtkAbstractParticleWriter()
{
  if (this->FileName)
    {
    delete []this->FileName;
    this->FileName = NULL;
    }
}

void vtkAbstractParticleWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "TimeStep: " << this->TimeStep << endl;
  os << indent << "TimeValue: " << this->TimeValue << endl;
  os << indent << "FileName: " << 
    (this->FileName ? this->FileName : "NONE") << endl;
}
