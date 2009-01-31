/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqAnimationModel.h,v $

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

#ifndef pqAnimationModel_h
#define pqAnimationModel_h

#include "QtWidgetsExport.h"

#include <QObject>
#include <QGraphicsScene>
#include <QStandardItemModel>
#include <QPolygonF>

class pqAnimationTrack;
class pqAnimationKeyFrame;
class QGraphicsView;

// a model that represents a collection of animation tracks
class QTWIDGETS_EXPORT pqAnimationModel : public QGraphicsScene
{
  Q_OBJECT
  Q_ENUMS(ModeType)
  Q_PROPERTY(ModeType mode READ mode WRITE setMode)
  Q_PROPERTY(int ticks READ ticks WRITE setTicks)
  Q_PROPERTY(double currentTime READ currentTime WRITE setCurrentTime)
  Q_PROPERTY(double startTime READ startTime WRITE setStartTime)
  Q_PROPERTY(double endTime READ endTime WRITE setEndTime)
  Q_PROPERTY(bool interactive READ interactive WRITE setInteractive)
public:

  /// Real or Sequence mode
  /// Real mode shows no tick marks for timesteps
  /// Sequence mode shows evenly spaced ticks for teach timestep
  ///  where the number of ticks can be controled by the ticks property
  enum ModeType
    {
    Real,
    Sequence
    };

  pqAnimationModel(QGraphicsView* p = 0);
  ~pqAnimationModel();
  
  /// the number of tracks
  int count();
  /// get a track at an index
  pqAnimationTrack* track(int);

  /// add a track
  pqAnimationTrack* addTrack();
  /// remove a track
  void removeTrack(pqAnimationTrack* track);

  /// get the animation mode
  ModeType mode() const;
  /// get the number of ticks
  int ticks() const;
  /// get the current time
  double currentTime() const;
  /// get the start time
  double startTime() const;
  /// get the end time
  double endTime() const;
  /// get whether this scene is interactive
  bool interactive() const;

  QAbstractItemModel* header();
  void setRowHeight(int);
  int rowHeight() const;

public slots:

  /// set the animation mode
  void setMode(ModeType);
  /// set the number of ticks
  void setTicks(int);
  /// set the current time
  void setCurrentTime(double);
  /// set the start time
  void setStartTime(double);
  /// set the end time
  void setEndTime(double);
  /// set whether this scene is interactive
  void setInteractive(bool);

signals:
  // emitted when a track is double clicked on
  void trackSelected(pqAnimationTrack*);
  // emitted when the current time was changed by this model
  void currentTimeSet(double);
  // emitted when the time of a keyframe was changed by this model
  void keyFrameTimeChanged(pqAnimationTrack* track, pqAnimationKeyFrame* kf, int end, double time);

protected slots:

  void resizeTracks();
  void trackNameChanged();

protected:
  QPolygonF timeBarPoly(double time);
  double positionFromTime(double time);
  double timeFromPosition(double pos);
  double timeFromTick(int tick);
  int tickFromTime(double pos);
  void drawForeground(QPainter* painter, const QRectF& rect);
  bool hitTestCurrentTimePoly(const QPointF& pos);
  pqAnimationTrack* hitTestTracks(const QPointF& pos);
  pqAnimationKeyFrame* hitTestKeyFrame(pqAnimationTrack* t, const QPointF& pos);

  bool eventFilter(QObject* w, QEvent* e);

  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* mouseEvent);
  void mousePressEvent(QGraphicsSceneMouseEvent* mouseEvent);
  void mouseMoveEvent(QGraphicsSceneMouseEvent* mouseEvent);
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* mouseEvent);

  double timeToNormalizedTime(double) const;
  double normalizedTimeToTime(double) const;

private:

  ModeType Mode;
  int    Ticks;
  double CurrentTime;
  double StartTime;
  double EndTime;
  int    RowHeight;
  bool   Interactive;
  
  // vars to support interaction
  bool   CurrentTimeGrabbed;
  double NewCurrentTime;
  pqAnimationTrack*   CurrentTrackGrabbed;
  pqAnimationKeyFrame*   CurrentKeyFrameGrabbed;
  int   CurrentKeyFrameEdge;
  QPair<double, double> InteractiveRange;
  QList<double> SnapHints;

  QList<pqAnimationTrack*> Tracks;

  // model that provides names of tracks
  QStandardItemModel Header;
};

#endif // pqAnimationModel_h

