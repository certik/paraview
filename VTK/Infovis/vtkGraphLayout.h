/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkGraphLayout.h,v $

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
// .NAME vtkGraphLayout - layout a graph in 2 or 3 dimensions
//
// .SECTION Description
// This class is a shell for many graph layout strategies which may be set
// using the SetLayoutStrategy() function.  The layout strategies do the
// actual work.
//
// .SECION Thanks
// Thanks to Brian Wylie from Sandia National Laboratories for adding incremental
// layout capabilities.

#ifndef __vtkGraphLayout_h
#define __vtkGraphLayout_h

#include "vtkAbstractGraphAlgorithm.h"

class vtkGraphLayoutStrategy;
class vtkEventForwarderCommand;

class VTK_INFOVIS_EXPORT vtkGraphLayout : public vtkAbstractGraphAlgorithm 
{
public:
  static vtkGraphLayout *New();
  vtkTypeRevisionMacro(vtkGraphLayout, vtkAbstractGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The layout strategy to use during graph layout.
  void SetLayoutStrategy(vtkGraphLayoutStrategy *strategy);
  vtkGetObjectMacro(LayoutStrategy, vtkGraphLayoutStrategy);
  
  // Description:
  // Ask the layout algorithm if the layout is complete
  virtual int IsLayoutComplete();

  // Description:
  // Get the modification time of the layout algorithm.
  virtual unsigned long GetMTime();

protected:
  vtkGraphLayout();
  ~vtkGraphLayout();

  vtkGraphLayoutStrategy* LayoutStrategy;

  // Description:
  // This intercepts events from the strategy object and re-emits them
  // as if they came from the layout engine itself.
  vtkEventForwarderCommand *EventForwarder;
  unsigned long ObserverTag;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  
private:

  vtkAbstractGraph *LastInput;
  unsigned long LastInputMTime;
  vtkAbstractGraph *InternalGraph;
  bool StrategyChanged;

  vtkGraphLayout(const vtkGraphLayout&);  // Not implemented.
  void operator=(const vtkGraphLayout&);  // Not implemented.
};

#endif
