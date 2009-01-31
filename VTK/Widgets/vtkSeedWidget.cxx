/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSeedWidget.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSeedWidget.h"
#include "vtkSeedRepresentation.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkHandleWidget.h"
#include "vtkHandleRepresentation.h"
#include "vtkCoordinate.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkEvent.h"
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkSeedWidget, "$Revision: 1.9 $");
vtkStandardNewMacro(vtkSeedWidget);


// The vtkSeedList is a PIMPLed vector<T>.
class vtkSeedList : public vtkstd::vector<vtkHandleWidget*> {};
typedef vtkstd::vector<vtkHandleWidget*>::iterator vtkSeedListIterator;


//----------------------------------------------------------------------
vtkSeedWidget::vtkSeedWidget()
{
  this->ManagesCursor = 1;
  this->WidgetState = vtkSeedWidget::Start;

  // The widgets for moving the seeds.
  this->Seeds = new vtkSeedList;

  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkWidgetEvent::AddPoint,
                                          this, vtkSeedWidget::AddPointAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent,
                                          vtkWidgetEvent::Completed,
                                          this, vtkSeedWidget::CompletedAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkSeedWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkSeedWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent,
                                          vtkEvent::NoModifier, 47, 1, NULL,
                                          vtkWidgetEvent::Delete,
                                          this, vtkSeedWidget::DeleteAction);
}

//----------------------------------------------------------------------
inline void vtkSeedWidget::DeleteSeed(int i)
{
  (*this->Seeds)[i]->RemoveObservers(vtkCommand::StartInteractionEvent);
  (*this->Seeds)[i]->RemoveObservers(vtkCommand::InteractionEvent);
  (*this->Seeds)[i]->RemoveObservers(vtkCommand::EndInteractionEvent);
  (*this->Seeds)[i]->Delete();
}

//----------------------------------------------------------------------
vtkSeedWidget::~vtkSeedWidget()
{
  // Loop over all seeds releasing their observes and deleting them
  for (unsigned int i=0; i<this->Seeds->size(); ++i)
    {
    this->DeleteSeed(i);
    }
  delete this->Seeds;
}

//----------------------------------------------------------------------
void vtkSeedWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    this->WidgetRep = vtkSeedRepresentation::New();
    }
}

//----------------------------------------------------------------------
void vtkSeedWidget::SetEnabled(int enabling)
{
  this->Superclass::SetEnabled(enabling);
  
  if ( ! enabling )
    {
    this->WidgetState = vtkSeedWidget::Start;
    vtkSeedListIterator siter;
    for (siter=this->Seeds->begin(); siter != this->Seeds->end(); ++siter )
      {
      (*siter)->SetEnabled(0);
      }
    }
}

//-------------------------------------------------------------------------
vtkHandleWidget *vtkSeedWidget::CreateHandleWidget(vtkSeedWidget *self, 
                                                   vtkSeedRepresentation *rep)
{
  // Create the handle widget or reuse an old one
  int currentHandleNumber = self->Seeds->size();
  vtkHandleWidget *widget = vtkHandleWidget::New();
  
  // Configure the handle widget
  widget->SetParent(self);
  widget->SetInteractor(self->Interactor);

  vtkHandleRepresentation *handleRep = rep->GetHandleRepresentation(currentHandleNumber);
  handleRep->SetRenderer(self->CurrentRenderer);
  widget->SetRepresentation(handleRep);

  // Now place the widget into the list of handle widgets (if not already there)
  self->Seeds->resize(currentHandleNumber+1,NULL);
  (*self->Seeds)[currentHandleNumber] = widget;

  return widget;
}


