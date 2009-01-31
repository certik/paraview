/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqPipelineModel.h,v $

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

/// \file pqPipelineModel.h
/// \date 4/14/2006

#ifndef _pqPipelineModel_h
#define _pqPipelineModel_h


#include "pqComponentsExport.h"
#include <QAbstractItemModel>

class pqDataRepresentation;
class pqPipelineModelFilter;
class pqPipelineModelInternal;
class pqPipelineModelItem;
class pqPipelineModelOutput;
class pqPipelineModelSource;
class pqPipelineSource;
class pqView;
class pqServer;
class pqServerManagerModel;
class pqServerManagerModelItem;
class QFont;
class QPixmap;


/// \class pqPipelineModel
/// \brief
///   The pqPipelineModel class is used to represent the pipeline graph
///   as a tree.
///
/// The top of the hierarchy is the server objects. Each connected
/// server is added to the root node. An object with no inputs is added
/// as a child of its server. The outputs of an object are added as its
/// children. Objects with multiple inputs present a problem.
///
/// Instead of repeating information in the tree, the object with
/// multiple inputs is moved to the server. All its outputs are treated
/// normally and added as children. In place of the object with more
/// than one input, a link object is placed in the child list of the
/// input objects. The link item represents a connection instead of a
/// pipeline object.
class PQCOMPONENTS_EXPORT pqPipelineModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  enum ItemType
    {
    Invalid = -1,
    Server = 0,
    Source,
    Filter,
    CustomFilter,
    Link,
    OutputPort,
    LastType = OutputPort
    };

