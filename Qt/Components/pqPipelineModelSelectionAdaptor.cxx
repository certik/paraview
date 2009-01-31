/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqPipelineModelSelectionAdaptor.cxx,v $

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
#include "pqPipelineModelSelectionAdaptor.h"

// Qt includes.
#include <QItemSelectionModel>
#include <QtDebug>

// ParaView includes.
#include "pqServerManagerSelectionModel.h"
#include "pqPipelineModel.h"

//-----------------------------------------------------------------------------
pqPipelineModelSelectionAdaptor::pqPipelineModelSelectionAdaptor(
  QItemSelectionModel* pipelineSelectionModel,
    pqServerManagerSelectionModel* smSelectionModel, QObject* _parent/*=0*/)
: pqSelectionAdaptor(pipelineSelectionModel, smSelectionModel, _parent)
{
  if (!qobject_cast<const pqPipelineModel*>(this->getQModel()))
    {
    qDebug() << "QItemSelectionModel must be a selection model for "
      " pqPipelineModel.";
    return;
    }
}

//-----------------------------------------------------------------------------
pqPipelineModelSelectionAdaptor::~pqPipelineModelSelectionAdaptor()
{
}

//-----------------------------------------------------------------------------
QModelIndex pqPipelineModelSelectionAdaptor::mapFromSMModel(
    pqServerManagerModelItem* item) const
{
  const pqPipelineModel* pM = qobject_cast<const pqPipelineModel*>(
    this->getQModel());
  return pM->getIndexFor(item);
}

//-----------------------------------------------------------------------------
pqServerManagerModelItem* pqPipelineModelSelectionAdaptor::mapToSMModel(
    const QModelIndex& index) const
{
  const pqPipelineModel* pM = qobject_cast<const pqPipelineModel*>(
    this->getQModel());
  return pM->getItemFor(index); 
}

