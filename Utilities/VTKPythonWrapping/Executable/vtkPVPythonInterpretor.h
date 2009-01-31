/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVPythonInterpretor.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPythonInterpretor - Encapsulates a single instance of a python
// interpretor.
// .SECTION Description
// Encapsulates a python interpretor. It also initializes the interpretor
// with the paths to the paraview libraries and modules. This object can 
// represent the main interpretor or any sub-interpretor.
// .SECTION Caveat
// Since this class uses a static variable to keep track of interpretor lock
// state, it is not safe to use vtkPVPythonInterpretor instances in different
// threads. It is however possible to use it in the same thread of a
// multithreaded application and use python C calls in the other thread(s).
// In that case, it is necessary to set MultithreadSupport to true.

#ifndef __vtkPVPythonInterpretor_h
#define __vtkPVPythonInterpretor_h

#include "vtkObject.h"

class vtkStdString;
class vtkPVPythonInterpretorInternal;
class VTK_EXPORT vtkPVPythonInterpretor : public vtkObject
{
public:
  static vtkPVPythonInterpretor* New();
  vtkTypeRevisionMacro(vtkPVPythonInterpretor, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initializes python and starts the interprestor's event loop
  // i.e. call // Py_Main().
  int PyMain(int argc, char** argv);
  
  // Description:
  // Initializes python and create a sub-interpretor context.
  // Subinterpretors don't get argc/argv, however, argv[0] is
  // used to set the executable path if not already set. The
  // executable path is used to locate paraview python modules.
  // This method calls ReleaseControl() at the end of the call, hence the newly
  // created interpretor is not the active interpretor when the control returns.
  // Use MakeCurrent() to make it active.
  int InitializeSubInterpretor(int argc, char** argv);

  // Description:
  // This method will make the sub-interpretor represented by this object
  // the active one. If MultithreadSupport is enabled, this method also acquires
  // the global interpretor lock. A MakeCurrent() call must have a corresponding
  // ReleaseControl() to avoid deadlocks.
  void MakeCurrent();

  // Description:
  // Helper function that executes a script using PyRun_SimpleString() - handles
  // some pesky details with DOS line endings.
  // This method calls MakeCurrent() at the start and ReleaseControl() at the
  // end, hence the interpretor will not be the active one when the control
  // returns from this call. 
  void RunSimpleString(const char* const script);

  // Description:
  // Helper function that calls execfile().
  void RunSimpleFile(const char* const filename);

  // Description:
  // Call in a subinterpretter to pause it and return control to the 
  // main interpretor. If MultithreadSupport is enabled, this method also
  // releases the global interpretor lock.
  // A MakeCurrent() call must have a corresponding
  // ReleaseControl() to avoid deadlocks.
  void ReleaseControl();

  // Description:
  // When the interpretor is used in a multithreaded environment, python
  // requires additional initialization/locking. Set this to true to enable
  // initialization and locking of the global interpretor, if application is
  // multithreaded with sub-interpretors being initialized by different threads.
  // Must be set before python is initialized. 
  // Currently, it is not safe to use vtkPVPythonInterpretor in different
  // threads, however it is safe to use vtkPVPythonInterpretor in one thread and
  // explicit python calls in another (using python global interpretor locks).
  static void SetMultithreadSupport(bool enable);
  static bool GetMultithreadSupport();

  // Description:
  // In some cases, the application may want to capture the output/error streams
  // dumped by the python interpretor. When enabled, the streams are captured
  // and output/error is collected which can be flushed by FlushMessages.
  // vtkCommand::ErrorEvent is fired when data is received on stderr and
  // vtkCommand::WarningEvent is fired when data is received on stdout.
  // from the python interpretor. Event data for both the events is the text
  // received.  This flag can be changed only before the interpretor is 
  // initialized. Changing it afterwards has no effect.
  vtkSetMacro(CaptureStreams, bool);
  vtkGetMacro(CaptureStreams, bool);

  // Description:
  // Flush any errors received from the python interpretor to the
  // vtkOutputWindow. Applicable only if CaptureStreams was true when the
  // interpretor was initialized.
  void FlushMessages();

  // Description:
  // Clears all received messages. Unlike FlushMessages, this call does not dump
  // it on the vtkOutputWindow.
  // Applicable only if CaptureStreams was true when the
  // interpretor was initialized.
  void ClearMessages();

protected:
  vtkPVPythonInterpretor();
  ~vtkPVPythonInterpretor();

  // Description:
  // Initialize the interpretor.
  virtual void InitializeInternal();

  char* ExecutablePath;
  vtkSetStringMacro(ExecutablePath);

  bool CaptureStreams;

  friend struct vtkPVPythonInterpretorWrapper;

  void DumpError(const char* string);
  void DumpOutput(const char* string);
  
private:
  vtkPVPythonInterpretor(const vtkPVPythonInterpretor&); // Not implemented.
  void operator=(const vtkPVPythonInterpretor&); // Not implemented.

  vtkPVPythonInterpretorInternal* Internal;
};

#endif

