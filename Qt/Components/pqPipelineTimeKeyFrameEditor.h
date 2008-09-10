/*=========================================================================

   Program:   ParaQ
   Module:    $RCSfile: pqPipelineTimeKeyFrameEditor.h,v $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#ifndef _pqPipelineTimeKeyFrameEditor_h
#define _pqPipelineTimeKeyFrameEditor_h

#include <QDialog>
#include "pqComponentsExport.h"

class pqAnimationScene;
class pqAnimationCue;

/// editor for editing pipeline time key frames
class PQCOMPONENTS_EXPORT pqPipelineTimeKeyFrameEditor : public QDialog
{
  typedef QDialog Superclass;
  Q_OBJECT
public:
  pqPipelineTimeKeyFrameEditor(pqAnimationScene* scene, pqAnimationCue* cue,
                               QWidget* p);
  ~pqPipelineTimeKeyFrameEditor();

public slots:
  /// read the key frame data and display it
  void readKeyFrameData();
  /// write the key frame data as edited by the user to the server manager
  void writeKeyFrameData();

protected slots:
  void updateState();

private:
  class pqInternal;
  pqInternal* Internal;
};


#endif
