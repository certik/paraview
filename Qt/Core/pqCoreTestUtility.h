/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqCoreTestUtility.h,v $

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

#ifndef _pqCoreTestUtility_h
#define _pqCoreTestUtility_h

#include "pqCoreExport.h"
#include "pqTestUtility.h"
#include <vtkIOStream.h>
#include <QStringList>

class QString;
class pqEventPlayer;
class pqEventTranslator;
class pqProcessModuleGUIHelper;
class vtkRenderWindow;
class vtkImageData;

/// Provides ParaView-specific functionality for regression testing
class PQCORE_EXPORT pqCoreTestUtility : public pqTestUtility
{
  Q_OBJECT

public:
  pqCoreTestUtility(QObject* parent = 0);
  ~pqCoreTestUtility();

public:
  /// Returns the absolute path to the PARAVIEW_DATA_ROOT in canonical form
  /// (slashes forward), or empty string
  static QString DataRoot();

  /// Returns the temporary test directory in which tests can write
  /// temporary outputs, difference images etc.
  static QString TestDirectory();

  /// Saves the contents of a render window to a file for later use as a
  /// reference image
  static bool SaveScreenshot(vtkRenderWindow* RenderWindow, const QString& File);

  /// Compares the contents of a render window to a reference image,
  /// returning true iff the two match within a given threshold
  static bool CompareImage(vtkRenderWindow* RenderWindow, 
                           const QString& ReferenceImage, 
                           double Threshold, 
                           ostream& Output, 
                           const QString& TempDirectory);

  /// Compares the test image to a reference image,
  /// returning true iff the two match within a given threshold
  static bool CompareImage(vtkImageData* testImage,
                           const QString& ReferenceImage, 
                           double Threshold, 
                           ostream& Output, 
                           const QString& TempDirectory);

public slots:
  void playTests(const QString& filename);
  void playTests(const QStringList& filenames); 

protected slots:
  void testSucceeded();
  void testFailed();

private:
  QStringList TestFilenames;
};

#endif // !_pqCoreTestUtility_h

