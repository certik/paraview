/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkMultiProcessController.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This will be the default.
#include "vtkMultiProcessController.h"
#include "vtkDummyController.h"
#include "vtkProcessGroup.h"
#include "vtkSubCommunicator.h"
#include "vtkToolkits.h"

#ifdef VTK_USE_MPI
#include "vtkMPIController.h"
#endif

#include "vtkCollection.h"
#include "vtkObjectFactory.h"
#include "vtkOutputWindow.h"

//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkMultiProcessController);

//----------------------------------------------------------------------------

// Helper class to contain the RMI information.  
// A subclass of vtkObject so that I can keep them in a collection.
class VTK_PARALLEL_EXPORT vtkMultiProcessControllerRMI : public vtkObject
{
public:
  static vtkMultiProcessControllerRMI *New(); 
  vtkTypeRevisionMacro(vtkMultiProcessControllerRMI, vtkObject);
  
  int Tag;
  vtkRMIFunctionType Function;
  void *LocalArgument;

  unsigned long Id;
  
protected:
  vtkMultiProcessControllerRMI() {};
  vtkMultiProcessControllerRMI(const vtkMultiProcessControllerRMI&);
  void operator=(const vtkMultiProcessControllerRMI&);
};

vtkCxxRevisionMacro(vtkMultiProcessControllerRMI, "$Revision: 1.29 $");
vtkStandardNewMacro(vtkMultiProcessControllerRMI);

vtkCxxRevisionMacro(vtkMultiProcessController, "$Revision: 1.29 $");

//----------------------------------------------------------------------------
// An RMI function that will break the "ProcessRMIs" loop.
void vtkMultiProcessControllerBreakRMI(void *localArg, 
                                       void *remoteArg, int remoteArgLength, 
                                       int vtkNotUsed(remoteId))
{
  vtkMultiProcessController *controller;
  remoteArg = remoteArg;
  remoteArgLength = remoteArgLength;
  controller = (vtkMultiProcessController*)(localArg);
  controller->SetBreakFlag(1);
}



//----------------------------------------------------------------------------
vtkMultiProcessController::vtkMultiProcessController()
{
  int i;

  this->RMICount = 1;
  
  this->RMIs = vtkCollection::New();
  
  this->SingleMethod = 0;
  this->SingleData = 0;   

  this->Communicator = 0;
  this->RMICommunicator = 0;
  
  for ( i = 0; i < VTK_MAX_THREADS; i++ )
    {
    this->MultipleMethod[i] = NULL;
    this->MultipleData[i] = NULL;
    }

  this->BreakFlag = 0;
  this->ForceDeepCopy = 1;

  this->OutputWindow = 0;

  // Define an rmi internally to exit from the processing loop.
  this->AddRMI(vtkMultiProcessControllerBreakRMI, this, BREAK_RMI_TAG);
}

