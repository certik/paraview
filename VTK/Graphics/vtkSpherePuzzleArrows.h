/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSpherePuzzleArrows.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSpherePuzzleArrows - Visualize permutation of the sphere puzzle.
// .SECTION Description
// vtkSpherePuzzleArrows creates 

#ifndef __vtkSpherePuzzleArrows_h
#define __vtkSpherePuzzleArrows_h

#include "vtkPolyDataAlgorithm.h"

class vtkCellArray;
class vtkPoints;
class vtkSpherePuzzle;

class VTK_GRAPHICS_EXPORT vtkSpherePuzzleArrows : public vtkPolyDataAlgorithm 
{
public:
  vtkTypeRevisionMacro(vtkSpherePuzzleArrows,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  static vtkSpherePuzzleArrows *New();

  // Description:
  vtkSetVectorMacro(Permutation,int,32);
  vtkGetVectorMacro(Permutation,int,32);
  void SetPermutationComponent(int comp, int val);
  void SetPermutation(vtkSpherePuzzle *puz);

protected:
  vtkSpherePuzzleArrows();
  ~vtkSpherePuzzleArrows();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  void AppendArrow(int id0, int id1, vtkPoints *pts, vtkCellArray *polys);
  
  int Permutation[32];

  double Radius;

private:
  vtkSpherePuzzleArrows(const vtkSpherePuzzleArrows&); // Not implemented
  void operator=(const vtkSpherePuzzleArrows&); // Not implemented
};

#endif
