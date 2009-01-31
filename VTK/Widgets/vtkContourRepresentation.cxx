/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkContourRepresentation.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkContourRepresentation.h"
#include "vtkHandleRepresentation.h"
#include "vtkCoordinate.h"
#include "vtkRenderer.h"
#include "vtkObjectFactory.h"
#include "vtkBox.h"
#include "vtkInteractorObserver.h"
#include "vtkMath.h"
#include "vtkPointPlacer.h"
#include "vtkContourLineInterpolator.h"
#include "vtkLine.h"
#include "vtkCamera.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkIntArray.h"

#include <vtkstd/set>
#include <vtkstd/algorithm>
#include <vtkstd/iterator>

vtkCxxRevisionMacro(vtkContourRepresentation, "$Revision: 1.20 $");
vtkCxxSetObjectMacro(vtkContourRepresentation, PointPlacer, vtkPointPlacer);
vtkCxxSetObjectMacro(vtkContourRepresentation, LineInterpolator, vtkContourLineInterpolator);

//----------------------------------------------------------------------
vtkContourRepresentation::vtkContourRepresentation()
{
  this->Internal = new vtkContourRepresentationInternals;
  
  this->PixelTolerance           = 7;
  this->WorldTolerance           = 0.001;
  this->PointPlacer              = NULL;
  this->LineInterpolator         = NULL;
  this->ActiveNode               = -1;
  this->NeedToRender             = 0;
  this->ClosedLoop               = 0;
  this->CurrentOperation         = vtkContourRepresentation::Inactive;
}

//----------------------------------------------------------------------
vtkContourRepresentation::~vtkContourRepresentation()
{
  this->SetPointPlacer(NULL);
  this->SetLineInterpolator(NULL);
  
  for(unsigned int i=0;i<this->Internal->Nodes.size();i++)
    {
    for (unsigned int j=0;j<this->Internal->Nodes[i]->Points.size();j++)
      {
      delete this->Internal->Nodes[i]->Points[j];
      }
    this->Internal->Nodes[i]->Points.clear();
    delete this->Internal->Nodes[i];
    }
  this->Internal->Nodes.clear(); 
  delete this->Internal;
}
  
//----------------------------------------------------------------------
void vtkContourRepresentation::AddNodeAtPositionInternal( double worldPos[3],
                                                          double worldOrient[9],
                                                          double displayPos[2] )
{
  // Add a new point at this position
  vtkContourRepresentationNode *node = new vtkContourRepresentationNode;
  node->WorldPosition[0] = worldPos[0];
  node->WorldPosition[1] = worldPos[1];
  node->WorldPosition[2] = worldPos[2];

  node->NormalizedDisplayPosition[0] = displayPos[0];
  node->NormalizedDisplayPosition[1] = displayPos[1];
  this->Renderer->DisplayToNormalizedDisplay( 
      node->NormalizedDisplayPosition[0],
      node->NormalizedDisplayPosition[1] );
  
  memcpy(node->WorldOrientation, worldOrient, 9*sizeof(double) );
  
  this->Internal->Nodes.push_back(node);
  
  if ( this->LineInterpolator && this->GetNumberOfNodes() > 1 )
    {
    // Give the line interpolator a chance to update the node. 
    this->LineInterpolator->UpdateNode( 
        this->Renderer, this, node->WorldPosition, this->GetNumberOfNodes()-1 );

    // Give the point placer a chance to validate the updated node. If its not
    // valid, discard the LineInterpolator's change.
    if ( !this->PointPlacer->ValidateWorldPosition( 
                node->WorldPosition, worldOrient ) )
      {
      node->WorldPosition[0] = worldPos[0];
      node->WorldPosition[1] = worldPos[1];
      node->WorldPosition[2] = worldPos[2];
      }
    }
  
  this->UpdateLines( this->Internal->Nodes.size()-1);
  this->NeedToRender = 1;
}

//----------------------------------------------------------------------
void vtkContourRepresentation::AddNodeAtPositionInternal( double worldPos[3],
                                                          double worldOrient[9],
                                                          int displayPos[2] )
{
  double dispPos[2];
  dispPos[0] = (double) displayPos[0];
  dispPos[1] = (double) displayPos[1];
  this->AddNodeAtPositionInternal( worldPos, worldOrient, dispPos );
}

