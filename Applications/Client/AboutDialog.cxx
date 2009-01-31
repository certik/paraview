/*=========================================================================

   Program: ParaView
   Module:    $RCS $

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

#include "AboutDialog.h"

#include "ui_AboutDialog.h"

#include "pqApplicationCore.h"
#include "pqOptions.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerResource.h"
#include "QtTestingConfigure.h"
#include "vtkProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkPVServerInformation.h"

#include <QHeaderView>
#include "vtksys/ios/sstream"

//-----------------------------------------------------------------------------
AboutDialog::AboutDialog(QWidget* Parent) :
  QDialog(Parent),
  Ui(new Ui::AboutDialog())
{
  this->Ui->setupUi(this);
  this->setObjectName("AboutDialog");

  // get extra information and put it in
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pqOptions* opts = pqOptions::SafeDownCast(pm->GetOptions());

  vtksys_ios::ostringstream str;
  vtkIndent indent;
  opts->PrintSelf(str, indent.GetNextIndent());
  str << ends;
  QString info = str.str().c_str();
  int idx = info.indexOf("Runtime information:");
  info = info.remove(0, idx);

  this->Ui->VersionLabel->setText(
    QString("<html><b>Version: <i>%1</i></b></html>").arg(PARAVIEW_VERSION_FULL));

  this->AddClientInformation();
  this->AddServerInformation();

 // this->Ui->ClientInformation->append("<a href=\"http://www.paraview.org\">www.paraview.org</a>");
  //  this->Ui->ClientInformation->append("<a href=\"http://www.kitware.com\">www.kitware.com</a>");
  
  // For now, don't add any runtime information, it's 
  // incorrect for PV3 (made sense of PV2).
  // this->Ui->Information->append("\n");
  // this->Ui->Information->append(info);
  //this->Ui->ClientInformation->moveCursor(QTextCursor::Start);
 // this->Ui->ClientInformation->viewport()->setBackgroundRole(QPalette::Window);
}

//-----------------------------------------------------------------------------
AboutDialog::~AboutDialog()
{
  delete this->Ui;
}

//-----------------------------------------------------------------------------
inline void addItem(QTreeWidget* tree, const QString& key, const QString& value)
{
  QTreeWidgetItem* item = new QTreeWidgetItem(tree);
  item->setText(0, key);
  item->setText(1, value);
}
inline void addItem(QTreeWidget* tree, const QString& key, int value)
{
  ::addItem(tree, key, QString("%1").arg(value));
}

//-----------------------------------------------------------------------------
void AboutDialog::AddClientInformation()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pqOptions* opts = pqOptions::SafeDownCast(pm->GetOptions());

  QTreeWidget* tree = this->Ui->ClientInformation;

  ::addItem(tree, "Version", PARAVIEW_VERSION_FULL);
#if defined(PARAVIEW_ENABLE_PYTHON)
  ::addItem(tree, "Embeded Python", "On");
#else
  ::addItem(tree, "Embeded Python", "Off");
#endif

#if defined(QT_TESTING_WITH_PYTHON)
  ::addItem(tree, "Python Testing", "On");
#else 
  ::addItem(tree, "Python Testing", "Off");
#endif

  ::addItem(tree, "Disable Registry", opts->GetDisableRegistry()? "On" : "Off");
  ::addItem(tree, "Test Directory", opts->GetTestDirectory());
  ::addItem(tree, "Data Directory", opts->GetDataDirectory());
  tree->header()->setResizeMode(QHeaderView::ResizeToContents);
}

//-----------------------------------------------------------------------------
void AboutDialog::AddServerInformation()
{
  QTreeWidget* tree = this->Ui->ServerInformation;
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QList<pqServer*> servers = smmodel->findItems<pqServer*>();
  if (servers.size() > 0)
    {
    this->AddServerInformation(servers[0], tree);
    tree->header()->setResizeMode(QHeaderView::ResizeToContents);
    }
}

//-----------------------------------------------------------------------------
void AboutDialog::AddServerInformation(pqServer* server, QTreeWidget* tree)
{
  pqOptions* clientOptions = pqOptions::SafeDownCast(server->getOptions());
  vtkPVServerInformation* serverInfo = server->getServerInformation(); 

  if (!server->isRemote())
    {
    ::addItem(tree, "Remote Connection", "No");
    ::addItem(tree, "Render View Type", server->getRenderViewXMLName());
    return;
    }
 
  const pqServerResource& resource = server->getResource();
  QString scheme = resource.scheme();
  bool separate_render_server = (scheme == "cdsrs" || scheme == "cdsrsrc");
  bool reverse_connection = (scheme == "csrc" || scheme == "cdsrsrc");
  ::addItem(tree, "Remote Connection", "Yes");
  ::addItem(tree, "Render View Type", server->getRenderViewXMLName());
  ::addItem(tree, "Separate Render Server", separate_render_server? "Yes":"No");
  ::addItem(tree, "Reverse Connection", reverse_connection? "Yes" : "No");

  int port;
  if (separate_render_server)
    {
    if (!reverse_connection)
      {
      ::addItem(tree, "Data Server Host", resource.dataServerHost());
      }
    port = resource.dataServerPort();
    ::addItem(tree, "Data Server Port", port==-1? clientOptions->GetDataServerPort(): port);
    if (!reverse_connection)
      {
      ::addItem(tree, "Render Server Host", resource.renderServerHost());
      }
    port = resource.renderServerPort();
    ::addItem(tree, "Render Server Port", port==-1? clientOptions->GetRenderServerPort() : port);
    }
  else
    {
    if (!reverse_connection)
      {
      ::addItem(tree, "Server Host", resource.host());
      }
    port = resource.port();
    ::addItem(tree, "Server Port", port==-1?clientOptions->GetServerPort():port);
    }
  // TODO: handle separate render server partitions.
  ::addItem(tree, "Number of Processes", server->getNumberOfPartitions());
  
  ::addItem(tree, "Disable Remote Rendering",
    serverInfo->GetRemoteRendering()? "Off":"On");
  ::addItem(tree, "IceT",
    serverInfo->GetUseIceT()? "On":"Off");

  if (serverInfo->GetTileDimensions()[0] > 0)
    {
    ::addItem(tree, "Tile Display", "On");
    ::addItem(tree, "Tile Dimensions", QString("(%1, %2)").arg(
        serverInfo->GetTileDimensions()[0]).arg(
        serverInfo->GetTileDimensions()[1]));
    ::addItem(tree, "Tile Mullions", QString("(%1, %2)").arg(
        serverInfo->GetTileMullions()[0]).arg(
        serverInfo->GetTileMullions()[1]));
    }
  else
    {
    ::addItem(tree, "Tile Display", "Off");
    }

  ::addItem(tree, "Write AVI Animations",
    serverInfo->GetAVISupport()? "On": "Off");
}

