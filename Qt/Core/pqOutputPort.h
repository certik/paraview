/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqOutputPort.h,v $

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
#ifndef __pqOutputPort_h 
#define __pqOutputPort_h

#include "pqServerManagerModelItem.h"
#include "pqCoreExport.h"

class pqDataRepresentation;
class pqPipelineSource;
class pqServer;
class pqView;
class vtkPVDataInformation;

/// pqOutputPort is a server manager model item for an output port of any
/// pqPipelineSource item. This makes it possible to refer to a particular
/// output port in the server manager model. The pqPipelineSource keeps
/// references to all its output ports. The only way to access pqOutputPort
/// items is through the pqPipelineSource. One can obtain the pqPipelineSource
/// item from a pqOutputPort using getSource().
/// Once the outputs can be named, we will change this class to use output port
/// names instead of numbers.
class PQCORE_EXPORT pqOutputPort : public pqServerManagerModelItem
{
  Q_OBJECT
  typedef pqServerManagerModelItem Superclass;
public:
  pqOutputPort(pqPipelineSource* source, int portno);
  virtual ~pqOutputPort();

  /// Returns the pqPipelineSource whose output port this is.
  pqPipelineSource* getSource() const
    { return this->Source; }

  /// Returns the server connection on which this output port exists.
  pqServer* getServer() const;

  /// Returns the port number of the output port which this item represents.
  int getPortNumber() const
    { return this->PortNumber; }

  /// Returns the number of consumers connected to this output port. 
  int getNumberOfConsumers() const;

  /// Get the consumer at a particular index on this output port.
  pqPipelineSource* getConsumer(int index) const;

  /// Returns a list of representations for this output port in the given view.
  /// If view == NULL, returns all representations of this port.
  QList<pqDataRepresentation*> getRepresentations(pqView* view) const;

  /// Returns the first representation for this output port in the given view.
  /// If view is NULL, returns 0.
  pqDataRepresentation* getRepresentation(pqView* view) const;

  /// Returns a list of render modules in which this output port
  /// has representations added (the representations may not be visible).
  QList<pqView*> getViews() const;

  /// This method updates all render modules to which all  
  /// representations for this source belong, if force is true, it for an 
  /// immediate render otherwise render on idle.
  void renderAllViews(bool force=false);

  /// Returns the current data information at this output port. 
  /// If \c update is true, then the pipeline is updated before obtaining the
  /// data information, otherwise the current data information is returned.
  vtkPVDataInformation* getDataInformation(bool update) const;

signals:
  /// Fired when a connection is added between this output port and a consumer.
  void connectionAdded(pqOutputPort* port, pqPipelineSource* consumer);
  void preConnectionAdded(pqOutputPort* port, pqPipelineSource* consumer);

  /// Fired when a connection is removed between this output port and a consumer.
  void connectionRemoved(pqOutputPort* port, pqPipelineSource* consumer);
  void preConnectionRemoved(pqOutputPort* port, pqPipelineSource* consumer);

  /// fired when a representation is added.
  void representationAdded(pqOutputPort* source, pqDataRepresentation* repr);

  /// fired when a representation is removed.
  void representationRemoved(pqOutputPort* source, pqDataRepresentation* repr);

  /// Fired when the visbility of a representation for the source changes.
  /// Also fired when representationAdded or representationRemoved is fired
  /// since that too implies change in source visibility.
  void visibilityChanged(pqOutputPort* source, pqDataRepresentation* repr);

protected slots:
  void onRepresentationVisibilityChanged();

protected:
  friend class pqPipelineFilter;
  friend class pqDataRepresentation;

  /// called by pqPipelineSource when the connections change.
  void removeConsumer(pqPipelineSource *);
  void addConsumer(pqPipelineSource*);

  /// Called by pqPipelineSource when the representations are added/removed.
  void addRepresentation(pqDataRepresentation*);
  void removeRepresentation(pqDataRepresentation*);

  pqPipelineSource* Source;
  int PortNumber;

private:
  pqOutputPort(const pqOutputPort&); // Not implemented.
  void operator=(const pqOutputPort&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};

#endif


