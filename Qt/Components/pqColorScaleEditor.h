/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqColorScaleEditor.h,v $

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

/// \file pqColorScaleEditor.h
/// \date 2/14/2007

#ifndef _pqColorScaleEditor_h
#define _pqColorScaleEditor_h


#include "pqComponentsExport.h"
#include <QDialog>

class pqColorScaleEditorForm;
class pqPipelineRepresentation;
class pqScalarBarRepresentation;
class pqScalarsToColors;
class QHideEvent;
class QShowEvent;
class QString;
class vtkTransferFunctionViewer;


class PQCOMPONENTS_EXPORT pqColorScaleEditor : public QDialog
{
  Q_OBJECT

public:
  pqColorScaleEditor(QWidget *parent=0);
  virtual ~pqColorScaleEditor();

public slots:
  void setRepresentation(pqPipelineRepresentation *display);

protected:
  virtual void showEvent(QShowEvent *e);
  virtual void hideEvent(QHideEvent *e);

private slots:
  /// \name Color Scale Methods
  //@{
  void handleEditorPointMoved();
  void handleEditorPointMoveFinished();
  void handleEditorAddOrDelete();
  void handleEditorAdd(int index);
  void setColors();
  void changeCurrentColor();

  void handlePointsChanged();

  void handleEditorCurrentChanged();
  void setCurrentPoint(int index);
  void setValueFromText();
  void setOpacityFromText();

  void setColorSpace(int index);

  void savePreset();
  void loadPreset();

  void setComponent(int index);

  void setLogScale(bool on);

  void setAutoRescale(bool on);
  void rescaleToNewRange();
  void rescaleToDataRange();

  void setUseDiscreteColors(bool on);
  void setSizeFromText();
  void setSizeFromSlider(int tableSize);
  void setTableSize(int tableSize);

  void setScalarRange(double min, double max);
  //@}

  /// \name Color Legend Methods
  //@{
  void checkForLegend();
  void setLegendVisibility(bool visible);
  void updateLegendVisibility(bool visible);
  void setLegendName(const QString &text);
  void setLegendComponent(const QString &text);
  void setLegendTitle(const QString &name, const QString &component);
  void updateLegendTitle();
  //@}

  /// \name Cleanup Methods
  //@{
  void cleanupDisplay();
  void cleanupLegend();
  //@}

private:
  void loadBuiltinColorPresets();
  void loadColorPoints();
  void initColorScale();
  void enablePointControls();
  void updatePointValues();
  void enableRescaleControls(bool enable);
  void enableResolutionControls(bool enable);
  void updateScalarRange(double min, double max);
  void setLegend(pqScalarBarRepresentation *legend);
  void enableLegendControls(bool enable);

private:
  pqColorScaleEditorForm *Form;
  vtkTransferFunctionViewer *Viewer;
  pqPipelineRepresentation *Display;
  pqScalarsToColors *ColorMap;
  pqScalarBarRepresentation *Legend;
};

#endif
