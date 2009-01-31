/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqSpreadSheetView.cxx,v $

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

========================================================================*/
#include "pqSpreadSheetView.h"

// Server Manager Includes.
#include "vtkSMViewProxy.h"
#include "vtkSMSpreadSheetRepresentationProxy.h"
#include "vtkSMSourceProxy.h"

// Qt Includes.
#include <QHeaderView>
#include <QItemDelegate>
#include <QPointer>
#include <QTableView>
#include <QTextLayout>
#include <QTextOption>
#include <QPainter>
#include <QPen>
#include <QApplication>

// ParaView Includes.
#include "pqOutputPort.h"
#include "pqDataRepresentation.h"
#include "pqServer.h"
#include "pqSpreadSheetViewModel.h"
#include "pqSpreadSheetViewSelectionModel.h"
#include "pqPipelineSource.h"

//-----------------------------------------------------------------------------
class pqSpreadSheetView::pqDelegate : public QItemDelegate
{
  typedef QItemDelegate Superclass;
public:
  pqDelegate(QObject* _parent=0):Superclass(_parent)
  {
  }
  void beginPaint()
    {
    this->Top = QModelIndex();
    this->Bottom = QModelIndex();
    }
  void endPaint()
    {
    }

  virtual void paint (QPainter* painter, const QStyleOptionViewItem& option, 
    const QModelIndex& index) const 
    {
    const_cast<pqDelegate*>(this)->Top = (this->Top.isValid() && this->Top < index)?
      this->Top : index;
    const_cast<pqDelegate*>(this)->Bottom = (this->Bottom.isValid() && index < this->Bottom)?
      this->Bottom : index;

    this->Superclass::paint(painter, option, index);
    }

  // special text painter that does tab stops to line up multi-component data
  // at this point, multi-component data is already in string format and 
  // has '\t' characters separating each component
  // taken from QItemDelegate::drawDisplay and tweaked
  void drawDisplay(QPainter* painter, const QStyleOptionViewItem& option,
                   const QRect & r, const QString & text ) const
    {
      if (text.isEmpty())
          return;

      QPen pen = painter->pen();
      QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                                ? QPalette::Normal : QPalette::Disabled;
      if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
          cg = QPalette::Inactive;
      if (option.state & QStyle::State_Selected) {
          painter->fillRect(r, option.palette.brush(cg, QPalette::Highlight));
          painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
      } else {
          painter->setPen(option.palette.color(cg, QPalette::Text));
      }

      if (option.state & QStyle::State_Editing) {
          painter->save();
          painter->setPen(option.palette.color(cg, QPalette::Text));
          painter->drawRect(r.adjusted(0, 0, -1, -1));
          painter->restore();
      }

      const int textMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin) + 1;
      QRect textRect = r.adjusted(textMargin, 0, -textMargin, 0); // remove width padding
      this->TextOption.setWrapMode(QTextOption::ManualWrap);
      this->TextOption.setTextDirection(option.direction);
      this->TextOption.setAlignment(QStyle::visualAlignment(option.direction, option.displayAlignment));
      // assume this is representative of the largest number we'll show
      int len = option.fontMetrics.width("-8.88888e-8888 ");
      this->TextOption.setTabStop(len);
      this->TextLayout.setTextOption(this->TextOption);
      this->TextLayout.setFont(option.font);
      this->TextLayout.setText(this->replaceNewLine(text));

      QSizeF textLayoutSize = this->doTextLayout(textRect.width());

      if (textRect.width() < textLayoutSize.width()
          || textRect.height() < textLayoutSize.height()) {
          QString elided;
          int start = 0;
          int end = text.indexOf(QChar::LineSeparator, start);
          if (end == -1) {
              elided += option.fontMetrics.elidedText(text, option.textElideMode, textRect.width());
          } else while (end != -1) {
              elided += option.fontMetrics.elidedText(text.mid(start, end - start),
                                                      option.textElideMode, textRect.width());
              start = end + 1;
              end = text.indexOf(QChar::LineSeparator, start);
          }
          this->TextLayout.setText(elided);
          textLayoutSize = this->doTextLayout(textRect.width());
      }

