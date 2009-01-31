/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkOpenGLCoincidentTopologyResolutionPainter.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLCoincidentTopologyResolutionPainter.h"

#include "vtkMapper.h" // for VTK_RESOLVE_*
#include "vtkObjectFactory.h"

#ifndef VTK_IMPLEMENT_MESA_CXX
# include "vtkOpenGL.h"
#endif


#ifndef VTK_IMPLEMENT_MESA_CXX
vtkStandardNewMacro(vtkOpenGLCoincidentTopologyResolutionPainter);
vtkCxxRevisionMacro(vtkOpenGLCoincidentTopologyResolutionPainter, 
  "$Revision: 1.5 $");
#endif
//-----------------------------------------------------------------------------
vtkOpenGLCoincidentTopologyResolutionPainter::
vtkOpenGLCoincidentTopologyResolutionPainter()
{
  
}

//-----------------------------------------------------------------------------
vtkOpenGLCoincidentTopologyResolutionPainter::
~vtkOpenGLCoincidentTopologyResolutionPainter()
{
}

//-----------------------------------------------------------------------------
void vtkOpenGLCoincidentTopologyResolutionPainter::RenderInternal(
  vtkRenderer* renderer, vtkActor* actor, unsigned long typeflags)
{
  int resolve=0, zResolve=0;
  double zRes = 0.0;
  if ( this->ResolveCoincidentTopology )
    {
    resolve = 1;
    if ( this->ResolveCoincidentTopology == VTK_RESOLVE_SHIFT_ZBUFFER )
      {
      zResolve = 1;
      zRes = this->ZShift;
      }
    else
      {
#ifdef GL_VERSION_1_1
      if (this->OffsetFaces)
        {
        glEnable(GL_POLYGON_OFFSET_FILL);
        }
      else
        {
        glEnable(GL_POLYGON_OFFSET_LINE);
        glEnable(GL_POLYGON_OFFSET_POINT);
        }
      glPolygonOffset(this->PolygonOffsetFactor, this->PolygonOffsetUnits);
#endif      
      }
    }
 
  if (!zResolve)
    {
    this->Superclass::RenderInternal(renderer, actor, typeflags);
    }
  else
    {
    if (typeflags & vtkPainter::VERTS)
      {
      this->Superclass::RenderInternal(renderer, actor , vtkPainter::VERTS);
      }
    if (typeflags & vtkPainter::LINES || typeflags & vtkPainter::POLYS)
      {
      glDepthRange(zRes, 1.);
      this->Superclass::RenderInternal(renderer, actor, typeflags 
        & (vtkPainter::LINES | vtkPainter::POLYS));
      }
    if (typeflags & vtkPainter::STRIPS)
      {
      glDepthRange(2*zRes, 1.);
      this->Superclass::RenderInternal(renderer, actor , vtkPainter::STRIPS);
      }
    }

  if (resolve)
    {
    if ( zResolve )
      {
      glDepthRange(0., 1.);
      }
    else
      {
#ifdef GL_VERSION_1_1
      if (this->OffsetFaces)
        {
        glDisable(GL_POLYGON_OFFSET_FILL);
        }
      else
        {
        glDisable(GL_POLYGON_OFFSET_LINE);
        glDisable(GL_POLYGON_OFFSET_POINT);
        }
#endif
      }
    }
}


//-----------------------------------------------------------------------------
void vtkOpenGLCoincidentTopologyResolutionPainter::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
