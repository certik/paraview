/*=========================================================================

   Program: ParaView
Module:    $RCSfile: ProcessModuleGUIHelper.cxx,v $

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

#include "ProcessModuleGUIHelper.h"

#include <QTimer>
#include <QBitmap>
#include "MainWindow.h"

#include <pqApplicationCore.h>
#include <vtkObjectFactory.h>

#include "vtkPVConfig.h"

vtkStandardNewMacro(ProcessModuleGUIHelper);
vtkCxxRevisionMacro(ProcessModuleGUIHelper, "$Revision: 1.11 $");

//-----------------------------------------------------------------------------
ProcessModuleGUIHelper::ProcessModuleGUIHelper()
{
  QPixmap pix(":/pqClient/PVSplashScreen.png");
  this->Splash = new QSplashScreen(pix);
  this->Splash->setMask(pix.createMaskFromColor(QColor(Qt::transparent)));
  this->Splash->setAttribute(Qt::WA_DeleteOnClose);
  this->Splash->show();
}

//-----------------------------------------------------------------------------
ProcessModuleGUIHelper::~ProcessModuleGUIHelper()
{
}

//-----------------------------------------------------------------------------
QWidget* ProcessModuleGUIHelper::CreateMainWindow()
{
  pqApplicationCore::instance()->setApplicationName("ParaView" PARAVIEW_VERSION);
  pqApplicationCore::instance()->setOrganizationName("ParaView");
  QWidget* w = new MainWindow();
  QTimer::singleShot(10, this->Splash, SLOT(close()));
  return w;
}

//-----------------------------------------------------------------------------
void ProcessModuleGUIHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool ProcessModuleGUIHelper::compareView(const QString& ReferenceImage,
  double Threshold, ostream& Output, const QString& TempDirectory)
{
  if(MainWindow* const main_window = qobject_cast<MainWindow*>(this->GetMainWindow()))
  {
    return main_window->compareView(ReferenceImage, Threshold, Output, TempDirectory);
  }
  
  return false;
}
