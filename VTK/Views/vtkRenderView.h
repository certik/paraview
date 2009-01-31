/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkRenderView.h,v $

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
// .NAME vtkRenderView - A view containing a renderer.
//
// .SECTION Description
// vtkRenderView is a view which contains a vtkRenderer.  You may add vtkActors
// directly to the renderer, or add certain vtkDataRepresentation subclasses
// to the renderer.  The render view supports drag selection with the mouse to
// select cells.
//
// This class is also the parent class for any more specialized view which uses
// a renderer.

#ifndef __vtkRenderView_h
#define __vtkRenderView_h

#include "vtkView.h"

class vtkInteractorStyle;
class vtkRenderer;
class vtkRenderWindow;
class vtkViewTheme;

class VTK_VIEWS_EXPORT vtkRenderView : public vtkView
{
public:
  static vtkRenderView *New();
  vtkTypeRevisionMacro(vtkRenderView, vtkView);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Gets the renderer for this view.
  vtkGetObjectMacro(Renderer, vtkRenderer);
  
  // Description:
  // Set up a render window to use this view.
  // The superclass adds the renderer to the render window.
  // Subclasses should override this to set interactor, etc.
  virtual void SetupRenderWindow(vtkRenderWindow* win);

  // Description:
  // Get a handle to the render window.
  vtkRenderWindow* GetRenderWindow();
  
  // Description:
  // Apply a theme to the view.
  virtual void ApplyViewTheme(vtkViewTheme* theme);

  // Description:
  // Update the view.
  virtual void Update();
  
protected:
  vtkRenderView();
  ~vtkRenderView();
  
  // Description:
  // Called to process events.
  // Captures StartEvent events from the renderer and calls Update().
  // This may be overridden by subclasses to process additional events.
  virtual void ProcessEvents(vtkObject* caller, unsigned long eventId, 
    void* callData);
  
  // Description:
  // Called by the view when the renderer is about to render.
  virtual void PrepareForRendering() { }
  
  // Description:
  // Called when a representation's selection changed.
  virtual void RepresentationSelectionChanged(
    vtkDataRepresentation* rep,
    vtkSelection* selection);
  
  // Description:
  // Allow subclasses to change the interactor style.
  vtkGetObjectMacro(InteractorStyle, vtkInteractorStyle);
  void SetInteractorStyle(vtkInteractorStyle* style);
  
  vtkRenderer* Renderer;
  vtkInteractorStyle* InteractorStyle;
  
private:
  vtkRenderView(const vtkRenderView&);  // Not implemented.
  void operator=(const vtkRenderView&);  // Not implemented.
};

#endif
