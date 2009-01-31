/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkOpenGLPainterDeviceAdapter.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkOpenGLPainterDeviceAdapter.h"

#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkgl.h"

#include <vtkstd/algorithm>

#ifndef VTK_IMPLEMENT_MESA_CXX
#  include "vtkOpenGL.h"
#endif

#ifndef VTK_IMPLEMENT_MESA_CXX
vtkCxxRevisionMacro(vtkOpenGLPainterDeviceAdapter, "$Revision: 1.19 $");
vtkStandardNewMacro(vtkOpenGLPainterDeviceAdapter);
#endif
//-----------------------------------------------------------------------------
vtkOpenGLPainterDeviceAdapter::vtkOpenGLPainterDeviceAdapter()
{
  this->PointSize = 1.0;
  this->RangeNear = 0.0;
  this->RangeFar = 1.0;
  this->MaxStencil = 0;
}

//-----------------------------------------------------------------------------
vtkOpenGLPainterDeviceAdapter::~vtkOpenGLPainterDeviceAdapter()
{
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------

// This array is a map from VTK primitive identifiers (VTK_VERTEX, VTK_LINE,
// etc.) to OpenGL polygon primitive identifiers (GL_POINTS, GL_LINES, etc.)
// Note that some VTK polygon types (e.g. VTK_EMPTY_CELL and VTK_PIXEL) have no
// analogue in OpenGL.  Using them will undoubtedly result in an OpenGL error.
static const GLenum VTK2OpenGLPrimitive[] = {
  (GLenum)0xFFFF,       // 0 - VTK_EMPTY_CELL
  GL_POINTS,            // 1 - VTK_VERTEX
  GL_POINTS,            // 2 - VTK_POLY_VERTEX
  GL_LINES,             // 3 - VTK_LINE
  GL_LINE_STRIP,        // 4 - VTK_POLY_LINE
  GL_TRIANGLES,         // 5 - VTK_TRIANGLE
  GL_TRIANGLE_STRIP,    // 6 - VTK_TRIANGLE_STRIP
  GL_POLYGON,           // 7 - VTK_POLYGON
  (GLenum)0xFFFF,       // 8 - VTK_PIXEL
  GL_QUADS,             // 9 - VTK_QUAD
  GL_LINE_LOOP          // 10 - VTK_TETRA
};

static inline GLenum VTK2OpenGLType(int type)
{
  switch (type)
    {
#if VTK_SIZEOF_CHAR == 1
    case VTK_CHAR:              return GL_BYTE;
    case VTK_UNSIGNED_CHAR:     return GL_UNSIGNED_BYTE;
#elif VTK_SIZE_OF_CHAR == 2
    case VTK_CHAR:              return GL_SHORT;
    case VTK_UNSIGNED CHAR:     return GL_UNSIGNED_SHORT;
#endif

#if VTK_SIZEOF_SHORT == 1
    case VTK_SHORT:             return GL_BYTE;
    case VTK_UNSIGNED_SHORT:    return GL_UNSIGNED_BYTE;
#elif VTK_SIZEOF_SHORT == 2
    case VTK_SHORT:             return GL_SHORT;
    case VTK_UNSIGNED_SHORT:    return GL_UNSIGNED_SHORT;
#elif VTK_SIZEOF_SHORT == 4
    case VTK_SHORT:             return GL_INT;
    case VTK_UNSIGNED_SHORT:    return GL_UNSIGNED_INT;
#endif

#if VTK_SIZEOF_INT == 2
    case VTK_INT:               return GL_SHORT;
    case VTK_UNSIGNED_INT:      return GL_UNSIGNED_SHORT;
#elif VTK_SIZEOF_INT == 4
    case VTK_INT:               return GL_INT;
    case VTK_UNSIGNED_INT:      return GL_UNSIGNED_INT;
#endif

#if VTK_SIZEOF_LONG == 4
    case VTK_LONG:              return GL_INT;
    case VTK_UNSIGNED_LONG:     return GL_UNSIGNED_INT;
#endif

#if VTK_SIZEOF_ID_TYPE == 4
    case VTK_ID_TYPE:           return GL_INT;
#endif

#if VTK_SIZEOF_FLOAT == 4
    case VTK_FLOAT:             return GL_FLOAT;
#elif VTK_SIZEOF_FLOAT == 8
    case VTK_FLOAT:             return GL_DOUBLE;
#endif

#if VTK_SIZEOF_DOUBLE == 4
    case VTK_DOUBLE:            return GL_FLOAT;
#elif VTK_SIZEOF_DOUBLE == 8
    case VTK_DOUBLE:            return GL_DOUBLE;
#endif

    default:                    return GL_FALSE;
    }
}

static inline GLenum VTK2SignedOpenGLType(int type)
{
  switch (type)
    {
#if VTK_SIZEOF_CHAR == 1
    case VTK_CHAR:              return GL_BYTE;
    case VTK_UNSIGNED_CHAR:     return GL_BYTE;
#elif VTK_SIZE_OF_CHAR == 2
    case VTK_CHAR:              return GL_SHORT;
    case VTK_UNSIGNED CHAR:     return GL_SHORT;
#endif

#if VTK_SIZEOF_SHORT == 1
    case VTK_SHORT:             return GL_BYTE;
    case VTK_UNSIGNED_SHORT:    return GL_BYTE;
#elif VTK_SIZEOF_SHORT == 2
    case VTK_SHORT:             return GL_SHORT;
    case VTK_UNSIGNED_SHORT:    return GL_SHORT;
#elif VTK_SIZEOF_SHORT == 4
    case VTK_SHORT:             return GL_INT;
    case VTK_UNSIGNED_SHORT:    return GL_INT;
#endif

#if VTK_SIZEOF_INT == 2
    case VTK_INT:               return GL_SHORT;
    case VTK_UNSIGNED_INT:      return GL_SHORT;
#elif VTK_SIZEOF_INT == 4
    case VTK_INT:               return GL_INT;
    case VTK_UNSIGNED_INT:      return GL_INT;
#endif

#if VTK_SIZEOF_ID_TYPE == 4
    case VTK_ID_TYPE:           return GL_INT;
#endif

#if VTK_SIZEOF_LONG == 4
    case VTK_LONG:              return GL_INT;
    case VTK_UNSIGNED_LONG:     return GL_INT;
#endif

#if VTK_SIZEOF_FLOAT == 4
    case VTK_FLOAT:             return GL_FLOAT;
#elif VTK_SIZEOF_FLOAT == 8
    case VTK_FLOAT:             return GL_DOUBLE;
#endif

#if VTK_SIZEOF_DOUBLE == 4
    case VTK_DOUBLE:            return GL_FLOAT;
#elif VTK_SIZEOF_DOUBLE == 8
    case VTK_DOUBLE:            return GL_DOUBLE;
#endif

    default:                    return GL_FALSE;
    }
}

static inline GLenum VTK2UnsignedOpenGLType(int type)
{
  switch (type)
    {
#if VTK_SIZEOF_CHAR == 1
    case VTK_CHAR:              return GL_UNSIGNED_BYTE;
    case VTK_UNSIGNED_CHAR:     return GL_UNSIGNED_BYTE;
#elif VTK_SIZE_OF_CHAR == 2
    case VTK_CHAR:              return GL_UNSIGNED_SHORT;
    case VTK_UNSIGNED CHAR:     return GL_UNSIGNED_SHORT;
#endif

#if VTK_SIZEOF_SHORT == 1
    case VTK_SHORT:             return GL_UNSIGNED_BYTE;
    case VTK_UNSIGNED_SHORT:    return GL_UNSIGNED_BYTE;
#elif VTK_SIZEOF_SHORT == 2
    case VTK_SHORT:             return GL_UNSIGNED_SHORT;
    case VTK_UNSIGNED_SHORT:    return GL_UNSIGNED_SHORT;
#elif VTK_SIZEOF_SHORT == 4
    case VTK_SHORT:             return GL_UNSIGNED_INT;
    case VTK_UNSIGNED_SHORT:    return GL_UNSIGNED_INT;
#endif

#if VTK_SIZEOF_INT == 2
    case VTK_INT:               return GL_UNSIGNED_SHORT;
    case VTK_UNSIGNED_INT:      return GL_UNSIGNED_SHORT;
#elif VTK_SIZEOF_INT == 4
    case VTK_INT:               return GL_UNSIGNED_INT;
    case VTK_UNSIGNED_INT:      return GL_UNSIGNED_INT;
#endif

#if VTK_SIZEOF_ID_TYPE == 4
    case VTK_ID_TYPE:           return GL_UNSIGNED_INT;
#endif

#if VTK_SIZEOF_LONG == 4
    case VTK_LONG:              return GL_UNSIGNED_INT;
    case VTK_UNSIGNED_LONG:     return GL_UNSIGNED_INT;
#endif

    default:                    return GL_FALSE;
    }
}


//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::BeginPrimitive(int mode)
{
  glBegin(VTK2OpenGLPrimitive[mode]);
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::EndPrimitive()
{
  glEnd();
}

//-----------------------------------------------------------------------------
int vtkOpenGLPainterDeviceAdapter::IsAttributesSupported(int attribute)
{
  switch(attribute)
    {
  case vtkDataSetAttributes::NUM_ATTRIBUTES:
  case vtkDataSetAttributes::NORMALS:
  case vtkDataSetAttributes::SCALARS:
  case vtkDataSetAttributes::TCOORDS:
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::SendAttribute(int index, int numcomp,
  int type, const void *attribute, unsigned long offset/*=0*/)
{
  switch (index)
    {
  case vtkDataSetAttributes::NUM_ATTRIBUTES:     // Vertex
      if ((numcomp < 2) || (numcomp > 4))
        {
        vtkErrorMacro("Bad number of components.");
        return;
        }
      switch (VTK2SignedOpenGLType(type))
        {
        case GL_SHORT:
          switch (numcomp)
            {
            case 2: glVertex2sv((const GLshort *)attribute + offset);  break;
            case 3: glVertex3sv((const GLshort *)attribute + offset);  break;
            case 4: glVertex4sv((const GLshort *)attribute + offset);  break;
            }
          break;
        case GL_INT:
          switch (numcomp)
            {
            case 2: glVertex2iv((const GLint *)attribute + offset);  break;
            case 3: glVertex3iv((const GLint *)attribute + offset);  break;
            case 4: glVertex4iv((const GLint *)attribute + offset);  break;
            }
          break;
        case GL_FLOAT:
          switch (numcomp)
            {
            case 2: glVertex2fv((const GLfloat *)attribute + offset);  break;
            case 3: glVertex3fv((const GLfloat *)attribute + offset);  break;
            case 4: glVertex4fv((const GLfloat *)attribute + offset);  break;
            }
          break;
        case GL_DOUBLE:
          switch (numcomp)
            {
            case 2: glVertex2dv((const GLdouble *)attribute + offset);  break;
            case 3: glVertex3dv((const GLdouble *)attribute + offset);  break;
            case 4: glVertex4dv((const GLdouble *)attribute + offset);  break;
            }
          break;
        default:
          vtkErrorMacro("Unsupported type for vertices: " << type);
          return;
        }
      break;
  case vtkDataSetAttributes::NORMALS:     // Normal
      if (numcomp != 3)
        {
        vtkErrorMacro("Bad number of components.");
        return;
        }
      switch (VTK2SignedOpenGLType(type))
        {
        case GL_BYTE:
          glNormal3bv((const GLbyte *)attribute + offset);  break;
        case GL_SHORT:
          glNormal3sv((const GLshort *)attribute + offset);  break;
        case GL_INT:
          glNormal3iv((const GLint *)attribute + offset);  break;
        case GL_FLOAT:
          glNormal3fv((const GLfloat *)attribute + offset);  break;
        case GL_DOUBLE:
          glNormal3dv((const GLdouble *)attribute + offset);  break;
        default:
          vtkErrorMacro("Unsupported type for normals: " << type);
          return;
        }
      break;
  case vtkDataSetAttributes::SCALARS:     // Color
      if ((numcomp != 3) && (numcomp != 4))
        {
        vtkErrorMacro("Bad number of components.");
        return;
        }
      switch (VTK2OpenGLType(type))
        {
        case GL_BYTE:
          switch (numcomp)
            {
            case 3: glColor3bv((const GLbyte *)attribute + offset);  break;
            case 4: glColor4bv((const GLbyte *)attribute + offset);  break;
            }
          break;
        case GL_UNSIGNED_BYTE:
          switch (numcomp)
            {
            case 3: glColor3ubv((const GLubyte *)attribute + offset);  break;
            case 4: glColor4ubv((const GLubyte *)attribute + offset);  break;
            }
          break;
        case GL_SHORT:
          switch (numcomp)
            {
            case 3: glColor3sv((const GLshort *)attribute + offset);  break;
            case 4: glColor4sv((const GLshort *)attribute + offset);  break;
            }
          break;
        case GL_UNSIGNED_SHORT:
          switch (numcomp)
            {
            case 3: glColor3usv((const GLushort *)attribute + offset);  break;
            case 4: glColor4usv((const GLushort *)attribute + offset);  break;
            }
          break;
        case GL_INT:
          switch (numcomp)
            {
            case 3: glColor3iv((const GLint *)attribute + offset);  break;
            case 4: glColor4iv((const GLint *)attribute + offset);  break;
            }
          break;
        case GL_UNSIGNED_INT:
          switch (numcomp)
            {
            case 3: glColor3uiv((const GLuint *)attribute + offset);  break;
            case 4: glColor4uiv((const GLuint *)attribute + offset);  break;
            }
          break;
        case GL_FLOAT:
          switch (numcomp)
            {
            case 3: glColor3fv((const GLfloat *)attribute + offset);  break;
            case 4: glColor4fv((const GLfloat *)attribute + offset);  break;
            }
          break;
        case GL_DOUBLE:
          switch (numcomp)
            {
            case 3: glColor3dv((const GLdouble *)attribute + offset);  break;
            case 4: glColor4dv((const GLdouble *)attribute + offset);  break;
            }
          break;
        default:
          vtkErrorMacro("Unsupported type for colors: " << type);
          return;
        }
      break;
  case vtkDataSetAttributes::TCOORDS:     // Texture Coordinate
      if ((numcomp < 1) || (numcomp > 4))
        {
        vtkErrorMacro("Bad number of components.");
        return;
        }
      switch (VTK2SignedOpenGLType(type))
        {
        case GL_SHORT:
          switch (numcomp)
            {
            case 1: glTexCoord1sv((const GLshort *)attribute + offset);  break;
            case 2: glTexCoord2sv((const GLshort *)attribute + offset);  break;
            case 3: glTexCoord3sv((const GLshort *)attribute + offset);  break;
            case 4: glTexCoord4sv((const GLshort *)attribute + offset);  break;
            }
          break;
        case GL_INT:
          switch (numcomp)
            {
            case 1: glTexCoord1iv((const GLint *)attribute + offset);  break;
            case 2: glTexCoord2iv((const GLint *)attribute + offset);  break;
            case 3: glTexCoord3iv((const GLint *)attribute + offset);  break;
            case 4: glTexCoord4iv((const GLint *)attribute + offset);  break;
            }
          break;
        case GL_FLOAT:
          switch (numcomp)
            {
            case 1: glTexCoord1fv((const GLfloat *)attribute + offset);  break;
            case 2: glTexCoord2fv((const GLfloat *)attribute + offset);  break;
            case 3: glTexCoord3fv((const GLfloat *)attribute + offset);  break;
            case 4: glTexCoord4fv((const GLfloat *)attribute + offset);  break;
            }
          break;
        case GL_DOUBLE:
          switch (numcomp)
            {
            case 1: glTexCoord1dv((const GLdouble *)attribute + offset);  break;
            case 2: glTexCoord2dv((const GLdouble *)attribute + offset);  break;
            case 3: glTexCoord3dv((const GLdouble *)attribute + offset);  break;
            case 4: glTexCoord4dv((const GLdouble *)attribute + offset);  break;
            }
          break;
        default:
          vtkErrorMacro("Unsupported type for texture coordinates: " << type);
          return;
        }
      break;
    default:
      vtkErrorMacro("Unsupported attribute index: " << index);
      return;
    };
}

//-----------------------------------------------------------------------------

void vtkOpenGLPainterDeviceAdapter::SetAttributePointer(int index,
              int numcomponents,
              int type, int stride,
              const void *pointer)
{
  GLenum gltype;

  switch (index)
    {
  case vtkDataSetAttributes::NUM_ATTRIBUTES:     // Vertex
      gltype = VTK2SignedOpenGLType(type);
      switch (gltype)
        {
        case GL_SHORT:
        case GL_INT:
        case GL_FLOAT:
        case GL_DOUBLE:
          break;
        default:
          vtkErrorMacro("Unsupported type for vertices: " << type);
          return;
        }
      glVertexPointer(numcomponents, gltype, stride, pointer);
      break;
  case vtkDataSetAttributes::NORMALS:     // Normal
      gltype = VTK2SignedOpenGLType(type);
      switch (gltype)
        {
        case GL_BYTE:
        case GL_SHORT:
        case GL_INT:
        case GL_FLOAT:
        case GL_DOUBLE:
          break;
        default:
          vtkErrorMacro("Unsupported type for normals: " << type);
          return;
        }
      if (numcomponents != 3)
        {
        vtkErrorMacro("Unsupported number of components for normals.");
        return;
        }
      glNormalPointer(gltype, stride, pointer);
      break;
  case vtkDataSetAttributes::SCALARS:     // Color
      gltype = VTK2OpenGLType(type);
      switch (gltype)
        {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
        case GL_INT:
        case GL_UNSIGNED_INT:
        case GL_FLOAT:
        case GL_DOUBLE:
          break;
        default:
          vtkErrorMacro("Unsupported type for colors: " << type);
          return;
        }
      glColorPointer(numcomponents, gltype, stride, pointer);
      break;
  case vtkDataSetAttributes::TCOORDS:     // Texture Coordinate
      gltype = VTK2SignedOpenGLType(type);
      switch (gltype)
        {
        case GL_SHORT:
        case GL_INT:
        case GL_FLOAT:
        case GL_DOUBLE:
          break;
        default:
          vtkErrorMacro("Unsupported type for texture coordinates: " << type);
          return;
        }
      glTexCoordPointer(numcomponents, gltype, stride, pointer);
      break;
    default:
      vtkErrorMacro("Unsupported attribute index: " << index);
      return;
    };
}

//-----------------------------------------------------------------------------

void vtkOpenGLPainterDeviceAdapter::EnableAttributeArray(int index)
{
  switch (index)
    {
  case vtkDataSetAttributes::NUM_ATTRIBUTES:
      glEnableClientState(GL_VERTEX_ARRAY);  break;
  case vtkDataSetAttributes::NORMALS:
      glEnableClientState(GL_NORMAL_ARRAY);  break;
  case vtkDataSetAttributes::SCALARS:
      glEnableClientState(GL_COLOR_ARRAY);  break;
  case vtkDataSetAttributes::TCOORDS:
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);  break;
    default:
      vtkErrorMacro("Unsupported attribute index: " << index);
      return;
    };
}

void vtkOpenGLPainterDeviceAdapter::DisableAttributeArray(int index)
{
  switch (index)
    {
  case vtkDataSetAttributes::NUM_ATTRIBUTES:
      glDisableClientState(GL_VERTEX_ARRAY);  break;
  case vtkDataSetAttributes::NORMALS:
      glDisableClientState(GL_NORMAL_ARRAY);  break;
  case vtkDataSetAttributes::SCALARS:
      glDisableClientState(GL_COLOR_ARRAY);  break;
  case vtkDataSetAttributes::TCOORDS:
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);  break;
    default:
      vtkErrorMacro("Unsupported attribute index: " << index);
      return;
    };
}

//-----------------------------------------------------------------------------

void vtkOpenGLPainterDeviceAdapter::DrawArrays(int mode, vtkIdType first,
                                               vtkIdType count)
{
  glDrawArrays(VTK2OpenGLPrimitive[mode], (GLint)first, (GLsizei)count);
}

void vtkOpenGLPainterDeviceAdapter::DrawElements(int mode, vtkIdType count,
                                                 int type, void *indices)
{
  GLenum gltype = VTK2UnsignedOpenGLType(type);
  switch (gltype)
    {
    case GL_UNSIGNED_BYTE:
    case GL_UNSIGNED_SHORT:
    case GL_UNSIGNED_INT:
      break;
    default:
      if (type == VTK_ID_TYPE)
        {
        // This seems really inefficient for handling vtkIdType when they
        // are 64 bit, but OpenGL does not handle 64 bit indices.  What
        // else can I do?
        vtkIdType *oldarray = (vtkIdType *)indices;
        GLuint *newarray;
        newarray = new GLuint[count];
        vtkstd::copy(oldarray, oldarray + count, newarray);
        glDrawElements(VTK2OpenGLPrimitive[mode], (GLsizei)count,
                       GL_UNSIGNED_INT, newarray);
        delete[] newarray;
        return;
        }
      else
        {
        vtkErrorMacro("Invalid type for indices.");
        return;
        }
    }
  glDrawElements(VTK2OpenGLPrimitive[mode], (GLsizei)count, gltype, indices);
}

//-----------------------------------------------------------------------------

int vtkOpenGLPainterDeviceAdapter::Compatible(vtkRenderer *renderer)
{
  return renderer->IsA("vtkOpenGLRenderer");
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::MakeLighting(int mode)
{
  if (mode)
    {
    glEnable(GL_LIGHTING);
    }
  else
    {
    glDisable(GL_LIGHTING);
    }
}

//-----------------------------------------------------------------------------
int vtkOpenGLPainterDeviceAdapter::QueryLighting()
{
  if (glIsEnabled(GL_LIGHTING))
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::MakeMultisampling(int mode)
{
  if (mode)
    {
    glEnable(vtkgl::MULTISAMPLE);
    }
  else
    {
    glDisable(vtkgl::MULTISAMPLE);
    }
}

//-----------------------------------------------------------------------------
int vtkOpenGLPainterDeviceAdapter::QueryMultisampling()
{
  if (glIsEnabled(vtkgl::MULTISAMPLE))
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::MakeBlending(int mode)
{
  if (mode)
    {
    glEnable(GL_BLEND);
    }
  else
    {
    glDisable(GL_BLEND);
    }
}

//-----------------------------------------------------------------------------
int vtkOpenGLPainterDeviceAdapter::QueryBlending()
{
  if (glIsEnabled(GL_BLEND))
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::MakeVertexEmphasis(int mode)
{
  if (mode)
    {
    float s;
    glGetFloatv(GL_POINT_SIZE, &s);
    this->PointSize = s;
    glPointSize(4.0); //make verts large enough to be sure to overlap cell

    float nf[2];   //put verts just in front of associated cells
    glGetFloatv(GL_DEPTH_RANGE, nf);
    this->RangeNear = (double)nf[0];
    this->RangeFar = (double)nf[1];
    glDepthRange(0.0, nf[1]*0.98);

    glDepthMask(GL_FALSE); //prevent verts from interfering with each other

    if (this->MaxStencil == 0)
      {
      //if we don't have the stencil buffer, act as if everything fails
      //in order not to return invalid results
      glColorMask(0,0,0,0);
      }
    }
  else
    {
    glPointSize(this->PointSize);
    glDepthRange(this->RangeNear, this->RangeFar);
    glDepthMask(GL_TRUE);
    if (this->MaxStencil == 0)
      {
      glColorMask(1,1,1,1);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::WriteStencil(vtkIdType value)
{
  if (this->MaxStencil)
    {
    value = value % this->MaxStencil + 1;
    if (value == 1)
      {
      glClearStencil(0); //start over so don't write into some previous area
      }
    glStencilFunc(GL_ALWAYS, value, this->MaxStencil);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::TestStencil(vtkIdType value)
{
  if (this->MaxStencil)
    {
    value = value % this->MaxStencil + 1;
    glStencilFunc(GL_EQUAL, value, this->MaxStencil);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::Stencil(int on)
{
  if (on)
    {
    glEnable(GL_STENCIL_TEST);    
    GLint stencilbits;
    glGetIntegerv(GL_STENCIL_BITS, &stencilbits);
    this->MaxStencil = (1<<stencilbits)-1;
    }
  else
    {
    glDisable(GL_STENCIL_TEST);
    }
}


