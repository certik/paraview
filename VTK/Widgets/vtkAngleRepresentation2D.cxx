/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkAngleRepresentation2D.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAngleRepresentation2D.h"
#include "vtkPointHandleRepresentation2D.h"
#include "vtkLeaderActor2D.h"
#include "vtkCoordinate.h"
#include "vtkRenderer.h"
#include "vtkObjectFactory.h"
#include "vtkInteractorObserver.h"
#include "vtkMath.h"
#include "vtkWindow.h"

vtkCxxRevisionMacro(vtkAngleRepresentation2D, "$Revision: 1.10 $");
vtkStandardNewMacro(vtkAngleRepresentation2D);


//----------------------------------------------------------------------
vtkAngleRepresentation2D::vtkAngleRepresentation2D()
{
  // By default, use one of these handles
  this->HandleRepresentation  = vtkPointHandleRepresentation2D::New();

  this->Ray1 = vtkLeaderActor2D::New();
  this->Ray1->GetPositionCoordinate()->SetCoordinateSystemToWorld();
  this->Ray1->GetPosition2Coordinate()->SetCoordinateSystemToWorld();
  this->Ray1->SetArrowStyleToOpen();
  this->Ray1->SetArrowPlacementToPoint2();

  this->Ray2 = vtkLeaderActor2D::New();
  this->Ray2->GetPositionCoordinate()->SetCoordinateSystemToWorld();
  this->Ray2->GetPosition2Coordinate()->SetCoordinateSystemToWorld();
  this->Ray2->SetArrowStyleToOpen();
  this->Ray2->SetArrowPlacementToPoint2();

  this->Arc = vtkLeaderActor2D::New();
  this->Arc->GetPositionCoordinate()->SetCoordinateSystemToWorld();
  this->Arc->GetPosition2Coordinate()->SetCoordinateSystemToWorld();
  this->Arc->SetArrowPlacementToNone();
  this->Arc->SetLabel("Angle");
  this->Arc->SetLabelFormat(this->LabelFormat);
//  this->Arc->AutoLabelOn();
}

//----------------------------------------------------------------------
vtkAngleRepresentation2D::~vtkAngleRepresentation2D()
{
  this->Ray1->Delete();
  this->Ray2->Delete();
  this->Arc->Delete();
}

//----------------------------------------------------------------------
double vtkAngleRepresentation2D::GetAngle()
{
  return this->Arc->GetAngle();
}

