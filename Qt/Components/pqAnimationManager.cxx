/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqAnimationManager.cxx,v $

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
#include "pqAnimationManager.h"
#include "ui_pqAbortAnimation.h"
#include "ui_pqAnimationSettings.h"

#include "vtkProcessModule.h"
#include "vtkPVServerInformation.h"
#include "vtkSMAnimationSceneGeometryWriter.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxyManager.h"
#include "vtkSMServerProxyManagerReviver.h"

#include <QApplication>
#include <QFileInfo>
#include <QMap>
#include <QMessageBox>
#include <QPointer>
#include <QSize>
#include <QtDebug>

#include "pqAnimationCue.h"
#include "pqAnimationScene.h"
#include "pqAnimationSceneImageWriter.h"
#include "pqApplicationCore.h"
#include "pqEventDispatcher.h"
#include "pqFileDialog.h"
#include "pqObjectBuilder.h"
#include "pqProgressManager.h"
#include "pqProxy.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"
#include "pqView.h"

static inline int pqCeil(double val)
{
  return static_cast<int>(val+0.5);
}

#define SEQUENCE 0
#define REALTIME 1
#define SNAP_TO_TIMESTEPS 2

//-----------------------------------------------------------------------------
class pqAnimationManager::pqInternals
{
public:
  QPointer<pqServer> ActiveServer;
  QPointer<QWidget> ViewWidget;
  typedef QMap<pqServer*, QPointer<pqAnimationScene> > SceneMap;
  SceneMap Scenes;
  Ui::Dialog* AnimationSettingsDialog;

  QSize OldMaxSize;
  QSize OldSize;
};

//-----------------------------------------------------------------------------
pqAnimationManager::pqAnimationManager(QObject* _parent/*=0*/) 
:QObject(_parent)
{
  this->Internals = new pqAnimationManager::pqInternals();
  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(proxyAdded(pqProxy*)),
    this, SLOT(onProxyAdded(pqProxy*)));
  QObject::connect(smmodel, SIGNAL(proxyRemoved(pqProxy*)),
    this, SLOT(onProxyRemoved(pqProxy*)));

  QObject::connect(smmodel, SIGNAL(viewAdded(pqView*)),
    this, SLOT(updateViewModules()));
  QObject::connect(smmodel, SIGNAL(viewRemoved(pqView*)),
    this, SLOT(updateViewModules()));
}

//-----------------------------------------------------------------------------
pqAnimationManager::~pqAnimationManager()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqAnimationManager::setViewWidget(QWidget* w)
{
  this->Internals->ViewWidget = w;
}

