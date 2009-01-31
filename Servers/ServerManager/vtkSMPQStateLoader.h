/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMPQStateLoader.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPQStateLoader - State loader for ParaView client.
// .SECTION Description
// SMState file has render module states in it. Typically one can simply load 
// back the SMState and create new rendermodules from that SMState, just like 
// other proxies. However, when using MultiViewFactory, if a 
// MultiViewFactory exists, it must be used to obtain the 
// rendermodules, otherwise they will not work correctly. Hence, we provide 
// this loader. Set the MultiViewFactory on this loader before 
// loading the state. Then, when a render module is encountered in the state 
// the MultiViewFactory is requested to return
// a render module and the state is loaded on that render module.

#ifndef __vtkSMPQStateLoader_h
#define __vtkSMPQStateLoader_h

#include "vtkSMStateLoader.h"

class vtkSMRenderViewProxy;
//BTX
struct vtkSMPQStateLoaderInternals;
//ETX

class VTK_EXPORT vtkSMPQStateLoader : public vtkSMStateLoader
{
public:
  static vtkSMPQStateLoader* New();
  vtkTypeRevisionMacro(vtkSMPQStateLoader, vtkSMStateLoader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // For every request to create a render module, one from this list is used 
  // first, if possible
  virtual void AddPreferredRenderView(vtkSMRenderViewProxy*);
  void RemovePreferredRenderView(vtkSMRenderViewProxy*);
  void ClearPreferredRenderViews();

  // Description:
  // Set the name for the proxy to create when creating render views.
  // This is required since the type of render view created usually depends on
  // the type of connection/client etc.
  vtkSetStringMacro(RenderViewXMLName);

protected:
  vtkSMPQStateLoader();
  ~vtkSMPQStateLoader();

  // Description:
  // Overridden so that render modules and displays can be created of appropriate
  // types.
  virtual vtkSMProxy* NewProxyInternal(const char* xmlgroup, const char* xmlname);

  // Overridden to avoid registering the reused rendermodules twice.
  virtual void RegisterProxyInternal(const char* group, 
    const char* name, vtkSMProxy* proxy);

  char* RenderViewXMLName;

  vtkSMPQStateLoaderInternals *PQInternal;
private:
  vtkSMPQStateLoader(const vtkSMPQStateLoader&); // Not implemented.
  void operator=(const vtkSMPQStateLoader&); // Not implemented.
};

#endif

