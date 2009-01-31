/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMObject.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMObject - superclass for most server manager classes
// .SECTION Description
// vtkSMObject provides several methods common to most server manager
// classes. These are mostly for setting and getting singletons including
// the communication and process modules and the proxy manager.

#ifndef __vtkSMObject_h
#define __vtkSMObject_h

#include "vtkObject.h"

class vtkSMProxyManager;
class vtkSMApplication;

class VTK_EXPORT vtkSMObject : public vtkObject
{
public:
  static vtkSMObject* New();
  vtkTypeRevisionMacro(vtkSMObject, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Proxy manager singleton stores all proxy groups and instances.
  static vtkSMProxyManager* GetProxyManager();
  static void SetProxyManager(vtkSMProxyManager* pm);

  // Description:
  // Application singleton is used to initialize server manager
  // and manages global runtime information.
  static vtkSMApplication* GetApplication();
  static void SetApplication(vtkSMApplication* app);

protected:
  vtkSMObject();
  ~vtkSMObject();

  static vtkSMProxyManager* ProxyManager;
  static vtkSMApplication* Application;

private:
  vtkSMObject(const vtkSMObject&); // Not implemented
  void operator=(const vtkSMObject&); // Not implemented
};

#endif
