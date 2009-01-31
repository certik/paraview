/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkDummyController.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDummyCommunicator.h"
#include "vtkDummyController.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkDummyController, "$Revision: 1.6 $");
vtkStandardNewMacro(vtkDummyController);

vtkCxxSetObjectMacro(vtkDummyController, Communicator, vtkCommunicator);
vtkCxxSetObjectMacro(vtkDummyController, RMICommunicator, vtkCommunicator);

//----------------------------------------------------------------------------
vtkDummyController::vtkDummyController()
{
  this->Communicator = vtkDummyCommunicator::New();
  this->RMICommunicator = vtkDummyCommunicator::New();
}

vtkDummyController::~vtkDummyController()
{
  this->SetCommunicator(NULL);
  this->SetRMICommunicator(NULL);
}

void vtkDummyController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Communicator: " << this->Communicator << endl;
  os << indent << "RMICommunicator: " << this->RMICommunicator << endl;
}

//-----------------------------------------------------------------------------
void vtkDummyController::SingleMethodExecute()
{
  if (this->SingleMethod)
    {
    // Should we set the global controller here?  I'm going to say no since
    // we are not really a parallel job or at the very least not the global
    // controller.

    (this->SingleMethod)(this, this->SingleData);
    }
  else
    {
    vtkWarningMacro("SingleMethod not set.");
    }
}

//-----------------------------------------------------------------------------
void vtkDummyController::MultipleMethodExecute()
{
  int i = this->GetLocalProcessId();

  if (this->MultipleMethod[i])
    {
    // Should we set the global controller here?  I'm going to say no since
    // we are not really a parallel job or at the very least not the global
    // controller.

    (this->MultipleMethod[i])(this, this->MultipleData[i]);
    }
  else
    {
    vtkWarningMacro("MultipleMethod " << i << " not set.");
    }
}
