/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqSpreadSheetViewModel.h,v $

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
#ifndef __pqSpreadSheetViewModel_h 
#define __pqSpreadSheetViewModel_h

#include <QAbstractTableModel>
#include <QPair>
#include <QSet>
#include "pqCoreExport.h"
#include "vtkType.h" // needed for vtkIdType.

/// This is the model used by SpreadSheetView to show the data. This model works
/// with vtkSMSpreadSheetRepresentationProxy to fetch blocks of data from the
/// server and show them. It requires that vtkSMSpreadSheetRepresentationProxy
/// delivers vtkTable.
class vtkSMSpreadSheetRepresentationProxy;
class QItemSelectionModel;
class QItemSelection;
class vtkSelection;
class pqDataRepresentation;

class PQCORE_EXPORT pqSpreadSheetViewModel : public QAbstractTableModel
{
  Q_OBJECT
  typedef QAbstractTableModel Superclass;
public:
  pqSpreadSheetViewModel();
  ~pqSpreadSheetViewModel();

  /// Returns the selection model that shows the selection on the representation
  /// being shown by this model.
  QItemSelectionModel* selectionModel() const;

  /// Returns the number of rows.
  int rowCount(const QModelIndex& parent=QModelIndex()) const;

  /// Returns the number of columns.
  int columnCount(const QModelIndex& parent=QModelIndex()) const;

  /// Returns the data storeed under the given role for the item referred by the
  /// index.
  QVariant data(const QModelIndex& index, int role=Qt::DisplayRole) const;

  /// Returns the data for the given role and section in the header with the
  /// specified orientation.
  QVariant headerData (int section, Qt::Orientation orientation, 
    int role=Qt::DisplayRole) const; 

 
  /// Set/Get the data representation.
  void setRepresentation(pqDataRepresentation*);
  pqDataRepresentation* getRepresentation() const;

  /// Set/Get the representation proxy which is currently displayed in this
  /// model.
  vtkSMSpreadSheetRepresentationProxy* getRepresentationProxy() const;

  /// resets the model.
  void forceUpdate();

  /// Set the best estimate for the visible block. The model will request data
  /// (if not available) only for the most recently selected active block.
  void setActiveBlock(QModelIndex top, QModelIndex bottom);

  QModelIndex indexFor(int processId, vtkIdType index);

  /// Returns the field type for the data currently shown by this model.
  int getFieldType() const;

  /// returns the (process id, index) pair for all qt indices.
  QSet<QPair<vtkIdType, vtkIdType> > getVTKIndices(const QModelIndexList& indexes);
signals:
  void requestDelayedUpdate() const;

private slots:
  /// called to fetch data for all pending blocks.
  void delayedUpdate();

  /// called to fetch selection for all pending blocks.
  void delayedSelectionUpdate();

protected:
  /// Converts a vtkSelection to a QItemSelection.
  QItemSelection convertToQtSelection(vtkSelection*);

  /// Updates the selectionModel with the vtk selection provided by the
  /// representation for the current block. This simply adds to the current Qt
  /// selection, since representation can never give us the complete state of
  /// selection. 
  void updateSelectionForBlock(vtkIdType blocknumber);

  void setRepresentationProxy(vtkSMSpreadSheetRepresentationProxy*);

  /// Given an index into the model, check to see that its row number is
  /// less than the length of the data array associated with its column
  bool isDataValid(const QModelIndex &idx) const;

private:
  pqSpreadSheetViewModel(const pqSpreadSheetViewModel&); // Not implemented.
  void operator=(const pqSpreadSheetViewModel&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};

#endif


