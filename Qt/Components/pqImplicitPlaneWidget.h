/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqImplicitPlaneWidget.h,v $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#ifndef _pqImplicitPlaneWidget_h
#define _pqImplicitPlaneWidget_h

#include "pqComponentsExport.h"

#include "pqProxy.h"

#include "pq3DWidget.h"

class pqServer;
class vtkSMDoubleVectorProperty;

/// Provides a complete Qt UI for working with a 3D plane widget
class PQCOMPONENTS_EXPORT pqImplicitPlaneWidget : public pq3DWidget
{
  typedef pq3DWidget Superclass;
  
  Q_OBJECT
  
public:
  pqImplicitPlaneWidget(vtkSMProxy* refProxy, vtkSMProxy* proxy, QWidget* p = 0);
  ~pqImplicitPlaneWidget();

  /// A implicit plane widget can optionally have  a ScaleFactor/ScaleOrigin
  /// which affects how the widget is placed on reset bounds.
  void setScaleFactor(double scale[3]);
  void setScaleOrigin(double origin[3]);

public slots:
  /// Resets the bounds of the 3D widget to the reference proxy bounds.
  virtual void resetBounds();

  /// accept the changes
  void accept();
  /// reset the changes
  void reset();

protected:
  
  /// Makes the 3D widget plane visible (respects the overall visibility flag)
  virtual void showPlane();

  /// Hides the 3D widget plane
  virtual void hidePlane();

  /// Subclasses can override this method to map properties to
  /// GUI. Default implementation updates the internal datastructures
  /// so that default implementations can be provided for 
  /// accept/reset.
  virtual void setControlledProperty(const char* function,
    vtkSMProperty * controlled_property);

protected:
  /// Internal method to create the widget.
  void createWidget(pqServer*);

  /// Internal method to cleanup widget.
  void cleanupWidget();
  
private slots:
  /// Called to show/hide the 3D widget
  void onShow3DWidget(bool);
  /// Called to reset the 3D widget bounds to the bounding box proxy's
  void onResetBounds();
  /// Called to set the widget origin to the center of the bounding box proxy's data
  void onUseCenterBounds();
  /// Called to set the widget normal to the X axis
  void onUseXNormal();
  /// Called to set the widget normal to the Y axis
  void onUseYNormal();
  /// Called to set the widget normal to the Z axis
  void onUseZNormal();
  /// Called to set the widget normal to the camera direction
  void onUseCameraNormal();
  /// Called when the user starts dragging the 3D widget
  void on3DWidgetStartDrag();
  /// Called when the user stops dragging the 3D widget
  void on3DWidgetEndDrag();
  /// Called whenever the 3D widget visibility is modified
  void onWidgetVisibilityChanged(bool visible);

private:
  void get3DWidgetState(double* origin, double* normal);
  void set3DWidgetState(const double* origin, const double* normal);
  void setQtWidgetState(const double* origin, const double* normal);

  void setNormalProperty(vtkSMProperty*);
  void setOriginProperty(vtkSMProperty*);

  double ScaleFactor[3];
  double ScaleOrigin[3];
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif
