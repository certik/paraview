
/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkInitializationHelper.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInitializationHelper - help class for python modules
// .SECTION Description
// This class is used by the python modules when they are loaded from
// python (as opposed to pvpython). It simply initializes the server
// manager so that it can be used.

#ifndef __vtkInitializationHelper_h
#define __vtkInitializationHelper_h

#include "vtkObject.h"

class vtkDummyProcessModuleHelper;
class vtkPVMain;
class vtkPVOptions;
class vtkSMApplication;

class VTK_EXPORT vtkInitializationHelper : public vtkObject
{
public: 
  vtkTypeRevisionMacro(vtkInitializationHelper,vtkObject);

  // Description:
  // Initializes the server manager. Do not use the server manager
  // before calling this.
  static void Initialize(const char* executable);

  // Description:
  // Finalizes the server manager. Do not use the server manager
  // after calling this.
  static void Finalize();

protected:
  vtkInitializationHelper() {};
  virtual ~vtkInitializationHelper() {};

  static vtkPVMain* PVMain;
  static vtkSMApplication* Application;
  static vtkPVOptions* Options;
  static vtkDummyProcessModuleHelper* Helper;

private:

  vtkInitializationHelper(const vtkInitializationHelper&); // Not implemented
  void operator=(const vtkInitializationHelper&); // Not implemented
};

#endif
