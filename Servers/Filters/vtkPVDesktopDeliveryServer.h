/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVDesktopDeliveryServer.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVDesktopDeliveryServer - An object for remote rendering.
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
// .SECTION see also
// vtkPVDesktopDeliveryClient
//

#ifndef __vtkPVDesktopDeliveryServer_h
#define __vtkPVDesktopDeliveryServer_h

#include "vtkParallelRenderManager.h"

class vtkPVDesktopDeliveryServerRendererMap;

class vtkFloatArray;
class VTK_EXPORT vtkPVDesktopDeliveryServer : public vtkParallelRenderManager
{
public:
  vtkTypeRevisionMacro(vtkPVDesktopDeliveryServer, vtkParallelRenderManager);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkPVDesktopDeliveryServer *New();

  // Description:
  // Set/Get the controller that is attached to a vtkDesktopDeliveryClient.
  // This object will assume that the controller has two processors, and
  // that the controller on the opposite side of the controller has been
  // given to the server object.
  virtual void SetController(vtkMultiProcessController *controller);

  // Description:
  // Set/Get a parallel render manager that is used for parallel rendering.
  // If not set or set to NULL, rendering is done on the render window
  // directly in single process mode.  It will be assumed that the
  // ParallelRenderManager will have the final image stored at the local
  // processes.
  virtual void SetParallelRenderManager(vtkParallelRenderManager *prm);
  vtkGetObjectMacro(ParallelRenderManager, vtkParallelRenderManager);

  virtual void SetRenderWindow(vtkRenderWindow *renWin);

  // Description:
  // If on (the default) locally rendered images will be shipped back to
  // the client.  To facilitate this, the local rendering windows will be
  // resized based on the remote window settings.  If off, the images are
  // assumed to be displayed locally.  The render window maintains its
  // current size.
  virtual void SetRemoteDisplay(int);
  vtkGetMacro(RemoteDisplay, int);
  vtkBooleanMacro(RemoteDisplay, int);

  // Description:
  // Because the client may have many vtkPVDesktopDeliveryClient objects
  // attached to many render windows, separate renderers by the id associated
  // with the same id on the client side.
  virtual void AddRenderer(vtkRenderer *ren) { this->AddRenderer(0, ren); }
  virtual void RemoveRenderer(vtkRenderer *ren) { this->RemoveRenderer(0,ren); }
  virtual void RemoveAllRenderers() { this->RemoveAllRenderers(0); }
  virtual void AddRenderer(int id, vtkRenderer *ren);
  virtual void RemoveRenderer(int id, vtkRenderer *ren);
  virtual void RemoveAllRenderers(int id);

  virtual void InitializeRMIs();

  // Description:
  // DO NOT USE.  FOR INTERNAL USE ONLY.
  virtual void UseRendererSet(int id);

  // Description:
  // Unused. This is here to provide the same API as the desktop delivery
  // client. This is required by the render module proxy.
  void SetSquirtLevel (int) {}

  // Description:
  // Capture Z buffer from render window on end render. Works only when
  // ParallelRenderManager is 0.
  vtkSetMacro(CaptureZBuffer, int);
  vtkGetMacro(CaptureZBuffer, int);

  // Description:
  // Get Z buffer value from captured buffer, if any.
  // This API for ZBuffer should not be used when ParallelRenderManager is set
  // i.e. IceT is being used. In that case get the Z value from the
  // IceTRenderManager.
  float GetZBufferValue(int x, int y);

//BTX

  enum Tags {
    IMAGE_TAG=12433,
    IMAGE_SIZE_TAG=12434,
    REMOTE_DISPLAY_TAG=834340,
    TIMING_METRICS_TAG=834341,
    SQUIRT_OPTIONS_TAG=834342,
    IMAGE_PARAMS_TAG=834343,
    WINDOW_ID_RMI_TAG=502382,
    WINDOW_GEOMETRY_TAG=502383,
    RENDERER_VIEWPORT_TAG=342239
  };

  struct TimingMetrics {
    double ImageProcessingTime;
  };

  struct WindowGeometry {
    int Position[2];
    int GUISize[2];
    int Id;
    int AnnotationLayer;
  };

  struct SquirtOptions {
    int Enabled;
    int CompressLevel;
  };

  struct ImageParams {
    int RemoteDisplay;
    int SquirtCompressed;
    int NumberOfComponents;
    int BufferSize;
    int ImageSize[2];
  };

  enum TimingMetricSize {
    TIMING_METRICS_SIZE=sizeof(struct TimingMetrics)/sizeof(double)
  };
  enum WindowGeometrySize {
    WINDOW_GEOMETRY_SIZE=sizeof(struct WindowGeometry)/sizeof(int)
  };
  enum SquirtOptionSize {
    SQUIRT_OPTIONS_SIZE=sizeof(struct SquirtOptions)/sizeof(int)
  };
  enum ImageParamsSize {
    IMAGE_PARAMS_SIZE=sizeof(struct ImageParams)/sizeof(int)
  };

//ETX
  
protected:
  vtkPVDesktopDeliveryServer();
  virtual ~vtkPVDesktopDeliveryServer();

  virtual void PreRenderProcessing();
  virtual void PostRenderProcessing();
  virtual void LocalComputeVisiblePropBounds(vtkRenderer *, double bounds[6]);

  vtkParallelRenderManager *ParallelRenderManager;

  unsigned long StartParallelRenderTag;
  unsigned long EndParallelRenderTag;

  unsigned long ReceiveWindowIdTag;

  virtual void SetRenderWindowSize();

  virtual void ReadReducedImage();

  virtual void ReceiveWindowInformation();
  virtual void ReceiveRendererInformation(vtkRenderer *);

  int Squirt;
  int SquirtCompressionLevel;
  void SquirtCompress(vtkUnsignedCharArray *in, vtkUnsignedCharArray *out);

  int RemoteDisplay;

  vtkUnsignedCharArray *SquirtBuffer;

  vtkPVDesktopDeliveryServerRendererMap *RendererMap;

  int ClientWindowPosition[2];
  int ClientWindowSize[2];
  int ClientRequestedImageSize[2];
  int ClientGUISize[2];

  int AnnotationLayer;

  int ImageResized;
  int CaptureZBuffer;

  vtkFloatArray* ReducedZBuffer;
  vtkUnsignedCharArray *SendImageBuffer;
  unsigned long WindowIdRMIId;

private:
  vtkPVDesktopDeliveryServer(const vtkPVDesktopDeliveryServer &); //Not implemented
  void operator=(const vtkPVDesktopDeliveryServer &);    //Not implemented
};

#endif

