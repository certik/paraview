/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqProxyInformationWidget.h,v $

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
#ifndef _pqProxyInformationWidget_h
#define _pqProxyInformationWidget_h

#include <QWidget>
#include <QPointer>
#include "pqComponentsExport.h"

class pqOutputPort;

/// Widget which provides information about an output port of a source proxy
class PQCOMPONENTS_EXPORT pqProxyInformationWidget : public QWidget
{
  Q_OBJECT
public:
  /// constructor
  pqProxyInformationWidget(QWidget* p=0);
  /// destructor
  ~pqProxyInformationWidget();

  /// Set the display whose properties we want to edit. 
  void setOutputPort(pqOutputPort* outputport);

  /// get the proxy for which properties are displayed
  pqOutputPort* getOutputPort();

public slots:
  /// TODO: have this become automatic instead of relying on 
  /// the accept button in case another client modifies the pipeline.
  void updateInformation();

private:
  QPointer<pqOutputPort> OutputPort;
  class pqUi;
  pqUi* Ui;
};

#endif

