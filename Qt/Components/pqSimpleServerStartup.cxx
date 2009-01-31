/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqSimpleServerStartup.cxx,v $

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

#include "pqEditServerStartupDialog.h"
#include "pqServerStartupDialog.h"
#include "pqSimpleServerStartup.h"

#include <pqApplicationCore.h>
#include <pqCommandServerStartup.h>
#include <pqCreateServerStartupDialog.h>
#include <pqEditServerStartupDialog.h>
#include <pqOptions.h>
#include <pqServer.h>
#include <pqServerBrowser.h>
#include <pqServerManagerModel.h>
#include <pqServerResource.h>
#include <pqServerStartup.h>
#include <pqServerStartups.h>
#include <pqObjectBuilder.h>

#include <vtkProcessModule.h>
#include <vtkProcessModuleConnectionManager.h>
#include <vtkMath.h>
#include <vtkTimerLog.h>
#include <vtkPVXMLElement.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QEventLoop>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QtDebug>
#include <QTimer>

#include <vtkstd/map>

//////////////////////////////////////////////////////////////////////////////
// pqSimpleServerStartup::pqImplementation

class pqSimpleServerStartup::pqImplementation
{
public:
  pqImplementation() :
    Startup(0),
    StartupDialog(0),
    PortID(0),
    DataServerPortID(0),
    RenderServerPortID(0)
  {
    this->DoneWithConnect = false;
    this->Timer.setInterval(10);
  }
  
  ~pqImplementation()
  {
    this->reset();
  }

  void reset()
  {
    this->Startup = 0;
    this->Timer.stop();

    delete this->StartupDialog;
    this->StartupDialog = 0;
    
    if(this->PortID)
      {
      vtkProcessModule::GetProcessModule()->StopAcceptingConnections(
        this->PortID);
      this->PortID = 0;
      }
    if(this->DataServerPortID)
      {
      vtkProcessModule::GetProcessModule()->StopAcceptingConnections(
        this->DataServerPortID);
      this->DataServerPortID = 0;
      }
    if(this->RenderServerPortID)
      {
      vtkProcessModule::GetProcessModule()->StopAcceptingConnections(
        this->RenderServerPortID);
      this->RenderServerPortID = 0;
      }
    
    this->Options.clear();
    this->Server = pqServerResource();
  }

  /// Stores a reference to the startup configuration to be used
  pqServerStartup* Startup;
  /// Used to check the reverse-connection status periodically
  QTimer Timer;
  /// Modal dialog used to display progress during startup
  pqServerStartupDialog* StartupDialog;
  /// Listening port identifier returned by the server manager during reverse-connection startup
  int PortID;
  int DataServerPortID;
  int RenderServerPortID;
  /// Stores options defined by the user prior to startup
  pqServerStartup::OptionsT Options;
  /** Stores a complete description of the server to be started
  (differs from the startup server if the user has chosen nonstandard ports */
  pqServerResource Server;

  bool DoneWithConnect;
};

//////////////////////////////////////////////////////////////////////////////
// pqSimpleServerStartup

pqSimpleServerStartup::pqSimpleServerStartup(QObject* p) :
  Superclass(p),
  Implementation(new pqImplementation())
{
  QObject::connect(
    &this->Implementation->Timer,
    SIGNAL(timeout()),
    this,
    SLOT(monitorReverseConnections()));
  this->IgnoreConnectIfAlreadyConnected = true;
}

//-----------------------------------------------------------------------------
pqSimpleServerStartup::~pqSimpleServerStartup()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqSimpleServerStartup::startServerBlocking(pqServerStartup& startup)
{
  // startServer() returns immediately, before the connection set 
  // actually happens. To simplify our life, I am
  // simply creating a event loop here which will exit once the starter
  // has some response.
  QEventLoop loop;
  QObject::connect(this, SIGNAL(serverCancelled()), 
    &loop, SLOT(quit()));
  QObject::connect(this, SIGNAL(serverFailed()), 
    &loop, SLOT(quit()));
  QObject::connect(this, SIGNAL(serverStarted(pqServer*)), 
    &loop, SLOT(quit()));

  this->Implementation->DoneWithConnect = false;
  this->startServer(startup);

  // It is possible that the above call immediately leads to a successful
  // (or failed) connection. In which case the signals we are lisening for
  // are already fired. So, we start the event loop only if the 
  // signals are still to be fired.
  if (!this->Implementation->DoneWithConnect)
    {
    loop.exec();
    }
}