//-----------------------------------------------------------------------------
void pqAnimationManager::updateViewModules()
{
  pqAnimationScene* scene = this->getActiveScene();
  if (!scene)
    {
    return;
    }

  QList<pqView*> viewModules = 
    pqApplicationCore::instance()->getServerManagerModel()->
    findItems<pqView*>(this->Internals->ActiveServer);
  
  QList<pqSMProxy> viewList;
  foreach(pqView* view, viewModules)
    {
    viewList.push_back(pqSMProxy(view->getProxy()));
    } 

  emit this->beginNonUndoableChanges();

  vtkSMAnimationSceneProxy* sceneProxy = scene->getAnimationSceneProxy();
  pqSMAdaptor::setProxyListProperty(sceneProxy->GetProperty("ViewModules"),
    viewList);
  sceneProxy->UpdateProperty("ViewModules");

  emit this->endNonUndoableChanges();
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onProxyAdded(pqProxy* proxy)
{
  pqAnimationScene* scene = qobject_cast<pqAnimationScene*>(proxy);
  if (scene && !this->Internals->Scenes.contains(scene->getServer()))
    {
    this->Internals->Scenes[scene->getServer()] = scene;
    if (this->Internals->ActiveServer == scene->getServer())
      {
      emit this->activeSceneChanged(this->getActiveScene());
      }
    }
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onProxyRemoved(pqProxy* proxy)
{
  pqAnimationScene* scene = qobject_cast<pqAnimationScene*>(proxy);
  if (scene) 
    {
    this->Internals->Scenes.remove(scene->getServer());
    if (this->Internals->ActiveServer == scene->getServer())
      {
      emit this->activeSceneChanged(this->getActiveScene());
      }
    }
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onActiveServerChanged(pqServer* server)
{
  this->Internals->ActiveServer = server;
  if (server && !this->getActiveScene())
    {
    this->createActiveScene();
    }
  emit this->activeSceneChanged(this->getActiveScene());
}

//-----------------------------------------------------------------------------
pqAnimationScene* pqAnimationManager::getActiveScene() const
{
  return this->getScene(this->Internals->ActiveServer);
}

//-----------------------------------------------------------------------------
pqAnimationScene* pqAnimationManager::getScene(pqServer* server) const
{
  if (server && this->Internals->Scenes.contains(server))
    {
    return this->Internals->Scenes.value(server);
    }
  return 0;
}

//-----------------------------------------------------------------------------
pqAnimationScene* pqAnimationManager::createActiveScene() 
{
  if (this->Internals->ActiveServer)
    {
    pqObjectBuilder* builder = 
      pqApplicationCore::instance()->getObjectBuilder();
    pqAnimationScene* scene = builder->createAnimationScene(
      this->Internals->ActiveServer);
    
    // this will result in a call to onProxyAdded() and proper
    // signals will be emitted.
    if (!scene)
      {
      qDebug() << "Failed to create scene proxy.";
      }
    this->updateViewModules();
    return this->getActiveScene();
    }
  return 0;
}


//-----------------------------------------------------------------------------
pqAnimationCue* pqAnimationManager::getCue(
  pqAnimationScene* scene, vtkSMProxy* proxy, const char* propertyname, 
  int index) const
{
  return (scene? scene->getCue(proxy, propertyname, index) : 0);
}

//-----------------------------------------------------------------------------
// Called when user changes some property on the save animation dialog.
void pqAnimationManager::updateGUI()
{
  double framerate =
    this->Internals->AnimationSettingsDialog->frameRate->value();
  int num_frames = 
    this->Internals->AnimationSettingsDialog->spinBoxNumberOfFrames->value();
  double duration =  
    this->Internals->AnimationSettingsDialog->animationDuration->value();
  int frames_per_timestep =
    this->Internals->AnimationSettingsDialog->spinBoxFramesPerTimestep->value();

  vtkSMProxy* sceneProxy = this->getActiveScene()->getProxy();
  int playMode = pqSMAdaptor::getElementProperty(
    sceneProxy->GetProperty("PlayMode")).toInt();

  switch (playMode)
    {
  case SNAP_TO_TIMESTEPS:
    // get original number of frames.
    num_frames = pqSMAdaptor::getMultipleElementProperty(
      sceneProxy->GetProperty("TimeSteps")).size();
    num_frames = frames_per_timestep*num_frames;
    this->Internals->AnimationSettingsDialog->spinBoxNumberOfFrames->
      blockSignals(true);
    this->Internals->AnimationSettingsDialog->spinBoxNumberOfFrames->
      setValue(num_frames);
    this->Internals->AnimationSettingsDialog->spinBoxNumberOfFrames->
      blockSignals(false);
    // don't break, let it fall through to SEQUENCE.

  case SEQUENCE:
    this->Internals->AnimationSettingsDialog->animationDuration->
      blockSignals(true);
    this->Internals->AnimationSettingsDialog->animationDuration->setValue(
      num_frames/framerate);
    this->Internals->AnimationSettingsDialog->animationDuration->
      blockSignals(false);
    break;

  case REALTIME:
    this->Internals->AnimationSettingsDialog->spinBoxNumberOfFrames->
      blockSignals(true);
    this->Internals->AnimationSettingsDialog->spinBoxNumberOfFrames->setValue(
      static_cast<int>(duration*framerate));
    this->Internals->AnimationSettingsDialog->spinBoxNumberOfFrames->
      blockSignals(false);
    break;
    }
}

//-----------------------------------------------------------------------------
bool pqAnimationManager::saveAnimation()
{
  pqAnimationScene* scene = this->getActiveScene();
  if (!scene)
    {
    return false;
    }
  vtkSMAnimationSceneProxy* sceneProxy = scene->getAnimationSceneProxy();

  QDialog dialog;
  Ui::Dialog dialogUI;
  this->Internals->AnimationSettingsDialog = &dialogUI;
  dialogUI.setupUi(&dialog);

  // Cannot disconnect and save animation unless connected to a remote server.
  dialogUI.checkBoxDisconnect->setEnabled(
    this->Internals->ActiveServer->isRemote());

  // Set current size of the window.
  QSize viewSize = scene->getViewSize();
  dialogUI.spinBoxHeight->setValue(viewSize.height());
  dialogUI.spinBoxWidth->setValue(viewSize.width());

  // Frames per timestep is only shown
  // when saving in SNAP_TO_TIMESTEPS mode.
  dialogUI.spinBoxFramesPerTimestep->hide();
  dialogUI.labelFramesPerTimestep->hide();

  int playMode = pqSMAdaptor::getElementProperty(
    sceneProxy->GetProperty("PlayMode")).toInt();

  // Set current duration/frame rate/no. of. frames.
  double frame_rate = dialogUI.frameRate->value();
  // Save the current player property values.
  int num_frames = pqSMAdaptor::getElementProperty(
    sceneProxy->GetProperty("NumberOfFrames")).toInt();
  int duration = pqSMAdaptor::getElementProperty(
    sceneProxy->GetProperty("Duration")).toInt();
  int num_steps = pqSMAdaptor::getMultipleElementProperty(
    sceneProxy->GetProperty("TimeSteps")).size();
  int frames_per_timestep = pqSMAdaptor::getElementProperty(
    sceneProxy->GetProperty("FramesPerTimestep")).toInt();

  switch (playMode)
    {
  case SEQUENCE:
    dialogUI.spinBoxNumberOfFrames->setValue(num_frames);
    dialogUI.animationDuration->setEnabled(false);
    dialogUI.animationDuration->setValue(num_frames/frame_rate);
    break;

  case REALTIME:
    dialogUI.animationDuration->setValue(duration);
    dialogUI.spinBoxNumberOfFrames->setValue(
      static_cast<int>(duration*frame_rate));
    dialogUI.spinBoxNumberOfFrames->setEnabled(false);
    break;

  case SNAP_TO_TIMESTEPS:
    dialogUI.spinBoxNumberOfFrames->setValue(num_steps);
    dialogUI.animationDuration->setValue(num_steps*frame_rate);
    dialogUI.spinBoxNumberOfFrames->setEnabled(false);
    dialogUI.animationDuration->setEnabled(false);
    dialogUI.spinBoxFramesPerTimestep->show();
    dialogUI.spinBoxFramesPerTimestep->setValue(frames_per_timestep);
    dialogUI.labelFramesPerTimestep->show();
    break;
    }

  QObject::connect(
    dialogUI.animationDuration, SIGNAL(valueChanged(double)),
    this, SLOT(updateGUI()));
  QObject::connect(
    dialogUI.frameRate, SIGNAL(valueChanged(double)),
    this, SLOT(updateGUI()));
  QObject::connect(
    dialogUI.spinBoxNumberOfFrames, SIGNAL(valueChanged(int)),
    this, SLOT(updateGUI()));
  QObject::connect(
    dialogUI.spinBoxFramesPerTimestep, SIGNAL(valueChanged(int)),
    this, SLOT(updateGUI()));

  if (!dialog.exec())
    {
    this->Internals->AnimationSettingsDialog = 0;
    return false;
    }
  this->Internals->AnimationSettingsDialog = 0;

  bool disconnect_and_save = 
    (dialogUI.checkBoxDisconnect->checkState() == Qt::Checked);

  // Now obtain filename for the animation.
  vtkSmartPointer<vtkPVServerInformation> serverInfo;
  if (disconnect_and_save)
    {
    serverInfo = vtkProcessModule::GetProcessModule()->GetServerInformation(
      scene->getServer()->GetConnectionID());
    if (!serverInfo)
      {
      qWarning() << "Failed to locate server information about AVI support.";
      disconnect_and_save = false;
      }
    }
  else
    {
    // vtkPVServerInformation initialize AVI support in constructor for the
    // local process.
    serverInfo = vtkSmartPointer<vtkPVServerInformation>::New();
    }

  QString filters = "";
  if (serverInfo && serverInfo->GetAVISupport())
    {
    filters += "AVI files (*.avi);;";
    }
  filters +="JPEG images (*.jpg);;TIFF images (*.tif);;PNG images (*.png);;";
  filters +="All files(*)";

  // Create a server dialog is disconnect-and-save is true, else create a client
  // dialog.
  pqFileDialog *file_dialog = new pqFileDialog(
    disconnect_and_save?  scene->getServer() : 0,
    QApplication::activeWindow(),
    tr("Save Animation"), QString(), filters);
  file_dialog->setObjectName("FileSaveAnimationDialog");
  file_dialog->setFileMode(pqFileDialog::AnyFile);
  if (file_dialog->exec() != QDialog::Accepted)
    {
    delete file_dialog;
    return false;
    }

  QStringList files = file_dialog->getSelectedFiles();
  // essential to destroy file dialog, before we disconnect from the server, if
  // at all.
  delete file_dialog;

  if (files.size() == 0)
    {
    return false;
    }

  QString filename = files[0];

  // Update Scene properties based on user options. 
  emit this->beginNonUndoableChanges();

  switch (playMode)
    {
  case SEQUENCE:
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("NumberOfFrames"),
      dialogUI.spinBoxNumberOfFrames->value());
    break;

  case REALTIME:
    // Since even in real-time mode, while saving animation, it is played back 
    // in sequence mode, we change the NumberOfFrames instead of changing the
    // Duration. The spinBoxNumberOfFrames is updated to satisfy
    // duration * frame rate = number of frames.
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("NumberOfFrames"),
      dialogUI.spinBoxNumberOfFrames->value());
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("PlayMode"),
      SEQUENCE);
    break;

  case SNAP_TO_TIMESTEPS:
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("FramesPerTimestep"),
      dialogUI.spinBoxFramesPerTimestep->value());
    break;
    }

  sceneProxy->UpdateVTKObjects();

  QSize newSize(dialogUI.spinBoxWidth->value(),
    dialogUI.spinBoxHeight->value());

  // Enforce any view size conditions (such a multiple of 4). 
  int magnification = this->updateViewSizes(newSize, viewSize);
 
  if (disconnect_and_save)
    {
    pqServer* server = this->Internals->ActiveServer;
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

    vtkSMProxy* writer = pxm->NewProxy("writers", "AnimationSceneImageWriter");
    writer->SetConnectionID(server->GetConnectionID());
    pxm->RegisterProxy("animation", "writer", writer);
    writer->Delete();

    pqSMAdaptor::setElementProperty(writer->GetProperty("FileName"),
      filename.toAscii().data());
    pqSMAdaptor::setElementProperty(writer->GetProperty("Magnification"), 
      magnification); 
    pqSMAdaptor::setElementProperty(writer->GetProperty("FrameRate"),
      dialogUI.frameRate->value());
    writer->UpdateVTKObjects();

    // We save the animation offline.
    vtkSMProxy* cleaner = 
      pxm->NewProxy("connection_cleaners", "AnimationPlayer");
    cleaner->SetConnectionID(server->GetConnectionID());
    pxm->RegisterProxy("animation","cleaner",cleaner);
    cleaner->Delete();

    pqSMAdaptor::setProxyProperty(cleaner->GetProperty("Writer"), writer);
    cleaner->UpdateVTKObjects();

    vtkSMServerProxyManagerReviver* reviver = 
      vtkSMServerProxyManagerReviver::New();
    int status = reviver->ReviveRemoteServerManager(server->GetConnectionID());
    reviver->Delete();
    emit this->endNonUndoableChanges();
    pqApplicationCore::instance()->getObjectBuilder()->removeServer(server);
    this->restoreViewSizes();
    emit this->disconnectServer();
    return status;
    }

  vtkSMAnimationSceneImageWriter* writer = pqAnimationSceneImageWriter::New();
  writer->SetFileName(filename.toAscii().data());
  writer->SetMagnification(magnification);
  writer->SetAnimationScene(sceneProxy);
  writer->SetFrameRate(dialogUI.frameRate->value());

  pqProgressManager* progress_manager = 
    pqApplicationCore::instance()->getProgressManager();

  progress_manager->setEnableAbort(true);
  progress_manager->setEnableProgress(true);
  QObject::connect(progress_manager, SIGNAL(abort()), scene, SLOT(pause()));
  QObject::connect(scene, SIGNAL(tick(int)), this, SLOT(onTick(int)));
  QObject::connect(this, SIGNAL(saveProgress(const QString&, int)),
    progress_manager, SLOT(setProgress(const QString&, int)));
  progress_manager->lockProgress(this);
  bool status = writer->Save();
  progress_manager->unlockProgress(this);
  QObject::disconnect(progress_manager, 0, scene, 0);
  QObject::disconnect(scene, 0, this, 0);
  QObject::disconnect(this, 0, progress_manager, 0);
  progress_manager->setEnableProgress(false);
  progress_manager->setEnableAbort(false);
  writer->Delete();

  // Restore, duration and number of frames.
  switch (playMode)
    {
  case SEQUENCE:
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("NumberOfFrames"), 
      num_frames);
    break;

  case REALTIME:
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("NumberOfFrames"),
      num_frames);
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("PlayMode"),
      REALTIME);
    break;

  case SNAP_TO_TIMESTEPS:
    pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("FramesPerTimestep"),
      frames_per_timestep);
    break;
    }
  sceneProxy->UpdateVTKObjects();

  this->restoreViewSizes();
  emit this->endNonUndoableChanges();
  return status;
}

