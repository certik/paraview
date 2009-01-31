/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkTreeLayoutStrategy.h,v $

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
// .NAME vtkTreeLayoutStrategy - hierarchical layout
//
// .SECTION Description
// Assigns points to the nodes of a tree in either a standard or radial layout.
// The standard layout places each level on a horizontal line, while the
// radial layout places each level on a concentric circle.
// You may specify the sweep angle of the tree which constrains the tree
// to be contained within a wedge. Also, you may indicate the log scale of
// the tree, which diminishes the length of arcs at lower levels of the tree.
// Values near zero give a large proportion of the space to the tree levels
// near the root, while values near one give nearly equal proportions of space
// to all tree levels.
//
// The user may also specify an array to use to indicate the distance from the
// root, either vertically (for standard layout) or radially
// (for radial layout).  You specify this with SetDistanceArrayName().

#ifndef __vtkTreeLayoutStrategy_h
#define __vtkTreeLayoutStrategy_h

#include "vtkGraphLayoutStrategy.h"

class VTK_INFOVIS_EXPORT vtkTreeLayoutStrategy : public vtkGraphLayoutStrategy 
{
public:
  static vtkTreeLayoutStrategy *New();

  vtkTypeRevisionMacro(vtkTreeLayoutStrategy, vtkGraphLayoutStrategy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Perform the tree layout.
  void Layout();

  // Description:
  // The sweep angle of the tree.
  // For a standard tree layout, this should be between 0 and 180.
  // For a radial tree layout, this can be between 0 and 360.
  vtkSetClampMacro(Angle, double, 0, 360);
  vtkGetMacro(Angle, double);

  // Description:
  // If set, the tree is laid out with levels on concentric circles 
  // around the root. If unset (default), the tree is laid out with 
  // levels on horizontal lines.
  vtkSetMacro(Radial, bool);
  vtkGetMacro(Radial, bool);
  vtkBooleanMacro(Radial, bool);

  // Description:
  // The spacing of tree levels. Levels near zero give more space
  // to levels near the root, while levels near one (the default)
  // create evenly-spaced levels.
  vtkSetClampMacro(LogSpacingValue, double, 0.0, 1.0);
  vtkGetMacro(LogSpacingValue, double);

  // Description:
  // The spacing of leaves.  Levels near one evenly space leaves
  // with no gaps between subtrees.  Levels near zero creates
  // large gaps between subtrees.
  vtkSetClampMacro(LeafSpacing, double, 0.0, 1.0);
  vtkGetMacro(LeafSpacing, double);

  // Description:
  vtkSetStringMacro(DistanceArrayName);
  vtkGetStringMacro(DistanceArrayName);

protected:
  vtkTreeLayoutStrategy();
  ~vtkTreeLayoutStrategy();

  double Angle;
  bool Radial;
  double LogSpacingValue;
  double LeafSpacing;
  char* DistanceArrayName;

private:

  vtkTreeLayoutStrategy(const vtkTreeLayoutStrategy&);  // Not implemented.
  void operator=(const vtkTreeLayoutStrategy&);  // Not implemented.
};

#endif

