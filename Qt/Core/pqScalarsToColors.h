/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqScalarsToColors.h,v $

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
#ifndef __pqScalarsToColors_h
#define __pqScalarsToColors_h

#include "pqProxy.h"
#include <QPair>

class pqScalarsToColorsInternal;
class pqScalarBarRepresentation;
class pqRenderView;

/// pqScalarsToColors is a represents a vtkScalarsToColors proxy.
class PQCORE_EXPORT pqScalarsToColors : public pqProxy
{
  Q_OBJECT
public:
  pqScalarsToColors(const QString& group, const QString& name,
    vtkSMProxy* proxy, pqServer* server, QObject* parent=NULL);
  virtual ~pqScalarsToColors();

  /// Returns the first scalar bar visible in the given render module,
  /// if any.
  pqScalarBarRepresentation* getScalarBar(pqRenderView* ren) const;

  /// Returns if the lookup table's scalar range is locked.
  bool getScalarRangeLock() const;

  /// Set the scalar range if the range specified is greater than
  /// the current scalar range. This call respects the ScalarRangeLock.
  /// If the lock is set, then this call has no effect.
  /// If ScalarRangeLock is false, then this call will 
  /// move all control points uniformly to fit the new range.
  void setWholeScalarRange(double min, double max);

  /// Sets the scalar range. 
  /// Does not consider the ScalarRangeLock. Moves all control points
  /// uniformly to fit the new range.
  void setScalarRange(double min, double max);

  /// Returns the current scalar range. If number of RGBPoints is 0,
  /// then the scalar range is not defined (in which case this method
  /// returns (0, 0).
  QPair<double, double> getScalarRange() const;

  enum Mode
    {
    MAGNITUDE = 0,
    COMPONENT = 1
    };

  // Set the color mode (component/magnitude) and 
  // component to color by. When mode is magnitude, component is ignored.
  void setVectorMode(Mode mode, int component);

  // Returns the vector mode and component used by the Lookup table. 
  // When vector mode is MAGNITUDE, value returned by
  // getVectorComponent() is indeterminate.
  Mode getVectorMode() const;
  int getVectorComponent() const;

public slots:
  // This method checks if this LUT is used by any display,
  // if not, it hides all the scalars bar showing this LUT.
  void hideUnusedScalarBars();

  /// Set the scalar range lock.
  void setScalarRangeLock(bool lock);

  /// Triggers a build on the lookup table.
  void build();
signals:
  /// Fired after a new scalar bar is added or removed from this LUT.
  void scalarBarsChanged();

protected:
  friend class pqScalarBarRepresentation;

  void addScalarBar(pqScalarBarRepresentation*);
  void removeScalarBar(pqScalarBarRepresentation*);

private:
  pqScalarsToColorsInternal* Internal;
};

#endif