// The following methods are the callbacks that the seed widget responds to. 
//-------------------------------------------------------------------------
void vtkSeedWidget::AddPointAction(vtkAbstractWidget *w)
{
  vtkSeedWidget *self = reinterpret_cast<vtkSeedWidget*>(w);

  // Need to distinguish between placing handles and manipulating handles
  if ( self->WidgetState == vtkSeedWidget::MovingSeed ||
       self->WidgetState == vtkSeedWidget::PlacedSeeds )
    {
    return;
    }

  // compute some info we need for all cases
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  // When a seed is placed, a new handle widget must be created and enabled.
  int state = self->WidgetRep->ComputeInteractionState(X,Y);
  if ( state == vtkSeedRepresentation::NearSeed )
    {
    self->WidgetState = vtkSeedWidget::MovingSeed;
//    int currentHandleNumber = reinterpret_cast<vtkSeedRepresentation*>(self->WidgetRep)->
//      GetActiveHandle();

    // Invoke an event on ourself for the handles 
    self->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
    self->Superclass::StartInteraction();
    self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
    }

  else //we are placing a new seed
    {
    self->WidgetState = vtkSeedWidget::PlacingSeeds;
    double e[3]; e[2]=0.0;
    e[0] = static_cast<double>(X);
    e[1] = static_cast<double>(Y);
    

    vtkSeedRepresentation *rep = reinterpret_cast<vtkSeedRepresentation*>(self->WidgetRep);
    // if the handle representation is constrained, check to see if
    // the position follows the constraint.
    if(!rep->GetHandleRepresentation()->CheckConstraint(self->GetCurrentRenderer(), e))
      {
      return ;
      }
    int currentHandleNumber = rep->CreateHandle(e);
    vtkHandleWidget *currentHandle = self->CreateHandleWidget(self,rep);
    rep->SetSeedDisplayPosition(currentHandleNumber,e);
    currentHandle->SetEnabled(1);
    self->InvokeEvent(vtkCommand::PlacePointEvent,(void*)&(currentHandleNumber));
    self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
    }

  self->EventCallbackCommand->SetAbortFlag(1);
  self->Render();
}


//-------------------------------------------------------------------------
void vtkSeedWidget::CompletedAction(vtkAbstractWidget *w)
{
  vtkSeedWidget *self = reinterpret_cast<vtkSeedWidget*>(w);

  // Do something only if we are in the middle of placing the seeds
  if ( self->WidgetState != vtkSeedWidget::PlacingSeeds )
    {
    return;
    }
  
  // All we do is set the state to placed
  self->WidgetState = vtkSeedWidget::PlacedSeeds;
  self->EventCallbackCommand->SetAbortFlag(1);
}

//-------------------------------------------------------------------------
void vtkSeedWidget::CompleteInteraction()
{
  this->WidgetState = vtkSeedWidget::PlacedSeeds;
  this->EventCallbackCommand->SetAbortFlag(1);
}
//-------------------------------------------------------------------------
void vtkSeedWidget::RestartInteraction()
{
  this->WidgetState = vtkSeedWidget::Start;
}

//-------------------------------------------------------------------------
void vtkSeedWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkSeedWidget *self = reinterpret_cast<vtkSeedWidget*>(w);

  // Do nothing if outside
  if ( self->WidgetState == vtkSeedWidget::Start ||
       self->WidgetState == vtkSeedWidget::PlacedSeeds )
    {
    return;
    }

  // else we are moving a seed
  self->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
  self->RequestCursorShape(VTK_CURSOR_HAND);
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  self->Render();
}

//-------------------------------------------------------------------------
void vtkSeedWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkSeedWidget *self = reinterpret_cast<vtkSeedWidget*>(w);

  // Do nothing if outside
  if ( self->WidgetState != vtkSeedWidget::MovingSeed )
    {
    return;
    }

  // Invoke event for seed handle
  self->WidgetState = vtkSeedWidget::PlacingSeeds;
  self->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);
  self->EventCallbackCommand->SetAbortFlag(1);
  self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  self->Superclass::EndInteraction();
  self->Render();
}

//-------------------------------------------------------------------------
void vtkSeedWidget::DeleteAction(vtkAbstractWidget *w)
{
  vtkSeedWidget *self = reinterpret_cast<vtkSeedWidget*>(w);

  // Do nothing if outside
  if ( self->WidgetState != vtkSeedWidget::PlacingSeeds )
    {
    return;
    }

  // Remove last seed
  vtkSeedRepresentation *rep = reinterpret_cast<vtkSeedRepresentation*>(self->WidgetRep);
  rep->RemoveLastHandle();
  int last = self->Seeds->size()-1;
  (*self->Seeds)[last]->SetEnabled(0);
  self->DeleteSeed(last);
  self->Seeds->pop_back();

  // Got this event, abort processing if it
  self->EventCallbackCommand->SetAbortFlag(1);
  self->Render();
}

//----------------------------------------------------------------------
void vtkSeedWidget::SetProcessEvents(int pe)
{
  this->Superclass::SetProcessEvents(pe);

  vtkSeedListIterator iter = this->Seeds->begin();
  for (; iter != this->Seeds->end(); ++iter)
    {
    (*iter)->SetProcessEvents(pe);
    }
}

//----------------------------------------------------------------------
void vtkSeedWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);
}
