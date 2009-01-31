/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqFlatTreeViewEventTranslator.cxx,v $

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

#include "pqFlatTreeViewEventTranslator.h"

#include "pqFlatTreeView.h"
#include <QItemSelectionModel>
#include <QEvent>

static const QString str(const QModelIndex& index)
{
  QString result;
  for(QModelIndex i = index; i.isValid(); i = i.parent())
    {
    result = "/" + QString().setNum(i.row()) + result;
    }
  
  if(index.isValid())
    {
    result = result + "|" + QString().setNum(index.column());
    }
  
  return result;
}

pqFlatTreeViewEventTranslator::pqFlatTreeViewEventTranslator(QObject* p)
  : pqWidgetEventTranslator(p),
  CurrentObject(0)
{
}

bool pqFlatTreeViewEventTranslator::translateEvent(QObject* Object, QEvent* Event, bool& /*Error*/)
{
  pqFlatTreeView* const object = qobject_cast<pqFlatTreeView*>(Object);
  if(!object)
    return false;
    
  switch(Event->type())
    {
    case QEvent::Enter:
      this->CurrentObject = object;
      connect(object->getSelectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(onCurrentChanged(const QModelIndex&, const QModelIndex&)));
      return true;
      break;
    case QEvent::Leave:
      disconnect(Object, 0, this, 0);
      disconnect(object->getSelectionModel(), 0, this, 0);
      this->CurrentObject = 0;
      return true;
      break;
    default:
      break;
    }

  return false;
}

void pqFlatTreeViewEventTranslator::onCurrentChanged(const QModelIndex& current, const QModelIndex& /*previous*/)
{
  emit recordEvent(this->CurrentObject, "currentChanged", str(current));
}