      const QSize layoutSize(textRect.width(), int(textLayoutSize.height()));
      const QRect layoutRect = QStyle::alignedRect(option.direction, option.displayAlignment,
                                                    layoutSize, textRect);
      this->TextLayout.draw(painter, layoutRect.topLeft(), QVector<QTextLayout::FormatRange>(), layoutRect);
    }
    
  QSizeF doTextLayout(int lineWidth) const
    {
      QFontMetrics fontMetrics(this->TextLayout.font());
      int leading = fontMetrics.leading();
      qreal height = 0;
      qreal widthUsed = 0;
      this->TextLayout.beginLayout();
      while (true) {
          QTextLine line = this->TextLayout.createLine();
          if (!line.isValid())
              break;
          line.setLineWidth(lineWidth);
          height += leading;
          line.setPosition(QPointF(0, height));
          height += line.height();
          widthUsed = qMax(widthUsed, line.naturalTextWidth());
      }
      this->TextLayout.endLayout();
      return QSizeF(widthUsed, height);
    }
  static QString replaceNewLine(QString text)
    {
      const QChar nl = QLatin1Char('\n');
      for (int i = 0; i < text.count(); ++i)
        if (text.at(i) == nl)
          text[i] = QChar::LineSeparator;
      return text;
    }

  QModelIndex Top;
  QModelIndex Bottom;
  mutable QTextLayout TextLayout;
  mutable QTextOption TextOption;
};

//-----------------------------------------------------------------------------
/// As one scrolls through the table view, the view requests the data for all
/// elements scrolled through, not only the ones eventually visible. We do this
/// trick with pqTableView and pqDelegate to make the model request the data
/// only for the region eventually visible to the user.
class pqSpreadSheetView::pqTableView : public QTableView
{
  typedef QTableView Superclass;
protected:
  virtual void paintEvent (QPaintEvent* pevent)
    {
    pqSpreadSheetView::pqDelegate* del =
      dynamic_cast<pqDelegate*>(this->itemDelegate());
    pqSpreadSheetViewModel* smodel = 
      qobject_cast<pqSpreadSheetViewModel*>(this->model());
    if (del)
      {
      del->beginPaint();
      }
    this->Superclass::paintEvent(pevent);
    if (del)
      {
      del->endPaint();
      smodel->setActiveBlock(del->Top, del->Bottom);
      }
    }
public:
  /// HACK: to disable the corner widget (Qt 4.3 has API to do this).
  void disableCornerWidget()
    {
    QList<QWidget*> _children = this->findChildren<QWidget*>();
    foreach (QWidget* child, _children)
      {
      if (strcmp(child->metaObject()->className(), "QAbstractButton") == 0)
        {
        child->setEnabled(false);
        }
      }
    }
};

//-----------------------------------------------------------------------------
class pqSpreadSheetView::pqInternal
{
public:
  pqInternal():Model(), SelectionModel(&this->Model)
  {
  pqSpreadSheetView::pqTableView* table = new pqSpreadSheetView::pqTableView();
  table->setAlternatingRowColors(true);
  table->disableCornerWidget();

  pqSpreadSheetView::pqDelegate* delegate = new pqSpreadSheetView::pqDelegate(table);

  table->setItemDelegate(delegate);
  this->Table= table;
  this->Table->setModel(&this->Model);
  this->Table->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->Table->setSelectionModel(&this->SelectionModel);
  this->Table->horizontalHeader()->setMovable(true);
  }

  ~pqInternal()
    {
    delete this->Table;
    }

  QPointer<QTableView> Table;
  pqSpreadSheetViewModel Model;
  pqSpreadSheetViewSelectionModel SelectionModel;
};


