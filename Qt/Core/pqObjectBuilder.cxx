/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqObjectBuilder.cxx,v $

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
#include "pqObjectBuilder.h"

#include "vtkProcessModuleConnectionManager.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSMCompoundProxy.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

#include <QtDebug>
#include <QFileInfo>

#include "pqAnimationScene.h"
#include "pqApplicationCore.h"
#include "pqComparativeRenderView.h"
#include "pqDataRepresentation.h"
#include "pqNameCount.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqPluginManager.h"
#include "pqRenderView.h"
#include "pqScalarBarRepresentation.h"
#include "pqScalarsToColors.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"
#include "pqView.h"
#include "pqViewModuleInterface.h"

//-----------------------------------------------------------------------------
pqObjectBuilder::pqObjectBuilder(QObject* _parent/*=0*/) :QObject(_parent)
{
  this->NameGenerator = new pqNameCount();
}

//-----------------------------------------------------------------------------
pqObjectBuilder::~pqObjectBuilder()
{
  delete this->NameGenerator;
}


//-----------------------------------------------------------------------------
pqPipelineSource* pqObjectBuilder::createSource(const QString& sm_group,
    const QString& sm_name, pqServer* server)
{
  vtkSMProxy* proxy = 
    this->createProxyInternal(sm_group, sm_name, server, "sources");
  if (proxy)
    {
    pqPipelineSource* source = pqApplicationCore::instance()->
      getServerManagerModel()->findItem<pqPipelineSource*>(proxy);

    // initialize the source.
    source->setDefaultPropertyValues();
    source->setModifiedState(pqProxy::UNINITIALIZED);

    emit this->sourceCreated(source);
    emit this->proxyCreated(source);
    return source;
    }
  return 0;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqObjectBuilder::createFilter(
  const QString& group, const QString& name, pqPipelineSource* input)
{
  QMap<QString, QList<pqOutputPort*> > namedInputs;
  QList<pqOutputPort*> inputs;
  inputs.push_back(input->getOutputPort(0));
  namedInputs["Input"] = inputs;

  return this->createFilter(group, name, namedInputs, input->getServer());
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqObjectBuilder::createFilter(
  const QString& group, const QString& name,
  QMap<QString, QList<pqOutputPort*> > namedInputs,
  pqServer* server)
{
  vtkSMProxy* proxy = 
    this->createProxyInternal(group, name, server, "sources");
  if (!proxy)
    {
    return 0;
    }

  pqPipelineSource* filter = pqApplicationCore::instance()->getServerManagerModel()->
    findItem<pqPipelineSource*>(proxy);
  if (!filter)
    {
    qDebug() << "Failed to locate pqPipelineSource for the created proxy "
      << group << ", " << name;
    return 0;
    }

  // Now for every input port, connect the inputs.
  QMap<QString, QList<pqOutputPort*> >::iterator mapIter;
  for (mapIter = namedInputs.begin(); mapIter != namedInputs.end(); ++mapIter)
    {
    QString input_port_name = mapIter.key();
    QList<pqOutputPort*> &inputs = mapIter.value();

    vtkSMProperty* prop = proxy->GetProperty(input_port_name.toAscii().data());
    if (!prop)
      {
      qCritical() << "Failed to locate input property "<< input_port_name;
      continue;
      }

    foreach (pqOutputPort* opPort, inputs)
      {
      pqSMAdaptor::addInputProperty(prop, opPort->getSource()->getProxy(),
        opPort->getPortNumber());
      }

    proxy->UpdateVTKObjects();
    prop->UpdateDependentDomains();
    }

  // Set default property values.
  filter->setDefaultPropertyValues();
  filter->setModifiedState(pqProxy::UNINITIALIZED);

  emit this->filterCreated(filter);
  emit this->proxyCreated(filter);
  return filter;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqObjectBuilder::createCustomFilter(const QString& sm_name,
    pqServer* server, pqPipelineSource* input/*=0*/)
{
  vtkSMProxy* proxy = 
    this->createProxyInternal(QString(), sm_name, server, "sources");
  if (!proxy)
    {
    return 0;
    }

  pqPipelineSource* filter = pqApplicationCore::instance()->
    getServerManagerModel()->findItem<pqPipelineSource*>(proxy);
  if (!filter)
    {
    qDebug() << "Failed to locate pqPipelineSource for the created custom filter proxy "
      << sm_name;
    return 0;
    }

  vtkSMProperty* inputProperty = proxy->GetProperty("Input");
  if (inputProperty && input)
    {
    pqSMAdaptor::setProxyProperty(inputProperty, input->getProxy());
    proxy->UpdateVTKObjects();
    inputProperty->UpdateDependentDomains();
    }

  // Set default property values.
  filter->setDefaultPropertyValues();
  filter->setModifiedState(pqProxy::UNINITIALIZED);
  emit this->customFilterCreated(filter);
  emit this->proxyCreated(filter);
  return filter;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqObjectBuilder::createReader(const QString& sm_group,
    const QString& sm_name, const QStringList& files, pqServer* server)
{
  if (files.empty())
    {
    return 0;
    }

  QFileInfo fileInfo(files[0]);

  vtkSMProxy* proxy = 
    this->createProxyInternal(sm_group, sm_name, server, "sources", 
      fileInfo.fileName());
  if (!proxy)
    {
    return 0;
    }

  pqPipelineSource* reader = pqApplicationCore::instance()->
    getServerManagerModel()->findItem<pqPipelineSource*>(proxy);
  if (!reader)
    {
    qDebug() << "Failed to locate pqPipelineSource for the created proxy "
      << sm_group << ", " << sm_name;
    return 0;
    }

  QString pname = this->getFileNamePropertyName(proxy);
  if (!pname.isEmpty())
    {
    vtkSMStringVectorProperty* prop = 
      vtkSMStringVectorProperty::SafeDownCast(
        proxy->GetProperty(pname.toAscii().data()));
    if (!prop)
      {
      return 0;
      }

    unsigned int numElems = files.size();
    if (numElems == 1 || !prop->GetRepeatCommand())
      {
      pqSMAdaptor::setElementProperty(prop, files[0]);
      }
    else
      {
      QList<QVariant> values;
      foreach (QString file, files)
        {
        values.push_back(file);
        }
      pqSMAdaptor::setMultipleElementProperty(prop, values);
      }
    proxy->UpdateVTKObjects();
    prop->UpdateDependentDomains();
    }
  reader->setDefaultPropertyValues();
  reader->setModifiedState(pqProxy::UNINITIALIZED);

  emit this->readerCreated(reader, files[0]);
  emit this->proxyCreated(reader);
  return reader;
}
//-----------------------------------------------------------------------------
void pqObjectBuilder::destroy(pqPipelineSource* source)
{
  if (!source)
    {
    qDebug() << "Cannot remove null source.";
    return;
    }

  if (source->getNumberOfConsumers())
    {
    qDebug() << "Cannot remove source with consumers.";
    return;
    }

  emit this->destroying(source);

  // * remove inputs.
  // TODO: this step should not be necessary, but it currently
  // is :(. Needs some looking into.
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    source->getProxy()->GetProperty("Input"));
  if (pp)
    {
    pp->RemoveAllProxies();
    }

  // * remove all representations.
  QList<pqDataRepresentation*> reprs = source->getRepresentations(0);
  foreach (pqDataRepresentation* repr, reprs)
    {
    if (repr)
      {
      this->destroy(repr);
      }
    }

  // * Unregister proxy.
  this->destroyProxyInternal(source);
}

//-----------------------------------------------------------------------------
pqView* pqObjectBuilder::createView(const QString& type,
  pqServer* server)
{
  if (!server)
    {
    qDebug() << "Cannot create view without server.";
    return NULL;
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* proxy= 0;
  if(type == pqRenderView::renderViewType())
    {
    proxy = server->newRenderView();
    }
  else if (type == pqComparativeRenderView::comparativeRenderViewType())
    {
    QString xmlname = server->getRenderViewXMLName();
    xmlname = "Comparative" + xmlname;
    proxy = pxm->NewProxy("views", xmlname.toAscii().data());
    }
  else
    {
    QObjectList ifaces =
      pqApplicationCore::instance()->getPluginManager()->interfaces();
    foreach(QObject* iface, ifaces)
      {
      pqViewModuleInterface* vmi = qobject_cast<pqViewModuleInterface*>(iface);
      if(vmi && vmi->viewTypes().contains(type))
        {
        proxy = vmi->createViewProxy(type);
        break;
        }
      }
    }

  if (!proxy)
    {
    qDebug() << "Failed to create a proxy for the requested view type.";
    return NULL;
    }

  proxy->SetConnectionID(server->GetConnectionID());

  QString name = QString("%1%2").arg(proxy->GetXMLName()).arg(
    this->NameGenerator->GetCountAndIncrement(proxy->GetXMLName()));
  pxm->RegisterProxy("views", name.toAscii().data(), proxy);
  proxy->Delete();

  pqServerManagerModel* model = 
    pqApplicationCore::instance()->getServerManagerModel();

  pqView* view = model->findItem<pqView*>(proxy);
  if (view)
    {
    view->setDefaultPropertyValues();
    emit this->viewCreated(view);
    emit this->proxyCreated(view);
    }
  else
    {
    qDebug() << "Cannot locate the pqView for the " 
      << "view module proxy.";
    }

  return view;
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroy(pqView* view)
{
  if (!view)
    {
    return;
    }

  emit this->destroying(view);

  // Get a list of all reprs belonging to this render module. We delete
  // all the reprs that belong only to this render module.
  QList<pqRepresentation*> reprs = view->getRepresentations();

  // Unregister the proxy....the rest of the GUI will(rather should) manage itself!
  QString name = view->getSMName();

  this->destroyProxyInternal(view);

  // Now clean up any orphan reprs.
  foreach (pqRepresentation* repr, reprs)
    {
    if (repr)
      {
      this->destroyProxyInternal(repr);
      }
    }
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqObjectBuilder::createDataRepresentation(
  pqOutputPort* opPort, pqView* view)
{
  if (!opPort || !view)
    {
    qCritical() <<"Missing required attribute.";
    return NULL;
    }

  if(!view->canDisplay(opPort))
    {
    // View cannot display this source, nothing to do here.
    return NULL;
    }

  vtkSMProxy* reprProxy = 0; 

  pqPipelineSource* source = opPort->getSource();

  // HACK to create correct representation for text sources/filters.
  QString srcProxyName = source->getProxy()->GetXMLName();
  if ( (srcProxyName == "TextSource" || srcProxyName == "TimeToTextConvertor"
      || srcProxyName == "TimeToTextConvertorSource") && 
    qobject_cast<pqRenderView*>(view))
    {
    reprProxy = vtkSMObject::GetProxyManager()->NewProxy(
      "representations", "TextSourceRepresentation");
    }
  else
    {
    reprProxy = view->getViewProxy()->CreateDefaultRepresentation(
      source->getProxy(), opPort->getPortNumber());
    }

  // Could not determine representation proxy to create.
  if (!reprProxy)
    {
    return NULL;
    }

  reprProxy->SetConnectionID(view->getServer()->GetConnectionID());
  
  // (for undo/redo to work).
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  QString name = QString("DataRepresentation%1").arg(
    this->NameGenerator->GetCountAndIncrement("DataRepresentation"));
  pxm->RegisterProxy("representations", name.toAscii().data(), reprProxy);
  reprProxy->Delete();

  vtkSMProxy* viewModuleProxy = view->getProxy();

  // Set the reprProxy's input.
  pqSMAdaptor::setInputProperty(reprProxy->GetProperty("Input"), 
    source->getProxy(), opPort->getPortNumber());
  reprProxy->UpdateVTKObjects();

  // Add the reprProxy to render module.
  pqSMAdaptor::addProxyProperty(
    viewModuleProxy->GetProperty("Representations"), reprProxy);
  viewModuleProxy->UpdateVTKObjects();

  pqApplicationCore* core= pqApplicationCore::instance();
  pqDataRepresentation* repr = core->getServerManagerModel()->
    findItem<pqDataRepresentation*>(reprProxy);
  if (repr)
    {
    repr->setDefaultPropertyValues();

    emit this->dataRepresentationCreated(repr);
    emit this->proxyCreated(repr);
    }
  return repr;
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroy(pqRepresentation* repr)
{
  if (!repr)
    {
    return;
    }

  emit this->destroying(repr);

  // Remove repr from the view module.
  pqView* view = repr->getView();
  if (view)
    {
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      view->getProxy()->GetProperty("Representations"));
    pp->RemoveProxy(repr->getProxy());
    view->getProxy()->UpdateVTKObjects();
    }

  // If this repr has a lookuptable, we hide all scalar bars for that
  // lookup table unless there is some other repr that's using it.
  pqScalarsToColors* stc =0;
  if (pqDataRepresentation* dataRepr = qobject_cast<pqDataRepresentation*>(repr))
    {
    stc = dataRepr->getLookupTable();
    }

  this->destroyProxyInternal(repr);

  if (stc)
    {
    // this hides scalar bars only if the LUT is not used by
    // any other repr. This must happen after the repr has 
    // been deleted.
    stc->hideUnusedScalarBars();
    }
}

//-----------------------------------------------------------------------------
pqScalarBarRepresentation* pqObjectBuilder::createScalarBarDisplay(
    pqScalarsToColors* lookupTable, pqView* view)
{
  if (!lookupTable || !view)
    {
    return 0;
    }

  if (lookupTable->getServer() != view->getServer())
    {
    qCritical() << "LUT and View are on different servers!";
    return 0;
    }

  pqServer* server = view->getServer();
  vtkSMProxy* scalarBarProxy = this->createProxyInternal(
    "representations", "ScalarBarWidgetRepresentation", server, "scalar_bars");

  if (!scalarBarProxy)
    {
    return 0;
    }
  pqScalarBarRepresentation* scalarBar = 
    pqApplicationCore::instance()->getServerManagerModel()->
    findItem<pqScalarBarRepresentation*>(scalarBarProxy);
  pqSMAdaptor::setProxyProperty(scalarBarProxy->GetProperty("LookupTable"),
    lookupTable->getProxy());
  scalarBarProxy->UpdateVTKObjects();

  pqSMAdaptor::addProxyProperty(view->getProxy()->GetProperty("Representations"),
    scalarBarProxy);
  view->getProxy()->UpdateVTKObjects();
  scalarBar->setDefaultPropertyValues();

  emit this->scalarBarDisplayCreated(scalarBar);
  emit this->proxyCreated(scalarBar);
  return scalarBar;
}

//-----------------------------------------------------------------------------
pqAnimationScene* pqObjectBuilder::createAnimationScene(pqServer* server)
{
  vtkSMProxy* proxy = 
    this->createProxyInternal("animation", "AnimationScene", server, "animation");
  if (proxy)
    {
    proxy->SetServers(vtkProcessModule::CLIENT);
    proxy->UpdateVTKObjects();

    pqAnimationScene* scene = pqApplicationCore::instance()->
      getServerManagerModel()->findItem<pqAnimationScene*>(proxy);

    // initialize the scene.
    scene->setDefaultPropertyValues();
    emit this->proxyCreated(scene);
    return scene;
    }

  return 0;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqObjectBuilder::createProxy(const QString& sm_group, 
    const QString& sm_name, pqServer* server, 
    const QString& reg_group, const QString& reg_name/*=QString()*/)
{
  vtkSMProxy* proxy = this->createProxyInternal(
    sm_group, sm_name, server, reg_group, reg_name);
  if (proxy)
    {
    emit this->proxyCreated(proxy);
    }
  return proxy;
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroy(pqProxy* proxy)
{
  emit this->destroying(proxy);

  this->destroyProxyInternal(proxy);
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroySources(pqServer* server)
{
  pqServerManagerModel* model = 
    pqApplicationCore::instance()->getServerManagerModel();
  pqObjectBuilder* builder =
    pqApplicationCore::instance()->getObjectBuilder();

  QList<pqPipelineSource*> sources = model->findItems<pqPipelineSource*>(server);
  while(!sources.isEmpty())
    {
    for(int i=0; i<sources.size(); i++)
      {
      if(sources[i]->getNumberOfConsumers() == 0)
        {
        builder->destroy(sources[i]);
        sources[i] = NULL;
        }
      }
    sources.removeAll(NULL);
    }
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroyLookupTables(pqServer* server)
{
  pqServerManagerModel* model = 
    pqApplicationCore::instance()->getServerManagerModel();
  pqObjectBuilder* builder =
    pqApplicationCore::instance()->getObjectBuilder();

  QList<pqScalarsToColors*> luts = model->findItems<pqScalarsToColors*>(server);
  foreach (pqScalarsToColors* lut, luts)
    {
    builder->destroy(lut);
    }

  QList<pqScalarBarRepresentation*> scalarbars = 
    model->findItems<pqScalarBarRepresentation*>(server);
  foreach (pqScalarBarRepresentation* sb, scalarbars)
    {
    builder->destroy(sb);
    }
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroyPipelineProxies(pqServer* server)
{
  this->destroySources(server);
  this->destroyLookupTables(server);
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroyAllProxies(pqServer* server)
{
  if (!server)
    {
    qDebug() << "Server cannot be NULL.";
    return;
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  pxm->UnRegisterProxies(server->GetConnectionID());
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqObjectBuilder::createProxyInternal(const QString& sm_group, 
  const QString& sm_name, pqServer* server, 
  const QString& reg_group, const QString& reg_name/*=QString()*/)
{
  if (!server)
    {
    qDebug() << "server cannot be null";
    return 0;
    }

  QString actual_regname = reg_name;
  if (reg_name.isEmpty())
    {
    actual_regname = QString("%1%2").arg(sm_name).arg(
      this->NameGenerator->GetCountAndIncrement(sm_name));
    }

  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkSmartPointer<vtkSMProxy> proxy;
  if (sm_group.isEmpty())
    {
    proxy.TakeReference(pxm->NewCompoundProxy(sm_name.toAscii().data()));
    }
  else
    {
    proxy.TakeReference(
      pxm->NewProxy(sm_group.toAscii().data(), sm_name.toAscii().data()));
    }

  if (!proxy.GetPointer())
    {
    qCritical() << "Failed to create proxy: " << sm_group << ", " << sm_name;
    return 0;
    }
  proxy->SetConnectionID(server->GetConnectionID());

  pxm->RegisterProxy(reg_group.toAscii().data(), 
    actual_regname.toAscii().data(), proxy);
  return proxy;
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::destroyProxyInternal(pqProxy* proxy)
{
  if (proxy)
    {
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    pxm->UnRegisterProxy(proxy->getSMGroup().toAscii().data(), 
      proxy->getSMName().toAscii().data(), proxy->getProxy());
    }
}

//-----------------------------------------------------------------------------
QString pqObjectBuilder::getFileNamePropertyName(vtkSMProxy* proxy) const
{
  // Find the first property that has a vtkSMFileListDomain. Assume that
  // it is the property used to set the filename.
  vtkSmartPointer<vtkSMPropertyIterator> piter;
  piter.TakeReference(proxy->NewPropertyIterator());
  piter->Begin();
  while(!piter->IsAtEnd())
    {
    vtkSMProperty* prop = piter->GetProperty();
    if (prop->IsA("vtkSMStringVectorProperty"))
      {
      vtkSmartPointer<vtkSMDomainIterator> diter;
      diter.TakeReference(prop->NewDomainIterator());
      diter->Begin();
      while(!diter->IsAtEnd())
        {
        if (diter->GetDomain()->IsA("vtkSMFileListDomain"))
          {
          return piter->GetKey();
          }
        diter->Next();
        }
      if (!diter->IsAtEnd())
        {
        break;
        }
      }
    piter->Next();
    }

  return QString::Null();
}

//-----------------------------------------------------------------------------
pqServer* pqObjectBuilder::createServer(const pqServerResource& resource)
{

  // TODO: we should have code to make all kinds of server connections in one
  // place.  Right now its split between here and pqComponents.

  // Create a modified version of the resource that only contains server information
  const pqServerResource server_resource = resource.schemeHostsPorts();

  // See if the server is already created.
  pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
  pqServer *server = smModel->findServer(server_resource);
  if(!server)
    {
    // TEMP: ParaView only allows one server connection. Remove this
    // code when it supports multiple server connections.
    if(smModel->getNumberOfItems<pqServer*>() > 0)
      {
      this->removeServer(smModel->getItemAtIndex<pqServer*>(0));
      }

    // Based on the server resource, create the correct type of server ...
    vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
    vtkIdType id = vtkProcessModuleConnectionManager::GetNullConnectionID();
    if(server_resource.scheme() == "builtin")
      {
      id = pm->ConnectToSelf();
      }
    else if(server_resource.scheme() == "cs")
      {
      id = pm->ConnectToRemote(
        resource.host().toAscii().data(),
        resource.port(11111));
      }
    else if(server_resource.scheme() == "csrc")
      {
      qWarning() << "Server reverse connections not supported yet\n";
      }
    else if(server_resource.scheme() == "cdsrs")
      {
      id = pm->ConnectToRemote(
        server_resource.dataServerHost().toAscii().data(),
        server_resource.dataServerPort(11111),
        server_resource.renderServerHost().toAscii().data(),
        server_resource.renderServerPort(22221));
      }
    else if(server_resource.scheme() == "cdsrsrc")
      {
      qWarning() << "Data server/render server reverse connections not supported yet\n";
      }
    else
      {
      qCritical() << "Unknown server type: " << server_resource.scheme() << "\n";
      }

    if(id != vtkProcessModuleConnectionManager::GetNullConnectionID())
      {
      server = smModel->findServer(id);
      server->setResource(server_resource);
      emit this->finishedAddingServer(server);
      }
    }

  return server;
}

//-----------------------------------------------------------------------------
void pqObjectBuilder::removeServer(pqServer* server)
{
  if (!server)
    {
    qDebug() << "No server to remove.";
    return;
    }

  pqApplicationCore* core = pqApplicationCore::instance();
  core->getServerManagerModel()->beginRemoveServer(server);
  this->destroyAllProxies(server);
  vtkProcessModule::GetProcessModule()->Disconnect(
    server->GetConnectionID());
  core->getServerManagerModel()->endRemoveServer();
}

