/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqTimeKeeper.h,v $

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
#ifndef __pqTimeKeeper_h
#define __pqTimeKeeper_h

#include <QPair>
#include "pqProxy.h"

class pqPipelineSource;
class pqView;
class vtkObject;
class vtkSMProxy;

/// pqTimeKeeper is pqProxy for "TimeKeeper" proxy. A timekeeper is
/// created by default per connection. pqServer keeps a pointer to the 
/// connection's time keeper. A time keeper keeps track of the
/// global time and timesteps available currently.
class PQCORE_EXPORT pqTimeKeeper : public pqProxy 
{
  Q_OBJECT
public:
  pqTimeKeeper(const QString& group, const QString& name,
    vtkSMProxy* timekeeper, pqServer* server, QObject* parent=0);
  virtual ~pqTimeKeeper();

  /// Returns the number of timestep values
  /// known to this time keeper.
  int getNumberOfTimeStepValues() const;

  /// Returns the timestep value at the given index.
  /// index < getNumberOfTimeStepValues().
  double getTimeStepValue(int index) const;

  /// Returns the maximum index in the timestep values
  /// for the given time for which timestep value[index] <= time.
  int getTimeStepValueIndex(double time) const;

  /// Returns the time range. 
  /// Return (0,0) is getNumberOfTimeStepValues() == 0.
  QPair<double, double> getTimeRange() const;

  /// Returns the current time.
  double getTime() const;

  /// Update the current time.
  void setTime(double time);

signals:
  /// Fired when the keeper updates the times.
  void timeStepsChanged();

  /// Fired when the current time changes.
  void timeChanged();


protected slots:
  void propertyModified(vtkObject*, unsigned long, void*, void* callData);
  
  /// Called when a source is added.
  void sourceAdded(pqPipelineSource*);

  /// Called when a source is removed.
  void sourceRemoved(pqPipelineSource*);

  /// Called when a view is added.
  void viewAdded(pqView*);

  /// Called when a view is removed.
  void viewRemoved(pqView*);

protected:
  void updateTimeKeeperProxy();

  void cleanupTimes(pqPipelineSource*);
private:
  pqTimeKeeper(const pqTimeKeeper&); // Not implemented.
  void operator=(const pqTimeKeeper&); // Not implemented.

  void propertyModified(pqPipelineSource*);

  class pqInternals;
  pqInternals* Internals;
};

#endif

