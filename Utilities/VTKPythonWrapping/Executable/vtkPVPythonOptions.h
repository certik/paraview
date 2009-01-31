/*=========================================================================
  
  Program:   ParaView
  Module:    $RCSfile: vtkPVPythonOptions.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPythonOptions - ParaView options storage
// .SECTION Description
// An object of this class represents a storage for ParaView options
// 
// These options can be retrieved during run-time, set using configuration file
// or using Command Line Arguments.

#ifndef __vtkPVPythonOptions_h
#define __vtkPVPythonOptions_h

#include "vtkPVOptions.h"

class VTK_EXPORT vtkPVPythonOptions : public vtkPVOptions
{
public:
  static vtkPVPythonOptions* New();
  vtkTypeRevisionMacro(vtkPVPythonOptions,vtkPVOptions);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkGetStringMacro(PythonScriptName);

protected:
  // Description:
  // Default constructor.
  vtkPVPythonOptions();

  // Description:
  // Destructor.
  virtual ~vtkPVPythonOptions();

  // Description:
  // Initialize arguments.
  virtual void Initialize();

  // Description:
  // After parsing, process extra option dependencies.
  virtual int PostProcess(int argc, const char* const* argv);

  // Description:
  // This method is called when wrong argument is found. If it returns 0, then
  // the parsing will fail.
  virtual int WrongArgument(const char* argument);

  // Options:
  vtkSetStringMacro(PythonScriptName);
  char* PythonScriptName;

private:
  vtkPVPythonOptions(const vtkPVPythonOptions&); // Not implemented
  void operator=(const vtkPVPythonOptions&); // Not implemented
};

#endif // #ifndef __vtkPVPythonOptions_h


