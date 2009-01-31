/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkGraphToPolyData.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkGraphToPolyData - convert a vtkGraph to vtkPolyData
//
// .SECTION Description
// Converts a vtkGraph to a vtkPolyData.  This assumes that the points
// of the graph have already been filled (perhaps by vtkGraphLayout),
// and coverts all the edge of the graph into lines in the polydata.
// The vertex data is passed along to the point data, and the edge data
// is passed along to the cell data.
//
// Only the owned graph edges (i.e. edges with ghost level 0) are copied
// into the vtkPolyData.

#ifndef __vtkGraphToPolyData_h
#define __vtkGraphToPolyData_h

#include "vtkPolyDataAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkGraphToPolyData : public vtkPolyDataAlgorithm 
{
public:
  static vtkGraphToPolyData *New();
  vtkTypeRevisionMacro(vtkGraphToPolyData,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the input type of the algorithm to vtkGraph.
  int FillInputPortInformation(int port, vtkInformation* info);

protected:
  vtkGraphToPolyData();
  ~vtkGraphToPolyData() {}

  // Description:
  // Convert the vtkGraph into vtkPolyData.
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
private:
  vtkGraphToPolyData(const vtkGraphToPolyData&);  // Not implemented.
  void operator=(const vtkGraphToPolyData&);  // Not implemented.
};

#endif
