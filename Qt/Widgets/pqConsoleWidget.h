/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqConsoleWidget.h,v $

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

#ifndef _pqConsoleWidget_h
#define _pqConsoleWidget_h

#include "QtWidgetsExport.h"

#include <QWidget>
#include <QTextCharFormat>

/**
  Qt widget that provides an interactive console - you can send text to the console by calling printString(),
  and receive user input by connecting to the executeCommand() slot.
  
  \sa pqPythonShell, pqOutputWindow
*/
  
class QTWIDGETS_EXPORT pqConsoleWidget :
  public QWidget
{
  Q_OBJECT
  
public:
  pqConsoleWidget(QWidget* Parent);
  virtual ~pqConsoleWidget();

  /// Returns the current formatting that will be used by printString
  QTextCharFormat getFormat();
  /// Sets formatting that will be used by printString
  void setFormat(const QTextCharFormat& Format);
  
signals:
  /// Signal emitted whenever the user enters a command
  void executeCommand(const QString& Command);

public slots:
  /// Writes the supplied text to the console
  void printString(const QString& Text);

  /// Clears the contents of the console
  void clear();

  /// Puts out an input accepting prompt.
  /// It is recommended that one uses prompt instead of printString() to print
  /// an input prompt since this call ensures that the prompt is shown on a new
  /// line.
  void prompt(const QString& text);
private:
  pqConsoleWidget(const pqConsoleWidget&);
  pqConsoleWidget& operator=(const pqConsoleWidget&);

  void internalExecuteCommand(const QString& Command);

  class pqImplementation;
  pqImplementation* const Implementation;
  friend class pqImplementation;
};

#endif // !_pqConsoleWidget_h

