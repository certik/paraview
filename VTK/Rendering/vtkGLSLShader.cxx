/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkGLSLShader.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkGLSLShader.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLTexture.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLShader.h"

#include <vtkgl.h>
//#include <GL/glu.h>

#include <sys/types.h>
#include <vtkstd/string>
#include <vtkstd/vector>


#if 1
#define GLSLprintOpenGLError() GLSLprintOglError(__FILE__, __LINE__)
static int GLSLprintOglError(const char *vtkNotUsed(file), int vtkNotUsed(line))
{
  //Returns 1 if an OpenGL error occurred, 0 otherwise.
  GLenum glErr;
  int    retCode = 0;

  glErr = glGetError();
  while (glErr != GL_NO_ERROR)
    {
    //printf("glError in file %s @ line %d: %s\n", file, line, gluErrorString(glErr));
    cout << "Error!" << endl;
    retCode = 1;
    glErr = glGetError();
    }
  return retCode;
}
#endif

static void printLogInfo( GLuint shader, int useOpenGL2, const char* filename)
{
#if 1
  int isShader;
  if (useOpenGL2)
    {
    isShader = vtkgl::IsShader(shader);
    }
  else
    {
    glGetError();
    GLint objectType;
    vtkgl::GetObjectParameterivARB(shader, vtkgl::OBJECT_TYPE_ARB, &objectType);
    if (   (glGetError() == GL_NO_ERROR)
        && ((GLenum)objectType == vtkgl::SHADER_OBJECT_ARB))
      {
      isShader = 1;
      }
    else
      {
      isShader = 0;
      }
    }
  if (isShader)
    {
    cout << "GLSL Shader." << endl;
    }
  else
    {
    cout << "Not a GLSL Shader!!!." << endl;
    return;
    }

  // Check scope
  GLint type;
  if (useOpenGL2)
    {
    vtkgl::GetShaderiv(shader, vtkgl::SHADER_TYPE, &type);
    }
  else
    {
    vtkgl::GetObjectParameterivARB(shader, vtkgl::OBJECT_SUBTYPE_ARB, &type);
    }
  // I know.  Technically if OpenGL 2.0 is not supported I should be checking
  // against VERTEX_SHADER_ARB and FRAGMENT_SHADER_ARB, but the respective
  // specifications for each has them set to the same value.
  if( type == static_cast<GLint>(vtkgl::VERTEX_SHADER) )
    {
    cout << "GLSL Vertex Shader." << endl;
    }
  else if( type == static_cast<GLint>(vtkgl::FRAGMENT_SHADER) )
    {
    cout << "GLSL Fragment Shader." << endl;
    }
  else
    {
    cout << "Not a GLSL Shader???" << endl;
    }

  GLint compiled = 0;
  GLsizei maxLength = 0;
  if (useOpenGL2)
    {
    vtkgl::GetShaderiv(shader, vtkgl::COMPILE_STATUS, &compiled);
    vtkgl::GetShaderiv(shader, vtkgl::INFO_LOG_LENGTH, &maxLength);
    }
  else
    {
    vtkgl::GetObjectParameterivARB(shader, vtkgl::OBJECT_COMPILE_STATUS_ARB,
                                   &compiled);
    vtkgl::GetObjectParameterivARB(shader, vtkgl::OBJECT_INFO_LOG_LENGTH_ARB,
                                   &maxLength );
    }

  vtkgl::GLchar* info = new vtkgl::GLchar[maxLength];
  GLsizei charsWritten = 0;

  if (useOpenGL2)
    {
    vtkgl::GetShaderInfoLog( shader, maxLength, &charsWritten, info );
    }
  else
    {
    vtkgl::GetInfoLogARB(shader, maxLength, &charsWritten, info);
    }

  cout << "Compiled Status: " << compiled << endl;
  if( info )
    {
    cout << "Log message: " << filename << endl << (char *)info << endl;
    }


  GLSLprintOpenGLError();
#endif
}