//-----------------------------------------------------------------------------
pqSpreadSheetView::pqSpreadSheetView(
 const QString& group, const QString& name, 
    vtkSMViewProxy* viewModule, pqServer* server, 
    QObject* _parent/*=NULL*/):
   pqView(spreadsheetViewType(), group, name, viewModule, server, _parent)
{
  this->Internal = new pqInternal();
  QObject::connect(this, SIGNAL(representationAdded(pqRepresentation*)),
    this, SLOT(onAddRepresentation(pqRepresentation*)));
  QObject::connect(this, SIGNAL(representationRemoved(pqRepresentation*)),
    this, SLOT(onRemoveRepresentation(pqRepresentation*)));
  QObject::connect(
    this, SIGNAL(representationVisibilityChanged(pqRepresentation*, bool)),
    this, SLOT(updateRepresentationVisibility(pqRepresentation*, bool)));
  QObject::connect(this, SIGNAL(endRender()), this, SLOT(onEndRender()));

  QObject::connect(
    &this->Internal->SelectionModel, SIGNAL(selection(vtkSMSourceProxy*)),
    this, SLOT(onCreateSelection(vtkSMSourceProxy*)));
  
  foreach(pqRepresentation* rep, this->getRepresentations())
    {
    this->onAddRepresentation(rep);
    }
}

//-----------------------------------------------------------------------------
pqSpreadSheetView::~pqSpreadSheetView()
{
  delete this->Internal;
}


//-----------------------------------------------------------------------------
QWidget* pqSpreadSheetView::getWidget()
{
  return this->Internal->Table;
}

//-----------------------------------------------------------------------------
void pqSpreadSheetView::onAddRepresentation(pqRepresentation* repr)
{
  this->updateRepresentationVisibility(repr, repr->isVisible());
}


//-----------------------------------------------------------------------------
void pqSpreadSheetView::onRemoveRepresentation(pqRepresentation* repr)
{
  if (repr && repr->getProxy() == this->Internal->Model.getRepresentationProxy())
    {
    this->Internal->Model.setRepresentation(0);
    }
}

//-----------------------------------------------------------------------------
void pqSpreadSheetView::updateRepresentationVisibility(
  pqRepresentation* repr, bool visible)
{
  if (!visible && repr && 
    this->Internal->Model.getRepresentationProxy() == repr->getProxy())
    {
    this->Internal->Model.setRepresentation(0);
    }
  if (!visible || !repr)
    {
    return;
    }

  // If visible, turn-off visibility of all other representations.
  QList<pqRepresentation*> reprs = this->getRepresentations();
  foreach (pqRepresentation* cur_repr, reprs)
    {
    if (cur_repr != repr)
      {
      cur_repr->setVisible(false);
      }
    }

  this->Internal->Model.setRepresentation(qobject_cast<pqDataRepresentation*>(repr));
}

//-----------------------------------------------------------------------------
void pqSpreadSheetView::onEndRender()
{
  // cout << "Render" << endl;
  this->Internal->Model.forceUpdate();
}

//-----------------------------------------------------------------------------
bool pqSpreadSheetView::canDisplay(pqOutputPort* opPort) const
{
  return (opPort && opPort->getServer()->GetConnectionID() == 
    this->getServer()->GetConnectionID());
}

//-----------------------------------------------------------------------------
void pqSpreadSheetView::onCreateSelection(vtkSMSourceProxy* selSource)
{
  pqDataRepresentation* repr = this->Internal->Model.getRepresentation();
  if (repr)
    {
    pqOutputPort* opport = repr->getOutputPortFromInput();
    vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(
      opport->getSource()->getProxy());
    input->CleanSelectionInputs(opport->getPortNumber());
    if (selSource)
      {
      input->SetSelectionInput(
        opport->getPortNumber(), selSource, 0);
      }
    emit this->selected(opport);
    }
  else
    {
    emit this->selected(0);
    }
}
