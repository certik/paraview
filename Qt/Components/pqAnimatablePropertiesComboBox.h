/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqAnimatablePropertiesComboBox.h,v $

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
#ifndef __pqAnimatablePropertiesComboBox_h 
#define __pqAnimatablePropertiesComboBox_h

#include <QComboBox>
#include "pqComponentsExport.h"

class vtkSMProxy;

/// pqAnimatablePropertiesComboBox is a combo box that can list the animatable 
/// properties of any proxy.
class PQCOMPONENTS_EXPORT pqAnimatablePropertiesComboBox : public QComboBox
{
  Q_OBJECT
  typedef QComboBox Superclass;

public:
  pqAnimatablePropertiesComboBox(QWidget* parent=0);
  ~pqAnimatablePropertiesComboBox();

  /// Returns the source whose properties are currently being listed, if any.
  vtkSMProxy* source() const;

  vtkSMProxy* getCurrentProxy() const;
  QString getCurrentPropertyName() const;
  int getCurrentIndex() const;

  /// Sometimes, we want the combo to show a empty field that does not represent
  /// any property. Set this to true to use such a field.
  void setUseBlankEntry(bool b)
    { this->UseBlankEntry = b;}

public slots:
  /// Set the source whose properties this widget should list. If source is
  /// null, the widget gets disabled.
  void setSource(vtkSMProxy* proxy);

  /// Set source without calling buildPropertyList() internally. Thus the user
  /// will explicitly call addSMProperty to add properties.
  void setSourceWithoutProperties(vtkSMProxy* proxy);

  /// Add a property to the widget.
  void addSMProperty(const QString& label, const QString& propertyname, int index);

protected slots:
  /// Builds the property list.
  void buildPropertyList();

private:
  pqAnimatablePropertiesComboBox(const pqAnimatablePropertiesComboBox&); // Not implemented.
  void operator=(const pqAnimatablePropertiesComboBox&); // Not implemented.

  void buildPropertyListInternal(vtkSMProxy* proxy, const QString& labelPrefix);
  void addSMPropertyInternal(const QString& label, 
    vtkSMProxy* proxy, const QString& propertyname, int index);

  bool UseBlankEntry;
public:
  class pqInternal;
private:
  pqInternal* Internal;
};

#endif