//-----------------------------------------------------------------------------
void pqSimpleServerStartup::startServer(pqServerStartup& startup)
{
  // Get the named startup ...
  this->Implementation->Startup = &startup;
  this->Implementation->Server = startup.getServer();

  // If requested server is already running, nothing needs to be done.
  if (this->IgnoreConnectIfAlreadyConnected)
    {
    if (pqServer* const existing_server =
      pqApplicationCore::instance()->getServerManagerModel()->findServer(
        this->Implementation->Server))
      {
      this->started(existing_server);
      return;
      }
    }

  // Prompt the user for runtime server arguments ...
  if(!this->promptRuntimeArguments())
    {
    this->cancelled();
    return;
    }    

  this->disconnectAllServers();

  // Branch based on the connection type - builtin, forward, or reverse ...
  if(startup.getServer().scheme() == "builtin")
    {
    this->startBuiltinConnection();
    }
  else if(startup.getServer().scheme() == "cs" || startup.getServer().scheme() == "cdsrs")
    {
    this->startForwardConnection();
    }
  else if(startup.getServer().scheme() == "csrc" || startup.getServer().scheme() == "cdsrsrc")
    {
    this->startReverseConnection();
    }
  else
    {
    qCritical() << "Unknown server scheme: " << startup.getServer().scheme();
    this->failed();
    }
}

//-----------------------------------------------------------------------------
void pqSimpleServerStartup::startServer(const pqServerResource& server)
{
  pqServerStartups& server_startups = 
    pqApplicationCore::instance()->serverStartups();

  // There may be zero, one, or more-than-one startup already configured for this server
  const pqServerStartups::StartupsT startups =
    server_startups.getStartups(server);
    
  // No startup, yet, so prompt the user to create one ...
  if(0 == startups.size())
    {
    pqCreateServerStartupDialog create_server_dialog(server);
    if(QDialog::Accepted == create_server_dialog.exec())
      {
      pqEditServerStartupDialog edit_server_dialog(
        server_startups,
        create_server_dialog.getName(),
        create_server_dialog.getServer());
      if(QDialog::Accepted == edit_server_dialog.exec())
        {
        if(pqServerStartup* const startup = server_startups.getStartup(
          create_server_dialog.getName()))
          {
          this->startServer(*startup);
          }
        }
      }
    }
  // Exactly one startup, so just use it already ...
  else if(1 == startups.size())
    {
    if(pqServerStartup* const startup = server_startups.getStartup(startups[0]))
      {
      this->startServer(*startup);
      }
    }
  else
    {
    // More than one startup. If we are already connected 
    // to one of the startups that provide the data then we do thing,
    // else prompt the user to pick one ...

    if (this->IgnoreConnectIfAlreadyConnected)
      {
      // Are we connected to one of the startups?
      foreach (QString startup_name, startups)
        {
        pqServerStartup* const startup = 
          server_startups.getStartup(startup_name);
        if (pqApplicationCore::instance()->getServerManagerModel()->findServer(
            startup->getServer()))
          {
          // we call startServer so it performs all the 
          // necessary actions.
          this->startServer(*startup);
          return;
          }
        }
      }

    pqServerBrowser dialog(server_startups);
    dialog.setMessage(
      QString(tr("Pick the configuration for starting %1")).arg(
        server.schemeHosts().toURI()));
    QStringList ignoreList;
    pqServerStartups::StartupsT allStartups = server_startups.getStartups();
    foreach (QString current_startup, allStartups)
      {
      if (!startups.contains(current_startup))
        {
        ignoreList << current_startup;
        }
      }
    dialog.setIgnoreList(ignoreList);
    
    if(QDialog::Accepted == dialog.exec())
      {
      if(dialog.getSelectedServer())
        {
        this->startServer(*dialog.getSelectedServer());
        }
      }
    else
      {
      this->cancelled();
      }
    }
}

