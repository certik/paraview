/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMIceTMultiDisplayRenderViewProxy.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMIceTMultiDisplayRenderViewProxy - multi display using IceT.
// .SECTION Description
// vtkSMIceTMultiDisplayRenderViewProxy is the view proxy used when using tile
// displays.

#ifndef __vtkSMIceTMultiDisplayRenderViewProxy_h
#define __vtkSMIceTMultiDisplayRenderViewProxy_h

#include "vtkSMIceTDesktopRenderViewProxy.h"

class VTK_EXPORT vtkSMIceTMultiDisplayRenderViewProxy : 
  public vtkSMIceTDesktopRenderViewProxy
{
public:
  static vtkSMIceTMultiDisplayRenderViewProxy* New();
  vtkTypeRevisionMacro(vtkSMIceTMultiDisplayRenderViewProxy, 
    vtkSMIceTDesktopRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Key used to indicate to the representations about client collection
  // decision.
  static vtkInformationIntegerKey* CLIENT_COLLECT();

  // When set, it indicates that the client will always render, irrespective of
  // whether remote-rendering is enabled.
  static vtkInformationIntegerKey* CLIENT_RENDER();

  // Description:
  // Select the threshold under which any geometry is collected on the client.
  // When the data size exceeds the threshold, the client shows outlines instead
  // of actual data. Size is in kilobytes.
  vtkSetMacro(CollectGeometryThreshold, double);
  vtkGetMacro(CollectGeometryThreshold, double);

  // Description:
  // Set this flag to indicate whether to calculate the reduction factor for
  // use in tree composite (or client server) when still rendering.
  vtkSetMacro(StillRenderImageReductionFactor, int);
  vtkGetMacro(StillRenderImageReductionFactor, int);

//BTX
protected:
  vtkSMIceTMultiDisplayRenderViewProxy();
  ~vtkSMIceTMultiDisplayRenderViewProxy();

  // Description:
  // Called at the end of CreateVTKObjects().
  virtual void EndCreateVTKObjects();

  // Description:
  // Method called before Still Render is called.
  // Used to perform some every-still-render-setup actions.
  virtual void BeginStillRender();

  // Description:
  // Method called before Interactive Render.
  // Used to perform some every-interactive-render-setup actions.
  virtual void BeginInteractiveRender();

  // Description:
  // Creates a new vtkSMRepresentationStrategy subclass based on the type
  // requested.
  // Overridden to set KeepLODPipelineUpdated flag on all representation
  // strategies.
  virtual vtkSMRepresentationStrategy* NewStrategyInternal(
    int dataType);

  // Description:
  // Indicates if we should render using compositing.
  // Multi-display should always use server-side rendering.
  virtual bool GetCompositingDecision(
    unsigned long totalMemory, int stillRender);

  // Description:
  // Given the memory size in bytes, determine the client collection decision.
  bool GetClientCollectionDecision(unsigned long totalMemory);

  // Description:
  // Method used to update the information object with the client collection
  // decision.
  void SetClientCollect(bool decision);

  double CollectGeometryThreshold;
  int StillRenderImageReductionFactor;

  bool LastClientCollectionDecision;
private:
  vtkSMIceTMultiDisplayRenderViewProxy(const vtkSMIceTMultiDisplayRenderViewProxy&); // Not implemented.
  void operator=(const vtkSMIceTMultiDisplayRenderViewProxy&); // Not implemented.
//ETX
};

#endif

