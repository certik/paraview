/*=========================================================================

Program:   ParaView
Module:    $RCSfile: vtkInitializationHelper.cxx,v $

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInitializationHelper.h"

/*
 * Make sure all the kits register their classes with vtkInstantiator.
 * Since ParaView uses Tcl wrapping, all of VTK is already compiled in
 * anyway.  The instantiators will add no more code for the linker to
 * collect.
 */
#include "vtkCommonInstantiator.h"
#include "vtkFilteringInstantiator.h"
#include "vtkGenericFilteringInstantiator.h"
#include "vtkIOInstantiator.h"
#include "vtkImagingInstantiator.h"
#include "vtkGraphicsInstantiator.h"

#include "vtkRenderingInstantiator.h"
#include "vtkVolumeRenderingInstantiator.h"
#include "vtkHybridInstantiator.h"
#include "vtkParallelInstantiator.h"

#include "vtkPVServerCommonInstantiator.h"
#include "vtkPVFiltersInstantiator.h"
#include "vtkPVServerManagerInstantiator.h"
#include "vtkClientServerInterpreter.h"

#include "vtkDummyProcessModuleHelper.h"
#include "vtkPVMain.h"
#include "vtkPVOptions.h"
#include "vtkProcessModule.h"
#include "vtkSMApplication.h"
#include "vtkSMProperty.h"

#include <vtkstd/string>

vtkCxxRevisionMacro(vtkInitializationHelper, "$Revision: 1.2 $");

static void vtkInitializationHelperInit(vtkProcessModule* pm);

//----------------------------------------------------------------------------
// ClientServer wrapper initialization functions.
extern "C" void vtkCommonCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkFilteringCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkGenericFilteringCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkImagingCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkInfovisCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkGraphicsCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkIOCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkRenderingCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkVolumeRenderingCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkHybridCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkWidgetsCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkParallelCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkPVServerCommonCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkPVFiltersCS_Initialize(vtkClientServerInterpreter*);
extern "C" void vtkXdmfCS_Initialize(vtkClientServerInterpreter *);

vtkDummyProcessModuleHelper* vtkInitializationHelper::Helper = 0;
vtkPVMain* vtkInitializationHelper::PVMain = 0;
vtkPVOptions* vtkInitializationHelper::Options = 0;
vtkSMApplication* vtkInitializationHelper::Application = 0;

//----------------------------------------------------------------------------
void vtkInitializationHelper::Initialize(const char* executable)
{
  if (!executable)
    {
    vtkGenericWarningMacro("Executable name has to be defined.");
    return;
    }
  if (PVMain)
    {
    vtkGenericWarningMacro("Python module already initialize. Skipping.");
    return;
    }
  vtkPVMain::SetInitializeMPI(0); // don't use MPI even when available.
  PVMain = vtkPVMain::New();
  Options = vtkPVOptions::New();
  Options->SetProcessType(vtkPVOptions::PVCLIENT);
  // This process module does nothing
  Helper = vtkDummyProcessModuleHelper::New();
  // Pass the program name to make option parser happier
  char* argv = new char[strlen(executable)+1];
  strcpy(argv, executable);
  // First initialization
  PVMain->Initialize(Options, Helper, vtkInitializationHelperInit, 1, &argv);
  Application = vtkSMApplication::New();
  Application->Initialize();
  vtkSMProperty::SetCheckDomains(0);
  vtkProcessModule::GetProcessModule()->SupportMultipleConnectionsOn();
  // Initialize everything else
  PVMain->Run(Options);
  delete[] argv;
}

//----------------------------------------------------------------------------
void vtkInitializationHelper::Finalize()
{
  vtkSMObject::SetProxyManager(0);
  if (PVMain)
    {
    PVMain->Delete();
    PVMain = 0;
    }
  if (Application)
    {
    Application->Delete();
    Application = 0;
    }
  if (Helper)
    {
    Helper->Delete();
    Helper = 0;
    }
  if (Options)
    {
    Options->Delete();
    Options = 0;
    }
  vtkProcessModule::SetProcessModule(0);
}

//----------------------------------------------------------------------------
void vtkInitializationHelperInit(vtkProcessModule* pm)
{
  // Initialize built-in wrapper modules.
  vtkCommonCS_Initialize(pm->GetInterpreter());
  vtkFilteringCS_Initialize(pm->GetInterpreter());
  vtkGenericFilteringCS_Initialize(pm->GetInterpreter());
  vtkImagingCS_Initialize(pm->GetInterpreter());
  vtkInfovisCS_Initialize(pm->GetInterpreter());
  vtkGraphicsCS_Initialize(pm->GetInterpreter());
  vtkIOCS_Initialize(pm->GetInterpreter());
  vtkRenderingCS_Initialize(pm->GetInterpreter());
  vtkVolumeRenderingCS_Initialize(pm->GetInterpreter());
  vtkHybridCS_Initialize(pm->GetInterpreter());
  vtkWidgetsCS_Initialize(pm->GetInterpreter());
  vtkParallelCS_Initialize(pm->GetInterpreter());
  vtkPVServerCommonCS_Initialize(pm->GetInterpreter());
  vtkPVFiltersCS_Initialize(pm->GetInterpreter());
  vtkXdmfCS_Initialize(pm->GetInterpreter());
}
