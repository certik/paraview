/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqProcessModuleGUIHelper.cxx,v $

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
#include "pqProcessModuleGUIHelper.h"

#include "pqApplicationCore.h"
#include "pqPluginManager.h"
#include "pqOutputWindowAdapter.h"
#include "pqOutputWindow.h"
#include "pqCoreTestUtility.h"
#include "pqOptions.h"

#include <vtkObjectFactory.h>
#include <vtkProcessModuleConnectionManager.h>
#include <vtkProcessModule.h>
#include <vtkSMApplication.h>
#include <vtkSmartPointer.h>
#include <vtkSMProperty.h>
#include <vtkSMProxy.h>
#include <vtkSMProxyManager.h>
#include <vtkTimerLog.h>

#include <QApplication>
#include <QWidget>
#include <QTimer>
#include <QStringList>
////////////////////////////////////////////////////////////////////////////
// pqProcessModuleGUIHelper::pqImplementation

class pqProcessModuleGUIHelper::pqImplementation
{
public:
  pqImplementation() :
    OutputWindowAdapter(vtkSmartPointer<pqOutputWindowAdapter>::New()),
    OutputWindow(0),
    SMApplication(vtkSMApplication::New()),
    ApplicationCore(0),
    Window(0),
    EnableProgress(false),
    ReadyEnableProgress(false),
    LastProgress(0)
  {
    // Redirect Qt debug output to VTK ...
    qInstallMsgHandler(QtMessageOutput);
  }
  
  ~pqImplementation()
  {
    this->SMApplication->Finalize();
    this->SMApplication->Delete();
    delete this->Window;
    delete this->OutputWindow;
    delete this->ApplicationCore;
  }

  /// Routes Qt debug output through the VTK output window mechanism
  static void QtMessageOutput(QtMsgType type, const char *msg)
  {
    switch(type)
      {
      case QtDebugMsg:
        vtkOutputWindow::GetInstance()->DisplayText(msg);
        break;
      case QtWarningMsg:
        vtkOutputWindow::GetInstance()->DisplayErrorText(msg);
        break;
      case QtCriticalMsg:
        vtkOutputWindow::GetInstance()->DisplayErrorText(msg);
        break;
      case QtFatalMsg:
        vtkOutputWindow::GetInstance()->DisplayErrorText(msg);
        break;
      }
  }

  /// Converts VTK debug output into Qt signals
  vtkSmartPointer<pqOutputWindowAdapter> OutputWindowAdapter;
  /// Displays VTK debug output in a console window
  pqOutputWindow* OutputWindow;

  vtkSMApplication* SMApplication;
  pqApplicationCore* ApplicationCore;
  QWidget* Window;
  bool EnableProgress;
  bool ReadyEnableProgress;
  double LastProgress;
  pqCoreTestUtility TestUtility;
};

////////////////////////////////////////////////////////////////////////////
// pqProcessModuleGUIHelper

vtkCxxRevisionMacro(pqProcessModuleGUIHelper, "$Revision: 1.21 $");
//-----------------------------------------------------------------------------
pqProcessModuleGUIHelper::pqProcessModuleGUIHelper() :
  Implementation(new pqImplementation())
{
}

//-----------------------------------------------------------------------------
pqProcessModuleGUIHelper::~pqProcessModuleGUIHelper()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqProcessModuleGUIHelper::disableOutputWindow()
{
  this->Implementation->OutputWindowAdapter->setActive(false);
}

//-----------------------------------------------------------------------------
void pqProcessModuleGUIHelper::showOutputWindow()
{
  this->Implementation->OutputWindow->show();
  this->Implementation->OutputWindow->raise();
  this->Implementation->OutputWindow->activateWindow();
}

//-----------------------------------------------------------------------------
int pqProcessModuleGUIHelper::RunGUIStart(int argc, char** argv, 
  int vtkNotUsed(numServerProcs), int vtkNotUsed(myId))
{
  this->Implementation->SMApplication->Initialize();
  vtkSMProperty::SetCheckDomains(0);
  this->Implementation->SMApplication->ParseConfigurationFiles();
  
  if (!this->InitializeApplication(argc, argv))
    {
    return 1;
    }
  
  int status = 1;
  if (this->Implementation->Window)
    {
    this->Implementation->Window->show();
    
    // get the tester going when the application starts
    if(this->TestUtility())
      {
      if(pqOptions* const options = pqOptions::SafeDownCast(
             vtkProcessModule::GetProcessModule()->GetOptions()))
        {
        QMetaObject::invokeMethod(this->TestUtility(), "playTests",
          Qt::QueuedConnection,
          Q_ARG(QStringList, options->GetTestFiles()));
        }
      }

    // load client plugins
    pqPluginManager* pluginManager = 
      pqApplicationCore::instance()->getPluginManager();
    pluginManager->loadPlugins(NULL);

    // // Create the default connection.
    // pqServerResource resource = pqServerResource("builtin:");
    // this->Implementation->ApplicationCore->createServer(resource);

    // Starts the event loop.
    QCoreApplication* app = QApplication::instance();
    status = app->exec();
    }
  this->FinalizeApplication();
  
  // If there were any errors from Qt / VTK, ensure that we return an error code
  if(!status && this->Implementation->OutputWindowAdapter->getErrorCount())
    {
    status = 1;
    }
  
  return status;
}