#if 0
static void printAttributeInfo(GLuint program, const char* vtkNotUsed(filename))
{
  // print all uniform attributes
  GLint numAttrs;
  vtkgl::GetProgramiv( program, vtkgl::ACTIVE_UNIFORMS, &numAttrs);
  if( numAttrs == GL_INVALID_VALUE )
    {
    cout << "GL_INVALID_VALUE for number of attributes." << endl;
    }
  else if( numAttrs == GL_INVALID_OPERATION )
    {
    cout << "GL_INVALID_OPERATION for number of attributes." << endl;
    }
  else if( numAttrs == GL_INVALID_ENUM )
    {
    cout << "GL_INVALID_ENUM for number of attributes." << endl;
    }
  else if( numAttrs == GL_INVALID_OPERATION )
    {
    cout << "GL_INVALID_OPERATION for number of attributes." << endl;
    }
  else
    {
    cout << numAttrs << " Uniform parameters:" << endl;
    }

  GLint maxLength;
  vtkgl::GetProgramiv( program, vtkgl::ACTIVE_UNIFORM_MAX_LENGTH, &maxLength);
  GLint id;
  for( id=0; id<numAttrs; id++ )
    {
    vtkgl::GLchar *name = new vtkgl::GLchar[maxLength];
    GLint length;
    GLint size;
    GLenum type;
    vtkgl::GetActiveUniform( program, id, maxLength, &length, &size, &type, name);
    if( name )
      {
      cout << "\t" << (char *)name << endl;
      }
    delete[] name;
    }
  cout << endl;
}
#endif

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkGLSLShader);
vtkCxxRevisionMacro(vtkGLSLShader, "$Revision: 1.11 $");

//-----------------------------------------------------------------------------
vtkGLSLShader::vtkGLSLShader()
{
  this->Shader = 0;
  this->Program = 0;
  this->UseOpenGL2 = 0;
}

//-----------------------------------------------------------------------------
vtkGLSLShader::~vtkGLSLShader()
{
  this->ReleaseGraphicsResources(0);
}

//-----------------------------------------------------------------------------
void vtkGLSLShader::ReleaseGraphicsResources(vtkWindow*)
{
  if (this->IsShader())
    {
    if (this->UseOpenGL2)
      {
      vtkgl::DeleteShader(this->Shader);
      }
    else
      {
      vtkgl::DeleteObjectARB(this->Shader);
      }
    this->Shader = 0;
    }
}

