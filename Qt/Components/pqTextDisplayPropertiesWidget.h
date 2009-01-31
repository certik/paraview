/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqTextDisplayPropertiesWidget.h,v $

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
#ifndef __pqTextDisplayPropertiesWidget_h
#define __pqTextDisplayPropertiesWidget_h

#include "pqDisplayPanel.h"

/// This is a display editor widget for Text displays. 
/// Can only work with pqTextDisplay objects.
class PQCOMPONENTS_EXPORT pqTextDisplayPropertiesWidget : public pqDisplayPanel
{
  Q_OBJECT
public:
  pqTextDisplayPropertiesWidget(pqRepresentation* display, QWidget* parent=0);
  virtual ~pqTextDisplayPropertiesWidget();

protected slots:
  void onVisibilityChanged(int);
  void onTextLocationChanged(bool);

private:
  /// Set the display whose properties this editor will edit.
  void setDisplay(pqRepresentation* display);

  class pqInternal;
  pqInternal* Internal;

private:
  pqTextDisplayPropertiesWidget(const pqTextDisplayPropertiesWidget&); // Not implemented.
  void operator=(const pqTextDisplayPropertiesWidget&); // Not implemented.
};

#endif