//-----------------------------------------------------------------------------
int pqProcessModuleGUIHelper::InitializeApplication(int vtkNotUsed(argc), 
           char** vtkNotUsed(argv))
{
  this->Implementation->ApplicationCore = this->CreateApplicationCore();
  
  // Redirect VTK debug output to a Qt window ...
  this->Implementation->OutputWindow = new pqOutputWindow(0);
  this->Implementation->OutputWindow->setAttribute(Qt::WA_QuitOnClose, false);
  this->Implementation->OutputWindow->connect(this->Implementation->OutputWindowAdapter, 
    SIGNAL(displayText(const QString&)), SLOT(onDisplayText(const QString&)));
  this->Implementation->OutputWindow->connect(this->Implementation->OutputWindowAdapter, 
    SIGNAL(displayErrorText(const QString&)), SLOT(onDisplayErrorText(const QString&)));
  this->Implementation->OutputWindow->connect(this->Implementation->OutputWindowAdapter, 
    SIGNAL(displayWarningText(const QString&)), SLOT(onDisplayWarningText(const QString&)));
  this->Implementation->OutputWindow->connect(this->Implementation->OutputWindowAdapter, 
    SIGNAL(displayGenericWarningText(const QString&)), SLOT(onDisplayGenericWarningText(const QString&)));
  vtkOutputWindow::SetInstance(Implementation->OutputWindowAdapter);

  this->Implementation->Window = this->CreateMainWindow();

  return 1;
}

//-----------------------------------------------------------------------------
pqApplicationCore* pqProcessModuleGUIHelper::CreateApplicationCore()
{
  return new pqApplicationCore();
}

//-----------------------------------------------------------------------------
void pqProcessModuleGUIHelper::FinalizeApplication()
{
  delete this->Implementation->Window;
  this->Implementation->Window = 0;
  delete this->Implementation->ApplicationCore;
  this->Implementation->ApplicationCore = 0;
}

//-----------------------------------------------------------------------------
void pqProcessModuleGUIHelper::SendPrepareProgress()
{
  // set flag that we want a delayed progress 
  // in other words, the progress will be enabled when a current
  // process is taking long enough
  this->Implementation->ReadyEnableProgress = true;

  // I am disabling this. It does not have to be reset after every filter.
  // This way filters like plot XXX over time produce much nicer progress.
  // this->Implementation->LastProgress = vtkTimerLog::GetUniversalTime();
}

//-----------------------------------------------------------------------------
void pqProcessModuleGUIHelper::SendCleanupPendingProgress()
{
  this->Implementation->ReadyEnableProgress = false;
  if(this->Implementation->EnableProgress)
    {
    this->Implementation->ApplicationCore->cleanupPendingProgress();
    }
  this->Implementation->EnableProgress = false;
}

//-----------------------------------------------------------------------------
void pqProcessModuleGUIHelper::SetLocalProgress(const char* text, 
  int progress)
{
  // forgive those who don't call SendPrepareProgress beforehand
  if(this->Implementation->EnableProgress == false &&
     this->Implementation->ReadyEnableProgress == false &&
     progress == 0)
    {
    this->SendPrepareProgress();
    return;
    }
  
  // forgive those who don't cleanup or want to go the extra mile
  if(progress >= 100)
    {
    this->SendCleanupPendingProgress();
    return;
    }

  // only forward progress events to the GUI if we get at least .05 seconds
  // since the last time we forwarded the progress event
  double lastprog = vtkTimerLog::GetUniversalTime();
  if ( lastprog - this->Implementation->LastProgress < .05 )
    {
    return;
    }

  // We will show progress. Reset timer.
  this->Implementation->LastProgress = vtkTimerLog::GetUniversalTime();

  // delayed progress starting so the progress bar doesn't flicker
  // so much for the quick operations
  if(this->Implementation->EnableProgress == false)
    {
    this->Implementation->EnableProgress = true;
    this->Implementation->ApplicationCore->prepareProgress();
    }
  
  this->Implementation->LastProgress = lastprog;
  
  // chop of "vtk" prefix
  if ( strlen(text) > 4 && text[0] == 'v' && text[1] == 't' && text[2] == 'k' )
    {
    text += 3;
    }
  /*
  this->ModifiedEnableState = 1;
  this->SetStatusText(text);
  this->GetProgressGauge()->SetValue(val);
  */
  this->Implementation->ApplicationCore->sendProgress(text, progress);
  //cout << (name? name : "(null)") << " : " << progress << endl;
  // Here we would call something like
  // this->Window->SetProgress(name, progress). 
  // Then the Window can update the progress bar, or something.
}

//-----------------------------------------------------------------------------
void pqProcessModuleGUIHelper::ExitApplication()
{
  QCoreApplication* app = QApplication::instance();
  if(app)
    {
    app->exit();
    }
}

//-----------------------------------------------------------------------------
void pqProcessModuleGUIHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  //os << indent << "Application: " << this->Implementation->Application << endl;
}

//-----------------------------------------------------------------------------
bool pqProcessModuleGUIHelper::compareView(
  const QString& vtkNotUsed(referenceImage), 
  double vtkNotUsed(threshold), ostream& vtkNotUsed(output), 
  const QString& vtkNotUsed(tempDirectory))
{
  return false;
}

//-----------------------------------------------------------------------------
QWidget* pqProcessModuleGUIHelper::GetMainWindow()
{
  return this->Implementation->Window;
}

//-----------------------------------------------------------------------------
pqTestUtility* pqProcessModuleGUIHelper::TestUtility()
{
  return &this->Implementation->TestUtility;
}