//-----------------------------------------------------------------------------
int vtkGLSLShader::IsCompiled()
{
  GLint value = 0;
  if( this->IsShader() )
    {
    if (this->UseOpenGL2)
      {
      vtkgl::GetShaderiv( static_cast<GLuint>(this->Shader),
                          vtkgl::COMPILE_STATUS,
                          &value );
      }
    else
      {
      vtkgl::GetObjectParameterivARB(this->Shader,
                                     vtkgl::OBJECT_COMPILE_STATUS_ARB, &value);
      }
    }
  if( value == 1 )
    {
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
int vtkGLSLShader::IsShader()
{
  if( this->Shader )
    {
    if (this->UseOpenGL2)
      {
      if( vtkgl::IsShader( static_cast<GLuint>(this->Shader) ) == GL_TRUE )
        {
        return 1;
        }
      }
    else
      {
      glGetError();
      GLint objectType;
      vtkgl::GetObjectParameterivARB(this->Shader,
                                     vtkgl::OBJECT_TYPE_ARB, &objectType);
      if (   (glGetError() == GL_NO_ERROR)
          && ((GLenum)objectType == vtkgl::SHADER_OBJECT_ARB))
        {
        return 1;
        }
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkGLSLShader::LoadShader()
{
  // if we have a shader, don't create a new one
  if( !this->IsShader() )
    {
    // create an empty shader object
    switch (this->XMLShader->GetScope())
      {
    case vtkXMLShader::SCOPE_VERTEX:
      if (this->UseOpenGL2)
        {
        this->Shader = vtkgl::CreateShader( vtkgl::VERTEX_SHADER );
        }
      else
        {
        this->Shader = vtkgl::CreateShaderObjectARB(vtkgl::VERTEX_SHADER_ARB);
        }
      break;
      
    case vtkXMLShader::SCOPE_FRAGMENT:
      if (this->UseOpenGL2)
        {
        this->Shader = vtkgl::CreateShader( vtkgl::FRAGMENT_SHADER );
        }
      else
        {
        this->Shader = vtkgl::CreateShaderObjectARB(vtkgl::FRAGMENT_SHADER_ARB);
        }
      break;
      }
    }
}

//-----------------------------------------------------------------------------
int vtkGLSLShader::Compile()
{
  if (this->IsCompiled())
    {
    return 1;
    }

  // Later, an XMLShader may not be necessary if the source is set by the
  // application.
  // For now, we need an XMLShader
  if (!this->XMLShader)
    {
    return 0;
    }

  if (!this->XMLShader->GetCode())
    {
    vtkErrorMacro("Shader doesn't have any code!");
    return 0;
      
    }

  if (this->IsCompiled())
    {
    return 1;
    }

  // create a shader context if needed.
  this->LoadShader();

  if( !this->IsShader() )
    {
    vtkErrorMacro( "Shader not loaded!!!" << endl );
    if( this->Shader && this->XMLShader->GetName() )
      {
      printLogInfo(static_cast<GLuint>(this->Shader), this->UseOpenGL2,
                   this->XMLShader->GetName());
      }
    return 0;
    }

  // if we have the source available, try to load it
  // Load the shader as a single string seems to work best
  const vtkgl::GLchar* source = 
    static_cast<const vtkgl::GLchar*>(this->XMLShader->GetCode());
  
  // Since the entire shader is sent to GL as a single string, the number of
  // lines (second argument) is '1'.
  if (this->UseOpenGL2)
    {
    vtkgl::ShaderSource( static_cast<GLuint>(this->Shader), 1, &source, NULL );
    }
  else
    {
    vtkgl::ShaderSourceARB(this->Shader, 1, &source, NULL);
    }

  // make sure the source has been loaded
  // print an error log if the shader is not compiled
  if (this->UseOpenGL2)
    {
    vtkgl::CompileShader(static_cast<GLuint>(this->Shader));
    }
  else
    {
    vtkgl::CompileShaderARB(this->Shader);
    }

  if( !this->IsCompiled() )
    {
    vtkErrorMacro( "Shader not compiled!!!" << endl );
    if( this->Shader && this->XMLShader->GetName() )
      {
      printLogInfo(static_cast<GLuint>(this->Shader), this->UseOpenGL2,
                   this->XMLShader->GetName());
      }
    return 0;
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkGLSLShader::SetUniformParameter(const char* name, int numValues, 
  const int* values)
{
  if( !this->IsShader() )
    {
    return;
    }
  while (glGetError() != GL_NO_ERROR)
    {
    vtkErrorMacro(<< "Found unchecked OpenGL error.");
    }
  GLint loc = static_cast<GLint>(this->GetUniformLocation(name));
  if (loc == -1)
    {
    return;
    }
  const GLint *v = reinterpret_cast<const GLint *>(values);
  if (this->UseOpenGL2)
    {
    switch(numValues)
      {
      case 1: vtkgl::Uniform1iv(loc, 1, v); break;
      case 2: vtkgl::Uniform2iv(loc, 1, v); break;
      case 3: vtkgl::Uniform3iv(loc, 1, v); break;
      case 4: vtkgl::Uniform4iv(loc, 1, v); break;
      default:
        vtkErrorMacro("Number of values not supported: " << numValues);
      }
    }
  else
    {
    switch(numValues)
      {
      case 1: vtkgl::Uniform1ivARB(loc, 1, v); break;
      case 2: vtkgl::Uniform2ivARB(loc, 1, v); break;
      case 3: vtkgl::Uniform3ivARB(loc, 1, v); break;
      case 4: vtkgl::Uniform4ivARB(loc, 1, v); break;
      default:
        vtkErrorMacro("Number of values not supported: " << numValues);
      }
    }
  while (glGetError() != GL_NO_ERROR)
    {
    vtkErrorMacro(<< "OpenGL error when setting uniform variable int["
                  << numValues << "] " << name << ".\n"
                  << "Perhaps there is a type mismatch.");
    }
}

//-----------------------------------------------------------------------------
void vtkGLSLShader::SetUniformParameter(const char* name, int numValues, 
  const float* values)
{
  if( !this->IsShader() )
    {
    return;
    }
  while (glGetError() != GL_NO_ERROR)
    {
    vtkErrorMacro(<< "Found unchecked OpenGL error.");
    }
  GLint loc = static_cast<GLint>(this->GetUniformLocation(name));
  if (loc == -1)
    {
    return;
    }
  if (this->UseOpenGL2)
    {
    switch(numValues)
      {
      case 1: vtkgl::Uniform1fv(loc, 1, values); break;
      case 2: vtkgl::Uniform2fv(loc, 1, values); break;
      case 3: vtkgl::Uniform3fv(loc, 1, values); break;
      case 4: vtkgl::Uniform4fv(loc, 1, values); break;
      default:
        vtkErrorMacro("Number of values not supported: " << numValues);
      }
    }
  else
    {
    switch(numValues)
      {
      case 1: vtkgl::Uniform1fvARB(loc, 1, values); break;
      case 2: vtkgl::Uniform2fvARB(loc, 1, values); break;
      case 3: vtkgl::Uniform3fvARB(loc, 1, values); break;
      case 4: vtkgl::Uniform4fvARB(loc, 1, values); break;
      default:
        vtkErrorMacro("Number of values not supported: " << numValues);
      }
    }
  while (glGetError() != GL_NO_ERROR)
    {
    vtkErrorMacro(<< "OpenGL error when setting uniform variable int["
                  << numValues << "] " << name << ".\n"
                  << "Perhaps there is a type mismatch.");
    }
}

//-----------------------------------------------------------------------------
void vtkGLSLShader::SetUniformParameter(const char* name, int numValues, 
  const double* values)
{
  if( !this->IsShader() )
    {
    return;
    }
  float* fvalues = new float [numValues];

  for (int i=0; i<numValues; i++)
    {
    fvalues[i] = static_cast<float>(values[i]);
    }
  this->SetUniformParameter(name, numValues, fvalues);
  delete []fvalues;
}

//-----------------------------------------------------------------------------
void vtkGLSLShader:: SetMatrixParameter(const char* name, int numValues, 
  int order, const float* value)
{
  if( !this->IsShader() )
    {
    return;
    }
  int transpose = (order == vtkShader::RowMajor) ? 1 : 0;

  GLint loc = static_cast<GLint>(this->GetUniformLocation(name));
  if (loc == -1)
    {
    return;
    }
  if (this->UseOpenGL2)
    {
    switch (numValues)
      {
      case 2*2: vtkgl::UniformMatrix2fv(loc, 1, transpose, value); break;
      case 3*3: vtkgl::UniformMatrix3fv(loc, 1, transpose, value); break;
      case 4*4: vtkgl::UniformMatrix4fv(loc, 1, transpose, value); break;
      default:
        vtkErrorMacro("Number of values not supported: " << numValues);
      }
    }
  else
    {
      switch (numValues)
        {
        case 2*2: vtkgl::UniformMatrix2fvARB(loc, 1, transpose, value); break;
        case 3*3: vtkgl::UniformMatrix3fvARB(loc, 1, transpose, value); break;
        case 4*4: vtkgl::UniformMatrix4fvARB(loc, 1, transpose, value); break;
        default:
          vtkErrorMacro("Number of values not supported: " << numValues);
        }
    }
}

//-----------------------------------------------------------------------------
void vtkGLSLShader:: SetMatrixParameter(const char* name, int numValues, 
  int order, const double* value)
{
  if( !this->IsShader() )
    {
    return;
    }
  float *v = new float[numValues];
  for (int i=0; i < numValues; i++)
    {
    v[i] = static_cast<float>(value[i]);
    }
  this->SetMatrixParameter(name, numValues, order, v);
  delete []v;
}


//-----------------------------------------------------------------------------
void vtkGLSLShader::SetMatrixParameter(const char*, const char*, const char*)
{
  if( !this->IsShader() )
    {
    return;
    }
  vtkErrorMacro("GLSL does not support any system matrices!");
}

//-----------------------------------------------------------------------------
void vtkGLSLShader::SetSamplerParameter(const char* name, vtkTexture* ,
                                        int textureIndex)
{
  if( !this->IsShader() )
    {
    return;
    }
  this->SetUniformParameter(name, 1, &textureIndex);
}

//-----------------------------------------------------------------------------
int vtkGLSLShader::GetUniformLocation( const char* name )
{
  if( !this->IsShader() )
    {
    return -1;
    }
  if( !name )
    {
    vtkErrorMacro( "NULL uniform shader parameter name.");
    return -1;
    }

  if (this->UseOpenGL2)
    {
    if( vtkgl::IsProgram(this->GetProgram())!=GL_TRUE)
      {
      vtkErrorMacro( "NULL shader program.");
      return -1;
      }
    }
  else
    {
    glGetError();
    GLint objectType;
    vtkgl::GetObjectParameterivARB(this->Program,
                                   vtkgl::OBJECT_TYPE_ARB, &objectType);
    if (   (glGetError() != GL_NO_ERROR)
        || ((GLenum)objectType != vtkgl::PROGRAM_OBJECT_ARB))
      {
      return -1;
      }
    }

  int location;
  if (this->UseOpenGL2)
    {
    location = vtkgl::GetUniformLocation( this->GetProgram(), name );
    }
  else
    {
    location = vtkgl::GetUniformLocationARB( this->GetProgram(), name );
    }
  if( location == -1 )
    {
    vtkErrorMacro( "No such shader parameter. " << name );
    }

  return location;
}

//-----------------------------------------------------------------------------
void vtkGLSLShader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "Program: " << this->Program << endl;
  os << indent << "UseOpenGL2: " << this->UseOpenGL2 << endl;
}
