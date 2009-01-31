/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkHierarchicalDataSet.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalDataSet.h"

#include "vtkDataSet.h"
#include "vtkHierarchicalDataInformation.h"
#include "vtkHierarchicalDataIterator.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkHierarchicalDataSet, "$Revision: 1.9.46.1 $");
vtkStandardNewMacro(vtkHierarchicalDataSet);

vtkInformationKeyMacro(vtkHierarchicalDataSet,LEVEL,Integer);

//----------------------------------------------------------------------------
vtkHierarchicalDataSet::vtkHierarchicalDataSet()
{
}

//----------------------------------------------------------------------------
vtkHierarchicalDataSet::~vtkHierarchicalDataSet()
{
}

//----------------------------------------------------------------------------
vtkHierarchicalDataInformation* 
vtkHierarchicalDataSet::GetHierarchicalDataInformation()
{
  return vtkHierarchicalDataInformation::SafeDownCast(
    this->GetMultiGroupDataInformation());
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataSet::SetHierarchicalDataInformation(
  vtkHierarchicalDataInformation* info)
{
  this->Superclass::SetMultiGroupDataInformation(info);
}

//----------------------------------------------------------------------------
void vtkHierarchicalDataSet::AddDataSet(vtkInformation* index, vtkDataObject* dobj)
{
  if (index->Has(INDEX()) && index->Has(LEVEL()))
    {
    this->SetDataSet(index->Get(LEVEL()), index->Get(INDEX()), dobj);
    }
  else
    {
    this->Superclass::AddDataSet(index, dobj);
    }
}

//----------------------------------------------------------------------------
vtkDataObject* vtkHierarchicalDataSet::GetDataSet(vtkInformation* index)
{
  if (index->Has(INDEX()) && index->Has(LEVEL()))
    {
    return this->GetDataSet(index->Get(LEVEL()), index->Get(INDEX()));
    }
  return this->Superclass::GetDataSet(index);
}

//----------------------------------------------------------------------------
vtkHierarchicalDataSet* vtkHierarchicalDataSet::GetData(vtkInformation* info)
{
  return
    info? vtkHierarchicalDataSet::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkHierarchicalDataSet*
vtkHierarchicalDataSet::GetData(vtkInformationVector* v, int i)
{
  return vtkHierarchicalDataSet::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
vtkCompositeDataIterator* vtkHierarchicalDataSet::NewIterator()
{
  vtkMultiGroupDataIterator* iter = vtkHierarchicalDataIterator::New();
  iter->SetDataSet(this);
  return iter;
}
//----------------------------------------------------------------------------
void vtkHierarchicalDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

