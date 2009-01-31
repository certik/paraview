/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkOutputStream.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOutputStream.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkOutputStream, "$Revision: 1.2 $");
vtkStandardNewMacro(vtkOutputStream);

//----------------------------------------------------------------------------
vtkOutputStream::vtkOutputStream()
{
  this->Stream = 0;
}

//----------------------------------------------------------------------------
vtkOutputStream::~vtkOutputStream()
{
  this->SetStream(0);
}

//----------------------------------------------------------------------------
void vtkOutputStream::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Stream: " << (this->Stream? "set" : "none") << "\n";
}

//----------------------------------------------------------------------------
int vtkOutputStream::StartWriting()
{
  if(!this->Stream)
    {
    vtkErrorMacro("StartWriting() called with no Stream set.");
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkOutputStream::EndWriting()
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkOutputStream::Write(const unsigned char* data, unsigned long length)
{
  return this->Write(reinterpret_cast<const char*>(data), length);
}

//----------------------------------------------------------------------------
int vtkOutputStream::Write(const char* data, unsigned long length)
{
  return (this->Stream->write(data, length)? 1:0);
}
