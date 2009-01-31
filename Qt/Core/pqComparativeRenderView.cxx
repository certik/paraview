/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqComparativeRenderView.cxx,v $

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
#include "pqComparativeRenderView.h"

// Server Manager Includes.
#include "QVTKWidget.h"
#include "vtkCollection.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkPVServerInformation.h"
#include "vtkSmartPointer.h"
#include "vtkSMComparativeViewProxy.h"
#include "vtkSMRenderViewProxy.h"

// Qt Includes.
#include <QMap>
#include <QPointer>
#include <QSet>
#include <QGridLayout>

// ParaView Includes.
#include "pqServer.h"
#include "pqSMAdaptor.h"

class pqComparativeRenderView::pqInternal
{
public:
  QMap<vtkSMViewProxy*, QPointer<QVTKWidget> > RenderWidgets;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;

  pqInternal()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    }
};

//-----------------------------------------------------------------------------
pqComparativeRenderView::pqComparativeRenderView(
  const QString& group,
  const QString& name, 
  vtkSMViewProxy* viewProxy,
  pqServer* server, 
  QObject* _parent):
  Superclass(group, name, viewProxy, server, _parent)
{
  this->Internal = new pqInternal();
  this->Internal->VTKConnect->Connect(
    viewProxy, vtkCommand::ConfigureEvent,
    this, SLOT(onComparativeVisLayoutChanged()));
}

//-----------------------------------------------------------------------------
pqComparativeRenderView::~pqComparativeRenderView()
{
  foreach (QVTKWidget* widget, this->Internal->RenderWidgets.values())
    {
    delete widget;
    }

  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqComparativeRenderView::initialize()
{
  this->Superclass::initialize();
  this->onComparativeVisLayoutChanged();
}

//-----------------------------------------------------------------------------
void pqComparativeRenderView::setDefaultPropertyValues()
{
  //this->getComparativeRenderViewProxy()->Build(3, 3);
  this->Superclass::setDefaultPropertyValues();

  vtkPVServerInformation* serverInfo = this->getServer()->getServerInformation();
  if (serverInfo && serverInfo->GetTileDimensions()[0])
    {
    // change default layout to match the tile displays.
    pqSMAdaptor::setMultipleElementProperty(
      this->getProxy()->GetProperty("Dimensions"), 0, 
      serverInfo->GetTileDimensions()[0]);
    pqSMAdaptor::setMultipleElementProperty(
      this->getProxy()->GetProperty("Dimensions"), 1, 
      serverInfo->GetTileDimensions()[1]);
    this->getProxy()->UpdateVTKObjects();
    }

}

//-----------------------------------------------------------------------------
QWidget* pqComparativeRenderView::createWidget() 
{
  QWidget* widget = new QWidget();
  return widget;
}

//-----------------------------------------------------------------------------
vtkSMComparativeViewProxy* pqComparativeRenderView::getComparativeRenderViewProxy() const
{
  return vtkSMComparativeViewProxy::SafeDownCast(this->getProxy());
}

//-----------------------------------------------------------------------------
vtkSMRenderViewProxy* pqComparativeRenderView::getRenderViewProxy() const
{
  return vtkSMRenderViewProxy::SafeDownCast(
    this->getComparativeRenderViewProxy()->GetRootView());
}

//-----------------------------------------------------------------------------
void pqComparativeRenderView::onComparativeVisLayoutChanged()
{
  // Create QVTKWidgets for new view modules and destroy old ones.
  vtkCollection* currentViews =  vtkCollection::New();
  
  vtkSMComparativeViewProxy* compView = vtkSMComparativeViewProxy::SafeDownCast(
    this->getProxy());
  compView->GetViews(currentViews);

  QSet<vtkSMViewProxy*> currentViewsSet;

  currentViews->InitTraversal();
  vtkSMViewProxy* temp = vtkSMViewProxy::SafeDownCast(
    currentViews->GetNextItemAsObject());
  for (; temp !=0; temp = vtkSMViewProxy::SafeDownCast(currentViews->GetNextItemAsObject()))
    {
    currentViewsSet.insert(temp);
    }

  QSet<vtkSMViewProxy*> oldViews = QSet<vtkSMViewProxy*>::fromList(
    this->Internal->RenderWidgets.keys());
  
  QSet<vtkSMViewProxy*> removed = oldViews - currentViewsSet;
  QSet<vtkSMViewProxy*> added = currentViewsSet - oldViews;

  // Destroy old QVTKWidgets widgets.
  foreach (vtkSMViewProxy* key, removed)
    {
    QVTKWidget* item = this->Internal->RenderWidgets.take(key);
    delete item;
    }

  // Create QVTKWidgets for new ones.
  foreach (vtkSMViewProxy* key, added)
    {
    vtkSMRenderViewProxy* renView = vtkSMRenderViewProxy::SafeDownCast(key);
    renView->UpdateVTKObjects();

    QVTKWidget* widget = new QVTKWidget();
    widget->SetRenderWindow(renView->GetRenderWindow());
    widget->installEventFilter(this);
    widget->setContextMenuPolicy(Qt::NoContextMenu);
    this->Internal->RenderWidgets[key] = widget;
    }

  // Now layout the views.
  int dimensions[2];
  compView->GetDimensions(dimensions);

  // destroy the old layout and create a new one. 
  QWidget* widget = this->getWidget();
  delete widget->layout();

  QGridLayout* layout = new QGridLayout(widget);
  layout->setSpacing(1);
  layout->setMargin(0);
  for (int x=0; x < dimensions[0]; x++)
    {
    for (int y=0; y < dimensions[1]; y++)
      {
      int index = y*dimensions[0]+x;
      vtkSMViewProxy* view = vtkSMViewProxy::SafeDownCast(
        currentViews->GetItemAsObject(index));
      QVTKWidget* vtkwidget = this->Internal->RenderWidgets[view];
      layout->addWidget(vtkwidget, y, x);
      }
    }
  
  currentViews->Delete();
}
