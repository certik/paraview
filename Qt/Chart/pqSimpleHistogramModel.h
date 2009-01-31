/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqSimpleHistogramModel.h,v $

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

/// \file pqSimpleHistogramModel.h
/// \date 8/15/2006

#ifndef _pqSimpleHistogramModel_h
#define _pqSimpleHistogramModel_h


#include "QtChartExport.h"
#include "pqHistogramModel.h"

class pqChartValue;
class pqSimpleHistogramModelInternal;


/// \class pqSimpleHistogramModel
/// \brief
///   The pqSimpleHistogramModel class is a generic histogram model.
class QTCHART_EXPORT pqSimpleHistogramModel : public pqHistogramModel
{
  Q_OBJECT

public:
  /// \brief
  ///   Creates a histogram list model object.
  /// \param parent The parent object.
  pqSimpleHistogramModel(QObject *parent=0);
  virtual ~pqSimpleHistogramModel();

  /// \name pqHistogramModel Methods
  //@{
  virtual int getNumberOfBins() const;
  virtual void getBinValue(int index, pqChartValue &bin) const;
  virtual void getBinRange(int index, pqChartValue &min,
      pqChartValue &max) const;

  virtual void getRangeX(pqChartValue &min, pqChartValue &max) const;
  virtual void getRangeY(pqChartValue &min, pqChartValue &max) const;
  //@}

  /// \name Data Setup Methods
  //@{
  void startModifyingData();
  void finishModifyingData();

  void addBinRangeBoundary(const pqChartValue &value);
  void removeBinRangeBoundary(int index);
  void clearBinRangeBoundaries();

  void generateBoundaries(const pqChartValue &min, const pqChartValue &max,
      int intervals);

  void setBinValue(int index, const pqChartValue &value);
  //@}

private:
  void updateXRange();
  void updateYRange();

private:
  pqSimpleHistogramModelInternal *Internal; ///< Stores the histogram data.
};

#endif
