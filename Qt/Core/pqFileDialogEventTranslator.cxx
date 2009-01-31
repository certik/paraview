/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqFileDialogEventTranslator.cxx,v $

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

#include "pqFileDialogEventTranslator.h"
#include "pqCoreTestUtility.h"

#include <pqFileDialog.h>

#include <QEvent>
#include <QtDebug>

pqFileDialogEventTranslator::pqFileDialogEventTranslator(QObject* p) 
  : pqWidgetEventTranslator(p),
  CurrentObject(0)
{
}

bool pqFileDialogEventTranslator::translateEvent(QObject* Object, QEvent* Event, bool& /*Error*/)
{
  // Capture input for pqFileDialog and all its children ...
  pqFileDialog* object = 0;
  for(QObject* o = Object; o; o = o->parent())
    {
    object = qobject_cast<pqFileDialog*>(o);
    if(object)
      break;
    }
  if(!object)
    return false;

  // Handle remaining enter / leave events for the dialog only ...
  if(object == Object)
    {
    switch(Event->type())
      {
      case QEvent::Enter:
        this->CurrentObject = object;
        connect(object, SIGNAL(fileAccepted(const QString&)), this, SLOT(onFilesSelected(const QString&)));
        connect(object, SIGNAL(rejected()), this, SLOT(onCancelled()));
        break;
      case QEvent::Leave:
        disconnect(Object, 0, this, 0);
        this->CurrentObject = 0;
        break;
      default:
        break;
      }
    }
      
  return true;
}

void pqFileDialogEventTranslator::onFilesSelected(const QString& file)
{
  const QString data_directory = pqCoreTestUtility::DataRoot();
  if(data_directory.isEmpty())
    {
    qCritical() << "You must set the PARAVIEW_DATA_ROOT environment variable to play-back file selections.";
    return;
    }

  QString cleanedFile = file;
  cleanedFile.replace('\\', '/');
  
  if(cleanedFile.indexOf(data_directory, 0, Qt::CaseInsensitive) == 0)
    {
    cleanedFile.replace(data_directory, "$PARAVIEW_DATA_ROOT", Qt::CaseInsensitive);
    }
  else
    {
    qCritical() << "You must choose a file under the PARAVIEW_DATA_ROOT directory to record file selections.";
    return;
    }
  
  emit recordEvent(this->CurrentObject, "filesSelected", cleanedFile);
}

void pqFileDialogEventTranslator::onCancelled()
{
  emit recordEvent(this->CurrentObject, "cancelled", "");
}