//-----------------------------------------------------------------------------
void pqSimpleServerStartup::reset()
{
  this->Implementation->reset();
  QObject::disconnect(
    pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverAdded(pqServer*)),
    this,
    SLOT(finishReverseConnection(pqServer*)));
}

//-----------------------------------------------------------------------------
void pqSimpleServerStartup::cancelled()
{
  this->reset();
  this->Implementation->DoneWithConnect = true;
  emit this->serverCancelled();
}

//-----------------------------------------------------------------------------
void pqSimpleServerStartup::failed()
{
  this->reset();
  this->Implementation->DoneWithConnect = true;
  emit this->serverFailed();
}

//-----------------------------------------------------------------------------
void pqSimpleServerStartup::started(pqServer* server)
{
  this->reset();
  this->Implementation->DoneWithConnect = true;
  emit this->serverStarted(server);
}

//-----------------------------------------------------------------------------
bool pqSimpleServerStartup::promptRuntimeArguments()
{
  // See whether we need to prompt the user, if not we're done ...
  vtkPVXMLElement* xml = this->Implementation->Startup->getConfiguration();

  if(!xml)
    {
    return true;
    }
  
  vtkPVXMLElement* xml_options = xml->FindNestedElementByName("Options");
  if(!xml_options)
    {
    return true;
    }

  // Dynamically-generate a dialog based on user options ...
  QDialog dialog;
  dialog.setWindowTitle("Start " + this->Implementation->Server.toURI());
  
  QGridLayout* const layout = new QGridLayout();
  dialog.setLayout(layout);

  const int label_column = 0;
  const int widget_column = 1;
  
  typedef vtkstd::map<vtkPVXMLElement*, QWidget*> widgets_t;
  widgets_t widgets;
  
  vtkstd::map<QString, QString> true_values;
  vtkstd::map<QString, QString> false_values;

  int num = xml_options->GetNumberOfNestedElements();
  for(int i=0; i<num; i++)
    {
    vtkPVXMLElement* xml_option = xml_options->GetNestedElement(i);
    if(QString(xml_option->GetName()) == "Option")
      {
      const QString option_name = xml_option->GetAttribute("name");
      const QString option_label = xml_option->GetAttribute("label");
      const bool option_readonly = QString(xml_option->GetAttribute("readonly")) == "true";

      vtkPVXMLElement* xml_type = xml_option->GetNestedElement(0);
      if(xml_type)
        {
        const int row = layout->rowCount();
        
        layout->addWidget(new QLabel(option_label + ":"), row, label_column, Qt::AlignLeft | Qt::AlignVCenter);
        
        if(QString(xml_type->GetName()) == "Range")
          {
          const QString range_type = xml_type->GetAttribute("type");
          if(range_type == "int")
            {
            const QString widget_min = xml_type->GetAttribute("min");
            const QString widget_max = xml_type->GetAttribute("max");
            const QString widget_step = xml_type->GetAttribute("step");
            const QString widget_default = xml_type->GetAttribute("default");

            QSpinBox* const widget = new QSpinBox();
            widget->setMinimum(widget_min.toInt());
            widget->setMaximum(widget_max.toInt());
            widget->setSingleStep(widget_step.toInt());
            if(widget_default == "random")
              {
              // We need a seed that changes every execution. Get the
              // universal time as double and then add all the bytes
              // together to get a nice seed without causing any overflow.
              long rseed = 0;
              double atime = vtkTimerLog::GetUniversalTime()*1000;
              char* tc = (char*)&atime;
              for (unsigned int ic=0; ic<sizeof(double); ic++)
                {
                rseed += tc[ic];
                }
              vtkMath::RandomSeed(rseed);
              widget->setValue(static_cast<int>(
                vtkMath::Random(widget_min.toInt(), widget_max.toInt())));
              }
            else
              {
              widget->setValue(widget_default.toInt());
              }
            widget->setEnabled(!option_readonly);
            
            layout->addWidget(widget, row, widget_column, Qt::AlignLeft | Qt::AlignVCenter);
            widgets[xml_option] = widget;
            }
          else if(range_type == "double")
            {
            const QString widget_min = xml_type->GetAttribute("min");
            const QString widget_max = xml_type->GetAttribute("max");
            const QString widget_step = xml_type->GetAttribute("step");
            const QString widget_precision = xml_type->GetAttribute("precision");
            const QString widget_default = xml_type->GetAttribute("default");

            QDoubleSpinBox* const widget = new QDoubleSpinBox();
            widget->setMinimum(widget_min.toDouble());
            widget->setMaximum(widget_max.toDouble());
            widget->setSingleStep(widget_step.toDouble());
            widget->setDecimals(widget_precision.toInt());
            widget->setValue(widget_default.toDouble());
            widget->setEnabled(!option_readonly);
            
            layout->addWidget(widget, row, widget_column, Qt::AlignLeft | Qt::AlignVCenter);
            widgets[xml_option] = widget;
            }
          }
        else if(QString(xml_type->GetName()) == "String")
          {
          const QString widget_default = xml_type->GetAttribute("default");
          QLineEdit* const widget = new QLineEdit(widget_default);
          widget->setEnabled(!option_readonly);
          layout->addWidget(widget, row, widget_column, Qt::AlignLeft | Qt::AlignVCenter);
          widgets[xml_option] = widget;
          }
        else if(QString(xml_type->GetName()) == "Boolean")
          {
          const QString widget_true = xml_type->GetAttribute("true");
          const QString widget_false = xml_type->GetAttribute("false");
          const QString widget_default = xml_type->GetAttribute("default");
          
          QCheckBox* const widget = new QCheckBox();
          widget->setChecked(widget_default == "true");
          widget->setEnabled(!option_readonly);
          
          layout->addWidget(widget, row, widget_column, Qt::AlignLeft | Qt::AlignVCenter);
          widgets[xml_option] = widget;
          true_values[option_name] = widget_true;
          false_values[option_name] = widget_false;
          }
        else if(QString(xml_type->GetName()) == "Enumeration")
          {
          const QString widget_default = xml_type->GetAttribute("default");
          QComboBox* const widget = new QComboBox();

          int numchild = xml_type->GetNumberOfNestedElements();
          for(int j=0; j<numchild; j++)
            {
            vtkPVXMLElement* xml_enumeration = xml_type->GetNestedElement(j);
            if(QString(xml_enumeration->GetName()) == "Entry") 
              {
              const QString xml_value = xml_enumeration->GetAttribute("value");
              const QString xml_label = xml_enumeration->GetAttribute("label");
              widget->addItem(xml_label, xml_value);
              }
            }
          
          widget->setCurrentIndex(widget->findData(widget_default));
          widget->setEnabled(!option_readonly);
          
          layout->addWidget(widget, row, widget_column, Qt::AlignLeft | Qt::AlignVCenter);
          widgets[xml_option] = widget;
          }
        }
      }
    }
    
  layout->setRowStretch(layout->rowCount(), 1);
  
  QHBoxLayout* const button_layout = new QHBoxLayout();
  button_layout->addStretch(1);
  
  QPushButton* const ok_button = new QPushButton(tr("Connect"));
  QObject::connect(ok_button, SIGNAL(clicked()), &dialog, SLOT(accept()));
  button_layout->addWidget(ok_button);
  
  QPushButton* const cancel_button = new QPushButton(tr("Cancel"));
  QObject::connect(cancel_button, SIGNAL(clicked()), &dialog, SLOT(reject()));
  button_layout->addWidget(cancel_button);
  
  layout->addLayout(button_layout, layout->rowCount(), 0, 1, 2);
  
  if(QDialog::Accepted != dialog.exec())
    return false;

  for(widgets_t::const_iterator option = widgets.begin(); option != widgets.end(); ++option)
    {
    vtkPVXMLElement* xml_option = option->first;
    const QString option_name = xml_option->GetAttribute("name");
    QWidget* const option_widget = option->second;
    QString val;
    
    if(QSpinBox* const widget1 = qobject_cast<QSpinBox*>(option_widget))
      {
      val = QString::number(widget1->value());
      }
    else if(QDoubleSpinBox* const widget2 = qobject_cast<QDoubleSpinBox*>(option_widget))
      {
      val = QString::number(widget2->value());
      }
    else if(QLineEdit* const widget3 = qobject_cast<QLineEdit*>(option_widget))
      {
      val = widget3->text();
      }
    else if(QCheckBox* const widget4 = qobject_cast<QCheckBox*>(option_widget))
      {
      val = widget4->isChecked() ? true_values[option_name] : false_values[option_name];
      }
    else if(QComboBox* const widget5 = qobject_cast<QComboBox*>(option_widget))
      {
      val = widget5->itemData(widget5->currentIndex()).toString();
      }
      
    this->Implementation->Options[option_name] = val;
    // remember defaults
    xml_option->GetNestedElement(0)->SetAttribute("default", val.toAscii().data());
    }

  if(this->Implementation->Options.contains("PV_SERVER_PORT"))
    {
    if(!this->Implementation->Options["PV_SERVER_PORT"].isEmpty())
      {
      this->Implementation->Server.setPort(this->Implementation->Options["PV_SERVER_PORT"].toInt());
      }
    }
    
  if(this->Implementation->Options.contains("PV_DATA_SERVER_PORT"))
    {
    if(!this->Implementation->Options["PV_DATA_SERVER_PORT"].isEmpty())
      {
      this->Implementation->Server.setDataServerPort(this->Implementation->Options["PV_DATA_SERVER_PORT"].toInt());
      }
    }
    
  if(this->Implementation->Options.contains("PV_RENDER_SERVER_PORT"))
    {
    if(!this->Implementation->Options["PV_RENDER_SERVER_PORT"].isEmpty())
      {
      this->Implementation->Server.setRenderServerPort(this->Implementation->Options["PV_RENDER_SERVER_PORT"].toInt());
      }
    }

  return true;
}

