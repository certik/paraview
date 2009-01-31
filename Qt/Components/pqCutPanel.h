/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqCutPanel.h,v $

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

#ifndef _pqCutPanel_h
#define _pqCutPanel_h

#include "pqComponentsExport.h"

#include "pqObjectPanel.h"

class pqImplicitPlaneWidget;

/// Custom panel for the Cut filter that manages a 3D widget for interactive cutting
class PQCOMPONENTS_EXPORT pqCutPanel :
  public pqObjectPanel
{
  typedef pqObjectPanel Superclass;

  Q_OBJECT

public:
  pqCutPanel(pqProxy* proxy, QWidget* p);
  ~pqCutPanel();

  pqImplicitPlaneWidget* getImplicitPlaneWidget();
 
public slots:
  /// Called when the panel becomes active. Default implemnetation does
  /// nothing.
  virtual void select();

  /// Called when the panel becomes inactive. Default implemnetation does
  /// nothing.
  virtual void deselect();

private slots:
  /// Called if the user accepts pending modifications
  void onAccepted();
  /// Called if the user rejects pending modifications
  void onRejected();


private:
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif

