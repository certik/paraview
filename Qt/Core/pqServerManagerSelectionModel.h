/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqServerManagerSelectionModel.h,v $

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
#ifndef __pqServerManagerSelectionModel_h
#define __pqServerManagerSelectionModel_h

#include <QObject>
#include <QList>
#include <QItemSelectionModel>
#include <QPointer>
#include "pqCoreExport.h"
#include "pqServerManagerModelItem.h"

class pqServerManagerModel;
class pqServerManagerModelItem;
class pqServerManagerSelection;
class pqServerManagerSelectionModelInternal;

// This is a QItemSelectionModel-like selection model for the
// pqServerManagerModel. pqServerManagerSelectionModel is part
// of the "Synchronized Selection" mechanism, which makes it 
// possible for different Qt views based on different Qt models, all
// of which are based on thq pqServerManagerModel to coordintae selection
// state.
class PQCORE_EXPORT pqServerManagerSelectionModel : public QObject
{
  Q_OBJECT
public:
  // Supported selections flags. These are a subset of 
  // QItemSelectionModel::SelectionFlags.
  enum SelectionFlag {
    NoUpdate       = QItemSelectionModel::NoUpdate,
    Clear          = QItemSelectionModel::Clear,
    Select         = QItemSelectionModel::Select,
    Deselect       = QItemSelectionModel::Deselect, 
    ClearAndSelect = Clear | Select
  };
  Q_DECLARE_FLAGS(SelectionFlags, SelectionFlag)

public:
  pqServerManagerSelectionModel(pqServerManagerModel* model, 
    QObject* parent=NULL);
  virtual ~pqServerManagerSelectionModel();

  // Returns the item that is current, on NULL if
  // there is no current.
  pqServerManagerModelItem* currentItem() const;

  // Set the current item. command can be used to indicate
  // if the current item should be selected/deselected, or
  // all selection cleared.
  void setCurrentItem(pqServerManagerModelItem* item, 
    pqServerManagerSelectionModel::SelectionFlags command);

  // Returns true if the item is selected.
  bool isSelected(pqServerManagerModelItem* item) const;

  // Returns the pqServerManagerModel operated on by the selection model.
  pqServerManagerModel* model() const;

  // Returns the list of selected items.
  const pqServerManagerSelection* selectedItems() const;

public slots:
  void select(pqServerManagerModelItem* item, 
    pqServerManagerSelectionModel::SelectionFlags command);
  void select(const pqServerManagerSelection& items,
    pqServerManagerSelectionModel::SelectionFlags command);

signals:
  void currentChanged(pqServerManagerModelItem* item);
  void selectionChanged(const pqServerManagerSelection& selected,
    const pqServerManagerSelection& deselected);

private:
  pqServerManagerSelectionModelInternal* Internal;
  // Cleans up QPointers pointing to null objects in the selection.
  void purge();
};

inline bool operator==(const QPointer<pqServerManagerModelItem>& lhs, 
                       const QPointer<pqServerManagerModelItem>& rhs)
{
  return static_cast<pqServerManagerModelItem*>(lhs) == 
    static_cast<pqServerManagerModelItem*>(rhs);
}

inline uint qHash(const QPointer<pqServerManagerModelItem>& index)
{
  return reinterpret_cast<size_t>(static_cast<pqServerManagerModelItem*>(index));
}

// This is a selection set. For now, it's simply a QList.
class PQCORE_EXPORT pqServerManagerSelection : 
  public QList<QPointer<pqServerManagerModelItem> >
{
};

Q_DECLARE_METATYPE(pqServerManagerSelection);

#endif


