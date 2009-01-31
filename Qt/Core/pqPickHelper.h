/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqPickHelper.h,v $

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

#ifndef __pqPickHelper_h
#define __pqPickHelper_h

#include "pqCoreExport.h"
#include <QObject>

class pqRenderView;
class pqView;

/*! \brief Utility to switch interactor styles in 3D views.
 *
 * pqPickHelper standardizes the mechanism by which 3D views
 * are switched between interaction and world point picking modes.
 * this is used to pick center of camera rotation and will be used in 
 * location probe
 */
class PQCORE_EXPORT pqPickHelper : public QObject
{
  Q_OBJECT
public:
  pqPickHelper(QObject* parent=NULL);
  virtual ~pqPickHelper();

  //for internal use only, this is how mouse press and release events
  //are processed internally
  void processEvents(unsigned long event);

  /// Returns the currently picked render view.
  pqRenderView* getRenderView() const;

  //BTX
  enum Modes
  {
    INTERACT,
    PICK
  };
  //ETX

public slots:
  /// Set active view. If a view has been set previously, this method ensures
  /// that it is not in pick mode.
  void setView(pqView*);

  void beginPick();
  void endPick();

signals:
  /// fired after mouse up in pick mode
  void pickFinished(double x, double y, double z);

  /// fired by beginPick() and endPick().
  void picking(bool);

  void enabled(bool enable);

  /// Fired with pick mode changes. 
  /// \c mode is enum Modes{...}.
  void modeChanged(int mode);

protected:
  int setPickOn(int mode);
  int setPickOff();
  int Mode;
  int Xe, Ye;

private:
  class pqInternal;
  class vtkPQPickObserver;

  pqInternal* Internal;
};

#endif