//----------------------------------------------------------------------
int vtkContourRepresentation::AddNodeAtWorldPosition( double worldPos[3],
                                                      double worldOrient[9] )
{
  // Check if this is a valid location
  if ( !this->PointPlacer->ValidateWorldPosition( worldPos, worldOrient ) )
    {
    return 0;
    }

  double displayPos[2];
  this->GetRendererComputedDisplayPositionFromWorldPosition( 
                          worldPos, worldOrient, displayPos );
  this->AddNodeAtPositionInternal( worldPos, worldOrient, displayPos );

  return 1;
}

//----------------------------------------------------------------------
int vtkContourRepresentation::AddNodeAtWorldPosition( double worldPos[3] )
{
  // Check if this is a valid location
  if ( !this->PointPlacer->ValidateWorldPosition( worldPos ) )
    {
    return 0;
    }

  double worldOrient[9] = {1.0,0.0,0.0,
                           0.0,1.0,0.0,
                           0.0,0.0,1.0};
  
  double displayPos[2];
  this->GetRendererComputedDisplayPositionFromWorldPosition( 
                          worldPos, worldOrient, displayPos );
  this->AddNodeAtPositionInternal( worldPos, worldOrient, displayPos );

  return 1;
}

//----------------------------------------------------------------------
int vtkContourRepresentation::AddNodeAtDisplayPosition(double displayPos[2])
{
  double worldPos[3];
  double worldOrient[9];
  
  // Compute the world position from the display position
  // based on the concrete representation's constraints
  // If this is not a valid display location return 0
  if ( !this->PointPlacer->ComputeWorldPosition( this->Renderer,
                                                 displayPos, worldPos,
                                                 worldOrient) )
    {
    return 0;
    }
  
  this->AddNodeAtPositionInternal( worldPos, worldOrient, displayPos );  
  return 1;
}
//----------------------------------------------------------------------
int vtkContourRepresentation::AddNodeAtDisplayPosition(int displayPos[2])
{
  double doubleDisplayPos[2];
  doubleDisplayPos[0] = displayPos[0];
  doubleDisplayPos[1] = displayPos[1];
  return this->AddNodeAtDisplayPosition( doubleDisplayPos );
  
}
//----------------------------------------------------------------------
int vtkContourRepresentation::AddNodeAtDisplayPosition(int X, int Y)
{
  double displayPos[2];
  displayPos[0] = X;
  displayPos[1] = Y;
  return this->AddNodeAtDisplayPosition( displayPos );
  
}

//----------------------------------------------------------------------
int vtkContourRepresentation::ActivateNode( double displayPos[2] )
{
  // Find closest node to this display pos that
  // is within PixelTolerance
  
  int closestNode = -1;
  double closestDistance2 = VTK_DOUBLE_MAX;
  
  unsigned int i;
  double limit = this->PixelTolerance * this->PixelTolerance;
  
  for ( i = 0; i < this->Internal->Nodes.size(); i++ )
    {
    double currDisplayPos[2];
    this->GetNthNodeDisplayPosition( i, currDisplayPos );
    
    double currDistance2 =
      (currDisplayPos[0] - displayPos[0])*
      (currDisplayPos[0] - displayPos[0]) +
      (currDisplayPos[1] - displayPos[1])*
      (currDisplayPos[1] - displayPos[1]);
    
    if ( currDistance2 < limit &&
         currDistance2 < closestDistance2 )
      {
      closestNode = i;
      closestDistance2 = currDistance2;
      }
    }

  if ( closestNode != this->ActiveNode )
    {
    this->ActiveNode = closestNode;
    this->NeedToRender = 1;
    }

  return ( this->ActiveNode >= 0 );
}
//----------------------------------------------------------------------
int vtkContourRepresentation::ActivateNode( int displayPos[2] )
{
  double doubleDisplayPos[2];
  
  doubleDisplayPos[0] = displayPos[0];
  doubleDisplayPos[1] = displayPos[1];
  return this->ActivateNode( doubleDisplayPos );
}

//----------------------------------------------------------------------
int vtkContourRepresentation::ActivateNode( int X, int Y )
{
  double doubleDisplayPos[2];
  
  doubleDisplayPos[0] = X;
  doubleDisplayPos[1] = Y;
  return this->ActivateNode( doubleDisplayPos );
}


//----------------------------------------------------------------------
int vtkContourRepresentation::SetActiveNodeToWorldPosition( double worldPos[3],
                                                            double worldOrient[9] )
{
  if ( this->ActiveNode < 0 ||
       static_cast<unsigned int>(this->ActiveNode) >= this->Internal->Nodes.size() )
    {
    return 0;
    }
  
  // Check if this is a valid location
  if ( !this->PointPlacer->ValidateWorldPosition( worldPos, worldOrient ) )
    {
    return 0;
    }
  
  this->SetNthNodeWorldPositionInternal( this->ActiveNode, 
                                         worldPos, 
                                         worldOrient );
  return 1;
}

