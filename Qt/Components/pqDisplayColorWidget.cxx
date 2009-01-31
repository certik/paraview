/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqDisplayColorWidget.cxx,v $

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

#include "pqDisplayColorWidget.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVGeometryInformation.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMStringVectorProperty.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QList>
#include <QRegExp>
#include <QtDebug>
#include <QTimer>

#include "pqApplicationCore.h"
#include "pqPipelineRepresentation.h"
#include "pqUndoStack.h"

//-----------------------------------------------------------------------------
pqDisplayColorWidget::pqDisplayColorWidget( QWidget *p ) :
  QWidget( p ),
  BlockEmission(false)
{
  this->CellDataIcon = new QIcon(":/pqWidgets/Icons/pqCellData16.png");
  this->PointDataIcon = new QIcon(":/pqWidgets/Icons/pqPointData16.png");
  this->SolidColorIcon = new QIcon(":/pqWidgets/Icons/pqSolidColor16.png");

  this->Layout  = new QHBoxLayout( this );
  this->Layout->setMargin(0);
  this->Layout->setSpacing(6);

  this->Variables = new QComboBox( this );
  this->Variables->setMaxVisibleItems(60);
  this->Variables->setObjectName("Variables");
  this->Variables->setMinimumSize( QSize( 150, 0 ) );

  this->Layout->setMargin( 0 );
  this->Layout->setSpacing( 1 );
  this->Layout->addWidget(this->Variables);


  QObject::connect(this->Variables, SIGNAL(currentIndexChanged(int)), 
    SLOT(onVariableActivated(int)));
  QObject::connect(this, 
    SIGNAL(variableChanged(pqVariableType, const QString&)),
    this,
    SLOT(onVariableChanged(pqVariableType, const QString&)));

  this->VTKConnect = vtkEventQtSlotConnect::New();

  pqUndoStack* stack = pqApplicationCore::instance()->getUndoStack();
  if (stack)
    {
    QObject::connect(this, SIGNAL(begin(const QString&)),
      stack, SLOT(beginUndoSet(const QString&)));
    QObject::connect(this, SIGNAL(end()),
      stack, SLOT(endUndoSet()));
    }
}

//-----------------------------------------------------------------------------
pqDisplayColorWidget::~pqDisplayColorWidget()
{
  delete this->Layout;
  delete this->Variables;
  delete this->CellDataIcon;
  delete this->PointDataIcon;
  delete this->SolidColorIcon;
  
  this->Layout = 0;
  this->Variables = 0;
  this->VTKConnect->Delete();
}