//-----------------------------------------------------------------------------
int pqAnimationManager::updateViewSizes(QSize newSize, QSize currentSize)
{
  QSize requested_newSize = newSize;
  int &width = newSize.rwidth();
  int &height = newSize.rheight();
  if ((width % 4) > 0)
    {
    width -= width % 4;
    }
  if ((height % 4) > 0)
    {
    height -= height % 4;
    }

  if (requested_newSize != newSize)
    {
    QMessageBox::warning(NULL, "Resolution Changed",
      QString("The requested resolution has been changed from (%1, %2)\n").arg(
        requested_newSize.width()).arg(requested_newSize.height()) + 
      QString("to (%1, %2) to match format specifications.").arg(
        newSize.width()).arg(newSize.height()));
    }

  int magnification = 1;

  // If newSize > currentSize, then magnification is involved.
  int temp = pqCeil(newSize.width()/static_cast<double>(currentSize.width()));
  magnification = (temp> magnification)? temp: magnification;

  temp = pqCeil(newSize.height()/static_cast<double>(currentSize.height()));
  magnification = (temp > magnification)? temp : magnification;

  newSize = newSize/magnification;

  if (!this->Internals->ViewWidget)
    {
    qDebug() << "ViewWidget must be set to the parent of all views.";
    }
  else
    {
    this->Internals->OldSize = this->Internals->ViewWidget->size();
    this->Internals->OldMaxSize = this->Internals->ViewWidget->maximumSize();
    this->Internals->ViewWidget->setMaximumSize(newSize);
    this->Internals->ViewWidget->resize(newSize);
    pqEventDispatcher::processEventsAndWait(1);
    }

  return magnification;
}

//-----------------------------------------------------------------------------
void pqAnimationManager::restoreViewSizes()
{
  if (this->Internals->ViewWidget)
    {
    this->Internals->ViewWidget->setMaximumSize(this->Internals->OldMaxSize);
    this->Internals->ViewWidget->resize(this->Internals->OldSize);
    }
}

//-----------------------------------------------------------------------------
bool pqAnimationManager::saveGeometry(const QString& filename, 
  pqView* view)
{
  if (!view)
    {
    return false;
    }

  pqAnimationScene* scene = this->getActiveScene();
  if (!scene)
    {
    return false;
    }
  vtkSMAnimationSceneProxy* sceneProxy = scene->getAnimationSceneProxy();

  vtkSMAnimationSceneGeometryWriter* writer = vtkSMAnimationSceneGeometryWriter::New();
  writer->SetFileName(filename.toAscii().data());
  writer->SetAnimationScene(sceneProxy);
  writer->SetViewModule(view->getProxy());
  bool status = writer->Save();
  writer->Delete();
  return status;
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onTick(int progress)
{
  emit this->saveProgress("Saving Animation", progress);
}