//----------------------------------------------------------------------
int vtkContourRepresentation::SetActiveNodeToWorldPosition( double worldPos[3] )
{
  if ( this->ActiveNode < 0 ||
       static_cast<unsigned int>(this->ActiveNode) >= this->Internal->Nodes.size() )
    {
    return 0;
    }
  
  // Check if this is a valid location
  if ( !this->PointPlacer->ValidateWorldPosition( worldPos ) )
    {
    return 0;
    }
  
  double worldOrient[9] = {1.0,0.0,0.0,
                           0.0,1.0,0.0,
                           0.0,0.0,1.0};
  
  this->SetNthNodeWorldPositionInternal( this->ActiveNode, 
                                         worldPos,
                                         worldOrient );
  return 1;
}

//----------------------------------------------------------------------
int vtkContourRepresentation::SetActiveNodeToDisplayPosition( double displayPos[2] )
{
  if ( this->ActiveNode < 0 ||
       static_cast<unsigned int>(this->ActiveNode) >= this->Internal->Nodes.size() )
    {
    return 0;
    }
  
  double worldPos[3];
  double worldOrient[9];
  
  // Compute the world position from the display position
  // based on the concrete representation's constraints
  // If this is not a valid display location return 0
  if ( !this->PointPlacer->ComputeWorldPosition( this->Renderer,
                                                 displayPos, worldPos,
                                                 worldOrient ) )
    {
    return 0;
    }
  
  this->SetNthNodeWorldPositionInternal( this->ActiveNode, 
                                         worldPos,
                                         worldOrient );
  return 1;
}
//----------------------------------------------------------------------
int vtkContourRepresentation::SetActiveNodeToDisplayPosition( int displayPos[2] )
{
  double doubleDisplayPos[2];
  doubleDisplayPos[0] = displayPos[0];
  doubleDisplayPos[1] = displayPos[1];
  return this->SetActiveNodeToDisplayPosition( doubleDisplayPos );
}

//----------------------------------------------------------------------
int vtkContourRepresentation::SetActiveNodeToDisplayPosition( int X, int Y )
{
  double doubleDisplayPos[2];
  doubleDisplayPos[0] = X;
  doubleDisplayPos[1] = Y;
  return this->SetActiveNodeToDisplayPosition( doubleDisplayPos );
}

//----------------------------------------------------------------------
int vtkContourRepresentation::GetActiveNodeWorldPosition( double pos[3] )
{
  return this->GetNthNodeWorldPosition( this->ActiveNode, pos );
}

//----------------------------------------------------------------------
int vtkContourRepresentation::GetActiveNodeWorldOrientation( double orient[9] )
{
  return this->GetNthNodeWorldOrientation( this->ActiveNode, orient );
}

//----------------------------------------------------------------------
int vtkContourRepresentation::GetActiveNodeDisplayPosition( double pos[2] )
{
  return this->GetNthNodeDisplayPosition( this->ActiveNode, pos );
}

//----------------------------------------------------------------------
int vtkContourRepresentation::GetNumberOfNodes()
{
  return this->Internal->Nodes.size();
}

//----------------------------------------------------------------------
int vtkContourRepresentation::GetNumberOfIntermediatePoints(int n)
{
  if ( n < 0 ||
       static_cast<unsigned int>(n) >= this->Internal->Nodes.size() )
    {
    return 0;
    }
  
  return this->Internal->Nodes[n]->Points.size();
}

//----------------------------------------------------------------------
int vtkContourRepresentation::GetIntermediatePointWorldPosition(int n, 
                                                                int idx, 
                                                                double point[3])
{
  if ( n < 0 ||
       static_cast<unsigned int>(n) >= this->Internal->Nodes.size() )
    {
    return 0;
    }
  
  if ( idx < 0 ||
       static_cast<unsigned int>(idx) >= this->Internal->Nodes[n]->Points.size() )
    {
    return 0;
    }
  
  point[0] = this->Internal->Nodes[n]->Points[idx]->WorldPosition[0];
  point[1] = this->Internal->Nodes[n]->Points[idx]->WorldPosition[1];
  point[2] = this->Internal->Nodes[n]->Points[idx]->WorldPosition[2];
  
  return 1;
}

