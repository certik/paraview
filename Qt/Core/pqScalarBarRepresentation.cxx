/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqScalarBarRepresentation.cxx,v $

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
#include "pqScalarBarRepresentation.h"

#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"

#include <QtDebug>
#include <QPointer>
#include <QRegExp>

#include "pqApplicationCore.h"
#include "pqPipelineRepresentation.h"
#include "pqProxy.h"
#include "pqScalarsToColors.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"

//-----------------------------------------------------------------------------
class pqScalarBarRepresentation::pqInternal
{
public:
  QPointer<pqScalarsToColors> LookupTable;
  vtkEventQtSlotConnect* VTKConnect;
};

//-----------------------------------------------------------------------------
pqScalarBarRepresentation::pqScalarBarRepresentation(const QString& group, const QString& name,
    vtkSMProxy* scalarbar, pqServer* server,
    QObject* _parent)
: pqRepresentation(group, name, scalarbar, server, _parent)
{
  this->AutoHidden = false;
  this->Internal = new pqScalarBarRepresentation::pqInternal();

  this->Internal->VTKConnect = vtkEventQtSlotConnect::New();
  this->Internal->VTKConnect->Connect(scalarbar->GetProperty("LookupTable"),
    vtkCommand::ModifiedEvent, this, SLOT(onLookupTableModified()));

  // load default values.
  this->onLookupTableModified();
}

//-----------------------------------------------------------------------------
pqScalarBarRepresentation::~pqScalarBarRepresentation()
{
  if (this->Internal->LookupTable)
    {
    this->Internal->LookupTable->removeScalarBar(this);
    this->Internal->LookupTable = 0;
    }

  this->Internal->VTKConnect->Disconnect();
  this->Internal->VTKConnect->Delete();
  delete this->Internal;
}

//-----------------------------------------------------------------------------
pqScalarsToColors* pqScalarBarRepresentation::getLookupTable() const
{
  return this->Internal->LookupTable;
}

//-----------------------------------------------------------------------------
void pqScalarBarRepresentation::onLookupTableModified()
{
  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();

  vtkSMProxy* curLUTProxy = 
    pqSMAdaptor::getProxyProperty(this->getProxy()->GetProperty("LookupTable"));
  pqScalarsToColors* curLUT = smmodel->findItem<pqScalarsToColors*>(curLUTProxy);

  if (curLUT == this->Internal->LookupTable)
    {
    return;
    }

  if (this->Internal->LookupTable)
    {
    this->Internal->LookupTable->removeScalarBar(this);
    }

  this->Internal->LookupTable = curLUT;
  if (this->Internal->LookupTable)
    {
    this->Internal->LookupTable->addScalarBar(this);
    }
}

//-----------------------------------------------------------------------------
QPair<QString, QString> pqScalarBarRepresentation::getTitle() const
{
  QString title = pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("Title")).toString();
  QRegExp reg("(.*)\\b(Magnitude|X|Y|Z|[0-9]+)\\b");
  if (!reg.exactMatch(title))
    {
    return QPair<QString, QString>(title, "");
    }
  return QPair<QString, QString>(reg.cap(1), reg.cap(2));
}

//-----------------------------------------------------------------------------
void pqScalarBarRepresentation::setTitle(const QString& name, const QString& comp)
{
  if (QPair<QString, QString>(name, comp) == this->getTitle())
    {
    return;
    }

  pqSMAdaptor::setElementProperty(this->getProxy()->GetProperty("Title"),
    (name + " " + comp).trimmed());
  this->getProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqScalarBarRepresentation::makeTitle(pqPipelineRepresentation* display)
{
  if (!this->Internal->LookupTable)
    {
    qDebug() << "Cannot setup title when not connected to any LUT.";
    return;
    }

  QString arrayname = display->getColorField(true);
  if (arrayname == "Solid Color" || arrayname == "")
    {
    // cannot decide title without array name.
    return;
    }

  pqScalarsToColors::Mode mode = this->Internal->LookupTable->getVectorMode();
  int component_no = this->Internal->LookupTable->getVectorComponent();
  int num_components = display->getColorFieldNumberOfComponents(
    display->getColorField(false));

  QString component = (num_components > 1)? "Magnitude" : "";
  if (num_components > 1 && 
    mode == pqScalarsToColors::COMPONENT && component_no >= 0)
    {
    if (num_components <= 3 && component_no < 3)
      {
      const char* titles[] = { "X", "Y", "Z"};
      component = titles[component_no];
      }
    else
      {
      component = QString::number(component_no);
      }
    }
  this->setTitle(arrayname, component);
}
