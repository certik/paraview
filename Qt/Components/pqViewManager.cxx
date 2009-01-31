/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqViewManager.cxx,v $

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
#include "pqViewManager.h"
#include "ui_pqEmptyView.h"

// VTK includes.
#include "vtkPVConfig.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMStateLoaderBase.h"

// Qt includes.
#include <QAction>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QList>
#include <QMap>
#include <QMenu>
#include <QMimeData>
#include <QPointer>
#include <QPushButton>
#include <QSet>
#include <QSignalMapper>
#include <QtDebug>
#include <QUuid>
#include <QApplication>


// ParaView includes.
#include "pqApplicationCore.h"
#include "pqCloseViewUndoElement.h"
#include "pqComparativeRenderView.h"
#include "pqMultiViewFrame.h"
#include "pqObjectBuilder.h"
#include "pqPluginManager.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSplitViewUndoElement.h"
#include "pqUndoStack.h"
#include "pqViewModuleInterface.h"
#include "pqXMLUtil.h"

#if WIN32
#include "process.h"
#define getpid _getpid
#else
#include "unistd.h"
#endif

template<class T>
uint qHash(const QPointer<T> key)
{
  return qHash((T*)key);
}
//-----------------------------------------------------------------------------
class pqViewManager::pqInternals 
{
public:
  QPointer<pqServer> ActiveServer;
  QPointer<pqView> ActiveView;
  QPointer<pqUndoStack> UndoStack;
  QMenu ConvertMenu;
  QSignalMapper* LookmarkSignalMapper;


  typedef QMap<pqMultiViewFrame*, QPointer<pqView> > FrameMapType;
  FrameMapType Frames;

  QList<QPointer<pqMultiViewFrame> > PendingFrames;
  QList<QPointer<pqView> > PendingViews;

  QSize MaxWindowSize;

  bool DontCreateDeleteViewsModules;

  // When a frame is being closed, we create an undo element
  // and  save it to be pushed on the stack after the 
  // view in the frame has been unregistered. The sequence
  // of operations on the undo stack is 
  // * unregister view
  // * close frame
  vtkSmartPointer<vtkSMUndoElement> CloseFrameUndoElement;
};

//-----------------------------------------------------------------------------
pqViewManager::pqViewManager(QWidget* _parent/*=null*/)
  : pqMultiView(_parent)
{
  this->Internal = new pqInternals();
  this->Internal->DontCreateDeleteViewsModules = false;
  this->Internal->MaxWindowSize = QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
  this->Internal->LookmarkSignalMapper = new QSignalMapper(this);
  QObject::connect(this->Internal->LookmarkSignalMapper, SIGNAL(mapped(QWidget*)), 
    this, SIGNAL(createLookmark(QWidget*)));


  pqServerManagerModel* smModel = 
    pqApplicationCore::instance()->getServerManagerModel();
  if (!smModel)
    {
    qDebug() << "pqServerManagerModel instance must be created before "
      <<"pqViewManager.";
    return;
    }

  // We need to know when new view modules are added.
  QObject::connect(smModel, SIGNAL(viewAdded(pqView*)),
    this, SLOT(onViewAdded(pqView*)));
  QObject::connect(smModel, SIGNAL(viewRemoved(pqView*)),
    this, SLOT(onViewRemoved(pqView*)));

  // Record creation/removal of frames.
  QObject::connect(this, SIGNAL(frameAdded(pqMultiViewFrame*)), 
    this, SLOT(onFrameAdded(pqMultiViewFrame*)));
  QObject::connect(this, SIGNAL(preFrameRemoved(pqMultiViewFrame*)), 
    this, SLOT(onPreFrameRemoved(pqMultiViewFrame*)));
  QObject::connect(this, SIGNAL(frameRemoved(pqMultiViewFrame*)), 
    this, SLOT(onFrameRemoved(pqMultiViewFrame*)));

  QObject::connect(this, 
    SIGNAL(afterSplitView(const Index&, Qt::Orientation, float, const Index&)),
    this, SLOT(onSplittingView(const Index&, Qt::Orientation, float, const Index&)));
  
  QObject::connect(&this->Internal->ConvertMenu, SIGNAL(triggered(QAction*)),
    this, SLOT(onConvertToTriggered(QAction*)));

  // Creates the default empty frame.
  this->init();

  qApp->installEventFilter(this);
}

