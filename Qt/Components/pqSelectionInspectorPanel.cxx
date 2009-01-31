/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqSelectionInspectorPanel.cxx,v $

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
#include "pqSelectionInspectorPanel.h"
#include "ui_pqSelectionInspectorPanel.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkProcessModule.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSelection.h"
#include "vtkSmartPointer.h"
#include "vtkSMClientDeliveryRepresentationProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkUnstructuredGrid.h"

#include <QPointer>
#include <QtDebug>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QItemDelegate>

#include "pqActiveView.h"
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
//#include "pqDataSetModel.h"
//#include "pqElementInspectorView.h"
#include "pqObjectBuilder.h"
#include "pqOutputPort.h"
#include "pqPipelineFilter.h"
#include "pqPipelineRepresentation.h"
#include "pqPipelineSource.h"
#include "pqPropertyLinks.h"
#include "pqProxy.h"
#include "pqRenderView.h"
#include "pqRubberBandHelper.h"
#include "pqSelectionManager.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerSelectionModel.h"
#include "pqSignalAdaptors.h"
#include "pqSignalAdaptorTreeWidget.h"
#include "pqSMAdaptor.h"
#include "pqTreeWidgetItemObject.h"

//////////////////////////////////////////////////////////////////////////////
class pqSelectionInspectorTreeItem : public pqTreeWidgetItemObject
{
public:
  pqSelectionInspectorTreeItem(const QStringList& l) 
    : pqTreeWidgetItemObject(l)
  {
  }
  virtual bool operator< ( const QTreeWidgetItem & other ) const  
  {
    int sortCol = treeWidget()->sortColumn();
    double myNumber = text(sortCol).toDouble();
    double otherNumber = other.text(sortCol).toDouble();
    return myNumber < otherNumber;
  }
};

//////////////////////////////////////////////////////////////////////////////
// pqSelectionInspectorPanel::pqImplementation

struct pqSelectionInspectorPanel::pqImplementation : public Ui::SelectionInspectorPanel
{
public:
  pqImplementation() 
    {
    this->SourceLinks = new pqPropertyLinks;
    this->RepLinks = new pqPropertyLinks;
    this->IndicesAdaptor = 0;
    this->SelectionSource = 0;
    // Selection Labels Properties
    this->SelectionColorAdaptor = 0;
    this->PointColorAdaptor = 0;
    this->PointFontFamilyAdaptor = 0;
    this->PointLabelAlignmentAdaptor = 0;
    this->CellColorAdaptor = 0;
    this->CellFontFamilyAdaptor = 0;
    this->CellLabelAlignmentAdaptor = 0;

    this->FieldTypeAdaptor = 0;
    this->SelectionTypeAdaptor = 0;
    this->ThresholdsAdaptor = 0;
    this->ThresholdScalarArrayAdaptor = 0;
    this->SelectionSource = 0;
    this->Representation = 0;
    this->InputSource = 0;
    this->VTKConnectSelInput = vtkEventQtSlotConnect::New();
    this->VTKConnectRep = vtkEventQtSlotConnect::New();
    }

  ~pqImplementation()
    {
    this->SourceLinks->removeAllPropertyLinks();
    this->RepLinks->removeAllPropertyLinks();
    delete this->SourceLinks;
    delete this->RepLinks;

    delete this->SelectionColorAdaptor;
    delete this->PointColorAdaptor;
    delete this->PointFontFamilyAdaptor;
    delete this->PointLabelAlignmentAdaptor;
    delete this->CellColorAdaptor;
    delete this->CellFontFamilyAdaptor;
    delete this->CellLabelAlignmentAdaptor;
    delete this->FieldTypeAdaptor;
    delete this->SelectionTypeAdaptor;
    delete this->ThresholdsAdaptor;
    delete this->ThresholdScalarArrayAdaptor;
    this->SelectionSource = 0;
    this->InputSource = 0;
    this->Representation = 0;
    this->VTKConnectSelInput->Delete();
    this->VTKConnectRep->Delete();
    }

  QPointer<pqSelectionManager> SelectionManager;
  QPointer<pqRubberBandHelper> RubberBandHelper;

  pqSignalAdaptorTreeWidget* IndicesAdaptor;

  QPointer<pqPipelineSource> InputSource;
  // The representation whose properties are being edited.
  QPointer<pqDataRepresentation> Representation;
  vtkSmartPointer<vtkSMSourceProxy> SelectionSource;

  // Selection Labels Properties
  vtkEventQtSlotConnect* VTKConnectSelInput;
  vtkEventQtSlotConnect* VTKConnectRep;
  pqPropertyLinks* SourceLinks;
  pqPropertyLinks* RepLinks;

  pqSignalAdaptorColor* SelectionColorAdaptor;
  pqSignalAdaptorColor *PointColorAdaptor;
  pqSignalAdaptorComboBox *PointFontFamilyAdaptor;
  pqSignalAdaptorComboBox *PointLabelAlignmentAdaptor;

  pqSignalAdaptorColor *CellColorAdaptor;
  pqSignalAdaptorComboBox *CellFontFamilyAdaptor;
  pqSignalAdaptorComboBox *CellLabelAlignmentAdaptor;

  pqSignalAdaptorComboBox *FieldTypeAdaptor;
  pqSignalAdaptorComboBox *SelectionTypeAdaptor;
  pqSignalAdaptorTreeWidget* ThresholdsAdaptor;
  pqSignalAdaptorComboBox *ThresholdScalarArrayAdaptor;

  bool UseProcessID;
};

/////////////////////////////////////////////////////////////////////////////////
// pqSelectionInspectorPanel

