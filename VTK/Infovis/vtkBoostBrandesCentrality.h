/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkBoostBrandesCentrality.h,v $

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
// .NAME vtkBoostBrandesCentrality - Compute Brandes betweenness centrality 
// on a vtkGraph

// .SECTION Description

// This vtk class uses the Boost brandes_betweeness_centrality
// generic algorithm to compute betweenness centrality on 
// the input graph (a vtkGraph).

// .SECTION See Also
// vtkGraph vtkGraphToBoostAdapter

#ifndef __vtkBoostBrandesCentrality_h
#define __vtkBoostBrandesCentrality_h

#include "vtkVariant.h" // For variant type

#include "vtkGraphAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkBoostBrandesCentrality : public vtkGraphAlgorithm 
{
public:
  static vtkBoostBrandesCentrality *New();
  vtkTypeRevisionMacro(vtkBoostBrandesCentrality, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkBoostBrandesCentrality();
  ~vtkBoostBrandesCentrality();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  
private:

  vtkIdType OriginVertexIndex;
  char* ArrayName;
  //BTX
  vtkVariant OriginValue;
  //ETX
  bool ArrayNameSet;
  
  // Description:
  // Using the convenience function for set strings internally
  vtkSetStringMacro(ArrayName);
  
  //BTX

  // Description:
  // This method is basically a helper function to find
  // the index of a specific value within a specific array
  vtkIdType GetVertexIndex(
    vtkAbstractArray *abstract,vtkVariant value);
  //ETX

  vtkBoostBrandesCentrality(const vtkBoostBrandesCentrality&);  // Not implemented.
  void operator=(const vtkBoostBrandesCentrality&);  // Not implemented.
};

#endif
