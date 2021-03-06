/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkActor2D.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkActor2D - a actor that draws 2D data
// .SECTION Description
// vtkActor2D is similar to vtkActor, but it is made to be used with two
// dimensional images and annotation.  vtkActor2D has a position but does not
// use a transformation matrix like vtkActor (see the superclass vtkProp
// for information on positioning vtkActor2D).  vtkActor2D has a reference to
// a vtkMapper2D object which does the rendering.

// .SECTION See Also
// vtkProp  vtkMapper2D vtkProperty2D

#ifndef __vtkActor2D_h
#define __vtkActor2D_h

#include "vtkProp.h"
#include "vtkCoordinate.h" // For vtkViewportCoordinateMacro

class vtkMapper2D;
class vtkProperty2D;

class VTK_FILTERING_EXPORT vtkActor2D : public vtkProp
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeRevisionMacro(vtkActor2D,vtkProp);

  // Description:
  // Creates an actor2D with the following defaults: 
  // position (0,0) (coordinate system is viewport);
  // at layer 0.
  static vtkActor2D* New();
  
  // Description:
  // Support the standard render methods.
  int RenderOverlay(vtkViewport *viewport);
  int RenderOpaqueGeometry(vtkViewport *viewport);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport *viewport);

  // Description:
  // Does this prop have some translucent polygonal geometry?
  virtual int HasTranslucentPolygonalGeometry();
  
  // Description:
  // Set/Get the vtkMapper2D which defines the data to be drawn.
  virtual void SetMapper(vtkMapper2D *mapper);
  vtkGetObjectMacro(Mapper, vtkMapper2D);

  // Description:
  // Set/Get the layer number in the overlay planes into which to render.
  vtkSetMacro(LayerNumber, int);
  vtkGetMacro(LayerNumber, int);

  // Description:
  // Returns this actor's vtkProperty2D.  Creates a property if one
  // doesn't already exist.
  vtkProperty2D* GetProperty();

  // Description:
  // Set this vtkProp's vtkProperty2D.
  virtual void SetProperty(vtkProperty2D*);

  // Description:
  // Get the PositionCoordinate instance of vtkCoordinate.
  // This is used for for complicated or relative positioning.
  // The position variable controls the lower left corner of the Actor2D
  vtkViewportCoordinateMacro(Position);

  // Description:
  // Set the Prop2D's position in display coordinates.
  void SetDisplayPosition(int,int);

  // Description:
  // Access the Position2 instance variable. This variable controls
  // the upper right corner of the Actor2D. It is by default
  // relative to Position and in normalized viewport coordinates.
  // Some 2D actor subclasses ignore the position2 variable
  vtkViewportCoordinateMacro(Position2);

  // Description:
  // Set/Get the height and width of the Actor2D. The value is expressed
  // as a fraction of the viewport. This really is just another way of
  // setting the Position2 instance variable.
  void SetWidth(double w);
  double GetWidth();
  void SetHeight(double h);
  double GetHeight();

  // Description:
  // Return this objects MTime.
  unsigned long GetMTime();

  // Description:
  // For some exporters and other other operations we must be
  // able to collect all the actors or volumes. These methods
  // are used in that process.
  virtual void GetActors2D(vtkPropCollection *pc);

  // Description:
  // Shallow copy of this vtkActor2D. Overloads the virtual vtkProp method.
  void ShallowCopy(vtkProp *prop);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Return the actual vtkCoordinate reference that the mapper should use
  // to position the actor. This is used internally by the mappers and should
  // be overridden in specialized subclasses and otherwise ignored.
  virtual vtkCoordinate *GetActualPositionCoordinate(void)
    { return this->PositionCoordinate; }

  // Description:
  // Return the actual vtkCoordinate reference that the mapper should use
  // to position the actor. This is used internally by the mappers and should
  // be overridden in specialized subclasses and otherwise ignored.
  virtual vtkCoordinate *GetActualPosition2Coordinate(void)
    { return this->Position2Coordinate; }

protected:
  vtkActor2D();
  ~vtkActor2D();

  vtkMapper2D *Mapper;
  int LayerNumber;
  vtkProperty2D *Property;
  vtkCoordinate *PositionCoordinate;
  vtkCoordinate *Position2Coordinate;

private:
  vtkActor2D(const vtkActor2D&);  // Not implemented.
  void operator=(const vtkActor2D&);  // Not implemented.
};

#endif