pqSelectionInspectorPanel::pqSelectionInspectorPanel(QWidget *p) :
  QWidget(p),
  Implementation(new pqImplementation())
{
  this->setObjectName("ElementInspectorWidget");

  QVBoxLayout* vboxlayout = new QVBoxLayout(this);
  vboxlayout->setSpacing(0);
  vboxlayout->setMargin(0);
  vboxlayout->setObjectName("vboxLayout");

  QWidget* container = new QWidget(this);
  container->setObjectName("scrollWidget");
  container->setSizePolicy(QSizePolicy::MinimumExpanding,
    QSizePolicy::MinimumExpanding);

  QScrollArea* s = new QScrollArea(this);
  s->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  s->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  s->setWidgetResizable(true);
  s->setObjectName("scrollArea");
  s->setFrameShape(QFrame::NoFrame);
  s->setWidget(container);
  vboxlayout->addWidget(s);

  this->Implementation->setupUi(container);
  this->setupGUI();

  pqApplicationCore* core = pqApplicationCore::instance();
  this->setSelectionManager((pqSelectionManager*)(core->manager("SelectionManager")));

  // Connect the view manager to the pqActiveView.
  QObject::connect(&pqActiveView::instance(),
    SIGNAL(changed(pqView*)),
    this, SLOT(onActiveViewChanged()));

  this->setEnabled(false);

  QObject::connect(this->Implementation->Indices, SIGNAL(navigatedPastEnd()),
    this, SLOT(growIndicesTable()));
}

