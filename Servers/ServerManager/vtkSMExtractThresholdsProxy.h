/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMExtractThresholdsProxy.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMExtractThresholdsProxy - proxy for extract threshold filters. 
// .SECTION Description
// vtkSMExtractThresholdsProxy has a subproxy which is the proxy for the 
// selection.

#ifndef __vtkSMExtractThresholdsProxy_h
#define __vtkSMExtractThresholdsProxy_h

#include "vtkSMSourceProxy.h"

class vtkDoubleArray;

class VTK_EXPORT vtkSMExtractThresholdsProxy : public vtkSMSourceProxy
{
public:
  static vtkSMExtractThresholdsProxy* New();
  vtkTypeRevisionMacro(vtkSMExtractThresholdsProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Update the VTK object on the server by pushing the values of
  // all modifed properties (un-modified properties are ignored).
  // If the object has not been created, it will be created first.
  virtual void UpdateVTKObjects();

  void SetScalarArray(int idx, int port, int conn,
                      int field, const char *name);

//BTX
protected:
  vtkSMExtractThresholdsProxy();
  ~vtkSMExtractThresholdsProxy();

  virtual void CreateVTKObjects();

  int Field;
  char *ArrayName;
private:
  vtkSMExtractThresholdsProxy(const vtkSMExtractThresholdsProxy&); // Not implemented.
  void operator=(const vtkSMExtractThresholdsProxy&); // Not implemented.
//ETX
};

#endif

