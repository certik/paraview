/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqLineEditNumberValidator.cxx,v $

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

/// \file pqLineEditNumberValidator.cxx
/// \date 3/23/2007

#include "pqLineEditNumberValidator.h"

#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>


pqLineEditNumberValidator::pqLineEditNumberValidator(bool decimalAllowed,
    QObject *parentObject)
  : QObject(parentObject)
{
  this->UsingDecimal = decimalAllowed;
}

bool pqLineEditNumberValidator::eventFilter(QObject *object, QEvent *e)
{
  QLineEdit *edit = qobject_cast<QLineEdit *>(object);
  if(edit && e->type() == QEvent::KeyPress)
    {
    QKeyEvent *ke = static_cast<QKeyEvent *>(e);
    if(ke->key() >= Qt::Key_0 && ke->key() <= Qt::Key_9)
      {
      return false;
      }
    else if(ke->key() == Qt::Key_Plus || ke->key() == Qt::Key_Minus ||
        ke->key() == Qt::Key_Period || ke->key() == Qt::Key_E)
      {
      return !this->UsingDecimal;
      }
    else if(ke->key() < Qt::Key_Escape)
      {
      return true;
      }
    }

  return false;
}


