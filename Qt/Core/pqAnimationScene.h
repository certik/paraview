/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqAnimationScene.h,v $

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

========================================================================*/
#ifndef __pqAnimationScene_h
#define __pqAnimationScene_h

#include "pqProxy.h"
#include <QPair>
#include <QSet>

class pqAnimationCue;
class QSize;
class vtkObject;
class vtkSMAnimationSceneProxy;

/// pqAnimationScene is a representation for a vtkSMAnimationScene
/// proxy. It provides API to access AnimationCues in the scene.
class PQCORE_EXPORT pqAnimationScene : public pqProxy
{
  Q_OBJECT
  typedef pqProxy Superclass;
public:
  pqAnimationScene(const QString& group, const QString& name,
    vtkSMProxy* proxy, pqServer* server, QObject* parent=NULL);
  virtual ~pqAnimationScene();

  /// returns the vtkSMAnimationSceneProxy.
  vtkSMAnimationSceneProxy* getAnimationSceneProxy() const;

  /// Returns the cue that animates the given 
  /// \c index of the given \c property on the given \c proxy, in this scene,
  /// if any.
  pqAnimationCue* getCue(vtkSMProxy* proxy, const char* propertyname, 
    int index) const;

  /// Creates and initializes a new cue that can animate
  /// the \c index of the \c property on the given \c proxy
  /// in this scene. This method does not check is such a cue already
  /// exists, use getCue() before calling this to avoid duplicates.
  pqAnimationCue* createCue(vtkSMProxy* proxy, const char* propertyname,
    int index);
  pqAnimationCue* createCue(vtkSMProxy* proxy, const char* propertyname,
    int index, const QString& cuetype);

  /// Removes all cues which animate the indicated proxy, if any.
  void removeCues(vtkSMProxy* proxy);
  
  /// Removes cue
  void removeCue(pqAnimationCue* cue);

  /// returns true is the cue is present in this scene.
  bool contains(pqAnimationCue*) const;

  /// Combines the sizes of all the view modules
  /// animated by the scene and returns the total view size.
  QSize getViewSize() const;

  /// Get the clock time range set on the animation scene proxy.
  QPair<double, double> getClockTimeRange() const;

  /// Get all the cues in this scene
  QSet<pqAnimationCue*> getCues() const;

  /// set default property values.
  /// Overridden to create the default pipeline time animation cue.
  virtual void setDefaultPropertyValues();

signals:
  /// Fired before a new cue is added to the scene.
  void preAddedCue(pqAnimationCue*);

  /// Fired after a new cue has been added to the scene.
  void addedCue(pqAnimationCue*);

  /// Fired before a cue is removed from the scene.
  void preRemovedCue(pqAnimationCue*);

  /// Fired after a cue has been removed from the scene.
  void removedCue(pqAnimationCue*);

  /// Fired after cues have been added/removed.
  void cuesChanged();

  /// Emitted when the play mode changes.
  void playModeChanged();

  /// Emitted when the looping state changes on the
  /// underlying proxy.
  void loopChanged();

  /// Emitted when the clock time ranges change.
  void clockTimeRangesChanged();

  /// Emitted when animation starts playing.
  void beginPlay();

  /// Emitted when animation ends playing.
  void endPlay();

  /// Emitted when playing animation. 
  /// Argument is the percent of play completed.
  void tick(int percentCompleted);

  /// Emitted when playing (or when animation time for the scene is set). 
  /// \c time is the animation clock time.
  void animationTime(double time);

  /// Emitted when the number of frames changes
  void frameCountChanged();
  
  /// Emitted when the number of timesteps changes
  void timeStepsChanged();

public slots:
  /// Play animation.
  void play();
  
  /// Pause animation.
  void pause();

  /// Set the animation time.
  void setAnimationTime(double time);

private slots:
  /// Called when the "Cues" property on the AnimationScene proxy
  /// is changed. Updates the internal datastructure to reflect the current
  /// state of the scene.
  void onCuesChanged();

  /// Called when timekeeper's timesteps change,
  /// we synchronize the ClockTimeRange on the scene proxy.
  void updateTimeRanges();  

  /// Called on animation tick.
  void onTick(vtkObject* caller, unsigned long, void*, void* info);
private:
  pqAnimationScene(const pqAnimationScene&); // Not implemented.
  void operator=(const pqAnimationScene&); // Not implemented.

  class pqInternals;
  pqInternals* Internals;

  void setupTimeTrack();
  pqAnimationCue* createCueInternal(const QString& cuetype,
    vtkSMProxy* proxy, const char* propertyname, int index);
};

#endif

