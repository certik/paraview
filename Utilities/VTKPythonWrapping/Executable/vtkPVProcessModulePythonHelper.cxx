/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVProcessModulePythonHelper.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVProcessModulePythonHelper.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVPythonInterpretor.h"
#include "vtkPVPythonOptions.h"
#include "vtkSMApplication.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"

#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkPVProcessModulePythonHelper, "$Revision: 1.16 $");
vtkStandardNewMacro(vtkPVProcessModulePythonHelper);

//----------------------------------------------------------------------------
vtkPVProcessModulePythonHelper::vtkPVProcessModulePythonHelper()
{
  this->SMApplication = vtkSMApplication::New();
  this->DisableConsole = false;
}

//----------------------------------------------------------------------------
vtkPVProcessModulePythonHelper::~vtkPVProcessModulePythonHelper()
{
  this->SMApplication->Finalize();
  this->SMApplication->Delete();
}

//----------------------------------------------------------------------------
void vtkPVProcessModulePythonHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "DisableConsole: " << this->DisableConsole << endl;
}

//----------------------------------------------------------------------------
int vtkPVProcessModulePythonHelper::RunGUIStart(int argc, char **argv, int numServerProcs, int myId)
{
  (void)myId;
  (void)numServerProcs;

  this->SMApplication->Initialize();
  vtkSMProperty::SetCheckDomains(0);

  vtkPVPythonOptions* boptions = vtkPVPythonOptions::SafeDownCast(this->ProcessModule->GetOptions());
  int res = 0; 


  vtkstd::vector<char*> vArg;
#define vtkPVStrDup(x) \
  strcpy(new char[ strlen(x) + 1], x)

  vArg.push_back(vtkPVStrDup(argv[0]));
  if ( boptions->GetPythonScriptName() )
    {
    vArg.push_back(vtkPVStrDup(boptions->GetPythonScriptName()));
    }
  else if (argc > 1)
    {
    vArg.push_back(vtkPVStrDup("-"));
    }
  for (int cc=1; cc < argc; cc++)
    {
    vArg.push_back(vtkPVStrDup(argv[cc]));
    }

  vtkPVPythonInterpretor* interpretor = vtkPVPythonInterpretor::New();
  if (this->DisableConsole)
    {
    if (!boptions->GetPythonScriptName())
      {
      vtkErrorMacro("No script specified");
      }
    else
      {
      res = interpretor->PyMain(vArg.size(), &*vArg.begin());
      }
    }
  else
    {
    res = interpretor->PyMain(vArg.size(), &*vArg.begin());
    }
  interpretor->Delete();

  vtkstd::vector<char*>::iterator it;
  for ( it = vArg.begin(); it != vArg.end(); ++ it )
    {
    delete [] *it;
    }

  // Exiting:  CLean up.
  return res;
}

//----------------------------------------------------------------------------
void vtkPVProcessModulePythonHelper::ExitApplication()
{ 
}

//----------------------------------------------------------------------------
void vtkPVProcessModulePythonHelper::SendPrepareProgress()
{
}

//----------------------------------------------------------------------------
void vtkPVProcessModulePythonHelper::SendCleanupPendingProgress()
{
}

//----------------------------------------------------------------------------
void vtkPVProcessModulePythonHelper::SetLocalProgress(const char*, int)
{
}

