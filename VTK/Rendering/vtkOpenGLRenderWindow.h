/*=========================================================================

Program:   Visualization Toolkit
Module:    $RCSfile: vtkOpenGLRenderWindow.h,v $

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLRenderWindow - OpenGL rendering window
// .SECTION Description
// vtkOpenGLRenderWindow is a concrete implementation of the abstract class
// vtkRenderWindow. vtkOpenGLRenderer interfaces to the OpenGL graphics
// library. Application programmers should normally use vtkRenderWindow
// instead of the OpenGL specific version.

#ifndef __vtkOpenGLRenderWindow_h
#define __vtkOpenGLRenderWindow_h

#include "vtkRenderWindow.h"

#include "vtkOpenGL.h" // Needed for GLuint.

class vtkIdList;

class VTK_RENDERING_EXPORT vtkOpenGLRenderWindow : public vtkRenderWindow
{
protected:
  long OldMonitorSetting;

public:
  vtkTypeRevisionMacro(vtkOpenGLRenderWindow,vtkRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the maximum number of multisamples
  static void SetGlobalMaximumNumberOfMultiSamples(int val);
  static int  GetGlobalMaximumNumberOfMultiSamples();

  // Description:
  // Update system if needed due to stereo rendering.
  virtual void StereoUpdate();

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBRGB... 
  virtual unsigned char *GetPixelData(int x,int y,int x2,int y2,int front);
  virtual int GetPixelData(int x,int y,int x2,int y2, int front,
                           vtkUnsignedCharArray *data);
  virtual int SetPixelData(int x,int y,int x2,int y2,unsigned char *data,
                           int front);
  virtual int SetPixelData(int x,int y,int x2,int y2,
                           vtkUnsignedCharArray *data, int front);

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBARGBA... 
  virtual float *GetRGBAPixelData(int x,int y,int x2,int y2,int front);
  virtual int GetRGBAPixelData(int x,int y,int x2,int y2, int front,
                               vtkFloatArray* data);
  virtual int SetRGBAPixelData(int x,int y,int x2,int y2, float *data,
                               int front, int blend=0);
  virtual int SetRGBAPixelData(int x,int y,int x2,int y2, vtkFloatArray *data,
                               int front, int blend=0);
  virtual void ReleaseRGBAPixelData(float *data);
  virtual unsigned char *GetRGBACharPixelData(int x,int y,int x2,int y2,
                                              int front);
  virtual int GetRGBACharPixelData(int x,int y,int x2,int y2, int front,
                                   vtkUnsignedCharArray *data);
  virtual int SetRGBACharPixelData(int x, int y, int x2, int y2,
                                   unsigned char *data, int front,
                                   int blend=0);  
  virtual int SetRGBACharPixelData(int x,int y,int x2,int y2,
                                   vtkUnsignedCharArray *data, int front,
                                   int blend=0);  

  // Description:
  // Set/Get the zbuffer data from an image
  virtual float *GetZbufferData( int x1, int y1, int x2, int y2 );
  virtual int GetZbufferData( int x1, int y1, int x2, int y2, float* z );
  virtual int GetZbufferData( int x1, int y1, int x2, int y2,
                              vtkFloatArray* z );
  virtual int SetZbufferData( int x1, int y1, int x2, int y2, float *buffer );
  virtual int SetZbufferData( int x1, int y1, int x2, int y2,
                              vtkFloatArray *buffer );

  // Description:
  // Make this window the current OpenGL context.
  void MakeCurrent() = 0;
  
  // Description:
  // Register a texture name with this render window.
  void RegisterTextureResource (GLuint id);

  // Description:
  // Get the size of the depth buffer.
  int GetDepthBufferSize();

  // Description:
  // Get the size of the color buffer.
  // Returns 0 if not able to determine otherwise sets R G B and A into buffer.
  int GetColorBufferSizes(int *rgba);
  
  // Description:
  // Initialize OpenGL for this window.
  virtual void OpenGLInit();

  // Description:
  // Return the OpenGL name of the back left buffer.
  // It is GL_BACK_LEFT if GL is bound to the window-system-provided
  // framebuffer. It is vtkgl::COLOR_ATTACHMENT0_EXT+i if GL is bound to an
  // application-created framebuffer object (GPU-based offscreen rendering)
  // It is used by vtkOpenGLCamera.
  unsigned int GetBackLeftBuffer();
  
  // Description:
  // Return the OpenGL name of the back right buffer.
  // It is GL_BACK_RIGHT if GL is bound to the window-system-provided
  // framebuffer. It is vtkgl::COLOR_ATTACHMENT0_EXT+i if GL is bound to an
  // application-created framebuffer object (GPU-based offscreen rendering)
  // It is used by vtkOpenGLCamera.
  unsigned int GetBackRightBuffer();
  
  // Description:
  // Return the OpenGL name of the front left buffer.
  // It is GL_FRONT_LEFT if GL is bound to the window-system-provided
  // framebuffer. It is vtkgl::COLOR_ATTACHMENT0_EXT+i if GL is bound to an
  // application-created framebuffer object (GPU-based offscreen rendering)
  // It is used by vtkOpenGLCamera.
  unsigned int GetFrontLeftBuffer();
  
protected:
  vtkOpenGLRenderWindow();
  ~vtkOpenGLRenderWindow();
  
  vtkIdList *TextureResourceIds;

  int GetPixelData(int x,int y,int x2,int y2,int front, unsigned char* data);
  int GetRGBAPixelData(int x,int y,int x2,int y2, int front, float* data);
  int GetRGBACharPixelData(int x,int y,int x2,int y2, int front,
                           unsigned char* data);
  
  // Description:
  // Create an offScreen window based on OpenGL framebuffer extension.
  // Return if the creation was successful or not.
  // \pre positive_width: width>0
  // \pre positive_height: height>0
  // \pre not_initialized: !OffScreenUseFrameBuffer
  // \post valid_result: (result==0 || result==1)
  //                     && (result implies OffScreenUseFrameBuffer)
  int CreateHardwareOffScreenWindow(int width, int height);
  
  // Description:
  // Destroy an offscreen window based on OpenGL framebuffer extension.
  // \pre initialized: OffScreenUseFrameBuffer
  // \post destroyed: !OffScreenUseFrameBuffer
  void DestroyHardwareOffScreenWindow();
  
  // Description:
  // Flag telling if a framebuffer-based offscreen is currently in use.
  int OffScreenUseFrameBuffer;
  
  // Description:
  // Variables used by the framebuffer-based offscreen method.
  int NumberOfFrameBuffers;
  unsigned int TextureObjects[4]; // really GLuint
  unsigned int FrameBufferObject; // really GLuint
  unsigned int DepthRenderBufferObject; // really GLuint
  
  // Description:
  // Create a not-off-screen window.
  virtual void CreateAWindow()=0;
  
  // Description:
  // Destroy a not-off-screen window.
  virtual void DestroyWindow()=0;
  
  unsigned int BackLeftBuffer;
  unsigned int BackRightBuffer;
  unsigned int FrontLeftBuffer;
  
private:
  vtkOpenGLRenderWindow(const vtkOpenGLRenderWindow&);  // Not implemented.
  void operator=(const vtkOpenGLRenderWindow&);  // Not implemented.
};

#endif