//----------------------------------------------------------------------
// The display position for a given world position must be re-computed
// from the world positions... It should not be queried from the renderer
// whose camera position may have changed
int vtkContourRepresentation::GetNthNodeDisplayPosition( 
                           int n, double displayPos[2] )
{
  if ( n < 0 ||
       static_cast<unsigned int>(n) >= this->Internal->Nodes.size() )
    {
    return 0;
    }
  
  double pos[4];
  pos[0] = this->Internal->Nodes[n]->WorldPosition[0];
  pos[1] = this->Internal->Nodes[n]->WorldPosition[1];
  pos[2] = this->Internal->Nodes[n]->WorldPosition[2];
  pos[3] = 1.0;
  
  this->Renderer->SetWorldPoint( pos );
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint( pos );
  
  displayPos[0] = pos[0];
  displayPos[1] = pos[1];
  return 1;
}

//----------------------------------------------------------------------
int vtkContourRepresentation::GetNthNodeWorldPosition( int n, double worldPos[3] )
{
  if ( n < 0 ||
       static_cast<unsigned int>(n) >= this->Internal->Nodes.size() )
    {
    return 0;
    }
  
  worldPos[0] = this->Internal->Nodes[n]->WorldPosition[0];
  worldPos[1] = this->Internal->Nodes[n]->WorldPosition[1];
  worldPos[2] = this->Internal->Nodes[n]->WorldPosition[2];
  return 1;
}

//----------------------------------------------------------------------
int vtkContourRepresentation::GetNthNodeWorldOrientation( int n, double worldOrient[9] )
{
  if ( n < 0 ||
       static_cast<unsigned int>(n) >= this->Internal->Nodes.size() )
    {
    return 0;
    }
 
  memcpy( worldOrient, this->Internal->Nodes[n]->WorldOrientation, 9*sizeof(double) );
  return 1;
}

//----------------------------------------------------------------------
void vtkContourRepresentation::SetNthNodeWorldPositionInternal( int n, double worldPos[3],
                                                                double worldOrient[9] )
{
  this->Internal->Nodes[n]->WorldPosition[0] = worldPos[0];
  this->Internal->Nodes[n]->WorldPosition[1] = worldPos[1];
  this->Internal->Nodes[n]->WorldPosition[2] = worldPos[2];
  
  this->GetRendererComputedDisplayPositionFromWorldPosition( 
        worldPos, worldOrient, this->Internal->Nodes[n]->NormalizedDisplayPosition );
  this->Renderer->DisplayToNormalizedDisplay( 
      this->Internal->Nodes[n]->NormalizedDisplayPosition[0],
      this->Internal->Nodes[n]->NormalizedDisplayPosition[1] );

  memcpy(this->Internal->Nodes[n]->WorldOrientation, worldOrient, 9*sizeof(double) );

  this->UpdateLines( n );
  this->NeedToRender = 1;
}

//----------------------------------------------------------------------
int vtkContourRepresentation::SetNthNodeWorldPosition( int n, double worldPos[3],
                                                       double worldOrient[9] )
{
  if ( n < 0 ||
       static_cast<unsigned int>(n) >= this->Internal->Nodes.size() )
    {
    return 0;
    }
  
  // Check if this is a valid location
  if ( !this->PointPlacer->ValidateWorldPosition( worldPos, worldOrient ) )
    {
    return 0;
    }

  this->SetNthNodeWorldPositionInternal( n, worldPos, worldOrient );
  return 1;
}

//----------------------------------------------------------------------
int vtkContourRepresentation::SetNthNodeWorldPosition( int n, double worldPos[3] )
{
  if ( n < 0 ||
       static_cast<unsigned int>(n) >= this->Internal->Nodes.size() )
    {
    return 0;
    }
  
  // Check if this is a valid location
  if ( !this->PointPlacer->ValidateWorldPosition( worldPos ) )
    {
    return 0;
    }
  
  double worldOrient[9] = {1.0,0.0,0.0,
                           0.0,1.0,0.0,
                           0.0,0.0,1.0};

  this->SetNthNodeWorldPositionInternal( n, worldPos, worldOrient );
  return 1;
}