//----------------------------------------------------------------------------
// This is an old comment that I do not know is true:
// (We need to have a "GetNetReferenceCount" to avoid memory leaks.)
vtkMultiProcessController::~vtkMultiProcessController()
{
  if ( this->OutputWindow &&
       (this->OutputWindow == vtkOutputWindow::GetInstance()) )
    {
    vtkOutputWindow::SetInstance(0);
    }

  if (this->OutputWindow)
    {
    this->OutputWindow->Delete();
    }

  this->RMIs->Delete();
  this->RMIs = NULL;
}

 
//----------------------------------------------------------------------------
void vtkMultiProcessController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  vtkMultiProcessControllerRMI *rmi;
  vtkIndent nextIndent = indent.GetNextIndent();
  
  os << indent << "Break flag: " << (this->BreakFlag ? "(yes)" : "(no)") 
     << endl;
  os << indent << "Force deep copy: " << (this->ForceDeepCopy ? "(yes)" : "(no)") 
     << endl;
  os << indent << "Output window: ";
  if (this->OutputWindow)
    {
    os << endl;
    this->OutputWindow->PrintSelf(os, nextIndent);
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "Communicator: ";
  if (this->Communicator)
    {
    os << endl;
    this->Communicator->PrintSelf(os, nextIndent);
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "RMI communicator: ";
  if (this->RMICommunicator)
    {
    os << endl;
    this->RMICommunicator->PrintSelf(os, nextIndent);
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "RMIs: \n";
  
  this->RMIs->InitTraversal();
  while ( (rmi = (vtkMultiProcessControllerRMI*)(this->RMIs->GetNextItemAsObject())) )
    {
    os << nextIndent << rmi->Tag << endl;
    }
  
}

//----------------------------------------------------------------------------
void vtkMultiProcessController::SetNumberOfProcesses(int num)
{
  if(this->Communicator)
    {
    this->Communicator->SetNumberOfProcesses(num);
    }
  else
    {
    vtkErrorMacro("Communicator not set.");
    }
}

//----------------------------------------------------------------------------
int vtkMultiProcessController::GetNumberOfProcesses()
{
  if(this->Communicator)
    {
    return this->Communicator->GetNumberOfProcesses();
    }
  else
    {
    vtkErrorMacro("Communicator not set.");
    return 0;
    }
}

//----------------------------------------------------------------------------
int vtkMultiProcessController::GetLocalProcessId()
{
  if(this->Communicator)
    {
    return this->Communicator->GetLocalProcessId();
    }
  else
    {
    vtkErrorMacro("Communicator not set.");
    return -1;
    }
}

//----------------------------------------------------------------------------
void vtkMultiProcessController::SetSingleMethod( vtkProcessFunctionType f, 
                                                 void *data )
{
  this->SingleMethod = f;
  this->SingleData   = data;
}

//----------------------------------------------------------------------------
// Set one of the user defined methods that will be run on NumberOfProcesses
// processes when MultipleMethodExecute is called. This method should be
// called with index = 0, 1, ..,  NumberOfProcesses-1 to set up all the
// required user defined methods
void vtkMultiProcessController::SetMultipleMethod( int index,
                                 vtkProcessFunctionType f, void *data )
{ 
  // You can only set the method for 0 through NumberOfProcesses-1
  if ( index >= this->GetNumberOfProcesses() )
    {
    vtkErrorMacro( << "Can't set method " << index << 
      " with a processes count of " << this->GetNumberOfProcesses() );
    }
  else
    {
    this->MultipleMethod[index] = f;
    this->MultipleData[index]   = data;
    }
}

//-----------------------------------------------------------------------------
vtkMultiProcessController *vtkMultiProcessController::CreateSubController(
                                                         vtkProcessGroup *group)
{
  if (group->GetCommunicator() != this->Communicator)
    {
    vtkErrorMacro(<< "Invalid group for creating a sub controller.");
    return NULL;
    }

  if (group->FindProcessId(this->GetLocalProcessId()) < 0)
    {
    // The group does not contain this process.  Just return NULL.
    return NULL;
    }

  vtkSubCommunicator *subcomm = vtkSubCommunicator::New();
  subcomm->SetGroup(group);

  // We only need a basic implementation of vtkMultiProcessController for the
  // subgroup, so we just use vtkDummyController here.  It's a bit of a misnomer
  // and may lead to confusion, but I think it's better than creating yet
  // another class we have to maintain.
  vtkDummyController *subcontroller = vtkDummyController::New();
  subcontroller->SetCommunicator(subcomm);
  subcontroller->SetRMICommunicator(subcomm);

  subcomm->Delete();

  return subcontroller;
}

//----------------------------------------------------------------------------
int vtkMultiProcessController::RemoveFirstRMI(int tag)
{
  vtkMultiProcessControllerRMI *rmi = NULL;
  this->RMIs->InitTraversal();
  while ( (rmi = (vtkMultiProcessControllerRMI*)(this->RMIs->GetNextItemAsObject())) )
    {
    if (rmi->Tag == tag)
      {
      this->RMIs->RemoveItem(rmi);
      return 1;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkMultiProcessController::RemoveRMI(unsigned long id)
{
  vtkMultiProcessControllerRMI *rmi = NULL;
  this->RMIs->InitTraversal();
  while ( (rmi = (vtkMultiProcessControllerRMI*)(this->RMIs->GetNextItemAsObject())) )
    {
    if (rmi->Id == id)
      {
      this->RMIs->RemoveItem(rmi);
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
unsigned long vtkMultiProcessController::AddRMI(vtkRMIFunctionType f, 
                                       void *localArg, int tag)
{
  vtkMultiProcessControllerRMI *rmi = vtkMultiProcessControllerRMI::New();

  // Remove any previously registered RMI handler for the tag.
  this->RemoveFirstRMI(tag);

  rmi->Tag = tag;
  rmi->Function = f;
  rmi->LocalArgument = localArg;
  rmi->Id = this->RMICount;
  this->RMICount++;

  this->RMIs->AddItem(rmi);
  rmi->Delete();

  return rmi->Id;
}

//----------------------------------------------------------------------------
void vtkMultiProcessController::TriggerRMI(int remoteProcessId, 
                                           void *arg, int argLength,
                                           int rmiTag)
{
  int triggerMessage[3];

  // Deal with sending RMI to ourself here for now.
  if (remoteProcessId == this->GetLocalProcessId())
    {
    this->ProcessRMI(remoteProcessId, arg, argLength, rmiTag);
    return;
    }
  
  triggerMessage[0] = rmiTag;
  triggerMessage[1] = argLength;
  
  // It is important for the remote process to know what process invoked it.
  // Multiple processes might try to invoke the method at the same time.
  // The remote method will know where to get additional args.
  triggerMessage[2] = this->GetLocalProcessId();

  this->RMICommunicator->Send(triggerMessage, 3, remoteProcessId, RMI_TAG);
  if (argLength > 0)
    {
    this->RMICommunicator->Send((char*)arg, argLength, remoteProcessId,  
                                 RMI_ARG_TAG);
    } 
}

//----------------------------------------------------------------------------
void vtkMultiProcessController::TriggerBreakRMIs()
{
  int idx, num;

  if (this->GetLocalProcessId() != 0)
    {
    vtkErrorMacro("Break should be triggered from process 0.");
    return;
    }

  num = this->GetNumberOfProcesses();
  for (idx = 1; idx < num; ++idx)
    {
    this->TriggerRMI(idx, NULL, 0, BREAK_RMI_TAG);
    }
}

//----------------------------------------------------------------------------
int vtkMultiProcessController::ProcessRMIs()
{
  return this->ProcessRMIs(1, 0);
}

//----------------------------------------------------------------------------
int vtkMultiProcessController::ProcessRMIs(int reportErrors, int dont_loop)
{
  int triggerMessage[3];
  unsigned char *arg = NULL;
  int error = RMI_NO_ERROR;
  
  do 
    {
    if (!this->RMICommunicator->Receive(triggerMessage, 3, ANY_SOURCE, RMI_TAG))
      {
      if (reportErrors)
        {
        vtkErrorMacro("Could not receive RMI trigger message.");
        }
      error = RMI_TAG_ERROR;
      break;
      }
    if (triggerMessage[1] > 0)
      {
      arg = new unsigned char[triggerMessage[1]];
      if (!this->RMICommunicator->Receive((char*)(arg), triggerMessage[1], 
          triggerMessage[2], RMI_ARG_TAG))
        {
        if (reportErrors)
          {
          vtkErrorMacro("Could not receive RMI argument.");
          }
        error = RMI_ARG_ERROR;
        break;
        }
      }
    this->ProcessRMI(triggerMessage[2], arg, triggerMessage[1], 
      triggerMessage[0]);
    if (arg)
      {
      delete [] arg;
      arg = NULL;
      }

    // check for break
    if (this->BreakFlag)
      {
      this->BreakFlag = 0;
      return error;
      }
    } while (!dont_loop);

  return error;
}


//----------------------------------------------------------------------------
void vtkMultiProcessController::ProcessRMI(int remoteProcessId, 
                                           void *arg, int argLength,
                                           int rmiTag)
{
  vtkMultiProcessControllerRMI *rmi = NULL;
  int found = 0;

  // find the rmi
  this->RMIs->InitTraversal();
  while ( !found &&
   (rmi = (vtkMultiProcessControllerRMI*)(this->RMIs->GetNextItemAsObject())) )
    {
    if (rmi->Tag == rmiTag)
      {
      found = 1;
      }
    }
  
  if ( ! found)
    {
    vtkErrorMacro("Process " << this->GetLocalProcessId() << 
                  " Could not find RMI with tag " << rmiTag);
    }
  else
    {
    if ( rmi->Function )
      {
      (*rmi->Function)(rmi->LocalArgument, arg, argLength, remoteProcessId);
      }     
    }
}


//============================================================================
// The intent is to give access to a processes controller from a static method.

vtkMultiProcessController *VTK_GLOBAL_MULTI_PROCESS_CONTROLLER = NULL;
//----------------------------------------------------------------------------
vtkMultiProcessController *vtkMultiProcessController::GetGlobalController()
{
  if (VTK_GLOBAL_MULTI_PROCESS_CONTROLLER == NULL)
    {
    return NULL;
    }
  
  return VTK_GLOBAL_MULTI_PROCESS_CONTROLLER->GetLocalController();
}
//----------------------------------------------------------------------------
// This can be overridden in the subclass to translate controllers.
vtkMultiProcessController *vtkMultiProcessController::GetLocalController()
{
  return VTK_GLOBAL_MULTI_PROCESS_CONTROLLER;
}
//----------------------------------------------------------------------------
// This can be overridden in the subclass to translate controllers.
void vtkMultiProcessController::SetGlobalController(
                                   vtkMultiProcessController *controller)
{
  VTK_GLOBAL_MULTI_PROCESS_CONTROLLER = controller;
}


