/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVDisplayInformation.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVDisplayInformation -
// .SECTION Description

#ifndef __vtkPVDisplayInformation_h
#define __vtkPVDisplayInformation_h

#include "vtkPVInformation.h"
class VTK_EXPORT vtkPVDisplayInformation : public vtkPVInformation
{
public:
  static vtkPVDisplayInformation* New();
  vtkTypeRevisionMacro(vtkPVDisplayInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single object into this object.
  virtual void CopyFromObject(vtkObject*);

  // Description:
  // Merge another information object. Calls AddInformation(info, 0).
  virtual void AddInformation(vtkPVInformation* info);

  // Description:
  // Manage a serialized version of the information.
  virtual void CopyToStream(vtkClientServerStream*);
  virtual void CopyFromStream(const vtkClientServerStream*);

  // Description:
  // CanOpenDisplay is set to 1 if a window can be opened on
  // the display.
  vtkGetMacro(CanOpenDisplay, int);

protected:
  vtkPVDisplayInformation();
  ~vtkPVDisplayInformation();

  int CanOpenDisplay;

private:
  vtkPVDisplayInformation(const vtkPVDisplayInformation&); // Not implemented
  void operator=(const vtkPVDisplayInformation&); // Not implemented
};

#endif
