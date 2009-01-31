/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkActorCollection.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActorCollection.h"

#include "vtkObjectFactory.h"
#include "vtkProperty.h"

vtkCxxRevisionMacro(vtkActorCollection, "$Revision: 1.12 $");
vtkStandardNewMacro(vtkActorCollection);

void vtkActorCollection::ApplyProperties(vtkProperty *p)
{
  vtkActor *actor;
  
  if ( p == NULL )
    {
    return;
    }
  
  vtkCollectionSimpleIterator ait;
  for ( this->InitTraversal(ait); (actor=this->GetNextActor(ait)); )
    {
    actor->GetProperty()->DeepCopy(p);
    }
}

//----------------------------------------------------------------------------
void vtkActorCollection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
