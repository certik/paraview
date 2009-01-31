/*=========================================================================

   Program:   ParaView
   Module:    $RCSfile: pqSMSignalAdaptors.h,v $

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

#ifndef pq_SMSignalAdaptors_h
#define pq_SMSignalAdaptors_h

#include <QObject>
#include <QVariant>
#include "pqComponentsExport.h"
class pqProxy;

/// signal adaptor to allow getting/setting/observing of a pseudo vtkSMProxy property
class PQCOMPONENTS_EXPORT pqSignalAdaptorProxy : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QVariant proxy READ proxy WRITE setProxy)
public:
  /// constructor requires a QObject, the name of the QString proxy name, and 
  /// a signal for property changes
  pqSignalAdaptorProxy(QObject* p, const char* Property, const char* signal);
  /// get the proxy
  QVariant proxy() const;
signals:
  /// signal the proxy changed
  void proxyChanged(const QVariant&);
public slots:
  /// set the proxy
  void setProxy(const QVariant&);
protected slots:
  void handleProxyChanged();
protected:
  QByteArray PropertyName;
};


// This adaptor is used to connect a QWidget (typically a QLineEdit)
// to a DoubleVectorProperty using a pqPropertyLink.
// It formats the text shown in the widget to a more
// readable form. 
class PQCOMPONENTS_EXPORT pqSignalAdaptorDouble : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString value READ value WRITE setValue)
public:
  pqSignalAdaptorDouble(QObject* parent, const char* property,
    const char* signal);
  ~pqSignalAdaptorDouble();

  QString value();
signals:
  void valueChanged(const QString&);

public slots:
  void setValue(const QString& value);

protected slots:
  void objectSignalFired();

protected:
  QString PropertyName;
};

#endif