//-----------------------------------------------------------------------------
void pqSimpleServerStartup::startBuiltinConnection()
{
  this->Implementation->StartupDialog =
    new pqServerStartupDialog(this->Implementation->Server, false);
  this->Implementation->StartupDialog->show();

  pqServer* const server = pqApplicationCore::instance()->getObjectBuilder()->
    createServer(pqServerResource("builtin:"));

  this->Implementation->StartupDialog->hide();
    
  if(server)
    {
    this->started(server);
    }
  else
    {
    this->failed();
    }
}

//-----------------------------------------------------------------------------
void pqSimpleServerStartup::startForwardConnection()
{
  this->Implementation->StartupDialog =
    new pqServerStartupDialog(this->Implementation->Server, false);
  this->Implementation->StartupDialog->show();
  
  QObject::connect(
    this->Implementation->Startup,
    SIGNAL(succeeded()),
    this,
    SLOT(forwardConnectServer()));
    
  QObject::connect(
    this->Implementation->Startup,
    SIGNAL(succeeded()),
    this->Implementation->StartupDialog,
    SLOT(hide()));
    
  QObject::connect(
    this->Implementation->Startup,
    SIGNAL(failed()),
    this,
    SLOT(failed()));
  
  QObject::connect(
    this->Implementation->Startup,
    SIGNAL(failed()),
    this->Implementation->StartupDialog,
    SLOT(hide()));
  
  // Special-case: ensure that PV_CONNECT_ID is propagated to the global
  // options object, so it is used by the connection
  if(pqOptions* const options =
    pqOptions::SafeDownCast(
      vtkProcessModule::GetProcessModule()->GetOptions()))
    {
    if(this->Implementation->Options.contains("PV_CONNECT_ID"))
      {
      options->SetConnectID(
        this->Implementation->Options["PV_CONNECT_ID"].toInt());
      }
    else
      {
      // If no connection id is specified in the connection options,
      // don't change the command line connection id.
      // options->SetConnectID(0);
      }
    }
  
  this->Implementation->Startup->execute(this->Implementation->Options);
}

