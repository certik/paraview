/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkHierarchicalPolyDataMapper.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalPolyDataMapper - a class that renders hierarchical polygonal data
// .SECTION Description
// Legacy class. Use vtkMultiGroupPolyDataMapper instead.
//
// .SECTION see also
// vtkPolyDataMapper

#ifndef __vtkHierarchicalPolyDataMapper_h
#define __vtkHierarchicalPolyDataMapper_h

#include "vtkMultiGroupPolyDataMapper.h"

class VTK_RENDERING_EXPORT vtkHierarchicalPolyDataMapper : public vtkMultiGroupPolyDataMapper 
{

public:
  static vtkHierarchicalPolyDataMapper *New();
  vtkTypeRevisionMacro(vtkHierarchicalPolyDataMapper, vtkMultiGroupPolyDataMapper);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkHierarchicalPolyDataMapper();
  ~vtkHierarchicalPolyDataMapper();
  
private:
  vtkHierarchicalPolyDataMapper(const vtkHierarchicalPolyDataMapper&);  // Not implemented.
  void operator=(const vtkHierarchicalPolyDataMapper&);    // Not implemented.
};

#endif
