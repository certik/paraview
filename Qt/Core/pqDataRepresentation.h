/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqDataRepresentation.h,v $

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
#ifndef __pqDataRepresentation_h
#define __pqDataRepresentation_h

#include "pqRepresentation.h"

class pqDataRepresentationInternal;
class pqOutputPort;
class pqPipelineSource;
class pqScalarsToColors;

// pqDataRepresentation is the superclass for a display for a pqPipelineSource 
// i.e. the input for this display proxy is a pqPiplineSource.
// This class manages the linking between the pqPiplineSource 
// and pqDataRepresentation.
class PQCORE_EXPORT pqDataRepresentation : public pqRepresentation
{
  Q_OBJECT
  typedef pqRepresentation Superclass;
public:
  pqDataRepresentation(const QString& group, const QString& name,
    vtkSMProxy* display, pqServer* server,
    QObject* parent=0);
  virtual ~pqDataRepresentation();

  // Get the source/filter of which this is a display.
  pqPipelineSource* getInput() const;

  /// Returns the input pqPipelineSource's output port to which this
  /// representation is connected.
  pqOutputPort* getOutputPortFromInput() const;

  /// Returns the lookuptable proxy, if any.
  /// Most consumer displays take a lookup table. This method 
  /// provides access to the Lookup table, if one exists.
  virtual vtkSMProxy* getLookupTableProxy();

  /// Returns the pqScalarsToColors object for the lookup table
  /// proxy if any.
  /// Most consumer displays take a lookup table. This method 
  /// provides access to the Lookup table, if one exists.
  virtual pqScalarsToColors* getLookupTable();

  /// Sets default values for the underlying proxy. 
  /// This is during the initialization stage of the pqProxy 
  /// for proxies created by the GUI itself i.e.
  /// for proxies loaded through state or created by python client
  /// this method won't be called. 
  /// The default implementation iterates over all properties
  /// of the proxy and sets them to default values. 
  virtual void setDefaultPropertyValues();

protected slots:
  // called when input property on display changes. We must detect if
  // (and when) the display is connected to a new proxy.
  virtual void onInputChanged();

protected:
  // Use this method to initialize the pqObject state using the
  // underlying vtkSMProxy. This needs to be done only once,
  // after the object has been created. 
  virtual void initialize() 
    {
    this->Superclass::initialize();
    this->onInputChanged();
    }


private:
  pqDataRepresentation(const pqDataRepresentation&); // Not implemented.
  void operator=(const pqDataRepresentation&); // Not implemented.

  pqDataRepresentationInternal* Internal;
};

#endif

