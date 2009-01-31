/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkLinesPainter.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLinesPainter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkObjectFactory.h"
#include "vtkPainterDeviceAdapter.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkUnsignedCharArray.h"

vtkStandardNewMacro(vtkLinesPainter);
vtkCxxRevisionMacro(vtkLinesPainter, "$Revision: 1.3 $");
//-----------------------------------------------------------------------------
vtkLinesPainter::vtkLinesPainter()
{
  this->SetSupportedPrimitive(vtkPainter::LINES | vtkPainter::POLYS);
  this->RenderPolys = 0;
}

//-----------------------------------------------------------------------------
vtkLinesPainter::~vtkLinesPainter()
{
}

//-----------------------------------------------------------------------------
#define vtkDrawPrimsMacro(prim,glVertFuncs,glInitFuncs) \
{ \
  vtkIdType nPts; unsigned short count = 0; \
  glInitFuncs \
  while (ptIds < endPtIds) \
    { \
    nPts = *ptIds; \
    ++ptIds; \
    device->BeginPrimitive(prim);\
    while (nPts > 0) \
      { \
      glVertFuncs \
      ++ptIds; \
      --nPts; \
      } \
    device->EndPrimitive();\
    if (++count == 10000) \
      { \
      cellNum += 10000; \
      count = 0; \
      this->UpdateProgress((double)(cellNum-cellNumStart)/totalCells); \
      if (ren->GetRenderWindow()->CheckAbortStatus()) \
        { \
        break; \
        } \
      } \
    } \
  cellNum += count; \
}
//-----------------------------------------------------------------------------
void vtkLinesPainter::RenderInternal(vtkRenderer* renderer, vtkActor* actor, 
  unsigned long typeflags)
{
  if (typeflags == vtkPainter::POLYS)
    {
    this->RenderPolys = 1;
    }
  else
    {
    this->RenderPolys = 0;
    }
  this->Superclass::RenderInternal(renderer, actor, typeflags);
}

//-----------------------------------------------------------------------------
int vtkLinesPainter::RenderPrimitive(unsigned long idx, vtkDataArray* n,
    vtkUnsignedCharArray* c, vtkDataArray* t, vtkRenderer* ren)
{
  vtkPoints* p = this->PolyData->GetPoints();
  vtkCellArray* ca = (this->RenderPolys)? this->PolyData->GetPolys() :
    this->PolyData->GetLines();
  vtkIdType cellNum = this->PolyData->GetVerts()->GetNumberOfCells();
  vtkIdType cellNumStart = cellNum;
  vtkIdType totalCells = ca->GetNumberOfCells();

  vtkPainterDeviceAdapter* device = ren->GetRenderWindow()->
    GetPainterDeviceAdapter();
  void *points = p->GetVoidPointer(0);
  void *normals = 0;
  void *tcoords = 0;
  unsigned char *colors = 0;
  if (ca->GetNumberOfCells() == 0)
    {
    return 1;
    }
  if (n)
    {
    normals = n->GetVoidPointer(0);
    }
  if (t)
    {
    tcoords = t->GetVoidPointer(0);
    }
  if (c)
    {
    colors = c->GetPointer(0);
    }
  vtkIdType *ptIds = ca->GetPointer();
  vtkIdType *endPtIds = ptIds + ca->GetNumberOfConnectivityEntries();
  int ptype = p->GetDataType();
  int ntype = (n)? n->GetDataType() : 0;
  int ttype = (t)? t->GetDataType() : 0;
  int tcomps = (t)? t->GetNumberOfComponents() : 0;
  int primitive = (this->RenderPolys)? VTK_TETRA : VTK_POLY_LINE;

  // since this painter does not deal with field colors specially,
  // we just ignore the flag.
  idx &= (~VTK_PDM_FIELD_COLORS);

  // draw all the elements, use fast path if available
  switch (idx)
    {
    case 0:
      vtkDrawPrimsMacro(primitive, 
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,;);
      break;
      
    case VTK_PDM_NORMALS:
      vtkDrawPrimsMacro(primitive,
        device->SendAttribute(vtkPointData::NORMALS, 3,
          ntype, normals, 3**ptIds);
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,;);
      break;
      
    case VTK_PDM_COLORS:
      vtkDrawPrimsMacro(primitive,
        device->SendAttribute(vtkPointData::SCALARS, 4,
          VTK_UNSIGNED_CHAR, colors + 4**ptIds);
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,;);
      break;
      
    case VTK_PDM_COLORS | VTK_PDM_OPAQUE_COLORS:
      vtkDrawPrimsMacro(primitive,
        device->SendAttribute(vtkPointData::SCALARS, 3,
          VTK_UNSIGNED_CHAR, colors + 4**ptIds);
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,;);
      break;

    case VTK_PDM_NORMALS | VTK_PDM_COLORS:
      vtkDrawPrimsMacro(primitive,
        device->SendAttribute(vtkPointData::NORMALS, 3,
          ntype, normals, 3**ptIds);
        device->SendAttribute(vtkPointData::SCALARS, 4,
          VTK_UNSIGNED_CHAR, colors + 4**ptIds);
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,;);
    break;
    
    case VTK_PDM_NORMALS | VTK_PDM_COLORS  | VTK_PDM_OPAQUE_COLORS:
      vtkDrawPrimsMacro(primitive,
        device->SendAttribute(vtkPointData::NORMALS, 3,
          ntype, normals, 3**ptIds);
        device->SendAttribute(vtkPointData::SCALARS, 4,
          VTK_UNSIGNED_CHAR, colors + 4**ptIds);
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,;);
    break;

    case VTK_PDM_TCOORDS:
      vtkDrawPrimsMacro(primitive, 
        device->SendAttribute(vtkPointData::TCOORDS, tcomps,
          ttype, tcoords, tcomps**ptIds);
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,;);
    break;

    case VTK_PDM_NORMALS | VTK_PDM_TCOORDS:
      vtkDrawPrimsMacro(primitive, 
        device->SendAttribute(vtkPointData::NORMALS, 3,
          ntype, normals, 3**ptIds);
        device->SendAttribute(vtkPointData::TCOORDS, tcomps,
          ttype, tcoords, tcomps**ptIds);
        device->SendAttribute(vtkPointData::NUM_ATTRIBUTES, 3,
          ptype, points, 3**ptIds);,;);
    break;
    
    default:
      return 0; // let the delegate painter handle this call.
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkLinesPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
