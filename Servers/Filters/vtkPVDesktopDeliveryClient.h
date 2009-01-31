/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVDesktopDeliveryClient.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVDesktopDeliveryClient - An object for remote rendering.
//
// .SECTION Description
//
// The two vtkPVDesktopDelivery objects (vtkPVDesktopDeliveryClient and
// vtkPVDesktopDeliveryServer) work together to enable interactive viewing of
// remotely rendered data.  On the client side, there may be multiple render
// windows arranged in a GUI, each having its own vtkPVDesktopDeliveryClient
// object attached.  The server has a single render window and
// vtkPVDesktopDeliveryServer.  All the vtkPVDesktopDeliveryClient objects
// connect to a single vtkPVDesktopDeliveryServer object.
//
// On the client side, each render window is assumed to be placed inside a
// parent GUI window.  The layout of the render windows in the GUI window must
// be given.  The following information must be given: a unique identifier for
// the window, the position of the render window in the parent GUI window, and
// the size of the parent GUI window.  The server will arrange the renderings
// in its single render window to match the layout given for the parent GUI
// window on the client side.
//
// .SECTION See Also
// vtkPVDesktopDeliveryServer
//

#ifndef __vtkPVDesktopDeliveryClient_h
#define __vtkPVDesktopDeliveryClient_h

#include "vtkParallelRenderManager.h"

class vtkCommand;

class VTK_EXPORT vtkPVDesktopDeliveryClient : public vtkParallelRenderManager
{
public:
  vtkTypeRevisionMacro(vtkPVDesktopDeliveryClient, vtkParallelRenderManager);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkPVDesktopDeliveryClient *New();

  // Description:
  // Generally, when you turn compositing off, you expect the image in the
  // local render window to remain unchanged.  If you are doing remote
  // display, then there is no point in rendering anything on the server
  // side.  Thus, if RemoteDisplay is on and UseCompositing is turned off,
  // then ParallelRendering is also turned off altogether.  Likewise,
  // ParallelRendering is turned back on when UseCompositing is turned on.
  virtual void SetUseCompositing(int v);

  // Description:
  // Set/Get the controller that is attached to a vtkDesktopDeliveryServer.
  // This object will assume that the controller has two processors, and
  // that the controller on the opposite side of the controller has been
  // given to the server object.
  virtual void SetController(vtkMultiProcessController *controller);

  virtual void ComputeVisiblePropBounds(vtkRenderer *ren, double bounds[6]);

  // Description:
  // Returns true if remote display is on.  If remote display is on, then
  // the RenderWindow will be updated with an image rendered on the client.
  // If not, the RenderWindow keeps the image its Renderers draw.  The
  // RemoteDisplay flag is determined by the server.  The remote display
  // is specified by the server, so the value may be out of date if an
  // image has not been rendered since the last time the value changed on
  // the server.
  vtkGetMacro(RemoteDisplay, int);

  vtkGetMacro(RemoteImageProcessingTime, double);
  vtkGetMacro(TransferTime, double);
  virtual double GetRenderTime() {
    return (this->RenderTime - this->RemoteImageProcessingTime);
  }
  virtual double GetImageProcessingTime() {
    return (  this->RemoteImageProcessingTime
        + this->TransferTime + this->ImageProcessingTime);
  }

  // For ParaView
  void SetSquirtLevel (int l)
  { 
    if (l == 0)
      {
      this->SquirtOff();
      }
    else
      {
      this->SquirtOn(); 
      this->SetSquirtCompressionLevel(l-1);
      }
  }

  // Description:
  // Enables or disables SQUIRT compression for image delivery.  By
  // default, compression is off.  Note that this function may be replaced
  // with a more universal image compression at a later date.
  vtkGetMacro(Squirt, int);
  vtkSetMacro(Squirt, int);
  vtkBooleanMacro(Squirt, int);

  // Description:
  // Sets the compression level used by SQUIRT.  Higher values result in
  // better compression but lower resolution in the color space (the size
  // of the image is unaffected by this option).
  vtkGetMacro(SquirtCompressionLevel, int);
  vtkSetClampMacro(SquirtCompressionLevel, int, 0, 5);

  // Description:
  // The client may have many render windows and associated desktop delivery
  // client objects that all attach to the same desktop delivery server.
  // This id distinguishes them.  The ids themselves do not matter so long
  // as they are unique.
  vtkGetMacro(Id, int);
  vtkSetMacro(Id, int);

  // Description:
  // Any renderer on this layer or higher will be considered annotation an
  // will be drawn on top of any image received from the server.  Set to 1
  // by default.
  vtkGetMacro(AnnotationLayer, int);
  vtkSetMacro(AnnotationLayer, int);

  // Description:
  // The location of the upper left corner of the render window in the GUI
  // with (0,0) being the upper most left position.  Not that this is
  // different than other most VTK positions, which are referenced from
  // the lower left.  This coordinate system was picked to correspond to
  // most GUI APIs.
  vtkGetVector2Macro(WindowPosition, int);
  vtkSetVector2Macro(WindowPosition, int);

  // Description:
  // The size of the GUI in which the render window is placed.  By default,
  // the size is the same as the render window.
  vtkGetVector2Macro(GUISize, int);
  vtkSetVector2Macro(GUISize, int);

  virtual void SetImageReductionFactorForUpdateRate(double desiredUpdateRate);
  float GetZBufferValue(int x, int y);

  // Description:
  // For internal use.
  virtual void ReceiveImageFromServer();

protected:
  vtkPVDesktopDeliveryClient();
  ~vtkPVDesktopDeliveryClient();

  virtual void PreRenderProcessing();
  virtual void PostRenderProcessing();

  int ServerProcessId;

  // Updated by UpdateServerInfo.
  int RemoteDisplay;
  double RemoteImageProcessingTime;
  double TransferTime;

  virtual void SendWindowInformation();
  virtual void SendRendererInformation(vtkRenderer *);

  // Squirt options (probably to be replaced later).
  int Squirt;
  int SquirtCompressionLevel;
  vtkUnsignedCharArray *SquirtBuffer;

  void SquirtDecompress(vtkUnsignedCharArray *in, vtkUnsignedCharArray *out);

  int UseCompositing;

  int Id;
  int AnnotationLayer;

  int WindowPosition[2];
  int GUISize[2];

  int ReceivedImageFromServer;
  vtkCommand *ReceiveImageCallback;
  
private:
  vtkPVDesktopDeliveryClient(const vtkPVDesktopDeliveryClient &); //Not implemented
  void operator=(const vtkPVDesktopDeliveryClient &); //Not implemented
};

#endif