//-----------------------------------------------------------------------------
pqSelectionInspectorPanel::~pqSelectionInspectorPanel()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::activeServerChanged(pqServer* server)
{
  this->Implementation->UseProcessID =
    (server && server->getNumberOfPartitions() > 1);

  bool show_pid = this->Implementation->UseProcessID
    && !this->Implementation->UseGlobalIDs->isChecked();
  this->Implementation->Indices->setColumnHidden(0, !show_pid);
  this->Implementation->ProcessIDRange->setVisible(
    this->Implementation->UseProcessID);
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::setSelectionManager(pqSelectionManager* selMan)
{
  if (this->Implementation->SelectionManager)
    {
    QObject::disconnect(this->Implementation->SelectionManager, 0, this, 0);
    }

  this->Implementation->SelectionManager = selMan;

  if (selMan)
    {
    QObject::connect(selMan, SIGNAL(selectionChanged(pqSelectionManager*)),
      this, SLOT(onSelectionChanged()));
    }
}


//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::setRubberBandHelper(pqRubberBandHelper* helper)
{
  if (this->Implementation->RubberBandHelper)
    {
    QObject::disconnect(this->Implementation->RubberBandHelper, 0, this, 0);
    }

  this->Implementation->RubberBandHelper = helper;

  if (helper)
    {
    QObject::connect(helper, SIGNAL(selectionModeChanged(int)),
      this, SLOT(onSelectionModeChanged(int)));
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::onSelectionChanged()
{
  // The selection has changed, either a new selection was created
  // or an old one cleared.
  pqOutputPort* port = 
    this->Implementation->SelectionManager->getSelectedPort();
  pqPipelineSource* input = port? port->getSource() : 0;
  int portnum = port? port->getPortNumber(): -1;
  this->setInputSource(input, portnum);

  pqRenderView* view = qobject_cast<pqRenderView*>(
    pqActiveView::instance().current());
  if (view)
    {
    pqDataRepresentation *repr = NULL;
    if(input)
      {
      repr = input->getRepresentation(pqActiveView::instance().current());
      }

    this->setRepresentation(repr);
    }

  if(input)
    {
    vtkSMSourceProxy *inputsrc = 
      vtkSMSourceProxy::SafeDownCast(input->getProxy());
    //this->setEnabled(true);
    this->setSelectionSource(inputsrc->GetSelectionInput(portnum));
    }
  else
    {
    this->setSelectionSource(0);
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::setInputSource(
  pqPipelineSource* input, int vtkNotUsed(portnum)) 
{
  if(this->Implementation->InputSource == input)
    {
    return;
    }

  if (this->Implementation->InputSource)
    {
    QObject::disconnect(this->Implementation->InputSource, 0, this, 0);
    }

  this->Implementation->InputSource = input;

  this->updateSurfaceInformationAndDomains();
  this->updateSurfaceSelectionIDRanges();

  this->updateThreholdDataArrays();
  this->updateSelectionLabelModes();

  vtkSMSourceProxy *inputsrc = 
    input ? vtkSMSourceProxy::SafeDownCast(input->getProxy()) : 0;
  if(inputsrc)
    {
    //this->setEnabled(true);

    if(this->Implementation->ThresholdScalarArray->count()==0)
      {
      this->Implementation->frameThresholds->setEnabled(false);
      }
    else
      {
      this->Implementation->frameThresholds->setEnabled(true);
      }
    }
  else
    {
//    this->setEnabled(false);
    }
  
}

//-----------------------------------------------------------------------------
/// set the proxy to repr display properties for
void pqSelectionInspectorPanel::setRepresentation(
  pqDataRepresentation* repr) 
{
  if(this->Implementation->Representation == repr)
    {
    return;
    }

  if(this->Implementation->Representation)
    {
    // break all old links.
    this->Implementation->RepLinks->removeAllPropertyLinks();
    }
  if (this->Implementation->Representation)
    {
    QObject::disconnect(this->Implementation->Representation, 0, this, 0);
    }

  this->Implementation->Representation = repr;

  if (!repr )
    {
    return;
    }
  else
    {
    this->Implementation->VTKConnectRep->Disconnect();
    this->Implementation->VTKConnectRep->Connect(
      repr->getProxy()->GetProperty("SelectionPointFieldDataArrayName"),
      vtkCommand::ModifiedEvent, this, 
      SLOT(updateSelectionPointLabelArrayName()),
      NULL, 0.0,
      Qt::QueuedConnection);
    this->Implementation->VTKConnectRep->Connect(
      repr->getProxy()->GetProperty("SelectionCellFieldDataArrayName"),
      vtkCommand::ModifiedEvent, this, 
      SLOT(updateSelectionCellLabelArrayName()),
      NULL, 0.0,
      Qt::QueuedConnection);
    }

  this->updateSelectionRepGUI();
}

//-----------------------------------------------------------------------------
/// set the proxy to repr display properties for
void pqSelectionInspectorPanel::setSelectionSource(
  vtkSMSourceProxy* source) 
{
  if(this->Implementation->SelectionSource == source)
    {
    return;
    }

  if(this->Implementation->SelectionSource.GetPointer())
    {
    // break all old links.
    this->Implementation->SourceLinks->removeAllPropertyLinks();
    }

  this->Implementation->SelectionSource = source;
  if (!source )
    {
    this->setEnabled(false);
    return;
    }
  else
    {
    this->setEnabled(true);
    this->Implementation->VTKConnectSelInput->Disconnect();
    this->Implementation->VTKConnectSelInput->Connect(
      source,
      vtkCommand::ModifiedEvent, this, 
      SLOT(updateSelectionSource()),
      NULL, 0.0,
      Qt::QueuedConnection);
    }
  this->updateSelectionSourceGUI();
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::setupGUI()
{
  this->Implementation->SelectionTypeAdaptor = new pqSignalAdaptorComboBox(
    this->Implementation->comboSelectionType);
  QObject::connect(this->Implementation->SelectionTypeAdaptor, 
    SIGNAL(currentTextChanged(const QString&)),
    this, SLOT(updateSelectionContentType(const QString&)), Qt::QueuedConnection);

  this->Implementation->FieldTypeAdaptor = new pqSignalAdaptorComboBox(
    this->Implementation->comboFieldType);

  // Updates the enable state of "Containing Cells" check box based on whether
  // we are doing a point selection or cell selection. 
  QObject::connect(this->Implementation->FieldTypeAdaptor, 
    SIGNAL(currentTextChanged(const QString&)),
    this, SLOT(updateSelectionFieldType(const QString&)), Qt::QueuedConnection);

  this->setupSurfaceSelectionGUI();
  this->setupFrustumSelectionGUI();
  this->setupThresholdSelectionGUI();
  this->setupSelelectionLabelGUI();

  QObject::connect(this->Implementation->SourceLinks, SIGNAL(qtWidgetChanged()),
    this, SLOT(updateAllSelectionViews()));
  QObject::connect(this->Implementation->RepLinks, SIGNAL(qtWidgetChanged()),
    this, SLOT(updateRepresentationViews()));
  //this->updateSelectionContentType("Surface");

  //this->Implementation->comboSelectionType->setEnabled(false);
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateSelectionRepGUI()
{
  vtkSMProxy* reprProxy = this->Implementation->Representation->getProxy();

  // ---------------Selection properties------------------------

  // setup for line width and point size
  this->Implementation->RepLinks->addPropertyLink(this->Implementation->Sel_StyleLineWidth,
    "value", SIGNAL(valueChanged(double)),
    reprProxy, reprProxy->GetProperty("SelectionLineWidth"));
  this->Implementation->RepLinks->addPropertyLink(this->Implementation->Sel_StylePointSize,
    "value", SIGNAL(valueChanged(double)),
    reprProxy, reprProxy->GetProperty("SelectionPointSize"));

  // setup for opacity
  this->Implementation->RepLinks->addPropertyLink(this->Implementation->Sel_StyleOpacity,
    "value", SIGNAL(valueChanged(double)),
    reprProxy, reprProxy->GetProperty("SelectionOpacity"));
  // setup for choosing color
  this->Implementation->RepLinks->addPropertyLink(this->Implementation->SelectionColorAdaptor,
    "color", SIGNAL(colorChanged(const QVariant&)),
    reprProxy, reprProxy->GetProperty("SelectionColor"));

  // Selection Label Properties

  // Point labels properties
  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->checkBoxLabelPoints, "checked", SIGNAL(stateChanged(int)),
    reprProxy, reprProxy->GetProperty("SelectionPointLabelVisibility"));

  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->toolButtonBold_Point, "checked", SIGNAL(toggled(bool)),
    reprProxy, reprProxy->GetProperty("SelectionPointLabelBold"), 1);
  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->toolButtonItalic_Point, "checked", SIGNAL(toggled(bool)),
    reprProxy, reprProxy->GetProperty("SelectionPointLabelItalic"), 1);
  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->toolButtonShadow_Point, "checked", SIGNAL(toggled(bool)),
    reprProxy, reprProxy->GetProperty("SelectionPointLabelShadow"), 1);

  this->Implementation->RepLinks->addPropertyLink(this->Implementation->PointColorAdaptor, 
    "color", SIGNAL(colorChanged(const QVariant&)),
    reprProxy, reprProxy->GetProperty("SelectionPointLabelColor"));
  this->Implementation->RepLinks->addPropertyLink(this->Implementation->PointFontFamilyAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    reprProxy, reprProxy->GetProperty("SelectionPointLabelFontFamily"));
  this->Implementation->RepLinks->addPropertyLink(this->Implementation->PointLabelAlignmentAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    reprProxy, reprProxy->GetProperty("SelectionPointLabelJustification"));

  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->spinBoxSize_Point, "value", SIGNAL(valueChanged(int)),
    reprProxy, reprProxy->GetProperty("SelectionPointLabelFontSize"), 1);

  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->spinBoxOpacity_Point, "value", SIGNAL(valueChanged(double)),
    reprProxy, reprProxy->GetProperty("SelectionPointLabelOpacity"));

  // Cell Labels properties
  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->checkBoxLabelCells, "checked", SIGNAL(stateChanged(int)),
    reprProxy, reprProxy->GetProperty("SelectionCellLabelVisibility"));

  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->toolButtonBold_Cell, "checked", SIGNAL(toggled(bool)),
    reprProxy, reprProxy->GetProperty("SelectionCellLabelBold"), 1);
  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->toolButtonItalic_Cell, "checked", SIGNAL(toggled(bool)),
    reprProxy, reprProxy->GetProperty("SelectionCellLabelItalic"), 1);
  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->toolButtonShadow_Cell, "checked", SIGNAL(toggled(bool)),
    reprProxy, reprProxy->GetProperty("SelectionCellLabelShadow"), 1);

  this->Implementation->RepLinks->addPropertyLink(this->Implementation->CellColorAdaptor, 
    "color", SIGNAL(colorChanged(const QVariant&)),
    reprProxy, reprProxy->GetProperty("SelectionCellLabelColor"));
  this->Implementation->RepLinks->addPropertyLink(this->Implementation->CellFontFamilyAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    reprProxy, reprProxy->GetProperty("SelectionCellLabelFontFamily"));
  this->Implementation->RepLinks->addPropertyLink(this->Implementation->CellLabelAlignmentAdaptor,
    "currentText", SIGNAL(currentTextChanged(const QString&)),
    reprProxy, reprProxy->GetProperty("SelectionCellLabelJustification"));

  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->spinBoxSize_Cell, "value", SIGNAL(valueChanged(int)),
    reprProxy, reprProxy->GetProperty("SelectionCellLabelFontSize"), 1);

  this->Implementation->RepLinks->addPropertyLink(
    this->Implementation->spinBoxOpacity_Cell, "value", SIGNAL(valueChanged(double)),
    reprProxy, reprProxy->GetProperty("SelectionCellLabelOpacity"));

}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateSelectionSource()
{
  pqOutputPort* port = 
    this->Implementation->SelectionManager->getSelectedPort();

  if (port)
    {
    vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(
      this->Implementation->InputSource->getProxy());

    if(input)
      {
      vtkSMSourceProxy* selSrc = 
        input->GetSelectionInput(port->getPortNumber());
      if(this->Implementation->SelectionSource != selSrc) 
        {
        this->setSelectionSource(selSrc);
        }
      }
    else
      {
      this->setEnabled(false);
      return;
      }
    }
  else
    {
    this->setEnabled(false);
    return;
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateSelectionSourceGUI()
{
  vtkSMProxy* selectionSource = this->Implementation->SelectionSource.GetPointer();

  //this->Implementation->SourceLinks->addPropertyLink(
  //  this->Implementation->SelectionTypeAdaptor, "currentText", 
  //  SIGNAL(currentTextChanged(const QString&)),
  //  selectionSource, selectionSource->GetProperty("ContentType"));
  this->onSelectionContentTypeChanged();

  this->Implementation->SourceLinks->addPropertyLink(
    this->Implementation->FieldTypeAdaptor, "currentText", 
    SIGNAL(currentTextChanged(const QString&)),
    selectionSource, selectionSource->GetProperty("FieldType"));

  //this->Implementation->SourceLinks->addPropertyLink(
  //  this->Implementation->checkboxPassThrough, "checked", SIGNAL(toggled(bool)),
  //  selectionSource, 
  //  selectionSource->GetProperty("PassThrough"));

  this->Implementation->SourceLinks->addPropertyLink(
    this->Implementation->checkboxContainCell, "checked", SIGNAL(toggled(bool)),
    selectionSource, 
    selectionSource->GetProperty("ContainingCells"));

  this->Implementation->SourceLinks->addPropertyLink(
    this->Implementation->checkboxInsideOut, "checked", SIGNAL(toggled(bool)),
    selectionSource, 
    selectionSource->GetProperty("InsideOut"));

  this->Implementation->SourceLinks->addPropertyLink(
    this->Implementation->IndicesAdaptor, "values", SIGNAL(valuesChanged()),
    selectionSource, selectionSource->GetProperty("IDs"));

  // Link Frustum selection properties
  this->Implementation->SourceLinks->addPropertyLink(
    this->Implementation->checkboxShowFrustum, "checked", SIGNAL(toggled(bool)),
    selectionSource, 
    selectionSource->GetProperty("ShowBounds"));

  // Link Threshold selection properties
  this->Implementation->SourceLinks->addPropertyLink(
    this->Implementation->ThresholdScalarArrayAdaptor, "currentText", 
    SIGNAL(currentTextChanged(const QString&)),
    selectionSource, selectionSource->GetProperty("ArrayName"));

  this->Implementation->SourceLinks->addPropertyLink(
    this->Implementation->ThresholdsAdaptor, "values", SIGNAL(valuesChanged()),
    selectionSource, selectionSource->GetProperty("Thresholds"));

  this->updateSelectionLabelEnableState();
//  this->updateSelectionLabelModes();
//  this->updateSurfaceIDConnections();

}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateThreholdDataArrays()
{
  this->Implementation->ThresholdScalarArray->clear();
  if(!this->Implementation->InputSource || 
    !this->Implementation->InputSource->getProxy())
    {
    //this->Implementation->stackedWidget->
    return;
    }
  vtkSMSourceProxy* sourceProxy = vtkSMSourceProxy::SafeDownCast(
    this->Implementation->InputSource->getProxy());
  vtkPVDataInformation* geomInfo = sourceProxy->GetDataInformation();

  vtkPVDataSetAttributesInformation* attrInfo;

  if (this->Implementation->comboFieldType->currentText() == QString("POINT"))
    {
    attrInfo = geomInfo->GetPointDataInformation();
    }
  else
    {
    attrInfo = geomInfo->GetCellDataInformation();
    }

  for(int i=0; i<attrInfo->GetNumberOfArrays(); i++)
    {
    if(attrInfo->IsArrayAnAttribute(i) == vtkDataSetAttributes::SCALARS)
      {
      this->Implementation->ThresholdScalarArray->addItem(
        attrInfo->GetArrayInformation(i)->GetName());
      }
    }
}
//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::setupSelelectionLabelGUI()
{
  // Selection Labels properties
  this->Implementation->SelectionColorAdaptor = new pqSignalAdaptorColor(
    this->Implementation->Sel_buttonColor,
    "chosenColor",
    SIGNAL(chosenColorChanged(const QColor&)), false);
  this->Implementation->PointColorAdaptor = new pqSignalAdaptorColor(
    this->Implementation->buttonColor_Point, "chosenColor", 
    SIGNAL(chosenColorChanged(const QColor&)), false);
  this->Implementation->PointFontFamilyAdaptor = new pqSignalAdaptorComboBox(
    this->Implementation->comboFontFamily_Point);

  QObject::connect(this->Implementation->comboLabelMode_Point, 
    SIGNAL(currentIndexChanged(const QString&)),
    this, SLOT(updatePointLabelMode(const QString&)), Qt::QueuedConnection);
  QObject::connect(this->Implementation->comboLabelMode_Point, 
    SIGNAL(currentIndexChanged(const QString&)), 
    this, SLOT(updateRepresentationViews()), Qt::QueuedConnection);

  this->Implementation->PointLabelAlignmentAdaptor = new pqSignalAdaptorComboBox(
    this->Implementation->comboTextAlign_Point);


  this->Implementation->CellColorAdaptor = new pqSignalAdaptorColor(
    this->Implementation->buttonColor_Cell, "chosenColor", 
    SIGNAL(chosenColorChanged(const QColor&)), false);
  this->Implementation->CellFontFamilyAdaptor = new pqSignalAdaptorComboBox(
    this->Implementation->comboFontFamily_Cell);

  QObject::connect(this->Implementation->comboLabelMode_Cell, 
    SIGNAL(currentIndexChanged(const QString&)),
    this, SLOT(updateCellLabelMode(const QString&)), Qt::QueuedConnection);
  QObject::connect(this->Implementation->comboLabelMode_Cell, 
    SIGNAL(currentIndexChanged(const QString&)), 
    this, SLOT(updateRepresentationViews()), Qt::QueuedConnection);

  this->Implementation->CellLabelAlignmentAdaptor = new pqSignalAdaptorComboBox(
    this->Implementation->comboTextAlign_Cell);

}

//-----------------------------------------------------------------------------
// Called whne the SMProperty for SelectionPointFieldDataArrayName changes. We
// update the Qt combobox accordingly.
void pqSelectionInspectorPanel::updateSelectionPointLabelArrayName()
{
  vtkSMProxy* reprProxy = this->Implementation->Representation->getProxy();
  if(!reprProxy)
    {
    return;
    }

  // Point Label
  vtkSMProperty* svp = reprProxy->
    GetProperty("SelectionPointFieldDataArrayName");
  if(!svp)
    {
    return;
    }

  QString text = pqSMAdaptor::getElementProperty(svp).toString();
  if(text.isEmpty())
    {
    return;
    }

  if(text == "vtkOriginalPointIds")
    {
    text = "Point IDs";
    }

  this->Implementation->comboLabelMode_Point->setCurrentIndex(
    this->Implementation->comboLabelMode_Point->findText(text));
} 

//-----------------------------------------------------------------------------
// Called when the SMProperty for SelectionCellFieldDataArrayName changes.
// We update the Qt combobox accordingly.
void pqSelectionInspectorPanel::updateSelectionCellLabelArrayName()
{
  vtkSMProxy* reprProxy = this->Implementation->Representation->getProxy();
  if(!reprProxy)
    {
    return;
    }

  // Cell Label
  vtkSMProperty* svp = reprProxy->
    GetProperty("SelectionCellFieldDataArrayName");
  if(!svp)
  {
    return;
  }

  QString text = pqSMAdaptor::getElementProperty(svp).toString();
  if(text.isEmpty())
    {
    return;
    }

  if(text == "vtkOriginalCellIds")
    {
    text = "Cell IDs";
    }

  this->Implementation->comboLabelMode_Cell->setCurrentIndex(
    this->Implementation->comboLabelMode_Cell->findText(text));
} 

//-----------------------------------------------------------------------------
// Called when the Qt combobox for point label mode changes. We update the 
// SMProperty accordingly.
void pqSelectionInspectorPanel::updatePointLabelMode(const QString& text)
{
  if(text.isEmpty())
    {
    return;
    }
  if(!this->Implementation->Representation)
    {
    return;
    }
  vtkSMProxy* reprProxy = this->Implementation->Representation->getProxy();
  if(!reprProxy)
    {
    return;
    }
  if(text == "Point IDs")
    {
    pqSMAdaptor::setElementProperty(
      reprProxy->GetProperty("SelectionPointFieldDataArrayName"),"vtkOriginalPointIds");
    }
  else
    {
    pqSMAdaptor::setElementProperty(
      reprProxy->GetProperty("SelectionPointFieldDataArrayName"),text);
    }
  reprProxy->UpdateVTKObjects();
} 

//-----------------------------------------------------------------------------
// Called when the Qt combobox for cell label mode changes. We update the
// SMProperty accordingly.
void pqSelectionInspectorPanel::updateCellLabelMode(const QString& text)
{
  if(text.isEmpty())
    {
    return;
    }
  if(!this->Implementation->Representation)
    {
    return;
    }
  vtkSMProxy* reprProxy = this->Implementation->Representation->getProxy();
  if(!reprProxy)
    {
    return;
    }

  if(text == "Cell IDs")
    {
    pqSMAdaptor::setElementProperty(
      reprProxy->GetProperty("SelectionCellFieldDataArrayName"),"vtkOriginalCellIds");
    }
  else
    {
    pqSMAdaptor::setElementProperty(
      reprProxy->GetProperty("SelectionCellFieldDataArrayName"),text);
    }

  reprProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateSelectionLabelEnableState()
{

  vtkSMProxy* input = this->Implementation->InputSource->getProxy();

  if (input)
    {
    this->Implementation->groupSelectionLabel->setEnabled(true);
    if(this->Implementation->checkBoxLabelCells->isChecked())
      {
      this->Implementation->groupBox_CellLabelStyle->setEnabled(true);
      }
    else
      {
      this->Implementation->groupBox_CellLabelStyle->setEnabled(false);
      }
    if(this->Implementation->checkBoxLabelPoints->isChecked())
      {
      this->Implementation->groupBox_PointLabelStyle->setEnabled(true);
      }
    else
      {
      this->Implementation->groupBox_PointLabelStyle->setEnabled(false);
      }
    }
  else
    { 
    this->Implementation->groupSelectionLabel->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateSelectionLabelModes()
{
  if(!this->Implementation->InputSource ||
    !this->Implementation->InputSource->getProxy())
    {
    return;
    }

  vtkSMProxy* inputSrc = this->Implementation->InputSource->getProxy();

  if (inputSrc)
    {
    vtkSMSourceProxy* sourceProxy = vtkSMSourceProxy::SafeDownCast(inputSrc);
    if(!sourceProxy)
      {
      return;
      }
    vtkPVDataInformation* geomInfo = sourceProxy->GetDataInformation();

    vtkPVDataSetAttributesInformation* attrInfo;

    this->Implementation->comboLabelMode_Point->clear();
    this->Implementation->comboLabelMode_Point->addItem("Point IDs");
    attrInfo = geomInfo->GetPointDataInformation();
    for(int i=0; i<attrInfo->GetNumberOfArrays(); i++)
      {
      QString arrayName = attrInfo->GetArrayInformation(i)->GetName();
      if(arrayName != "vtkOriginalPointIds") // "Point IDs"
        {
        this->Implementation->comboLabelMode_Point->addItem(arrayName);
        }
      }

    this->Implementation->comboLabelMode_Cell->clear();
    this->Implementation->comboLabelMode_Cell->addItem("Cell IDs");
    attrInfo = geomInfo->GetCellDataInformation();
    for(int i=0; i<attrInfo->GetNumberOfArrays(); i++)
      {
      QString arrayName = attrInfo->GetArrayInformation(i)->GetName();
      if(arrayName != "vtkOriginalCellIds") // "Cell IDs"
        {
        this->Implementation->comboLabelMode_Cell->addItem(arrayName);
        }
      }      
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::setupSurfaceSelectionGUI()
{
  this->Implementation->Indices->sortItems(0, Qt::AscendingOrder);

  this->Implementation->IndicesAdaptor=
    new pqSignalAdaptorTreeWidget(this->Implementation->Indices, true);

  this->Implementation->UseGlobalIDs->toggle();
  this->Implementation->UseGlobalIDs->toggle();

  //this->Implementation->SelectionSource.GetPointer()->SetServers(
  //  this->Implementation->Representation.GetPointer()->GetServers());

  QObject::connect(this->Implementation->IndicesAdaptor, SIGNAL(valuesChanged()),
    this, SLOT(updateAllSelectionViews()));
  QObject::connect(this->Implementation->UseGlobalIDs, SIGNAL(toggled(bool)),
    this, SLOT(updateSurfaceIDConnections()));

  // Link surface selection properties
  QObject::connect(this->Implementation->Delete, SIGNAL(clicked()),
    this, SLOT(deleteSelectedSurfaceSelection()));
  QObject::connect(this->Implementation->DeleteAll, SIGNAL(clicked()),
    this, SLOT(deleteAllSurfaceSelection()));
  QObject::connect(this->Implementation->NewValue, SIGNAL(clicked()),
    this, SLOT(newValueSurfaceSelection()));
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateSurfaceSelectionIDRanges()
{
  if(!this->Implementation->InputSource)
  {
    return;
  }
  vtkSMSourceProxy* sourceProxy = vtkSMSourceProxy::SafeDownCast(
    this->Implementation->InputSource->getProxy());
  if(!sourceProxy)
  {
    return;
  }
  vtkPVDataInformation* dataInfo = sourceProxy->GetDataInformation(0, false);

  if (!dataInfo)
  {
    return;
  }

  int numPartitions = 
    this->Implementation->InputSource->getServer()->getNumberOfPartitions();

  this->Implementation->ProcessIDRange->setText(
    QString("Process ID Range: 0 - %1").arg(numPartitions-1));

  vtkPVDataSetAttributesInformation* dsainfo = 0;
  vtkTypeInt64 numIndices = 0;
  if (this->Implementation->comboFieldType->currentText() == QString("CELL"))
    {
    numIndices = dataInfo->GetNumberOfCells();
    dsainfo = dataInfo->GetCellDataInformation();
    }
  else
    {
    numIndices = dataInfo->GetNumberOfPoints();
    dsainfo = dataInfo->GetPointDataInformation();
    }
  this->Implementation->IndexRange->setText(
    QString("Index Range: 0 - %1").arg(numIndices-1));

  vtkPVArrayInformation* gidsInfo = dsainfo->GetAttributeInformation(
    vtkDataSetAttributes::GLOBALIDS);
  if (gidsInfo)
  {
    double* range =gidsInfo->GetComponentRange(0);
    vtkTypeInt64 gid_min = static_cast<vtkTypeInt64>(range[0]);
    vtkTypeInt64 gid_max = static_cast<vtkTypeInt64>(range[1]);

    this->Implementation->GlobalIDRange->setText(
      QString("Global ID Range: %1 - %2").arg(gid_min).arg(gid_max));
  }
  else
  {
    this->Implementation->GlobalIDRange->setText("Global ID Range: <not available>");
  }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::deleteSelectedSurfaceSelection()
{
  QTreeWidget* activeTree = this->Implementation->Indices;

  QList<QTreeWidgetItem*> items = activeTree->selectedItems(); 
  foreach (QTreeWidgetItem* item, items)
  {
    delete item;
  }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::deleteAllSurfaceSelection()
{
  QTreeWidget* activeTree =  this->Implementation->Indices;
  activeTree->clear();
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::newValueSurfaceSelection()
{
  pqSignalAdaptorTreeWidget* adaptor = this->Implementation->IndicesAdaptor;
  QTreeWidget* activeTree = this->Implementation->Indices;

  QStringList value;
  // TODO: Use some good defaults.
  if (this->Implementation->UseGlobalIDs->isChecked())
    {
    value.push_back(QString::number(-1));
    }
  else
    {
    value.push_back(QString::number(0));
    }
  value.push_back(QString::number(0));

  pqSelectionInspectorTreeItem* item = new pqSelectionInspectorTreeItem(value);
  adaptor->appendItem(item);

  // change the current item and make it editable.
  activeTree->setCurrentItem(item, 0);
  activeTree->editItem(item, 0);
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateSurfaceInformationAndDomains()
{
  if(!this->Implementation->InputSource)
    {
    return;
    }
  vtkSMSourceProxy* sourceProxy = vtkSMSourceProxy::SafeDownCast(
    this->Implementation->InputSource->getProxy());
  if(!sourceProxy)
    {
    return;
    }
  vtkPVDataInformation* dataInfo = sourceProxy->GetDataInformation(0, false);

  if (!dataInfo)
    {
    return;
    }

  vtkPVDataSetAttributesInformation* dsainfo = 0;
  if (this->Implementation->comboFieldType->currentText() == QString("CELL"))
    {
    dsainfo = dataInfo->GetCellDataInformation();
    }
  else
    {
    dsainfo = dataInfo->GetPointDataInformation();
    }

  if (dsainfo->GetAttributeInformation(vtkDataSetAttributes::GLOBALIDS))
    {
    // We have global ids.
    this->Implementation->UseGlobalIDs->setEnabled(true);
    }
  else
    {
    this->Implementation->UseGlobalIDs->setCheckState(Qt::Unchecked);
    this->Implementation->UseGlobalIDs->setEnabled(false);
    }

}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateSelectionContentType(const QString& type)
{
  // Set up selection connections
  vtkSMProxy* selectionSource = this->Implementation->SelectionSource.GetPointer();
  if(!selectionSource)
    {
    return;
    }
  vtkSMProperty* idvp = selectionSource->GetProperty("ContentType");

  if(!idvp)
    {
    return;
    }

  if(type == QString("Thresholds"))
    {
    pqSMAdaptor::setElementProperty(
      idvp, vtkSelection::THRESHOLDS);
    }
  else if(type == QString("Frustum"))
    {
    pqSMAdaptor::setElementProperty(
      idvp, vtkSelection::FRUSTUM);
    }
  else if(type == QString("IDs"))
    {
    this->updateSurfaceIDConnections();
    }
  else //None
    {
    }

  // update the RubberBandHelper
  if(type == QString("Thresholds"))
    {
    //this->Implementation->RubberBandHelper->
    }
  else if(type == QString("Frustum"))
    {
    this->Implementation->RubberBandHelper->beginFrustumSelection();
    }
  else if(type == QString("IDs"))
    {
    this->Implementation->RubberBandHelper->beginSelection();
    }
  else //None
    {
    //return;
    this->Implementation->RubberBandHelper->endSelection();
    }

}

//-----------------------------------------------------------------------------
// Updates the enable state of "Containing Cells" check box based on whether
// we are doing a point selection or cell selection. 
void pqSelectionInspectorPanel::updateSelectionFieldType(const QString& type)
{
  if(type == QString("POINT"))
    {
    this->Implementation->checkboxContainCell->setEnabled(true);
    }
  else 
    {
    this->Implementation->checkboxContainCell->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
// Update the SMProperty for ContentType based on the GUI state of ContentType
// combo box and the UseGlobalIDs check box.
void pqSelectionInspectorPanel::updateSurfaceIDConnections()
{
  if(!this->Implementation->SelectionSource ||
    !this->Implementation->SelectionSource.GetPointer())
    {
    return;
    }

  if(this->Implementation->SelectionTypeAdaptor->currentText() != "IDs")
    {
    return;
    }

  vtkSMProxy* selectionSource = this->Implementation->SelectionSource.GetPointer();
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSource->GetProperty("ContentType"));
  QStringList headerLabels;
  headerLabels << "Process ID";
  if (this->Implementation->UseGlobalIDs->isChecked())
    {
    // convert selection must be called before the ContentType on the selection
    // source is changed otherwise the conversion wont result in any change.
    this->convertSelection(true);
    ivp->SetElement(0, vtkSelection::GLOBALIDS);
    headerLabels << "Global ID";
    // hide pid column
    this->Implementation->Indices->setColumnHidden(0, true);
    }
  else
    {
    // convert selection must be called before the ContentType on the selection
    // source is changed otherwise the conversion wont result in any change.
    this->convertSelection(false);
    ivp->SetElement(0, vtkSelection::INDICES);
    headerLabels << "Index";

    // show pid column if num of process is > 1
    this->Implementation->Indices->setColumnHidden(0,
      !this->Implementation->UseProcessID);
    }
  this->Implementation->Indices->setHeaderLabels(headerLabels);
  selectionSource->UpdateVTKObjects();

  this->updateAllSelectionViews();
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::setupFrustumSelectionGUI()
{
  // TODO: add widgets to interact with the the Frutum box 
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::setupThresholdSelectionGUI()
{
  this->Implementation->ThresholdScalarArrayAdaptor = new pqSignalAdaptorComboBox(
    this->Implementation->ThresholdScalarArray);

  QObject::connect(this->Implementation->buttonAddThresholds, SIGNAL(clicked()),
    this, SLOT(addThresholds()));

  this->Implementation->ThresholdsAdaptor=
    new pqSignalAdaptorTreeWidget(this->Implementation->thresholdRanges, true);

  QObject::connect(this->Implementation->Delete_Threshold, SIGNAL(clicked()),
    this, SLOT(deleteSelectedThresholds()));
  QObject::connect(this->Implementation->DeleteAll_Threshold, SIGNAL(clicked()),
    this, SLOT(deleteAllThresholds()));

  QObject::connect(this->Implementation->Threshold_Lower, SIGNAL(valueChanged(double)),
    this, SLOT(lowerThresholdChanged(double)), Qt::QueuedConnection);
  QObject::connect(this->Implementation->Threshold_Upper, SIGNAL(valueChanged(double)),
    this, SLOT(upperThresholdChanged(double)), Qt::QueuedConnection);

}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::deleteSelectedThresholds()
{
  QTreeWidget* activeTree = this->Implementation->thresholdRanges;

  QList<QTreeWidgetItem*> items = activeTree->selectedItems(); 
  foreach (QTreeWidgetItem* item, items)
  {
    delete item;
  }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::deleteAllThresholds()
{
  QTreeWidget* activeTree = this->Implementation->thresholdRanges;
  activeTree->clear();
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::lowerThresholdChanged(double val)
{
  // clamp the lower threshold if we need to
  if(this->Implementation->Threshold_Upper->value() < val)
  {
    this->Implementation->Threshold_Upper->setValue(val);
  }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::upperThresholdChanged(double val)
{
  // clamp the lower threshold if we need to
  if(this->Implementation->Threshold_Lower->value() > val)
  {
    this->Implementation->Threshold_Lower->setValue(val);
  }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::addThresholds()
{
  QStringList value;
  value.push_back(QString::number(
    this->Implementation->Threshold_Lower->value()));
  value.push_back(QString::number(
    this->Implementation->Threshold_Upper->value()));

  pqSelectionInspectorTreeItem* item = new pqSelectionInspectorTreeItem(value);
  this->Implementation->ThresholdsAdaptor->appendItem(item);
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateRepresentationViews()
{
  if (this->Implementation->Representation)
    {
    this->Implementation->Representation->
      getOutputPortFromInput()->renderAllViews(false);
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::updateAllSelectionViews()
{
  pqOutputPort* port = 
    this->Implementation->SelectionManager->getSelectedPort();

  if (port)
    {
    port->renderAllViews();
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::onSelectionModeChanged(int selMode)
{
  if(selMode == pqRubberBandHelper::SELECT)
    {
    this->Implementation->SelectionTypeAdaptor->setCurrentText("IDs");
    }
  else if(selMode == pqRubberBandHelper::FRUSTUM)
    {
    this->Implementation->SelectionTypeAdaptor->setCurrentText("Frustum");
    }
  else 
    {
    //this->Implementation->SelectionTypeAdaptor->setCurrentText("None");
    }
}

//-----------------------------------------------------------------------------
// Use SMProperty ContentType to update the GUI.
void pqSelectionInspectorPanel::onSelectionContentTypeChanged()
{
  // Set up selection connections
  vtkSMProxy* selectionSource = this->Implementation->SelectionSource.GetPointer();
  if(!selectionSource)
    {
    return;
    }

  vtkSMProperty* idvp = selectionSource->GetProperty("ContentType");
  if(!idvp)
    {
    return;
    }
  int contType = pqSMAdaptor::getElementProperty(idvp).toInt();

  if(contType == vtkSelection::INDICES || contType == vtkSelection::GLOBALIDS)
    {
    this->Implementation->SelectionTypeAdaptor->setCurrentText("IDs");
    this->Implementation->UseGlobalIDs->setCheckState(
      (contType == vtkSelection::GLOBALIDS)? Qt::Checked: Qt::Unchecked);
    }
  else if(contType == vtkSelection::FRUSTUM)
    {
    this->Implementation->SelectionTypeAdaptor->setCurrentText("Frustum");
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::onSelectionFieldTypeChanged()
{
  // Set up selection connections
  vtkSMProxy* selectionSource = this->Implementation->SelectionSource.GetPointer();
  if(!selectionSource)
  {
    return;
  }

  vtkSMProperty* idvp = selectionSource->GetProperty("FieldType");
  if(!idvp)
  {
    return;
  }
  int contType = pqSMAdaptor::getElementProperty(idvp).toInt();

  if(contType == vtkSelection::CELL)
    {
    this->Implementation->FieldTypeAdaptor->setCurrentText("CELL");
    }
  else if(contType == vtkSelection::POINT)
    {
    this->Implementation->FieldTypeAdaptor->setCurrentText("POINT");
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::onActiveViewChanged()
{
  pqRenderView* renView = qobject_cast<pqRenderView*>(
    pqActiveView::instance().current());
  if (!renView)
    {
    this->Implementation->groupSelectionLabel->setEnabled(false);
    }
  else
    {
    this->Implementation->groupSelectionLabel->setEnabled(true);
    pqOutputPort* port = 
      this->Implementation->SelectionManager->getSelectedPort();
    pqPipelineSource* input = port? port->getSource() : 0;

    pqDataRepresentation *repr = NULL;
    if(input)
      {
      repr = input->getRepresentation(renView);
      }

    this->setRepresentation(repr);
    }
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::convertSelection(bool toGIDs)
{
  vtkSMProxy* selectionSource = this->Implementation->SelectionSource.GetPointer();
  pqOutputPort* port = 
    this->Implementation->SelectionManager->getSelectedPort();
  if(!selectionSource || !port)
    {
    return;
    }

  if (toGIDs) // Convert INDICES to GLOBALIDS.
    {
    QList<vtkIdType> globalIds = 
      this->Implementation->SelectionManager->getGlobalIDs();

    // Now to set gids on the IDs property, we need to insert process numbers.
    QList<QVariant> ids;
    foreach (vtkIdType gid, globalIds)
      {
      ids.push_back(-1);
      ids.push_back(gid);
      }
    pqSMAdaptor::setMultipleElementProperty(
      selectionSource->GetProperty("IDs"), ids);
    }
  else  // Convert GLOBALIDS to INDICES.
    {
    QList<QPair<int, vtkIdType> > indices =
      this->Implementation->SelectionManager->getIndices();

    QList<QVariant> ids;
    for(int cc=0; cc < indices.size(); cc++)
      {
      QPair<int, vtkIdType> pair = indices[cc];
      ids.push_back(pair.first);
      ids.push_back(pair.second);
      }
    pqSMAdaptor::setMultipleElementProperty(
      selectionSource->GetProperty("IDs"), ids);
    }

  selectionSource->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqSelectionInspectorPanel::growIndicesTable()
{
  if ( this->Implementation->Indices->topLevelItemCount()>0)
    {
    pqSignalAdaptorTreeWidget* adaptor = this->Implementation->IndicesAdaptor;

    QTreeWidgetItem* curItem = this->Implementation->Indices->topLevelItem(
      this->Implementation->Indices->topLevelItemCount()-1);

    if (curItem)
      {
      QStringList value;
      for (int cc=0; cc < curItem->columnCount(); cc++)
        {
        value.push_back(curItem->text(cc));
        }

      pqSelectionInspectorTreeItem* item = new pqSelectionInspectorTreeItem(value);
      adaptor->appendItem(item);
      }
    }
}