//-----------------------------------------------------------------------------
pqViewManager::~pqViewManager()
{
  // Cleanup all render modules.
  foreach (pqMultiViewFrame* frame , this->Internal->Frames.keys())
    {
    if (frame)
      {
      this->onFrameRemovedInternal(frame);
      }
    }
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqViewManager::buildConvertMenu()
{
  this->Internal->ConvertMenu.clear();

  // Create actions for converting view types.
  QObjectList ifaces =
    pqApplicationCore::instance()->getPluginManager()->interfaces();
  foreach(QObject* iface, ifaces)
    {
    pqViewModuleInterface* vi = qobject_cast<pqViewModuleInterface*>(iface);
    if(vi)
      {
      QStringList viewtypes = vi->viewTypes();
      QStringList::iterator iter;
      for(iter = viewtypes.begin(); iter != viewtypes.end(); ++iter)
        {
        if ((*iter) == "TableView")
          {
          // Ignore these views for now.
          continue;
          }
        QAction* view_action = new QAction(vi->viewTypeName(*iter), this);
        view_action->setData(*iter);
        this->Internal->ConvertMenu.addAction(view_action);
        }
      }
    }
    
  QAction* view_action = new QAction("None", this);
  view_action->setData("None");
  this->Internal->ConvertMenu.addAction(view_action);
}

//-----------------------------------------------------------------------------
void pqViewManager::setActiveServer(pqServer* server)
{
  this->Internal->ActiveServer = server;
}

//-----------------------------------------------------------------------------
pqView* pqViewManager::getActiveView() const
{
  return this->Internal->ActiveView;
}

//-----------------------------------------------------------------------------
void pqViewManager::setUndoStack(pqUndoStack* stack)
{
  if (this->Internal->UndoStack)
    {
    QObject::disconnect(this->Internal->UndoStack, 0, this, 0);
    }

  this->Internal->UndoStack = stack;

  if (stack)
    {
    QObject::connect(this, SIGNAL(beginUndo(const QString&)),
      stack, SLOT(beginUndoSet(QString)));
    QObject::connect(this, SIGNAL(endUndo()), stack, SLOT(endUndoSet()));
    QObject::connect(this, SIGNAL(addToUndoStack(vtkUndoElement*)),
      stack, SLOT(addToActiveUndoSet(vtkUndoElement*)));
    QObject::connect(this, SIGNAL(beginNonUndoableChanges()),
      stack, SLOT(beginNonUndoableChanges()));
    QObject::connect(this, SIGNAL(endNonUndoableChanges()),
      stack, SLOT(endNonUndoableChanges()));
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::updateConversionActions(pqMultiViewFrame* frame)
{
  QString to_exclude;
  if (this->Internal->Frames.contains(frame))
    {
    to_exclude = this->Internal->Frames[frame]->getViewType();
    }

  bool server_exists = (pqApplicationCore::instance()->getServerManagerModel()->
    getNumberOfItems<pqServer*>() >= 1);
  foreach (QAction* action, this->Internal->ConvertMenu.actions())
    {
    action->setEnabled(server_exists && (to_exclude != action->data().toString()));
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::onFrameAdded(pqMultiViewFrame* frame)
{
  // We connect drag-drop signals event for empty frames.
  QObject::connect(frame, SIGNAL(dragStart(pqMultiViewFrame*)),
    this, SLOT(frameDragStart(pqMultiViewFrame*)));
  QObject::connect(frame, SIGNAL(dragEnter(pqMultiViewFrame*,QDragEnterEvent*)),
    this, SLOT(frameDragEnter(pqMultiViewFrame*,QDragEnterEvent*)));
  QObject::connect(frame, SIGNAL(dragMove(pqMultiViewFrame*,QDragMoveEvent*)),
    this, SLOT(frameDragMove(pqMultiViewFrame*,QDragMoveEvent*)));
  QObject::connect(frame, SIGNAL(drop(pqMultiViewFrame*,QDropEvent*)),
    this, SLOT(frameDrop(pqMultiViewFrame*,QDropEvent*)));

  // We need to know when the frame is resized, so that we can update view size
  // related properties on the view proxy.
  frame->installEventFilter(this);
  frame->MaximizeButton->show();
  frame->CloseButton->show();
  frame->SplitVerticalButton->show();
  frame->SplitHorizontalButton->show();

  frame->getContextMenu()->addSeparator();
  QAction* subAction = frame->getContextMenu()->addMenu(
    &this->Internal->ConvertMenu);
  subAction->setText("Convert To");

  QSignalMapper* sm = new QSignalMapper(frame);
  sm->setMapping(frame, frame);
  QObject::connect(frame, SIGNAL(activeChanged(bool)), sm, SLOT(map()));
  QObject::connect(sm, SIGNAL(mapped(QWidget*)), 
    this, SLOT(onActivate(QWidget*)));

  sm = new QSignalMapper(frame);
  sm->setMapping(frame, frame);
  QObject::connect(frame, SIGNAL(contextMenuRequested()), sm, SLOT(map()));
  QObject::connect(sm, SIGNAL(mapped(QWidget*)), 
    this, SLOT(onFrameContextMenuRequested(QWidget*)));

  // A newly added frames gets collected as an empty frame.
  // It will be used next time a view module is created.
  this->Internal->PendingFrames.removeAll(frame);
  this->Internal->PendingFrames.push_back(frame);

  frame->setActive(true);

  // HACK: When undo-redoing, a view may be registered before
  // a frame is created, in that case the view is added to
  // PendingViews and we assign it to the frame here.
  if (this->Internal->PendingViews.size() > 0)
    {
    pqView* view = this->Internal->PendingViews.takeAt(0);
    this->assignFrame(view);
    }

  // Setup the UI shown when no view is present in the frame.
  QWidget* emptyFrame = frame->emptyMainWidget();
  Ui::EmptyView ui;
  ui.setupUi(emptyFrame);

  this->buildConvertMenu();

  // Add buttons for all conversion actions.
  QList<QAction*> convertActions = 
    this->Internal->ConvertMenu.actions();
  foreach (QAction* action, convertActions)
    {
    QPushButton* button = new QPushButton(action->text(), frame);
    ui.ConvertActionsFrame->layout()->addWidget(button);
    button->addAction(action);
    QObject::connect(button, SIGNAL(clicked()),
      this, SLOT(onConvertToButtonClicked()));
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::onFrameRemovedInternal(pqMultiViewFrame* frame)
{
  QObject::disconnect(frame, SIGNAL(dragStart(pqMultiViewFrame*)),
    this, SLOT(frameDragStart(pqMultiViewFrame*)));
  QObject::disconnect(frame, SIGNAL(dragEnter(pqMultiViewFrame*,QDragEnterEvent*)),
    this, SLOT(frameDragEnter(pqMultiViewFrame*,QDragEnterEvent*)));
  QObject::disconnect(frame, SIGNAL(dragMove(pqMultiViewFrame*,QDragMoveEvent*)),
    this, SLOT(frameDragMove(pqMultiViewFrame*,QDragMoveEvent*)));
  QObject::disconnect(frame, SIGNAL(drop(pqMultiViewFrame*,QDropEvent*)),
    this, SLOT(frameDrop(pqMultiViewFrame*,QDropEvent*)));

  frame->removeEventFilter(this);
  this->Internal->PendingFrames.removeAll(frame);
  if (!this->Internal->Frames.contains(frame))
    {
    // A frame with no view module has been removed. 
    return;
    }

  // When a frame is removed, its render module is destroyed.
  pqView* view = this->Internal->Frames.take(frame);
  this->disconnect(frame, view);

  this->Internal->PendingFrames.removeAll(frame);

  // Generally, we destroy the view module when the frame goes away,
  // unless told otherwise.
  if (this->Internal->DontCreateDeleteViewsModules)
    {
    return;
    }

  // When a frame is removed, the contained view is also destroyed.
  if (view)
    {
    pqApplicationCore::instance()->getObjectBuilder()->destroy(view);
    }

}

//-----------------------------------------------------------------------------
void pqViewManager::onFrameRemoved(pqMultiViewFrame* frame)
{
  this->onFrameRemovedInternal(frame);

  if (this->Internal->CloseFrameUndoElement)
    {
    emit this->addToUndoStack(this->Internal->CloseFrameUndoElement);
    this->Internal->CloseFrameUndoElement = 0;
    }
  emit this->endUndo();

  // Now activate some frame, so that we have an active view.
  if (this->Internal->Frames.size() > 0)
    {
    pqMultiViewFrame* new_active_frame =
      this->Internal->Frames.begin().key();
    if (new_active_frame->active())
      {
      this->onActivate(new_active_frame);
      }
    else
      {
      new_active_frame->setActive(true);
      }
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::onPreFrameRemoved(pqMultiViewFrame* frame)
{
  emit this->beginUndo("Close View");

  vtkPVXMLElement* state = vtkPVXMLElement::New();
  this->saveState(state);

  pqMultiView::Index index = this->indexOf(frame);

  pqCloseViewUndoElement* elem = pqCloseViewUndoElement::New();
  elem->CloseView(index, state->GetNestedElement(0));
  this->Internal->CloseFrameUndoElement = elem;
  elem->Delete();
  state->Delete();
}

//-----------------------------------------------------------------------------
void pqViewManager::connect(pqMultiViewFrame* frame, pqView* view)
{
  if (!frame || !view)
    {
    return;
    }
  this->Internal->PendingFrames.removeAll(frame);

  QWidget* viewWidget = view->getWidget();
  if(viewWidget)
    {
    viewWidget->setParent(frame);
    frame->setMainWidget(viewWidget);
    viewWidget->setMaximumSize(this->Internal->MaxWindowSize);
    }
  else
    {
    frame->setMainWidget(NULL);
    }

  pqRenderView* const render_module = 
    qobject_cast<pqRenderView*>(view);
  if(render_module)
    {
    QAction* lookmarkAction = new QAction(QIcon(":/pqWidgets/Icons/pqLookmark16.png"), 
      "Lookmark", 
      this);
    lookmarkAction->setObjectName("LookmarkButton");
    frame->addTitlebarAction(lookmarkAction);

    lookmarkAction->setEnabled(true);
    this->Internal->LookmarkSignalMapper->setMapping(lookmarkAction, frame);
    QObject::connect(lookmarkAction, SIGNAL(triggered(bool)), 
      this->Internal->LookmarkSignalMapper, SLOT(map()));

    QAction* cameraAction = new QAction(QIcon(":/pqWidgets/Icons/pqEditCamera16.png"), 
      "Adjust Camera", 
      this);
    cameraAction->setObjectName("CameraButton");
    frame->addTitlebarAction(cameraAction);
    cameraAction->setEnabled(true);
    QObject::connect(cameraAction, SIGNAL(triggered()), 
      this, SLOT(onCameraTriggered()));
    }

  QAction* optionsAction = new QAction(
    QIcon(":/pqWidgets/Icons/pqOptions16.png"), "Edit View Options", this);
  optionsAction->setObjectName("OptionsButton");
  optionsAction->setEnabled(true);
  frame->addTitlebarAction(optionsAction);
  QObject::connect(optionsAction, SIGNAL(triggered()), 
    this, SIGNAL(viewOptionsRequested()));

  if (view->supportsUndo())
    {
    // Setup undo/redo connections if the view module
    // supports interaction undo.
    QAction* forwardAction = new QAction(QIcon(":/pqWidgets/Icons/pqRedoCamera24.png"), 
      "", 
      this);
    forwardAction->setObjectName("ForwardButton");
    frame->addTitlebarAction(forwardAction);
    forwardAction->setEnabled(false);

    QObject::connect(forwardAction, SIGNAL( triggered ()), 
      view, SLOT(redo()));
    QObject::connect(view, SIGNAL(canRedoChanged(bool)),
      forwardAction, SLOT(setEnabled(bool)));


    QAction* backAction = new QAction(QIcon(":/pqWidgets/Icons/pqUndoCamera24.png"), 
      "", 
      this);
    backAction->setObjectName("BackButton");
    frame->addTitlebarAction(backAction);
    backAction->setEnabled(false);
    
    QObject::connect(backAction, SIGNAL( triggered ()), 
      view, SLOT(undo()));
    QObject::connect(view, SIGNAL(canUndoChanged(bool)),
      backAction, SLOT(setEnabled(bool)));
    }



  this->Internal->Frames.insert(frame, view);
}

//-----------------------------------------------------------------------------
void pqViewManager::disconnect(pqMultiViewFrame* frame, pqView* view)
{
  if (!frame || !view)
    {
    return;
    }

  this->Internal->Frames.remove(frame);

  QWidget* viewWidget = view->getWidget();
  if(viewWidget)
    {
    viewWidget->setParent(NULL);
    viewWidget->removeEventFilter(this);
    }
  frame->setMainWidget(NULL);


  pqRenderView* const render_module = 
    qobject_cast<pqRenderView*>(view);
  if(render_module)
    {
    QAction *lookmarkAction= frame->getAction("LookmarkButton");
    if(lookmarkAction)
      {
      frame->removeTitlebarAction(lookmarkAction);
      delete lookmarkAction;
      }
    QAction *cameraAction= frame->getAction("CameraButton");
    if(cameraAction)
      {
      frame->removeTitlebarAction(cameraAction);
      delete cameraAction;
      }
    }

  QAction *optionsAction= frame->getAction("OptionsButton");
  if(optionsAction)
    {
    frame->removeTitlebarAction(optionsAction);
    delete optionsAction;
    }

  if (view->supportsUndo())
    {
    QAction *forwardAction= frame->getAction("ForwardButton");
    if(forwardAction)
      {
      frame->removeTitlebarAction(forwardAction);
      delete forwardAction;
      }


    QAction *backAction= frame->getAction("BackButton");
    if(backAction)
      {
      frame->removeTitlebarAction(backAction);
      delete backAction;
      }
    }

  this->Internal->PendingFrames.push_back(frame);
}

//-----------------------------------------------------------------------------
void pqViewManager::onViewAdded(pqView* view)
{
  this->assignFrame(view);
  this->updateViewPositions();
}

//-----------------------------------------------------------------------------
void pqViewManager::assignFrame(pqView* view)
{
  pqMultiViewFrame* frame = 0;
  if (this->Internal->PendingFrames.size() == 0)
    {
    // Create a new frame.
  
    if (this->Internal->UndoStack && (
      this->Internal->UndoStack->getInUndo() ||
      this->Internal->UndoStack->getInRedo()))
      {
      // HACK: If undo-redoing, don't split 
      // to create a new pane, it will be created 
      // as a part of the undo/redo.
      this->Internal->PendingViews.push_back(view);
      return;
      }

    // Locate frame to split.
    // If there is an active view, use it.
    pqMultiViewFrame* oldFrame = 0;
    if (this->Internal->ActiveView)
      {
      oldFrame = this->getFrame(this->Internal->ActiveView);
      }
    else if (this->Internal->Frames.size() > 0)
      {
      oldFrame = this->Internal->Frames.begin().key();
      }
    else
      {
      // There are no pending frames and not used frames, how
      // can that be? Hence, we are use it will happen,
      // atleast flag an error!
      qCritical() << "Internal state of frames has got messed up!";
      return;
      }
  
    this->Internal->DontCreateDeleteViewsModules = true;
    QSize cur_size = oldFrame->size();
    if (cur_size.width() > 1.15*cur_size.height()) 
        // give a slight preference to
        // vertical splitting.
                      
      {
      frame = this->splitWidgetHorizontal(oldFrame);
      }
    else
      {
      frame = this->splitWidgetVertical(oldFrame);
      }
    this->Internal->DontCreateDeleteViewsModules = false;
    }
  else
    {
    // It is possible that the active frame is empty, if so,
    // we use it.
    foreach (pqMultiViewFrame* curframe, this->Internal->PendingFrames)
      {
      if (curframe->active())
        {
        frame = curframe;
        break;
        }
      }
    if (!frame)
      {
      frame = this->Internal->PendingFrames.first();
      }
    this->Internal->PendingFrames.removeAll(frame);
    }

  if (frame)
    {
    this->connect(frame, view);
    if (frame->active())
      {
      this->onActivate(frame);
      }
    else
      {
      frame->setActive(true);
      }
    }
}

//-----------------------------------------------------------------------------
pqMultiViewFrame* pqViewManager::getFrame(pqView* view) const
{
  return qobject_cast<pqMultiViewFrame*>(view->getWidget()->parentWidget());
}

//-----------------------------------------------------------------------------
pqView* pqViewManager::getView(pqMultiViewFrame* frame) const
{
  pqInternals::FrameMapType::iterator iter = this->Internal->Frames.find(frame);
  if (iter != this->Internal->Frames.end())
    {
    return iter.value();
    }
  return 0;
}

//-----------------------------------------------------------------------------
void pqViewManager::onViewRemoved(pqView* view)
{
  pqMultiViewFrame* frame = this->getFrame(view);
  if (frame)
    {
    this->disconnect(frame, view);
    }

  this->Internal->PendingViews.removeAll(view);

  this->onActivate(frame);
}

//-----------------------------------------------------------------------------
void pqViewManager::onConvertToButtonClicked()
{
  QPushButton* button = qobject_cast<QPushButton*>(this->sender());
  if (!button)
    {
    return;
    }

  pqMultiViewFrame* frame = 0;

  // Try to locate the frame in which this button exists.
  QWidget* button_parent = button->parentWidget();
  while (button_parent)
    {
    frame = qobject_cast<pqMultiViewFrame*>(button_parent);
    if (frame)
      {
      break;
      }
    button_parent = button_parent->parentWidget();
    }

  if (!frame)
    {
    return;
    }

  // Make the frame active.
  frame->setActive(true);
  if (button->actions().size() > 0)
    {
    QAction* action = button->actions()[0];
    this->onConvertToTriggered(action);
    }
  else
    {
    qCritical() << "No actions!" << endl;
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::onConvertToTriggered(QAction* action)
{
  QString type = action->data().toString();

  // FIXME: We may want to fix this to use the active server instead.
  pqServer* server= pqApplicationCore::instance()->
    getServerManagerModel()->getItemAtIndex<pqServer*>(0);
  if (!server)
    {
    qDebug() << "No server present cannot convert view.";
    return;
    }

  emit this->beginUndo(QString("Convert View to %1").arg(type));

  pqObjectBuilder* builder = 
    pqApplicationCore::instance()-> getObjectBuilder();
  if (this->Internal->ActiveView)
    {
    builder->destroy(this->Internal->ActiveView);
    }

  if(type != "None")
    {
    builder->createView(type, server);
    }

  emit this->endUndo();
}

//-----------------------------------------------------------------------------
void pqViewManager::onFrameContextMenuRequested(QWidget* wid)
{
  this->buildConvertMenu();

  pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(wid);
  if (frame)
    {
    this->updateConversionActions(frame);
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::onActivate(QWidget* obj)
{
  if (!obj)
    {
    this->Internal->ActiveView = 0;
    emit this->activeViewChanged(this->Internal->ActiveView);
    return; 
    }

  pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(obj);
  if (!frame->active())
    {
    return;
    }


  pqView* view = this->Internal->Frames.value(frame);
  // If the frame does not have a view, view==NULL.
  this->Internal->ActiveView = view;

  // Make sure no other frame is active.
  foreach (pqMultiViewFrame* fr, this->Internal->Frames.keys())
    {
    if (fr != frame)
      {
      fr->setActive(false);
      }
    }
  foreach (pqMultiViewFrame* fr, this->Internal->PendingFrames)
    {
    if (fr != frame)
      {
      fr->setActive(false);
      }
    }

  emit this->activeViewChanged(this->Internal->ActiveView);
}

//-----------------------------------------------------------------------------
bool pqViewManager::eventFilter(QObject* caller, QEvent* e)
{
  if(e->type() == QEvent::MouseButtonPress)
    {
    QWidget* w = qobject_cast<QWidget*>(caller);
    if(w && this->isAncestorOf(w))
      {
      // If the new widget that is getting the focus is a child widget of any of the
      // frames, then the frame should be made active.
      QList<pqMultiViewFrame*> frames = this->Internal->Frames.keys();
      foreach (pqMultiViewFrame* frame, this->Internal->PendingFrames)
        {
        frames.push_back(frame);
        }

      foreach (pqMultiViewFrame* frame, frames)
        {
        if (frame->isAncestorOf(w))
          {
          frame->setActive(true);
          break;
          }
        }
      }
    }
  else if(QApplication::instance() != caller &&
          e->type() == QEvent::Resize)
    {
    // Update ViewPosition and GUISize properties on all view modules.
    this->updateViewPositions(); 
    }

  return pqMultiView::eventFilter(caller, e);
}

//-----------------------------------------------------------------------------
void pqViewManager::setMaxViewWindowSize(const QSize& win_size)
{
  this->Internal->MaxWindowSize = win_size.isEmpty()?
      QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX) : win_size;
  foreach (pqMultiViewFrame* frame, this->Internal->Frames.keys())
    {
    frame->mainWidget()->setMaximumSize(this->Internal->MaxWindowSize);
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::updateViewPositions()
{
  // find a rectangle that bounds all views
  QRect totalBounds;
  
  foreach(pqView* view, this->Internal->Frames)
    {
    QRect bounds = view->getWidget()->rect();
    bounds.moveTo(view->getWidget()->mapToGlobal(QPoint(0,0)));
    totalBounds |= bounds;
    }

  /// GUISize, ViewSize and ViewPosition properties are managed
  /// by the GUI, the undo/redo stack should not worry about 
  /// the changes made to them.
  emit this->beginNonUndoableChanges();

  // Now we loop thorough all view modules and set the GUISize/ViewPosition.
  foreach(pqView* view, this->Internal->Frames)
    {
    vtkSMIntVectorProperty* prop = 0;

    // set size containing all views
    prop = vtkSMIntVectorProperty::SafeDownCast(
      view->getProxy()->GetProperty("GUISize"));
    if(prop)
      {
      prop->SetElements2(totalBounds.width(), totalBounds.height());
      }

    // position relative to the bounds of all views
    prop = vtkSMIntVectorProperty::SafeDownCast(
      view->getProxy()->GetProperty("ViewPosition"));
    if(prop)
      {
      QPoint view_pos = view->getWidget()->mapToGlobal(QPoint(0,0));
      view_pos -= totalBounds.topLeft();
      prop->SetElements2(view_pos.x(), view_pos.y());
      }

    // size of each view.
    prop = vtkSMIntVectorProperty::SafeDownCast(
      view->getProxy()->GetProperty("ViewSize"));
    if (prop)
      {
      QRect bounds = view->getWidget()->rect();
      prop->SetElements2(bounds.width(), bounds.height());
      }
    // This is causing problems with tests.
    // view->getProxy()->UpdateProperty("ViewSize");
    }

  emit this->endNonUndoableChanges();
}

//-----------------------------------------------------------------------------
void pqViewManager::saveState(vtkPVXMLElement* root)
{
  vtkPVXMLElement* rwRoot = vtkPVXMLElement::New();
  rwRoot->SetName("ViewManager");
  rwRoot->AddAttribute("version", PARAVIEW_VERSION_FULL);
  root->AddNestedElement(rwRoot);
  rwRoot->Delete();

  // Save the window layout.
  this->pqMultiView::saveState(rwRoot);

  // Save the render module - window mapping.
  pqInternals::FrameMapType::Iterator iter = this->Internal->Frames.begin();
  for(; iter != this->Internal->Frames.end(); ++iter)
    {
    pqMultiViewFrame* frame = iter.key();
    pqView* view = iter.value();

    pqMultiView::Index index = this->indexOf(frame);
    vtkPVXMLElement* frameElem = vtkPVXMLElement::New();
    frameElem->SetName("Frame");
    frameElem->AddAttribute("index", index.getString().toAscii().data());
    frameElem->AddAttribute("view_module", view->getProxy()->GetSelfIDAsString());
    rwRoot->AddNestedElement(frameElem);
    frameElem->Delete();
    }
}

//-----------------------------------------------------------------------------
bool pqViewManager::loadState(vtkPVXMLElement* rwRoot, 
  vtkSMStateLoaderBase* loader)
{
  if (!rwRoot || !rwRoot->GetName() || strcmp(rwRoot->GetName(), "ViewManager"))
    {
    qDebug() << "Argument must be <ViewManager /> element.";
    return false;
    }

  // When state is loaded by the server manager,
  // the View Manager will have already layed out all the view modules
  // using a default/random scheme. The role of this method
  // is to re-arrange all the views based on the layout in the 
  // state file.
  this->Internal->DontCreateDeleteViewsModules = true; 

  // We remove all "randomly" layed out frames. Note that we are not
  // destroying the view modules, only the frames that got created
  // when the server manager state was getting loaded.
  foreach (pqMultiViewFrame* frame, this->Internal->Frames.keys())
    {
    this->removeWidget(frame);
    }
  foreach (pqMultiViewFrame* frame, this->Internal->PendingFrames)
    {
    if (frame)
      {
      this->removeWidget(frame);
      }
    }
  this->Internal->PendingFrames.clear();

  this->Superclass::loadState(rwRoot);
  this->Internal->DontCreateDeleteViewsModules = false; 
  
  this->Internal->Frames.clear();
  for(unsigned int cc=0; cc < rwRoot->GetNumberOfNestedElements(); cc++)
    {
    vtkPVXMLElement* elem = rwRoot->GetNestedElement(cc);
    if (strcmp(elem->GetName(), "Frame") == 0)
      {
      QString index_string = elem->GetAttribute("index");

      pqMultiView::Index index;
      index.setFromString(index_string);
      int id = 0;
      elem->GetScalarAttribute("view_module", &id);
      vtkSmartPointer<vtkSMProxy> viewModule;
      viewModule.TakeReference(loader->NewProxy(id));
      if (!viewModule.GetPointer())
        {
        qCritical() << "Failed to locate view module mentioned in state!";
        return false;
        }

      pqView* view = pqApplicationCore::instance()->getServerManagerModel()->
        findItem<pqView*>(viewModule);
      pqMultiViewFrame* frame = qobject_cast<pqMultiViewFrame*>(
        this->widgetOfIndex(index));
      if (frame && view)
        {
        this->connect(frame, view);
        }
      }
    }
  pqMultiViewFrame* frame = 0;
  if (this->Internal->Frames.size() > 0)
    {
    // Make the first frame active.
    frame = this->Internal->Frames.begin().key();
    }
  else if (this->Internal->PendingFrames.size() > 0)
    {
    frame = this->Internal->PendingFrames[0];
    }

  if (frame)
    {
    if (frame->active())
      {
      this->onActivate(frame);
      }
    else
      {
      frame->setActive(true);
      }
    }

  return true;
}

//-----------------------------------------------------------------------------
void pqViewManager::frameDragStart(pqMultiViewFrame* frame)
{
  QPixmap pixmap(":/pqWidgets/Icons/pqWindow16.png");

  QByteArray output;
  QDataStream dataStream(&output, QIODevice::WriteOnly);
  dataStream << frame->uniqueID();

  QString mimeType = QString("application/paraview3/%1").arg(getpid());

  QMimeData *mimeData = new QMimeData;
  mimeData->setData(mimeType, output);

  QDrag *drag = new QDrag(this);
  drag->setMimeData(mimeData);
  drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));
  drag->setPixmap(pixmap);

  drag->start();
}

//-----------------------------------------------------------------------------
void pqViewManager::frameDragEnter(pqMultiViewFrame*,
                                           QDragEnterEvent* e)
{
  QString mimeType = QString("application/paraview3/%1").arg(getpid());
  if(e->mimeData()->hasFormat(mimeType))
    {
    e->accept();
    }
  else
    {
    e->ignore();
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::frameDragMove(pqMultiViewFrame*,
                                          QDragMoveEvent* e)
{
  QString mimeType = QString("application/paraview3/%1").arg(getpid());
  if(e->mimeData()->hasFormat(mimeType))
    {
    e->accept();
    }
  else
    {
    e->ignore();
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::frameDrop(pqMultiViewFrame* acceptingFrame,
                                      QDropEvent* e)
{
  QString mimeType = QString("application/paraview3/%1").arg(getpid());
  if (e->mimeData()->hasFormat(mimeType))
    {
    QByteArray input= e->mimeData()->data(mimeType);
    QDataStream dataStream(&input, QIODevice::ReadOnly);

    QUuid uniqueID;
    dataStream>>uniqueID;

    pqMultiViewFrame* originatingFrame=NULL;
    pqMultiViewFrame* f;
    foreach(f, this->Internal->Frames.keys())
      {
      if(f->uniqueID()==uniqueID)
        {
        originatingFrame=f;
        break;
        }
      }
    if (!originatingFrame)
      {
      foreach (f, this->Internal->PendingFrames)
        {
        if (f->uniqueID() == uniqueID)
          {
          originatingFrame = f;
          break;
          }
        }
      }
    
    if(originatingFrame && originatingFrame != acceptingFrame)
      {
      this->hide(); 
      //Switch the originalFrame with the frame;

      Index originatingIndex=this->indexOf(originatingFrame);
      Index acceptingIndex=this->indexOf(acceptingFrame);
      pqMultiViewFrame *tempFrame= new pqMultiViewFrame;

      this->replaceView(originatingIndex,tempFrame);
      this->replaceView(acceptingIndex,originatingFrame);
      originatingIndex=this->indexOf(tempFrame);
      this->replaceView(originatingIndex,acceptingFrame);

      this->updateViewPositions();
      delete tempFrame;
      
      this->show();
      }
    e->accept();
    }
  else
    {
    e->ignore();
    }
}

//-----------------------------------------------------------------------------
void pqViewManager::onSplittingView(const Index& index, 
  Qt::Orientation orientation, float fraction, const Index& childIndex)
{
  emit this->beginUndo("Split View");

  pqSplitViewUndoElement* elem = pqSplitViewUndoElement::New();
  elem->SplitView(index, orientation, fraction, childIndex);
  emit this->addToUndoStack(elem);
  elem->Delete();

  emit this->endUndo();
}

//-----------------------------------------------------------------------------
void pqViewManager::onCameraTriggered()
{
  emit this->triggerCameraAdjustment(this->Internal->ActiveView);
}