//----------------------------------------------------------------------
int vtkContourRepresentation::SetNthNodeDisplayPosition( int n, double displayPos[2] )
{
  double worldPos[3];
  double worldOrient[9];
  
  // Compute the world position from the display position
  // based on the concrete representation's constraints
  // If this is not a valid display location return 0
  if ( !this->PointPlacer->ComputeWorldPosition( this->Renderer,
                                                 displayPos, worldPos,
                                                 worldOrient) )
    {
    return 0;
    }
  
  return this->SetNthNodeWorldPosition( n, worldPos, worldOrient );
}

//----------------------------------------------------------------------
int vtkContourRepresentation::SetNthNodeDisplayPosition( int n, int displayPos[2] )
{
  double doubleDisplayPos[2];
  doubleDisplayPos[0] = displayPos[0];
  doubleDisplayPos[1] = displayPos[1];
  return this->SetNthNodeDisplayPosition( n, doubleDisplayPos );
}

//----------------------------------------------------------------------
int vtkContourRepresentation::SetNthNodeDisplayPosition( int n, int X, int Y )
{
  double doubleDisplayPos[2];
  doubleDisplayPos[0] = X;
  doubleDisplayPos[1] = Y;
  return this->SetNthNodeDisplayPosition( n, doubleDisplayPos );
}

//----------------------------------------------------------------------
int vtkContourRepresentation::FindClosestPointOnContour( int X, int Y, 
                                                         double closestWorldPos[3], 
                                                         int *idx )
{
  // Make a line out of this viewing ray
  double p1[4], p2[4], *p3=NULL, *p4=NULL;
  
  double tmp1[4], tmp2[4];
  tmp1[0] = X;
  tmp1[1] = Y;
  tmp1[2] = 0.0;
  this->Renderer->SetDisplayPoint( tmp1 );
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(p1);
  
  tmp1[2] = 1.0;
  this->Renderer->SetDisplayPoint( tmp1 );
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(p2);

  double closestDistance2 = VTK_DOUBLE_MAX;
  int closestNode=0;

  // compute a world tolerance based on pixel
  // tolerance on the focal plane
  double fp[4];
  this->Renderer->GetActiveCamera()->GetFocalPoint(fp);
  fp[3] = 1.0;
  this->Renderer->SetWorldPoint(fp);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(tmp1);
  
  tmp1[0] = 0;
  tmp1[1] = 0;
  this->Renderer->SetDisplayPoint(tmp1);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(tmp2);
  
  tmp1[0] = this->PixelTolerance;
  this->Renderer->SetDisplayPoint(tmp1);
  this->Renderer->DisplayToWorld();
  this->Renderer->GetWorldPoint(tmp1);
  
  double wt2 = vtkMath::Distance2BetweenPoints(tmp1, tmp2);
  
  // Now loop through all lines and look for closest one within tolerance  
  for(unsigned int i=0;i<this->Internal->Nodes.size();i++)
    {
    for (unsigned int j=0;j<=this->Internal->Nodes[i]->Points.size();j++)
      {
      if ( j == 0 )
        {
        p3 = this->Internal->Nodes[i]->WorldPosition;
        if ( this->Internal->Nodes[i]->Points.size() )
          {
          p4 = this->Internal->Nodes[i]->Points[j]->WorldPosition;
          }
        else
          {
          if ( i < this->Internal->Nodes.size() - 1 )
            {
            p4 = this->Internal->Nodes[i+1]->WorldPosition;
            }
          else if ( this->ClosedLoop )
            {
            p4 = this->Internal->Nodes[0]->WorldPosition;         
            }
          }
        }
      else if ( j == this->Internal->Nodes[i]->Points.size() )
        {
        p3 = this->Internal->Nodes[i]->Points[j-1]->WorldPosition;
        if ( i < this->Internal->Nodes.size() - 1 )
          {
          p4 = this->Internal->Nodes[i+1]->WorldPosition;
          }
        else if ( this->ClosedLoop )
          {
          p4 = this->Internal->Nodes[0]->WorldPosition;         
          }
        else
          {
          // Shouldn't be able to get here (only if we don't have
          // a closed loop but we do have intermediate points after
          // the last node - contradictary conditions)
          continue;
          }
        }
      else
        {
        p3 = this->Internal->Nodes[i]->Points[j-1]->WorldPosition;
        p4 = this->Internal->Nodes[i]->Points[j]->WorldPosition;
        }
      
      // Now we have the four points - check closest intersection
      double u, v;
      
      if ( vtkLine::Intersection( p1, p2, p3, p4, u, v ) )
        {
        double p5[3], p6[3];
        p5[0] = p1[0] + u*(p2[0]-p1[0]);
        p5[1] = p1[1] + u*(p2[1]-p1[1]);
        p5[2] = p1[2] + u*(p2[2]-p1[2]);

        p6[0] = p3[0] + v*(p4[0]-p3[0]);
        p6[1] = p3[1] + v*(p4[1]-p3[1]);
        p6[2] = p3[2] + v*(p4[2]-p3[2]);
        
        double d = vtkMath::Distance2BetweenPoints(p5, p6);
        
        if ( d < wt2 && d < closestDistance2 )
          {
          closestWorldPos[0] = p6[0];
          closestWorldPos[1] = p6[1];
          closestWorldPos[2] = p6[2];
          closestDistance2 = d;
          closestNode = (int)i;
          }
        }
      else
        {
        double d = vtkLine::DistanceToLine( p3, p1, p2 );
        if ( d < wt2 && d < closestDistance2 )
          {
          closestWorldPos[0] = p3[0];
          closestWorldPos[1] = p3[1];
          closestWorldPos[2] = p3[2];
          closestDistance2 = d;
          closestNode = (int)i;
          }
        
        d = vtkLine::DistanceToLine( p4, p1, p2 );
        if ( d < wt2 && d < closestDistance2 )
          {
          closestWorldPos[0] = p4[0];
          closestWorldPos[1] = p4[1];
          closestWorldPos[2] = p4[2];
          closestDistance2 = d;
          closestNode = (int)i;
          }        
        }
      }
    }

  if ( closestDistance2 < VTK_DOUBLE_MAX )
    {
    if ( closestNode < this->GetNumberOfNodes() -1 )
      {
      *idx = closestNode+1;
      return 1;
      }
    else if ( this->ClosedLoop )
      {
      *idx = 0;
      return 1;
      }
    }
  
  return 0;
}

