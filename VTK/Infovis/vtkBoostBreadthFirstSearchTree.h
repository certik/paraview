/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkBoostBreadthFirstSearchTree.h,v $

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
// .NAME vtkBoostBreadthFirstSearchTree - Contructs a BFS tree from a graph
//
// .SECTION Description
//
// This vtk class uses the Boost breadth_first_search 
// generic algorithm to perform a breadth first search from a given
// a 'source' vertex on the input graph (a vtkGraph).
// The result is a tree with root node corresponding to the start node
// of the search.
//
// .SECTION See Also
// vtkGraph vtkGraphToBoostAdapter

#ifndef __vtkBoostBreadthFirstSearchTree_h
#define __vtkBoostBreadthFirstSearchTree_h

#include "vtkStdString.h" // For string type
#include "vtkVariant.h" // For variant type

#include "vtkTreeAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkBoostBreadthFirstSearchTree : public vtkTreeAlgorithm 
{
public:
  static vtkBoostBreadthFirstSearchTree *New();
  vtkTypeRevisionMacro(vtkBoostBreadthFirstSearchTree, vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set the index (into the vertex array) of the 
  // breadth first search 'origin' vertex.
  void SetOriginVertex(vtkIdType index);
  
  //BTX

  // Description:
  // Set the breadth first search 'origin' vertex.
  // This method is basically the same as above
  // but allows the application to simply specify
  // an array name and value, instead of having to
  // know the specific index of the vertex.
  void SetOriginVertex(vtkStdString arrayName, vtkVariant value);
  //ETX
  
  // Description:
  // Stores the graph vertex ids for the tree vertices in an array
  // named "GraphVertexId".  Default is off.
  vtkSetMacro(CreateGraphVertexIdArray, bool);
  vtkGetMacro(CreateGraphVertexIdArray, bool);
  vtkBooleanMacro(CreateGraphVertexIdArray, bool);

protected:
  vtkBoostBreadthFirstSearchTree();
  ~vtkBoostBreadthFirstSearchTree();

  int FillInputPortInformation(int port, vtkInformation* info);

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  
private:

  vtkIdType OriginVertexIndex;
  char* ArrayName;
  //BTX
  vtkVariant OriginValue;
  //ETX
  bool ArrayNameSet;
  bool CreateGraphVertexIdArray;
  
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

  vtkBoostBreadthFirstSearchTree(const vtkBoostBreadthFirstSearchTree&);  // Not implemented.
  void operator=(const vtkBoostBreadthFirstSearchTree&);  // Not implemented.
};

#endif