//-----------------------------------------------------------------------------
void pqSimpleServerStartup::forwardConnectServer()
{
  pqServer* const server =
    pqApplicationCore::instance()->getObjectBuilder()->
    createServer(this->Implementation->Server);

  if(server)
    {
    this->started(server);
    }
  else
    {
    this->failed();
    }
}

//-----------------------------------------------------------------------------
void pqSimpleServerStartup::disconnectAllServers()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smModel = core->getServerManagerModel();
  while (smModel->getNumberOfItems<pqServer*>() > 0)
    {
    core->getObjectBuilder()->removeServer(smModel->getItemAtIndex<pqServer*>(0));
    }
}

//-----------------------------------------------------------------------------
void pqSimpleServerStartup::startReverseConnection()
{
  vtkProcessModule* const process_module = vtkProcessModule::GetProcessModule();
  
  QObject::connect(
    pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverAdded(pqServer*)),
    this,
    SLOT(finishReverseConnection(pqServer*)));
  
  if(this->Implementation->Server.scheme() == "csrc")
    {
    this->Implementation->PortID = process_module->AcceptConnectionsOnPort(
      this->Implementation->Server.port(11111));
    }
  else if(this->Implementation->Server.scheme() == "cdsrsrc")
    {
    process_module->AcceptConnectionsOnPort(
      this->Implementation->Server.dataServerPort(11111),
      this->Implementation->Server.renderServerPort(22221),
      this->Implementation->DataServerPortID,
      this->Implementation->RenderServerPortID);
    }
    
  this->Implementation->StartupDialog =
    new pqServerStartupDialog(this->Implementation->Server, true);
  this->Implementation->StartupDialog->show();

  QObject::connect(
    this->Implementation->StartupDialog,
    SIGNAL(rejected()),
    this,
    SLOT(cancelled()));

  QObject::connect(
    this->Implementation->Startup,
    SIGNAL(succeeded()),
    &this->Implementation->Timer,
    SLOT(start()));
    
  QObject::connect(
    this->Implementation->Startup,
    SIGNAL(failed()),
    this,
    SLOT(failed()));
  
  QObject::connect(
    this->Implementation->Startup,
    SIGNAL(failed()),
    this->Implementation->StartupDialog,
    SLOT(hide()));

  QObject::connect(
    this->Implementation->Startup,
    SIGNAL(failed()),
    &this->Implementation->Timer,
    SLOT(stop()));

  // Special-case: ensure that PV_CONNECT_ID is propagated to the global
  // options object, so it is used by the connection
  if(pqOptions* const options =
    pqOptions::SafeDownCast(
      vtkProcessModule::GetProcessModule()->GetOptions()))
    {
    if(this->Implementation->Options.contains("PV_CONNECT_ID"))
      {
      options->SetConnectID(
        this->Implementation->Options["PV_CONNECT_ID"].toInt());
      }
    else
      {
      // If no connection id is specified in the connection options,
      // don't change the command line connection id.
      // options->SetConnectID(0);
      }
    }
  
  this->Implementation->Startup->execute(this->Implementation->Options);
}

//-----------------------------------------------------------------------------
void pqSimpleServerStartup::monitorReverseConnections()
{
  vtkProcessModule* const process_module = vtkProcessModule::GetProcessModule();
  if(-1 == process_module->MonitorConnections(10))
    {
    this->Implementation->Timer.stop();
    this->Implementation->StartupDialog->hide();
    this->failed();
    }
}

//-----------------------------------------------------------------------------
void pqSimpleServerStartup::finishReverseConnection(pqServer* server)
{
  QObject::disconnect(
    pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverAdded(pqServer*)),
    this,
    SLOT(finishReverseConnection(pqServer*)));

  server->setResource(this->Implementation->Server);

  this->Implementation->Timer.stop();
  this->Implementation->StartupDialog->hide();

  // It is essential that pqApplicationCore fires the finishedAddingServer()
  // signal on server creation. Make it fire explicitly.
  pqApplicationCore::instance()->getObjectBuilder()->fireFinishedAddingServer(server);
  this->started(server);
}

