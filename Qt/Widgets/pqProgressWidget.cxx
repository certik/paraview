/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqProgressWidget.cxx,v $

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
#include "pqProgressWidget.h"

#include "pqProgressBar.h"

#include <QApplication>
#include <QGridLayout>
#include <QProgressBar>
#include <QToolButton>


//-----------------------------------------------------------------------------
pqProgressWidget::pqProgressWidget(QWidget* _parent/*=0*/)
  : QWidget(_parent)
{
  QGridLayout *gridLayout = new QGridLayout(this);
  gridLayout->setSpacing(4);
  gridLayout->setMargin(0);
  gridLayout->setObjectName("gridLayout");

  this->ProgressBar = new pqProgressBar(this);
  this->ProgressBar->setObjectName("ProgressBar");
  this->ProgressBar->setOrientation(Qt::Horizontal);
  gridLayout->addWidget(this->ProgressBar, 0, 1, 1, 1);

  this->AbortButton = new QToolButton(this);
  this->AbortButton->setObjectName("AbortButton");
  this->AbortButton->setIcon(
    QIcon(QString::fromUtf8(":/QtWidgets/Icons/pqDelete16.png")));
  this->AbortButton->setIconSize(QSize(12, 12));
  this->AbortButton->setToolTip(
    QApplication::translate("Form", "Abort", 0, QApplication::UnicodeUTF8));

  this->AbortButton->setEnabled(false);
  QObject::connect(this->AbortButton, SIGNAL(pressed()),
    this, SIGNAL(abortPressed()));

  gridLayout->addWidget(this->AbortButton, 0, 0, 1, 1);
}


//-----------------------------------------------------------------------------
pqProgressWidget::~pqProgressWidget()
{
  delete this->ProgressBar;
  delete this->AbortButton;
}

//-----------------------------------------------------------------------------
void pqProgressWidget::setProgress(const QString& message, int value)
{
  this->ProgressBar->setProgress(message, value);
}

//-----------------------------------------------------------------------------
void pqProgressWidget::enableProgress(bool enabled)
{
  this->ProgressBar->enableProgress(enabled);
}

//-----------------------------------------------------------------------------
void pqProgressWidget::enableAbort(bool enabled)
{
  this->AbortButton->setEnabled(enabled);
}