//----------------------------------------------------------------------
int vtkContourRepresentation::AddNodeOnContour( int X, int Y )
{
  int idx;
  
  double worldPos[3];
  double worldOrient[9];
  
  // Compute the world position from the display position
  // based on the concrete representation's constraints
  // If this is not a valid display location return 0
  double displayPos[2];
  displayPos[0] = X;
  displayPos[1] = Y;
  if ( !this->PointPlacer->ComputeWorldPosition( this->Renderer,
                                                 displayPos, worldPos,
                                                 worldOrient) )
    {
    return 0;
    }
  
  double pos[3];
  if ( !this->FindClosestPointOnContour( X, Y, pos, &idx ) )
    {
    return 0;
    }
  
  if ( !this->PointPlacer->ComputeWorldPosition( this->Renderer,
                                                 displayPos, 
                                                 pos,
                                                 worldPos,
                                                 worldOrient) )
    {
    return 0;
    }
  
  // Add a new point at this position
  vtkContourRepresentationNode *node = new vtkContourRepresentationNode;
  node->WorldPosition[0] = worldPos[0];
  node->WorldPosition[1] = worldPos[1];
  node->WorldPosition[2] = worldPos[2];

  
  this->GetRendererComputedDisplayPositionFromWorldPosition( 
          worldPos, worldOrient, node->NormalizedDisplayPosition );
  this->Renderer->DisplayToNormalizedDisplay( 
         node->NormalizedDisplayPosition[0],
         node->NormalizedDisplayPosition[1] );
  
  memcpy(node->WorldOrientation, worldOrient, 9*sizeof(double) );
  
  this->Internal->Nodes.insert(this->Internal->Nodes.begin() + idx, node);
  
  this->UpdateLines( idx );
  this->NeedToRender = 1;

  return 1;
}

//----------------------------------------------------------------------
int vtkContourRepresentation::DeleteNthNode( int n )
{
  if ( n < 0 ||
       static_cast<unsigned int>(n) >= this->Internal->Nodes.size() )
    {
    return 0;
    }
  
  for (unsigned int j=0;j<this->Internal->Nodes[n]->Points.size();j++)
    {
    delete this->Internal->Nodes[n]->Points[j];
    }
  this->Internal->Nodes[n]->Points.clear();
  delete this->Internal->Nodes[n];
  this->Internal->Nodes.erase( this->Internal->Nodes.begin() + n );
  if ( n )
    {
    this->UpdateLines(n-1);
    }
  else
    {
    this->UpdateLines(this->GetNumberOfNodes()-1);
    }
  
  this->NeedToRender = 1;
  return 1;
}

//----------------------------------------------------------------------
int vtkContourRepresentation::DeleteActiveNode()
{
  return this->DeleteNthNode( this->ActiveNode );
}