//----------------------------------------------------------------------
void vtkAngleRepresentation2D::GetPoint1WorldPosition(double pos[3])
{
  this->Point1Representation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
void vtkAngleRepresentation2D::GetCenterWorldPosition(double pos[3])
{
  this->CenterRepresentation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
void vtkAngleRepresentation2D::GetPoint2WorldPosition(double pos[3])
{
  this->Point2Representation->GetWorldPosition(pos);
}

//----------------------------------------------------------------------
void vtkAngleRepresentation2D::SetPoint1DisplayPosition(double x[3])
{
  this->Point1Representation->SetDisplayPosition(x);
  double p[3];
  this->Point1Representation->GetWorldPosition(p);
  this->Point1Representation->SetWorldPosition(p);
  this->Ray1->GetPosition2Coordinate()->SetValue(p);
  this->Modified();
}

//----------------------------------------------------------------------
void vtkAngleRepresentation2D::SetCenterDisplayPosition(double x[3])
{
  this->CenterRepresentation->SetDisplayPosition(x);
  double p[3];
  this->CenterRepresentation->GetWorldPosition(p);
  this->CenterRepresentation->SetWorldPosition(p);
  this->Ray1->GetPositionCoordinate()->SetValue(p);
  this->Ray2->GetPositionCoordinate()->SetValue(p);
  this->Modified();
}

//----------------------------------------------------------------------
void vtkAngleRepresentation2D::SetPoint2DisplayPosition(double x[3])
{
  this->Point2Representation->SetDisplayPosition(x);
  double p[3];
  this->Point2Representation->GetWorldPosition(p);
  this->Point2Representation->SetWorldPosition(p);
  this->Ray2->GetPosition2Coordinate()->SetValue(p);
  this->Modified();
}

//----------------------------------------------------------------------
void vtkAngleRepresentation2D::GetPoint1DisplayPosition(double pos[3])
{
  this->Point1Representation->GetDisplayPosition(pos);
  pos[2] = 0.0;
}

//----------------------------------------------------------------------
void vtkAngleRepresentation2D::GetCenterDisplayPosition(double pos[3])
{
  this->CenterRepresentation->GetDisplayPosition(pos);
  pos[2] = 0.0;
}

//----------------------------------------------------------------------
void vtkAngleRepresentation2D::GetPoint2DisplayPosition(double pos[3])
{
  this->Point2Representation->GetDisplayPosition(pos);
  pos[2] = 0.0;
}

//----------------------------------------------------------------------
void vtkAngleRepresentation2D::BuildRepresentation()
{
  if ( this->GetMTime() > this->BuildTime ||
       this->Point1Representation->GetMTime() > this->BuildTime ||
       this->CenterRepresentation->GetMTime() > this->BuildTime ||
       this->Point2Representation->GetMTime() > this->BuildTime ||
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
    {
    this->Superclass::BuildRepresentation();

    double p1[3], p2[3], c[3];
    this->Point1Representation->GetDisplayPosition(p1);
    this->CenterRepresentation->GetDisplayPosition(c);
    this->Point2Representation->GetDisplayPosition(p2);

    // Compute the angle (only if necessary since we don't want
    // fluctuations in angle value as the camera moves, etc.)
    if ( this->GetMTime() > this->BuildTime )
      {
      if ( p1[0]-c[0] == 0.0 || p2[0]-c[0] == 0.0 )
         {
         return;
         }

      double theta1 = atan2(p1[1]-c[1],p1[0]-c[0]);
      double theta2 = atan2(p2[1]-c[1],p2[0]-c[0]);
      if ( (theta1 >= 0.0 && theta1 <= vtkMath::Pi() &&
            theta2 >= 0.0 && theta2 <= vtkMath::Pi()) ||
           (theta1 <= 0.0 && theta1 >= -vtkMath::Pi() &&
            theta2 <= 0.0 && theta2 >= -vtkMath::Pi()) )
        {
        ; //do nothin angles are fine
        }
      else if ( theta1 >= 0.0 && theta2 <= 0.0 )
        {
        if ( (theta1 - theta2) >= vtkMath::Pi() )
          {
          theta2 = theta2 + 2.0*vtkMath::Pi();
          }
        }
      else //if ( theta1 <= 0.0 && theta2 >= 0.0 )
        {
        if ( (theta2 - theta1) >= vtkMath::Pi() )
          {
          theta1 = theta1 + 2.0*vtkMath::Pi();
          }
        }
      char string[512];
      sprintf(string, this->LabelFormat,
        (theta1-theta2)*vtkMath::RadiansToDegrees());
      this->Arc->SetLabel(string);
      }

    // Place the label and place the arc
    double l1 = sqrt(vtkMath::Distance2BetweenPoints(c,p1));
    double l2 = sqrt(vtkMath::Distance2BetweenPoints(c,p2));

    // If too small or no render get out
    if ( l1 <= 5.0 || l2 <= 5.0 || !this->Renderer )
      {
      this->ArcVisibility = 0;
      return;
      }

    // Place the end points for the arc away from the tip of the two rays
    this->ArcVisibility = 1;
    this->Arc->SetLabelFormat(this->LabelFormat);
    const double rayPosition = 0.80;
    int i;
    double a1[3], a2[3], t1, t2, w1[4], w2[4], radius;
    double ray1[3], ray2[3], v[3], z[3];
    if ( l1 < l2 )
      {
      radius = rayPosition * l1;
      t1 = rayPosition;
      t2 = (l1/l2)*rayPosition;
      }
    else
      {
      radius = rayPosition * l2;
      t1 = (l2/l1)*rayPosition;
      t2 = rayPosition;
      }
    for (i=0; i<3; i++)
      {
      ray1[i] = p1[i]-c[i];
      ray2[i] = p2[i]-c[i];
      a1[i] = c[i] + t1*ray1[i];
      a2[i] = c[i] + t2*ray2[i];
      }
    double l = sqrt(vtkMath::Distance2BetweenPoints(a1,a2));
    vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer,a1[0],a1[1],a1[2],w1);
    vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer,a2[0],a2[1],a2[2],w2);
    this->Arc->GetPositionCoordinate()->SetValue(w1);
    this->Arc->GetPosition2Coordinate()->SetValue(w2);
    if ( l <= 0.0 )
      {
      this->Arc->SetRadius(0.0);
      }
    else
      {
      vtkMath::Cross(ray1,ray2,v);
      z[0] = z[1] = 0.0; z[2] = 1.0;
      if ( vtkMath::Dot(v,z) > 0.0 )
        {
        this->Arc->SetRadius(-radius/l);
        }
      else
        {
        this->Arc->SetRadius(radius/l);
        }
      }
    this->BuildTime.Modified();
    }
}

//----------------------------------------------------------------------
void vtkAngleRepresentation2D::ReleaseGraphicsResources(vtkWindow *w)
{
  this->Ray1->ReleaseGraphicsResources(w);
  this->Ray2->ReleaseGraphicsResources(w);
  this->Arc->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------
int vtkAngleRepresentation2D::RenderOverlay(vtkViewport *v)
{
  this->BuildRepresentation();

  int count=0;
  if ( this->Ray1Visibility )
    {
    count += this->Ray1->RenderOverlay(v);
    }
  if ( this->Ray2Visibility )
    {
    count += this->Ray2->RenderOverlay(v);
    }
  if ( this->ArcVisibility )
    {
    count += this->Arc->RenderOverlay(v);
    }

  return count;
}

//----------------------------------------------------------------------
void vtkAngleRepresentation2D::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
}
