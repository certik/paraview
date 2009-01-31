/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVMergeTables.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVMergeTables - used to merge rows in tables.
// .SECTION Description
// Simplified version of vtkMergeTables which simply combines tables merging
// columns. This assumes that each of the inputs either has exactly identical 
// columns or no columns at all. 
// .SECTION TODO
// We may want to merge this functionality into vtkMergeTables filter itself.

#ifndef __vtkPVMergeTables_h
#define __vtkPVMergeTables_h

#include "vtkTableAlgorithm.h"

class VTK_EXPORT vtkPVMergeTables : public vtkTableAlgorithm
{
public:
  static vtkPVMergeTables* New();
  vtkTypeRevisionMacro(vtkPVMergeTables, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPVMergeTables();
  ~vtkPVMergeTables();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

  virtual int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkPVMergeTables(const vtkPVMergeTables&); // Not implemented
  void operator=(const vtkPVMergeTables&); // Not implemented
//ETX
};

#endif