//----------------------------------------------------------------------
int vtkContourRepresentation::DeleteLastNode()
{
  return this->DeleteNthNode( this->Internal->Nodes.size() - 1 );
}

//----------------------------------------------------------------------
void vtkContourRepresentation::SetClosedLoop( int val )
{
  if ( this->ClosedLoop != val )
    {
    this->ClosedLoop = val;
    this->UpdateLines(this->GetNumberOfNodes()-1);
    this->Modified();
    }
}

//----------------------------------------------------------------------
void vtkContourRepresentation::UpdateLines( int index )
{
  int indices[2];

  if (this->LineInterpolator)
    {
    vtkIntArray *arr = vtkIntArray::New();
    this->LineInterpolator->GetSpan( index, arr, this );

    int nNodes = arr->GetNumberOfTuples();
    for (int i = 0; i < nNodes; i++)
      {
      arr->GetTupleValue( i, indices );
      this->UpdateLine( indices[0], indices[1] );
      }
    arr->Delete();
    }
  
  // A check to make sure that we have no line segments in
  // the last node if the loop is not closed
  if ( !this->ClosedLoop && this->GetNumberOfNodes() > 0 )
    {
    int idx = this->Internal->Nodes.size() -1;
    for (unsigned int j=0;j<this->Internal->Nodes[idx]->Points.size();j++)
      {
      delete this->Internal->Nodes[idx]->Points[j];
      }
    this->Internal->Nodes[idx]->Points.clear();
    }
  
  this->BuildLines();
}

//----------------------------------------------------------------------
int vtkContourRepresentation::AddIntermediatePointWorldPosition( int n,
                                                                 double pos[3] )
{
  if ( n < 0 ||
       static_cast<unsigned int>(n) >= this->Internal->Nodes.size() )
    {
    return 0;
    }
  
  vtkContourRepresentationPoint *point = new vtkContourRepresentationPoint;
  point->WorldPosition[0] = pos[0];
  point->WorldPosition[1] = pos[1];
  point->WorldPosition[2] = pos[2];
  
  double worldOrient[9] = {1.0,0.0,0.0,
                           0.0,1.0,0.0,
                           0.0,0.0,1.0};
  
  this->GetRendererComputedDisplayPositionFromWorldPosition( 
                          pos, worldOrient, point->NormalizedDisplayPosition );
  this->Renderer->DisplayToNormalizedDisplay( 
      point->NormalizedDisplayPosition[0], 
      point->NormalizedDisplayPosition[1] );
  
  this->Internal->Nodes[n]->Points.push_back(point);
  return 1;
}

//----------------------------------------------------------------------
int vtkContourRepresentation::GetNthNodeSlope( int n, double slope[3] )
{
  if ( n < 0 ||
       static_cast<unsigned int>(n) >= this->Internal->Nodes.size() )
    {
    return 0;
    }

  int idx1, idx2;
  
  if ( n == 0 && !this->ClosedLoop )
    {
    idx1 = 0;
    idx2 = 1;
    }
  else if ( n == this->GetNumberOfNodes()-1 && !this->ClosedLoop )
    {
    idx1 = this->GetNumberOfNodes()-2;
    idx2 = idx1+1;
    }
  else
    {
    idx1 = n - 1;
    idx2 = n + 1;
    
    if ( idx1 < 0 )
      {
      idx1 += this->GetNumberOfNodes();
      }
    if ( idx2 >= this->GetNumberOfNodes() )
      {
      idx2 -= this->GetNumberOfNodes();
      }
    }
  
  slope[0] = 
    this->Internal->Nodes[idx2]->WorldPosition[0] - 
    this->Internal->Nodes[idx1]->WorldPosition[0];
  slope[1] = 
    this->Internal->Nodes[idx2]->WorldPosition[1] - 
    this->Internal->Nodes[idx1]->WorldPosition[1];
  slope[2] = 
    this->Internal->Nodes[idx2]->WorldPosition[2] - 
    this->Internal->Nodes[idx1]->WorldPosition[2];

  vtkMath::Normalize( slope );
  return 1;
}

//----------------------------------------------------------------------
void vtkContourRepresentation::UpdateLine( int idx1, int idx2 )
{
  if ( !this->LineInterpolator )
    {
    return;
    }
  
  // Clear all the points at idx1
  for (unsigned int j=0;j<this->Internal->Nodes[idx1]->Points.size();j++)
    {
    delete this->Internal->Nodes[idx1]->Points[j];
    }
  this->Internal->Nodes[idx1]->Points.clear();
  
  this->LineInterpolator->InterpolateLine( this->Renderer, 
                                           this,
                                           idx1, idx2 );
}

