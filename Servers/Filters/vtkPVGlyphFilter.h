/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVGlyphFilter.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVGlyphFilter - Glyph filter
//
// .SECTION Description
// This is a subclass of vtkGlyph3D that allows selection of input scalars

#ifndef __vtkPVGlyphFilter_h
#define __vtkPVGlyphFilter_h

#include "vtkGlyph3D.h"

class vtkMaskPoints;

class VTK_EXPORT vtkPVGlyphFilter : public vtkGlyph3D
{
public:
  vtkTypeRevisionMacro(vtkPVGlyphFilter,vtkGlyph3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description
  static vtkPVGlyphFilter *New();

  // Description:
  // Limit the number of points to glyph
  vtkSetMacro(MaximumNumberOfPoints, int);
  vtkGetMacro(MaximumNumberOfPoints, int);

  // Description:
  // Get the number of processes used to run this filter.
  vtkGetMacro(NumberOfProcesses, int);
  
  // Description:
  // Set/get whether to mask points
  vtkSetMacro(UseMaskPoints, int);
  vtkGetMacro(UseMaskPoints, int);

  // Description:
  // Set/get flag to cause randomization of which points to mask.
  void SetRandomMode(int mode);
  int GetRandomMode();

  // Description:
  // In processing composite datasets, will check if a point
  // is visible as long as the dataset being process if a
  // vtkUniformGrid.
  virtual int IsPointVisible(vtkDataSet* ds, vtkIdType ptId);

protected:
  vtkPVGlyphFilter();
  ~vtkPVGlyphFilter();

  virtual int RequestData(vtkInformation *, 
                          vtkInformationVector **, 
                          vtkInformationVector *);
  virtual int RequestCompositeData(vtkInformation* request,
                                   vtkInformationVector** inputVector,
                                   vtkInformationVector* outputVector);

  virtual int FillInputPortInformation(int, vtkInformation*);

  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();
  
  vtkIdType GatherTotalNumberOfPoints(vtkIdType localNumPts);

  int MaskAndExecute(vtkIdType numPts, vtkIdType maxNumPts,
                     vtkDataSet* input,
                     vtkInformation* request,
                     vtkInformationVector** inputVector,
                     vtkInformationVector* outputVector);

  vtkMaskPoints *MaskPoints;
  int MaximumNumberOfPoints;
  int NumberOfProcesses;
  int UseMaskPoints;
  int InputIsUniformGrid;
  
  vtkIdType BlockMaxNumPts;
  vtkIdType BlockOnRatio;
  vtkIdType BlockPointCounter;
  vtkIdType BlockNextPoint;
  vtkIdType BlockNumPts;

  int RandomMode;

  virtual void ReportReferences(vtkGarbageCollector*);
private:
  vtkPVGlyphFilter(const vtkPVGlyphFilter&);  // Not implemented.
  void operator=(const vtkPVGlyphFilter&);  // Not implemented.
};

#endif
