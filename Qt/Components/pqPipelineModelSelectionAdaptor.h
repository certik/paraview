/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqPipelineModelSelectionAdaptor.h,v $

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
#ifndef __pqPipelineModelSelectionAdaptor_h
#define __pqPipelineModelSelectionAdaptor_h

#include "pqSelectionAdaptor.h"


// pqPipelineModelSelectionAdaptor is an adaptor that connects a
// QItemSelectionModel for a pqPipelineModel to a 
// pqServerManagerSelectionModel. When the selection in the QItemSelectionModel
// changes, the pqServerManagerSelectionModel will be updated and vice versa.
// Every model implemented on top of pqServerManagerModel that should
// participate in synchronized selections would typically define such 
// an adaptor.
class PQCOMPONENTS_EXPORT pqPipelineModelSelectionAdaptor : 
  public pqSelectionAdaptor
{
  Q_OBJECT

public:
  pqPipelineModelSelectionAdaptor(QItemSelectionModel* pipelineSelectionModel,
    pqServerManagerSelectionModel* smSelectionModel, QObject* parent=0);
  virtual ~pqPipelineModelSelectionAdaptor();

protected:
  // Maps a pqServerManagerModelItem to an index in the QAbstractItemModel.
  // Subclass must implement this method.
  QModelIndex mapFromSMModel( pqServerManagerModelItem* item) const;

  // Maps a QModelIndex to a pqServerManagerModelItem.
  // Subclass must implement this method.
  virtual pqServerManagerModelItem* mapToSMModel(
    const QModelIndex& index) const;

};


#endif

