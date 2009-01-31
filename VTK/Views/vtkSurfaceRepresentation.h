/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSurfaceRepresentation.h,v $

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
// .NAME vtkSurfaceRepresentation - Displays a geometric dataset as a surface.
//
// .SECTION Description
// vtkSurfaceRepresentation is used to show a geometric dataset in a view.
// The representation uses a vtkGeometryFilter to convert the dataset to
// polygonal data (e.g. volumetric data is converted to its external surface).
// The representation may then be added to vtkRenderView.

#ifndef __vtkSurfaceRepresentation_h
#define __vtkSurfaceRepresentation_h

#include "vtkDataRepresentation.h"

class vtkActor;
class vtkAlgorithmOutput;
class vtkDataObject;
class vtkExtractSelection;
class vtkGeometryFilter;
class vtkPolyDataMapper;
class vtkSelection;
class vtkView;

class VTK_VIEWS_EXPORT vtkSurfaceRepresentation : public vtkDataRepresentation
{
public:
  static vtkSurfaceRepresentation *New();
  vtkTypeRevisionMacro(vtkSurfaceRepresentation, vtkDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Sets the input pipeline connection to this representation.
  virtual void SetInputConnection(vtkAlgorithmOutput* conn);
  
  // Description:
  // Plugs the selection link into the internal pipeline.
  virtual void SetSelectionLink(vtkSelectionLink* link);
  
protected:
  vtkSurfaceRepresentation();
  ~vtkSurfaceRepresentation();
  
  // Decription:
  // Adds the representation to the view.  This is called from
  // vtkView::AddRepresentation().
  virtual bool AddToView(vtkView* view);
  
  // Decription:
  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().
  virtual bool RemoveFromView(vtkView* view);
  
  // Description:
  // Convert the selection to a type appropriate for sharing with other
  // representations through vtkSelectionLink.
  // If the selection cannot be applied to this representation, returns NULL.
  virtual vtkSelection* ConvertSelection(vtkView* view, vtkSelection* selection);
  
  // Description:
  // Internal pipeline objects.
  vtkGeometryFilter*    GeometryFilter;
  vtkPolyDataMapper*    Mapper;
  vtkActor*             Actor;
  vtkExtractSelection*  ExtractSelection;
  vtkGeometryFilter*    SelectionGeometryFilter;
  vtkPolyDataMapper*    SelectionMapper;
  vtkActor*             SelectionActor;
  
private:
  vtkSurfaceRepresentation(const vtkSurfaceRepresentation&);  // Not implemented.
  void operator=(const vtkSurfaceRepresentation&);  // Not implemented.
};

#endif