public:
  /// \brief
  ///   Creates an empty pipeline model.
  /// \param parent The parent object.
  pqPipelineModel(QObject *parent=0);

  /// \brief
  ///   Makes a copy of a pipeline model.
  /// \param other The pipeline model to copy.
  /// \param parent The parent object.
  pqPipelineModel(const pqPipelineModel &other, QObject *parent=0);

  /// \brief
  ///   Creates a pipeline model from a server manager model.
  /// \param other Used to build a pipeline model.
  /// \param parent The parent object.
  pqPipelineModel(const pqServerManagerModel &other, QObject *parent=0);
  virtual ~pqPipelineModel();

  /// \name QAbstractItemModel Methods
  //@{
  /// \brief
  ///   Gets the number of rows for a given index.
  /// \param parent The parent index.
  /// \return
  ///   The number of rows for the given index.
  virtual int rowCount(const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets the number of columns for a given index.
  /// \param parent The parent index.
  /// \return
  ///   The number of columns for the given index.
  virtual int columnCount(const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets whether or not the given index has child items.
  /// \param parent The parent index.
  /// \return
  ///   True if the given index has child items.
  virtual bool hasChildren(const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets a model index for a given location.
  /// \param row The row number.
  /// \param column The column number.
  /// \param parent The parent index.
  /// \return
  ///   A model index for the given location.
  virtual QModelIndex index(int row, int column,
      const QModelIndex &parent=QModelIndex()) const;

  /// \brief
  ///   Gets the parent for a given index.
  /// \param index The model index.
  /// \return
  ///   A model index for the parent of the given index.
  virtual QModelIndex parent(const QModelIndex &index) const;

  /// \brief
  ///   Gets the data for a given model index.
  /// \param index The model index.
  /// \param role The role to get data for.
  /// \return
  ///   The data for the given model index.
  virtual QVariant data(const QModelIndex &index,
      int role=Qt::DisplayRole) const;

  /// \brief
  ///   Gets the flags for a given model index.
  ///
  /// The flags for an item indicate if it is enabled, editable, etc.
  ///
  /// \param index The model index.
  /// \return
  ///   The flags for the given model index.
  virtual Qt::ItemFlags flags(const QModelIndex &index) const;

  /// \brief
  ///  Sets the role data for the item at index to value. Returns 
  ///  true if successful; otherwise returns false.
  bool setData(const QModelIndex &index, const QVariant& value, 
    int role = Qt::EditRole);
  //@}

  /// \name Object Mapping
  //@{
  /// \brief
  ///   Gets the server manager model item represented by the index.
  ///
  /// If the index represents a link item, the server manager model
  /// item returned will be the object the link points to.
  ///
  /// \param index The model index to look up.
  /// \return
  ///   A pointer to the server manager model item. Null is returned
  ///   if the index does not point to a server manager item.
  pqServerManagerModelItem *getItemFor(const QModelIndex &index) const;

  /// \brief
  ///   Gets the model index for the given server manager model item.
  /// \param item The server manager model item to look up.
  /// \return
  ///   The model index for the given item. The index will be invalid
  ///   if the item is not in the model.
  QModelIndex getIndexFor(pqServerManagerModelItem *item) const;

  /// \brief
  ///   Gets the type for the given index.
  /// \param index The model index to look up.
  /// \return
  ///   The type for the given index.
  ItemType getTypeFor(const QModelIndex &index) const;
  //@}

  /// \name Model Interaction
  //@{
  /// \brief
  ///   Gets the next model index in the tree.
  /// \param index The current index.
  /// \param root An alternate root for walking a subtree.
  /// \return
  ///   An index to the next item in the tree or an invalid index
  ///   when the end of the tree is reached.
  QModelIndex getNextIndex(const QModelIndex index,
      const QModelIndex &root=QModelIndex()) const;

  /// \brief
  ///   Gets whether or not the model indexes are editable.
  /// \return
  ///   True if the model indexes can be edited.
  bool isEditable() const {return this->Editable;}

  /// \brief
  ///   Sets whether or not the model indexes are editable.
  /// \param editable True if the model indexes can be edited.
  void setEditable(bool editable) {this->Editable = editable;}

  /// \brief
  ///   Gets whether or not the given index is selectable.
  /// \param index The model index.
  /// \return
  ///   True if the given index is selectable.
  bool isSelectable(const QModelIndex &index) const;

  /// \brief
  ///   Sets whether or not the given index is selectable.
  /// \param index The model index.
  /// \param selectable True if the index can be selected.
  void setSelectable(const QModelIndex &index, bool selectable);

  /// \brief
  ///   Sets whether of not an item subtree is selectable.
  /// \param item The root of the subtree.
  /// \param selectable True if the items can be selected.
  void setSubtreeSelectable(pqServerManagerModelItem *item, bool selectable);

  /// \brief
  ///   Sets the font hint for modified items.
  /// \param font The font to use for modified items.
  void setModifiedFont(const QFont &font);
  //@}

public slots:
  /// \name Model Modification Methods
  //@{
  /// \brief
  ///   Adds a server item to the pipeline model.
  ///
  /// The server is added as a child of the root node.
  ///
  /// \param server The server object to add.
  void addServer(pqServer *server);

  /// \brief
  ///   Sets up the model for a server removal.
  ///
  /// The closing book end event is not used. The information used
  /// in the process is cleaned up when the removing server signal is
  /// sent. This leaves nothing for the closing book end event to do.
  /// The removing server signal must be sent last in the cleanup
  /// process to avoid warnings.
  ///
  /// \param server The server that will be removed.
  void startRemovingServer(pqServer *server);

  /// \brief
  ///   Removes a server item from the pipeline model.
  /// \param server The server object to remove.
  void removeServer(pqServer *server);

  /// \brief
  ///   Adds a source to the pipeline model.
  ///
  /// The source is added to its server's list of sources. The source
  /// should have no inputs when added. The icon displayed for the
  /// source is determined by the type of source (source, filter or
  /// custom filter).
  ///
  /// \param source The source object to add.
  /// \sa pqPipelineModel::addConnection(pqPipelineSource *,
  ///   pqPipelineSource *, int)
  void addSource(pqPipelineSource *source);

  /// \brief
  ///   Removes a source from the pipeline model.
  ///
  /// The source should not have connected outputs when it is removed.
  ///
  /// \param source The source object to remove.
  /// \sa pqPipelineModel::removeConnection(pqPipelineSource *,
  ///   pqPipelineSource *, int)
  void removeSource(pqPipelineSource *source);

  /// \brief
  ///   Creates a connection between the source and sink.
  /// \param source The source object being connected.
  /// \param sink The sink object being connected.
  /// \param sourceOutputPort The ouput port on the source.
  /// \sa pqPipelineModel::addConnection(pqPipelineModelOutput *,
  ///   pqPipelineModelFilter *)
  void addConnection(pqPipelineSource *source, pqPipelineSource *sink,
      int sourceOutputPort);

  /// \brief
  ///   Disconnects the source and sink.
  /// \param source The source object being connected.
  /// \param sink The sink object being connected.
  /// \param sourceOutputPort The ouput port on the source.
  /// \sa pqPipelineModel::removeConnection(pqPipelineModelOutput *,
  ///   pqPipelineModelFilter *)
  void removeConnection(pqPipelineSource *source, pqPipelineSource *sink,
      int sourceOutputPort);
  //@}

  /// \name Model Update Methods
  //@{
  /// \brief
  ///   Updates the name column for the specified item.
  ///
  /// The model always returns the current name for an item. The view
  /// is notified so it can update the layout for the given index.
  ///
  /// \param item The server manager model item that changed.
  void updateItemName(pqServerManagerModelItem *item);

  /// \brief
  ///   Updates the display columns for the given source.
  /// \param source The source to update.
  void updateRepresentations(pqPipelineSource *source);

  /// \brief
  ///   Updates the icons in the current window column.
  ///
  /// The current window column shows whether or not the source is
  /// displayed in the current window. When the current window changes
  /// the entire column needs to be updated.
  ///
  /// \param module The current render module.
  void setView(pqView *module);
  //@}

signals:
  /// \brief
  ///   Emitted when the first child is added to a parent index.
  ///
  /// This signal can be used to expand the parent index when a child
  /// is added. This would always keep the tree expanded.
  ///
  /// \param index The parent index that now has children.
  void firstChildAdded(const QModelIndex &index);

  /// \brief
  ///   Emitted when an index is about to be moved in the hierarchy.
  ///
  /// This signal can be used to save the expanded state of the index
  /// and its subtree. When the index is restored, the expanded state
  /// can then be applied. The index row and parent will change in the
  /// move, so something else should be used to identify the index.
  /// The data in column zero can be used to identify the index when it
  /// is restored.
  ///
  /// \param index The index that will be moved.
  /// \sa pqPipelineModel::indexRestored(const QModelIndex &)
  void movingIndex(const QModelIndex &index);

  /// \brief
  ///   Emitted when an index is added back in the hierarchy.
  ///
  /// The signal is paired with a \c movingIndex signal. It is emitted
  /// after the index has been moved in the tree.
  ///
  /// \param index The index that was moved.
  /// \sa pqPipelineModel::movingIndex(const QModelIndex &)
  void indexRestored(const QModelIndex &index);

  /// \brief
  //    Emitted when the label for any index is changed via the GUI.
  //  
  //  \param index The index that was changed.
  //  \param name New name set by the user.
  void rename(const QModelIndex& index, const QString& name);

private:
  /// \brief
  ///   Handles the connection of the internal pipeline objects.
  ///
  /// A connection is created from source to sink. If the sink had no
  /// inputs, it will be moved from the server's list of sources to
  /// the source's list of outputs. If the sink was already on another
  /// source's output list, it will be moved back to the server's list
  /// of sources. A link item will be added to the two source's output
  /// lists in its place. If the sink was already on the server's list
  /// of sources because it had multiple inputs, a link item will be
  /// added to the source's list of outputs and the sink will remain
  /// where it is.
  ///
  /// \param source The source object being connected.
  /// \param sink The sink object being connected.
  /// \sa pqPipelineModel::removeConnection(pqPipelineModelOutput *,
  ///   pqPipelineModelFilter *)
  void addConnection(pqPipelineModelOutput *source,
      pqPipelineModelFilter *sink);

  /// \brief
  ///   Removes the connection of the internal pipeline objects.
  ///
  /// If the source is the only input of the sink, the sink will be
  /// moved to the server's source list. If the sink had two inputs,
  /// it will be moved to the other source's output list. If the sink
  /// had more inputs, the link item on the source's list will be
  /// removed and the link will stay where it is.
  ///
  /// \param source The source object being disconnected.
  /// \param sink The sink object being disconnected.
  /// \sa pqPipelineModel::addConnection(pqPipelineModelOutput *,
  ///   pqPipelineModelFilter *)
  void removeConnection(pqPipelineModelOutput *source,
      pqPipelineModelFilter *sink);

  /// \brief
  ///   Updates the display columns for sources displayed in the
  ///   render module.
  /// \param module The modified render module.
  void updateDisplays(pqView *module);

  /// \brief
  ///   Notifies the view that the input link items have changed.
  /// \param sink The modified item.
  /// \param column The column to use in the index.
  void updateInputLinks(pqPipelineModelFilter *sink, int column=0);

  /// \brief
  ///   Notifies the view that the output port items have changed.
  /// \param source The modified item.
  /// \param column The column to use in the index.
  void updateOutputPorts(pqPipelineModelSource* source, int column=0);

  /// \brief
  ///   Gets the pipeline model item for the server manager model item.
  /// \param item The server manager model item to look up.
  /// \return
  ///   A pointer to the associated pipeline model item.
  pqPipelineModelItem *getModelItemFor(pqServerManagerModelItem *item) const;

  /// \brief
  ///   Creates a model index for the pipeline model item.
  /// \param item The item to make an index for.
  /// \param column The column to set in the new index.
  /// \return
  ///   A model index for the specified item.
  QModelIndex makeIndex(pqPipelineModelItem *item, int column=0) const;

  /// Used to remove all the null entries in the item map.
  void cleanPipelineMap();

  /// \brief
  ///   Gets the next model item in the tree.
  /// \param item The current item.
  /// \param root An alternate root for walking a subtree.
  /// \return
  ///   A pointer to the next item in the tree or null when the end
  ///   of the tree is reached.
  pqPipelineModelItem *getNextModelItem(pqPipelineModelItem *item,
      pqPipelineModelItem *root=0) const;

  /// Initializes the list of pixmaps.
  void initializePixmaps();

private:
  pqPipelineModelInternal *Internal; ///< Stores the pipeline representation.
  QPixmap *PixmapList;               ///< Stores the item icons.
  bool Editable;                     ///< True if the model is editable.
};

#endif
