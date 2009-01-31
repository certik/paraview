/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqPQLookupTableManager.h,v $

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
#ifndef __pqPQLookupTableManager_h
#define __pqPQLookupTableManager_h

#include "pqLookupTableManager.h"
#include "pqComponentsExport.h"


/// pqPQLookupTableManager is an implementation specific to ParaView.
/// A lookup table is shared among all arrays with same name and
/// same number of components.
class PQCOMPONENTS_EXPORT pqPQLookupTableManager : public pqLookupTableManager
{
  Q_OBJECT
public:
  pqPQLookupTableManager(QObject* parent=0);
  virtual ~pqPQLookupTableManager();

  /// Get a LookupTable for the array with name \c arrayname 
  /// and component. component = -1 represents magnitude. 
  /// This subclass associates a LUT with arrayname:component
  /// pair. If  none exists, a new one will be created.
  pqScalarsToColors* getLookupTable(pqServer* server, const QString& arrayname,
    int number_of_components, int component);

public slots:
  /// Called to update scalar ranges of all lookup tables.
  virtual void updateLookupTableScalarRanges();

protected:
  /// Called when a new LUT pq object is created. 
  /// This happens as a result of either the GUI or python
  /// registering a LUT proxy.
  virtual void onAddLookupTable(pqScalarsToColors* lut);

  /// Called when a LUT is removed.
  virtual void onRemoveLookupTable(pqScalarsToColors* lut);

protected:
  /// creates a new LUT.
  pqScalarsToColors* createLookupTable(pqServer* server,
    const QString& arrayname, int number_of_components, int component);

private:
  pqPQLookupTableManager(const pqPQLookupTableManager&); // Not implemented.
  void operator=(const pqPQLookupTableManager&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};

#endif
