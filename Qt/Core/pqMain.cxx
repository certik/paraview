/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqMain.cxx,v $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "pqMain.h"
#include <QApplication>
#include "pqOptions.h"
#include "pqProcessModuleGUIHelper.h"

#include <vtkToolkits.h> // For VTK_USE_MPI
#include <vtkPVConfig.h> // Required to get build options for paraview
#include <vtkPVMain.h>
#include <vtkProcessModule.h>
#include <vtkPQConfig.h>

/*
 * Make sure all the kits register their classes with vtkInstantiator.
 * Since ParaView uses Tcl wrapping, all of VTK is already compiled in
 * anyway.  The instantiators will add no more code for the linker to
 * collect.
 */

#include <vtkCommonInstantiator.h>
#include <vtkFilteringInstantiator.h>
#include <vtkGenericFilteringInstantiator.h>
#include <vtkIOInstantiator.h>
#include <vtkImagingInstantiator.h>
#include <vtkInfovisInstantiator.h>
#include <vtkGraphicsInstantiator.h>

#include <vtkRenderingInstantiator.h>
#include <vtkVolumeRenderingInstantiator.h>
#include <vtkHybridInstantiator.h>
#include <vtkParallelInstantiator.h>

#include <vtkPVServerCommonInstantiator.h>
#include <vtkPVFiltersInstantiator.h>
#include <vtkPVServerManagerInstantiator.h>
#include <vtkClientServerInterpreter.h>

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

//----------------------------------------------------------------------------
void ParaViewInitializeInterpreter(vtkProcessModule* pm)
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

int pqMain::Run(QApplication& app, pqProcessModuleGUIHelper* helper)
{
  int argc = app.argc();
  char** argv = app.argv();

  vtkPVMain::SetInitializeMPI(0);  // pvClient never runs with MPI.
  vtkPVMain::Initialize(&argc, &argv); // Perform any initializations.

  vtkPVMain* pvmain = vtkPVMain::New();
  pqOptions* options = pqOptions::New();
  // We may define a PQCLIENT enum, if necessary.
  options->SetProcessType(vtkPVOptions::PVCLIENT);
 
  // This creates the Process Module and initializes it.
  int ret = pvmain->Initialize(options, helper, ParaViewInitializeInterpreter, 
                               argc, argv);
  if (!ret)
    {
    // Tell process module that we support Multiple connections.
    // This must be set before starting the event loop.
    vtkProcessModule::GetProcessModule()->SupportMultipleConnectionsOn();
    ret = helper->Run(options);
    }

  helper->Delete();
  options->Delete();
  pvmain->Delete();
  vtkPVMain::Finalize();
  vtkProcessModule::SetProcessModule(0);
  
  return ret;
}

