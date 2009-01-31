/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqMultiView.h,v $

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

#ifndef _pqMultiView_h
#define _pqMultiView_h

#include "pqComponentsExport.h"
#include <QStackedWidget>
#include <QList>

class pqMultiViewFrame;
class vtkPVXMLElement;
class QSplitter;

/// class to manage locations of multiple view widgets
class PQCOMPONENTS_EXPORT pqMultiView : public QStackedWidget
{
  Q_OBJECT
public:
  class PQCOMPONENTS_EXPORT Index: public QList<int>
  {
  public:
    QString getString() const;
    void setFromString(const QString& string);
  };
  
  pqMultiView(QWidget* parent = NULL);
  virtual ~pqMultiView();

  /// Must be called to initialize the first frame.
  void init();

  /// \brief
  ///   Resets the multi-view to its original state.
  /// \param removed Used to return all the removed widgets.
  virtual void reset(QList<QWidget*> &removed);

  /// replace a widget at index with a new widget, returns the old one
  virtual QWidget* replaceView(Index index, QWidget* widget);

  /// split a location in a direction
  /// a dummy widget is inserted and an index for it is returned
  virtual Index splitView(Index index, Qt::Orientation orientation);

  virtual Index splitView(pqMultiView::Index index, 
    Qt::Orientation orientation, float percent);

  pqMultiViewFrame* splitWidget(QWidget* widget, Qt::Orientation o,float percent);

  /// remove a widget inserted by replaceWidget or splitView
  virtual void removeView(QWidget* widget);

  /// get the index of the widget in its parent's layout
  Index indexOf(QWidget*) const;

  /// Returns the index of the parent. If index is the top
  /// most widget returns an empty index.
  Index parentIndex(const Index& index) const;

  /// Returns the orientation for the placement of the
  /// widget in its parent splitter.
  Qt::Orientation widgetOrientation(QWidget* widget) const;

  /// returns the ratio used while splitting the widget.
  float widgetSplitRatio(QWidget* widget) const;

  /// get the widget from an index
  QWidget* widgetOfIndex(Index index);

  virtual void saveState(vtkPVXMLElement *root);
  virtual void loadState(vtkPVXMLElement *root);
signals:
  /// signal for new frame added
  void frameAdded(pqMultiViewFrame*);
  /// signal for frame removed
  void frameRemoved(pqMultiViewFrame*);

  /// signal before a frame is removed
  void preFrameRemoved(pqMultiViewFrame*);

  // Fired to hide all frame decorations.
  void hideFrameDecorations();
  
  // Fired to show all frame decorations.
  void showFrameDecorations();

  /// Fired when a request is made to split the views
  void afterSplitView(const Index& index, 
    Qt::Orientation orientation, float percent, const Index& childIndex);

public slots:
  void removeWidget(QWidget *widget);
  pqMultiViewFrame* splitWidgetHorizontal(QWidget *widget);
  pqMultiViewFrame* splitWidgetVertical(QWidget *widget);

  /// hides the frame decorations.
  void hideDecorations();

  /// shows the frame decorations.
  void showDecorations();

protected slots:
  void maximizeWidget(QWidget*);
  void restoreWidget(QWidget*);


protected:

 // bool eventFilter(QObject*, QEvent* e);
  pqMultiViewFrame* splitWidget(QWidget*, Qt::Orientation);
  void setup(pqMultiViewFrame*);
  void cleanup(pqMultiViewFrame*);
  QFrame* SplitterFrame;
  QFrame* MaximizeFrame;
  pqMultiViewFrame* FillerFrame;

private:
  void saveSplitter(vtkPVXMLElement *element, QSplitter *splitter, int index);
  void restoreSplitter(QWidget *widget, vtkPVXMLElement *element);
  void cleanSplitter(QSplitter *splitter, QList<QWidget*> &removed);
  pqMultiViewFrame* CurrentMaximizedFrame;
};


#endif //_pqMultiView_h

