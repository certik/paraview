/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMSummaryHelperProxy.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSummaryHelperProxy
// .SECTION Description
//

#ifndef __vtkSMSummaryHelperProxy_h
#define __vtkSMSummaryHelperProxy_h

#include "vtkSMProxy.h"

class VTK_EXPORT vtkSMSummaryHelperProxy : public vtkSMProxy
{
public:
  static vtkSMSummaryHelperProxy* New();
  vtkTypeRevisionMacro(vtkSMSummaryHelperProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSMSummaryHelperProxy();
  ~vtkSMSummaryHelperProxy();

  virtual void CreateVTKObjects();

private:
  vtkSMSummaryHelperProxy(const vtkSMSummaryHelperProxy&); // Not implemented.
  void operator=(const vtkSMSummaryHelperProxy&); // Not implemented.
};


#endif

