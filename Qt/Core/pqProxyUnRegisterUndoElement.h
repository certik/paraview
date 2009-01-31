/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqProxyUnRegisterUndoElement.h,v $

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
#ifndef __pqProxyUnRegisterUndoElement_
#define __pqProxyUnRegisterUndoElement_

#include "vtkSMProxyUnRegisterUndoElement.h"
#include "pqCoreExport.h"

class PQCORE_EXPORT pqProxyUnRegisterUndoElement : public vtkSMProxyUnRegisterUndoElement
{
public:
  static pqProxyUnRegisterUndoElement* New();
  vtkTypeRevisionMacro(pqProxyUnRegisterUndoElement, vtkSMProxyUnRegisterUndoElement);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Undo the operation encapsulated by this element.
  // Overridden to set the helper proxies on undo.
  virtual int Undo();

  // Description:
  // Returns if this element can load the xml state for the given element.
  virtual bool CanLoadState(vtkPVXMLElement*);

  /// Description:
  /// Sets the information about the proxy that is getting unregistered.
  virtual void ProxyToUnRegister(const char* groupname, const char* proxyname, 
    vtkSMProxy* proxy);
protected:
  pqProxyUnRegisterUndoElement();
  ~pqProxyUnRegisterUndoElement();

private:
  pqProxyUnRegisterUndoElement(const pqProxyUnRegisterUndoElement&); // Not implemented.
  void operator=(const pqProxyUnRegisterUndoElement&); // Not implemented.
};

#endif

