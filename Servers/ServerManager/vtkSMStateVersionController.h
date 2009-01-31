/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMStateVersionController.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMStateVersionController
// .SECTION Description
// vtkSMStateVersionController is used to convert the state XML from any
// previously supported versions to the current version.

#ifndef __vtkSMStateVersionController_h
#define __vtkSMStateVersionController_h

#include "vtkSMStateVersionControllerBase.h"

class VTK_EXPORT vtkSMStateVersionController : public vtkSMStateVersionControllerBase
{
public:
  static vtkSMStateVersionController* New();
  vtkTypeRevisionMacro(vtkSMStateVersionController, vtkSMStateVersionControllerBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called before a state is loaded. 
  // The argument must be the root element for the state being loaded.
  // eg. for server manager state, it will point to <ServerManagerState />
  // element.
  // Returns false if the conversion failed, else true.
  virtual bool Process(vtkPVXMLElement* root);

  bool Process_3_0_To_3_1(vtkPVXMLElement* root);

//BTX
  bool ConvertViewModulesToViews(vtkPVXMLElement* parent);
  bool ConvertLegacyReader(vtkPVXMLElement* parent);
  bool ConvertPVAnimationSceneToAnimationScene(vtkPVXMLElement* parent);
protected:
  vtkSMStateVersionController();
  ~vtkSMStateVersionController();

private:
  vtkSMStateVersionController(const vtkSMStateVersionController&); // Not implemented
  void operator=(const vtkSMStateVersionController&); // Not implemented
//ETX
};

#endif

