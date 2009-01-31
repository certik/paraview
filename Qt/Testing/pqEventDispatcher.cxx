/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqEventDispatcher.cxx,v $

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

#include "pqEventDispatcher.h"

#include "pqEventPlayer.h"
#include "pqEventSource.h"

#include <QAbstractEventDispatcher>
#include <QtDebug>
#include <QTime>
#include <QTimer>
#include <QApplication>
#include <QEventLoop>
#include <QThread>

////////////////////////////////////////////////////////////////////////////
// pqEventDispatcher::pqImplementation

class pqEventDispatcher::pqImplementation
{
public:
  pqImplementation() :
    Source(0),
    Player(0),
    EventState(FlushEvents)
  {
    this->Timer.setSingleShot(true);
  }
  
  pqEventSource* Source;
  pqEventPlayer* Player;
  QTimer Timer;
  enum EventStates
    {
    FlushEvents,
    DoEvent,
    Done
    };
  int EventState;

  static int WaitTime;
};

int pqEventDispatcher::pqImplementation::WaitTime = 0;

////////////////////////////////////////////////////////////////////////////
// pqEventDispatcher

pqEventDispatcher::pqEventDispatcher() :
  Implementation(new pqImplementation())
{
  QObject::connect(this, SIGNAL(readyPlayNextEvent()),
                   this, SLOT(playNextEvent()));

  QObject::connect(&this->Implementation->Timer, SIGNAL(timeout()),
                   this, SLOT(checkPlayNextEvent()));
}

pqEventDispatcher::~pqEventDispatcher()
{
  delete this->Implementation;
}

void pqEventDispatcher::playEvents(pqEventSource& source, pqEventPlayer& player)
{
  if(this->Implementation->Source)
    {
    qCritical() << "Event dispatcher is already playing";
    return;
    }

  this->Implementation->Source = &source;
  this->Implementation->Player = &player;
    
  QApplication::setEffectEnabled(Qt::UI_General, false);

  this->Implementation->Timer.setInterval(1);
  this->Implementation->Timer.start();
  this->Implementation->EventState = pqImplementation::FlushEvents;
  this->Implementation->WaitTime = 0;
}

void pqEventDispatcher::checkPlayNextEvent()
{
  if(this->Implementation->EventState == pqImplementation::Done)
    {
    return;
    }
    
  this->Implementation->Timer.setInterval(1);

  // do an event every other time through here to be sure events are processed
  if(this->Implementation->WaitTime)
    {
    this->Implementation->Timer.setInterval(this->Implementation->WaitTime);
    this->Implementation->Timer.start();
    }
  else if(this->Implementation->EventState == pqImplementation::DoEvent)
    {
    pqEventDispatcher::processEventsAndWait(1);
    this->Implementation->EventState = pqImplementation::FlushEvents;
    this->Implementation->Timer.start();
    emit this->readyPlayNextEvent();
    }
  else
    {
    pqEventDispatcher::processEventsAndWait(1);
    this->Implementation->EventState = pqImplementation::DoEvent;
    this->Implementation->Timer.start();
    }
}

  
void pqEventDispatcher::playNextEvent()
{

  if(!this->Implementation->Source)
    {
    return;
    }

  QString object;
  QString command;
  QString arguments;
  
  // block signals as some event sources may interact with the event loop
  this->blockSignals(true);

  int result = this->Implementation->Source->getNextEvent(
                              object, command, arguments);
  this->blockSignals(false);

  if(result == pqEventSource::DONE)
    {
    this->stopPlayback();
    emit this->succeeded();
    return;
    }
  else if(result == pqEventSource::FAILURE)
    {
    this->stopPlayback();
    emit this->failed();
    return;
    }
    
  bool error = false;
  this->Implementation->Player->playEvent(object, command, arguments, error);
  if(error)
    {
    this->stopPlayback();
    emit this->failed();
    return;
    }
}

void pqEventDispatcher::stopPlayback()
{
  this->Implementation->Timer.stop();
  this->Implementation->EventState = pqImplementation::Done;
  
  this->Implementation->Source->stop();
    
  this->Implementation->Source = 0;
  this->Implementation->Player = 0;
  
  // ensure that everything is completed
  QCoreApplication::processEvents();
}

void pqEventDispatcher::processEventsAndWait(int ms)
{
  if(QThread::currentThread() == qApp->thread())
  {
    pqEventDispatcher::pqImplementation::WaitTime = ms <= 0 ? 1 : ms;
  }
  
  QEventLoop loop;
  QTimer::singleShot(ms, &loop, SLOT(quit()));
  loop.exec();
  
  if(QThread::currentThread() == qApp->thread())
  {
    pqEventDispatcher::pqImplementation::WaitTime = 0;
  }
}

