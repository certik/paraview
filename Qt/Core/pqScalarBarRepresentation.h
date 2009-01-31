/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqScalarBarRepresentation.h,v $

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
#ifndef __pqScalarBarRepresentation_h
#define __pqScalarBarRepresentation_h

#include "pqRepresentation.h"

class pqPipelineRepresentation;
class pqScalarsToColors;

// PQ object for a scalar bar. Keeps itself connected with the pqScalarsToColors
// object, if any.
class PQCORE_EXPORT pqScalarBarRepresentation : public pqRepresentation
{
  Q_OBJECT
public:
  pqScalarBarRepresentation(const QString& group, const QString& name,
    vtkSMProxy* scalarbar, pqServer* server,
    QObject* parent=0);
  virtual ~pqScalarBarRepresentation();

  /// Get the lookup table this scalar bar shows, if any.
  pqScalarsToColors* getLookupTable() const;

  /// Calls this method to set up a title for the scalar bar
  /// using the color by array name from the display.
  /// The component used to color with is obtained from the 
  /// LookupTable already stored by this object.
  void makeTitle(pqPipelineRepresentation* display);

  /// A scalar bar title is divided into two parts (any of which can be empty).
  /// Typically the first is the array name and the second is the component.
  /// This method returns the pair.
  QPair<QString, QString> getTitle() const;
  
  /// Set the title formed by combining two parts.
  void setTitle(const QString& name, const QString& component);

  /// set by pqPipelineRepresentation when it forces the visiblity of the scalar
  /// bar to be off.
  void setAutoHidden(bool h)
    { this->AutoHidden = h; }
  bool getAutoHidden() const
    { return this->AutoHidden; }

protected slots:
  void onLookupTableModified();

protected:
  /// flag set to true, when the scalarbar has been hidden by
  /// pqPipelineRepresentation and not explicitly by the user. Used to restore
  /// scalar bar visibility when the representation becomes visible.
  bool AutoHidden;

private:
  class pqInternal;
  pqInternal* Internal;
};



#endif