//----------------------------------------------------------------------
int vtkContourRepresentation::ComputeInteractionState(int vtkNotUsed(X), int vtkNotUsed(Y), int vtkNotUsed(modified))
{
  return this->InteractionState;
}

//---------------------------------------------------------------------
int vtkContourRepresentation::UpdateContour()
{
  this->PointPlacer->UpdateInternalState();
  
  if ( this->ContourBuildTime > this->PointPlacer->GetMTime() )
    {
    // Contour does not need to be rebuilt
    return 0;
    }
  
  unsigned int i;
  for(i=0; i<this->Internal->Nodes.size(); i++)
    {
    this->PointPlacer->
      UpdateWorldPosition( this->Renderer,
                           this->Internal->Nodes[i]->WorldPosition,                               
                           this->Internal->Nodes[i]->WorldOrientation );
    }
  
  for(i=0; (i+1)<this->Internal->Nodes.size(); i++)
    {
    this->UpdateLine(i, i+1);
    }
  
  if ( this->ClosedLoop )
    {
    this->UpdateLine( this->Internal->Nodes.size()-1, 0);
    }
  this->BuildLines();
   
  this->ContourBuildTime.Modified();
  
  return 1;
}

//----------------------------------------------------------------------
void vtkContourRepresentation
::GetRendererComputedDisplayPositionFromWorldPosition( double worldPos[3],
                                double worldOrient[9], int displayPos[2] )
{
  double dispPos[2]; 
  dispPos[0] = (double) displayPos[0]; 
  dispPos[1] = (double) displayPos[1];
  this->GetRendererComputedDisplayPositionFromWorldPosition( worldPos, 
                                                worldOrient, dispPos );
  displayPos[0] = (int) dispPos[0];
  displayPos[1] = (int) dispPos[1];
}

//----------------------------------------------------------------------
void vtkContourRepresentation
::GetRendererComputedDisplayPositionFromWorldPosition( double worldPos[3],
                                double * vtkNotUsed(worldOrient[9]), double displayPos[2] )
{
  double pos[4];
  pos[0] = worldPos[0];
  pos[1] = worldPos[1];
  pos[2] = worldPos[2];
  pos[3] = 1.0;
  
  this->Renderer->SetWorldPoint( pos );
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint( pos );
  
  displayPos[0] = pos[0];
  displayPos[1] = pos[1];
}

//----------------------------------------------------------------------
void vtkContourRepresentation::Initialize( vtkPolyData * pd )
{
  vtkPoints *points   = pd->GetPoints();
  vtkIdType nPoints = points->GetNumberOfPoints();
  if (nPoints <= 0)
    {
    return; // Yeah right.. build from nothing !
    }

  // Clear all existing nodes.
  for(unsigned int i=0;i<this->Internal->Nodes.size();i++)
    {
    for (unsigned int j=0;j<this->Internal->Nodes[i]->Points.size();j++)
      {
      delete this->Internal->Nodes[i]->Points[j];
      }
    this->Internal->Nodes[i]->Points.clear();
    delete this->Internal->Nodes[i];
    }
  this->Internal->Nodes.clear();

  vtkIdList *pointIds = pd->GetCell(0)->GetPointIds();

  // Get the worldOrient from the point placer
  double ref[3], displayPos[2], worldPos[3], worldOrient[9];
  ref[0] = 0.0; ref[1] = 0.0; ref[2] = 0.0;
  displayPos[0] = 0.0; displayPos[1] = 0.0;
  this->PointPlacer->ComputeWorldPosition(this->Renderer,
                                 displayPos, ref, worldPos, worldOrient );

  // Add nodes

  for ( vtkIdType i=0; i < nPoints; i++ )
    {
    double *p = points->GetPoint( i );
    this->AddNodeAtWorldPosition( p, worldOrient );
    }

  if ( pointIds->GetNumberOfIds() > nPoints )
    {
    this->ClosedLoopOn();
    }

  // Update the contour representation from the nodes using the line interpolator
  this->BuildRepresentation();

  // Show the contour.
  this->VisibilityOn();
}


//----------------------------------------------------------------------
void vtkContourRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "Pixel Tolerance: " << this->PixelTolerance <<"\n";
  os << indent << "World Tolerance: " << this->WorldTolerance <<"\n";
}

