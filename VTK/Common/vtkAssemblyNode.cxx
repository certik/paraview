/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkAssemblyNode.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAssemblyNode.h"
#include "vtkProp.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkAssemblyNode, "$Revision: 1.10 $");
vtkStandardNewMacro(vtkAssemblyNode);

vtkAssemblyNode::vtkAssemblyNode()
{
  this->ViewProp = NULL;
  this->Matrix = NULL;
}

vtkAssemblyNode::~vtkAssemblyNode()
{
  if ( this->Matrix )
    {
    this->Matrix->Delete();
    }
}

//----------------------------------------------------------------------------
// Don't do reference counting
void vtkAssemblyNode::SetViewProp(vtkProp *prop)
{
  this->ViewProp = prop;
}


void vtkAssemblyNode::SetMatrix(vtkMatrix4x4 *matrix)
{
  // delete previous
  if ( this->Matrix != NULL )
    {
    this->Matrix->Delete();
    this->Matrix = NULL;
    }
  // return if NULL matrix specified
  if ( matrix == NULL )
    {
    return;
    }

  // else create a copy of the matrix
  vtkMatrix4x4 *newMatrix = vtkMatrix4x4::New();
  newMatrix->DeepCopy(matrix);
  this->Matrix = newMatrix;
}

unsigned long vtkAssemblyNode::GetMTime()
{
  unsigned long propMTime=0;
  unsigned long matrixMTime=0;
  
  if ( this->ViewProp != NULL )
    {
    propMTime = this->ViewProp->GetMTime();
    }
  if ( this->Matrix != NULL )
    {
    matrixMTime = this->Matrix->GetMTime();
    }
  
  return (propMTime > matrixMTime ? propMTime : matrixMTime);
}

void vtkAssemblyNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->ViewProp )
    {
    os << indent << "ViewProp: " << this->ViewProp << "\n";
    }
  else
    {
    os << indent << "ViewProp: (none)\n";
    }

  if ( this->Matrix )
    {
    os << indent << "Matrix: " << this->Matrix << "\n";
    }
  else
    {
    os << indent << "Matrix: (none)\n";
    }
}

//----------------------------------------------------------------------------

// Disable warnings about qualifiers on return types.
#if defined(_COMPILER_VERSION)
# pragma set woff 3303
#endif
#if defined(__INTEL_COMPILER) 
# pragma warning (disable:858)
#endif

#ifndef VTK_LEGACY_REMOVE
# ifdef VTK_WORKAROUND_WINDOWS_MANGLE
#  undef SetProp
#  undef GetProp
void const vtkAssemblyNode::SetPropA(vtkProp* prop)
{
  VTK_LEGACY_REPLACED_BODY(vtkAssemblyNode::SetProp, "VTK 5.0",
                           vtkAssemblyNode::SetViewProp);
  this->SetViewProp(prop);
}
void const vtkAssemblyNode::SetPropW(vtkProp* prop)
{
  VTK_LEGACY_REPLACED_BODY(vtkAssemblyNode::SetProp, "VTK 5.0",
                           vtkAssemblyNode::SetViewProp);
  this->SetViewProp(prop);
}
vtkProp* const vtkAssemblyNode::GetPropA()
{
  VTK_LEGACY_REPLACED_BODY(vtkAssemblyNode::GetProp, "VTK 5.0",
                           vtkAssemblyNode::GetViewProp);
  return this->GetViewProp();
}
vtkProp* const vtkAssemblyNode::GetPropW()
{
  VTK_LEGACY_REPLACED_BODY(vtkAssemblyNode::GetProp, "VTK 5.0",
                           vtkAssemblyNode::GetViewProp);
  return this->GetViewProp();
}
# endif
void const vtkAssemblyNode::SetProp(vtkProp* prop)
{
  VTK_LEGACY_REPLACED_BODY(vtkAssemblyNode::SetProp, "VTK 5.0",
                           vtkAssemblyNode::SetViewProp);
  this->SetViewProp(prop);
}
vtkProp* const vtkAssemblyNode::GetProp()
{
  VTK_LEGACY_REPLACED_BODY(vtkAssemblyNode::GetProp, "VTK 5.0",
                           vtkAssemblyNode::GetViewProp);
  return this->GetViewProp();
}
#endif
