/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMTextWidgetRepresentationProxy.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMTextWidgetRepresentationProxy 
// .SECTION Description

#ifndef __vtkSMTextWidgetRepresentationProxy_h
#define __vtkSMTextWidgetRepresentationProxy_h

#include "vtkSMNewWidgetRepresentationProxy.h"

class vtkSMViewProxy;

class VTK_EXPORT vtkSMTextWidgetRepresentationProxy : public vtkSMNewWidgetRepresentationProxy
{
public:
  static vtkSMTextWidgetRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMTextWidgetRepresentationProxy, vtkSMNewWidgetRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);
  
protected:
//BTX
  vtkSMTextWidgetRepresentationProxy();
  ~vtkSMTextWidgetRepresentationProxy();
  
  virtual void CreateVTKObjects();

  // Description:
  // Called when a representation is added to a view. 
  // Returns true on success.
  // Currently a representation can be added to only one view.
  virtual bool AddToView(vtkSMViewProxy* view);

  // Description:
  // Called to remove a representation from a view.
  // Returns true on success.
  // Currently a representation can be added to only one view.
  virtual bool RemoveFromView(vtkSMViewProxy* view);

  int Visibility;

  vtkSMProxy* TextActorProxy;
  vtkSMProxy* TextPropertyProxy;
  
  vtkSMViewProxy* ViewProxy;

  friend class vtkSMTextSourceRepresentationProxy;

private:
  vtkSMTextWidgetRepresentationProxy(const vtkSMTextWidgetRepresentationProxy&); // Not implemented
  void operator=(const vtkSMTextWidgetRepresentationProxy&); // Not implemented
//ETX
};

#endif