//-----------------------------------------------------------------------------
QString pqDisplayColorWidget::getCurrentText() const
{
  return this->Variables->currentText();
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::clear()
{
  this->BlockEmission = true;
  this->Variables->clear();
  this->BlockEmission = false;
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::addVariable(pqVariableType type, 
  const QString& name)
{
  // Don't allow duplicates to creep in ...
  if(-1 != this->Variables->findData(this->variableData(type, name)))
    {
    return;
    }

  bool old_value = this->BlockEmission;
  this->BlockEmission = true;
  switch(type)
    {
    case VARIABLE_TYPE_NONE:
      this->Variables->addItem(*this->SolidColorIcon,
        "Solid Color", this->variableData(type, name));
      break;
    case VARIABLE_TYPE_NODE:
      this->Variables->addItem(*this->PointDataIcon, name, 
        this->variableData(type, name));
      break;
    case VARIABLE_TYPE_CELL:
      this->Variables->addItem(*this->CellDataIcon,
        name, this->variableData(type, name));
      break;
    }
  this->BlockEmission = old_value;
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::chooseVariable(pqVariableType type, 
  const QString& name)
{
  const int row = this->Variables->findData(variableData(type, name));
  if(row != -1)
    {
    this->Variables->setCurrentIndex(row);
    }
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::onVariableActivated(int row)
{
  if(this->BlockEmission)
    {
    return;
    }

  const QStringList d = this->Variables->itemData(row).toString().split("|");
  if(d.size() != 2)
    return;
    
  pqVariableType type = VARIABLE_TYPE_NONE;
  if(d[1] == "cell")
    {
    type = VARIABLE_TYPE_CELL;
    }
  else if(d[1] == "point")
    {
    type = VARIABLE_TYPE_NODE;
    }
    
  const QString name = d[0];
  
  emit variableChanged(type, name);
  emit this->modified();
}

//-----------------------------------------------------------------------------
const QString pqDisplayColorWidget::variableData(pqVariableType type, 
  const QString& name)
{
  switch(type)
    {
    case VARIABLE_TYPE_NONE:
      return name + "|none";
    case VARIABLE_TYPE_NODE:
      return name + "|point";
    case VARIABLE_TYPE_CELL:
      return name + "|cell";
    }
    
  return QString();
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::onVariableChanged(pqVariableType type, 
  const QString& name)
{
  pqPipelineRepresentation* display = this->getRepresentation();
  if (display)
    {
    emit this->begin("Color Change");
    switch(type)
      {
    case VARIABLE_TYPE_NONE:
      display->colorByArray(NULL, 0);
      break;
    case VARIABLE_TYPE_NODE:
      display->colorByArray(name.toAscii().data(),
        vtkSMDataRepresentationProxy::POINT_DATA);
      break;
    case VARIABLE_TYPE_CELL:
      display->colorByArray(name.toAscii().data(), 
        vtkSMDataRepresentationProxy::CELL_DATA);
      break;
      }
    emit this->end();
    display->renderViewEventually();
    }
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::updateGUI()
{
  this->BlockEmission = true;
  pqPipelineRepresentation* display = this->getRepresentation();
  if (display)
    {
    int index = this->AvailableArrays.indexOf(display->getColorField());
    if (index < 0)
      {
      index = 0;
      }
    this->Variables->setCurrentIndex(index);
    }
  this->BlockEmission = false;
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::setRepresentation(pqDataRepresentation* display) 
{
  if(display == this->Display)
    {
    return;
    }

  if (this->Display)
    {
    QObject::disconnect(this->Display, 0, this, 0);
    }

  this->VTKConnect->Disconnect();
  this->Display = qobject_cast<pqPipelineRepresentation*>(display);
  if(this->Display)
    {
    vtkSMProxy* repr = this->Display->getProxy();
    this->VTKConnect->Connect(repr->GetProperty("ColorAttributeType"),
      vtkCommand::ModifiedEvent, this, SLOT(reloadGUI()),
      NULL, 0.0,
      Qt::QueuedConnection);
    this->VTKConnect->Connect(repr->GetProperty("ColorArrayName"),
      vtkCommand::ModifiedEvent, this, SLOT(reloadGUI()),
      NULL, 0.0,
      Qt::QueuedConnection);
    this->VTKConnect->Connect(
      repr->GetProperty("Representation"), vtkCommand::ModifiedEvent, 
      this, SLOT(reloadGUI()),
      NULL, 0.0,
      Qt::QueuedConnection);

    // Every time the display updates, it is possible that the arrays available for 
    // coloring have changed, hence we reload the list.
    QObject::connect(this->Display, SIGNAL(updated()), 
      this, SLOT(reloadGUI()), Qt::QueuedConnection);
    }
  QTimer::singleShot(0, this, SLOT(reloadGUI()));
}

//-----------------------------------------------------------------------------
pqPipelineRepresentation* pqDisplayColorWidget::getRepresentation() const
{
  return this->Display;
}

//-----------------------------------------------------------------------------
void pqDisplayColorWidget::reloadGUI()
{
  this->BlockEmission = true;
  this->clear();

  pqPipelineRepresentation* display = this->getRepresentation();
  if (!display)
    {
    this->addVariable(VARIABLE_TYPE_NONE, "Solid Color");
    this->BlockEmission = false;
    this->setEnabled(false);
    return;
    }
  this->setEnabled(true);

  this->AvailableArrays = display->getColorFields();
  QRegExp regExpCell(" \\(cell\\)\\w*$");
  QRegExp regExpPoint(" \\(point\\)\\w*$");
  foreach(QString arrayName, this->AvailableArrays)
    {
    if (arrayName == "Solid Color")
      {
      this->addVariable(VARIABLE_TYPE_NONE, arrayName);
      }
    else if (regExpCell.indexIn(arrayName) != -1)
      {
      arrayName = arrayName.replace(regExpCell, "");
      this->addVariable(VARIABLE_TYPE_CELL, arrayName);
      }
    else if (regExpPoint.indexIn(arrayName) != -1)
      {
      arrayName = arrayName.replace(regExpPoint, "");
      this->addVariable(VARIABLE_TYPE_NODE, arrayName);
      }
    }

  this->BlockEmission = false;
  this->updateGUI();

  emit this->modified();
}

