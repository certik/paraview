/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqOutputPort.cxx,v $

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
#include "pqOutputPort.h"

// Server Manager Includes.
#include "vtkPVDataInformation.h"
#include "vtkSMCompoundProxy.h"
#include "vtkSMSourceProxy.h"

// Qt Includes.
#include <QtDebug>
#include <QList>

// ParaView Includes.
#include "pqDataRepresentation.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqTimeKeeper.h"
#include "pqView.h"

class pqOutputPort::pqInternal
{
public:
  QList<pqPipelineSource*> Consumers;
  QList<pqDataRepresentation*> Representations;
};

//-----------------------------------------------------------------------------
pqOutputPort::pqOutputPort(pqPipelineSource* source, int portno):
  Superclass(source),
  Source(source),
  PortNumber(portno)
{
  this->Internal = new pqInternal();

  /// Fire visibility changed signals when representations are added/removed.
  QObject::connect(this, 
    SIGNAL(representationAdded(pqOutputPort*, pqDataRepresentation*)),
    this, SIGNAL(visibilityChanged(pqOutputPort*, pqDataRepresentation*)));
  QObject::connect(this, 
    SIGNAL(representationRemoved(pqOutputPort*, pqDataRepresentation*)),
    this, SIGNAL(visibilityChanged(pqOutputPort*, pqDataRepresentation*)));
}

//-----------------------------------------------------------------------------
pqOutputPort::~pqOutputPort()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
pqServer* pqOutputPort::getServer() const
{
  return this->Source? this->Source->getServer():0;
}

//-----------------------------------------------------------------------------
vtkPVDataInformation* pqOutputPort::getDataInformation(bool update) const
{
  vtkSMCompoundProxy* compoundProxy = vtkSMCompoundProxy::SafeDownCast(
    this->getSource()->getProxy());

  vtkSMSourceProxy* source = compoundProxy?
    vtkSMSourceProxy::SafeDownCast(compoundProxy->GetConsumableProxy()):
    vtkSMSourceProxy::SafeDownCast(this->getSource()->getProxy());

  if (!source)
    {
    return 0;
    }


  if (update)
    {
    pqTimeKeeper* timekeeper = this->getSource()->getServer()->getTimeKeeper();
    double time = timekeeper->getTime();
    source->UpdatePipeline(time);
    }

  return source->GetDataInformation(this->PortNumber, false);
}

//-----------------------------------------------------------------------------
int pqOutputPort::getNumberOfConsumers() const
{
  return this->Internal->Consumers.size();
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqOutputPort::getConsumer(int index) const
{
  if (index < 0 || index >= this->Internal->Consumers.size())
    {
    qCritical() << "Invalid index: " << index ;
    return 0;
    }

  return this->Internal->Consumers[index];
}

//-----------------------------------------------------------------------------
void pqOutputPort::addConsumer(pqPipelineSource* cons)
{
  if (!this->Internal->Consumers.contains(cons))
    {
    emit this->preConnectionAdded(this, cons);
    this->Internal->Consumers.push_back(cons);
    emit this->connectionAdded(this, cons);
    }
}

//-----------------------------------------------------------------------------
void pqOutputPort::removeConsumer(pqPipelineSource* cons)
{
  if (this->Internal->Consumers.contains(cons))
    {
    emit this->preConnectionRemoved(this, cons);
    this->Internal->Consumers.removeAll(cons);
    emit this->connectionRemoved(this, cons);
    }
}

//-----------------------------------------------------------------------------
void pqOutputPort::addRepresentation(pqDataRepresentation* repr)
{
  if (!this->Internal->Representations.contains(repr))
    {
    QObject::connect(repr, SIGNAL(visibilityChanged(bool)),
      this, SLOT(onRepresentationVisibilityChanged()));

    this->Internal->Representations.push_back(repr);
    emit this->representationAdded(this, repr);
    }
}

//-----------------------------------------------------------------------------
void pqOutputPort::removeRepresentation(pqDataRepresentation* repr)
{
  this->Internal->Representations.removeAll(repr);
  QObject::disconnect(repr, 0, this, 0);
  emit this->representationRemoved(this, repr);
}

//-----------------------------------------------------------------------------
void pqOutputPort::onRepresentationVisibilityChanged()
{
  emit this->visibilityChanged(this, 
    qobject_cast<pqDataRepresentation*>(this->sender()));
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqOutputPort::getRepresentation(pqView* view) const
{
  if (view)
    {
    foreach (pqDataRepresentation* repr, this->Internal->Representations)
      {
      if (repr && (!view || repr->getView() == view))
        {
        return repr;
        }
      }
    }

  return 0;
}

//-----------------------------------------------------------------------------
QList<pqDataRepresentation*> pqOutputPort::getRepresentations(pqView* view) const
{
  QList<pqDataRepresentation*> list;
  foreach (pqDataRepresentation* repr, this->Internal->Representations)
    {
    if (repr && (!view || repr->getView() == view))
      {
      list.push_back(repr);
      }
    }

  return list;
}

//-----------------------------------------------------------------------------
QList<pqView*> pqOutputPort::getViews() const
{
  QList<pqView*> views;
  foreach(pqDataRepresentation* repr, this->Internal->Representations)
    {
    if (repr)
      {
      pqView* view= repr->getView();
      if (view && !views.contains(view))
        {
        views.push_back(view);
        }
      }
    }

  return views; 
}

//-----------------------------------------------------------------------------
void pqOutputPort::renderAllViews(bool force /*=false*/)
{
  QList<pqView*> views = this->getViews();
  foreach(pqView* view, views)
    {
    if (force)
      {
      view->forceRender();
      }
    else
      {
      view->render();
      }
    }
}

