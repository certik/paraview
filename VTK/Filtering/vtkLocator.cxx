/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkLocator.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLocator.h"

#include "vtkDataSet.h"
#include "vtkGarbageCollector.h"

vtkCxxRevisionMacro(vtkLocator, "$Revision: 1.3 $");

vtkCxxSetObjectMacro(vtkLocator,DataSet,vtkDataSet);

// Construct with automatic computation of divisions, averaging
// 25 points per bucket.
vtkLocator::vtkLocator()
{
  this->DataSet = NULL;
  this->MaxLevel = 8;
  this->Level = 8;
  this->Tolerance = 0.001;
  this->Automatic = 1;
  this->RetainCellLists = 1;
}

vtkLocator::~vtkLocator()
{
  // commented out because of compiler problems in g++
  //  this->FreeSearchStructure(); 
  this->SetDataSet(NULL);
}

void vtkLocator::Initialize()
{
  // free up hash table
  this->FreeSearchStructure();
}

void vtkLocator::Update()
{
  if (!this->DataSet)
    {
    vtkErrorMacro(<< "Input not set!");
    return;
    }
  if ((this->MTime > this->BuildTime) ||
      (this->DataSet->GetMTime() > this->BuildTime))
    {
    this->BuildLocator();
    }
}

void vtkLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->DataSet )
    {
    os << indent << "DataSet: " << this->DataSet << "\n";
    }
  else
    {
    os << indent << "DataSet: (none)\n";
    }

  os << indent << "Automatic: " << (this->Automatic ? "On\n" : "Off\n");
  os << indent << "Tolerance: " << this->Tolerance << "\n" ;
  os << indent << "Level: " << this->Level << "\n" ;
  os << indent << "MaxLevel: " << this->MaxLevel << "\n" ;
  os << indent << "Retain Cell Lists: " << (this->RetainCellLists ? "On\n" : "Off\n");
  os << indent << "Build Time: " << this->BuildTime.GetMTime() << "\n";
}

//----------------------------------------------------------------------------
void vtkLocator::Register(vtkObjectBase* o)
{
  this->RegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
void vtkLocator::UnRegister(vtkObjectBase* o)
{
  this->UnRegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
void vtkLocator::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->DataSet, "DataSet");
}
