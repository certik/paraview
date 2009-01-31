/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqSplitViewUndoElement.cxx,v $

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
#include "pqSplitViewUndoElement.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"

#include "pqApplicationCore.h"

vtkStandardNewMacro(pqSplitViewUndoElement);
vtkCxxRevisionMacro(pqSplitViewUndoElement, "$Revision: 1.3 $");
//-----------------------------------------------------------------------------
pqSplitViewUndoElement::pqSplitViewUndoElement()
{
}

//-----------------------------------------------------------------------------
pqSplitViewUndoElement::~pqSplitViewUndoElement()
{
}

//-----------------------------------------------------------------------------
int pqSplitViewUndoElement::Redo()
{
  if (!this->XMLElement)
    {
    vtkErrorMacro("Invalid state.");
    return 0;
    }

  return this->RedoInternal();
}

//-----------------------------------------------------------------------------
int pqSplitViewUndoElement::Undo()
{
  if (!this->XMLElement)
    {
    vtkErrorMacro("Invalid state.");
    return 0;
    }

  return this->UndoInternal();
}

//-----------------------------------------------------------------------------
int pqSplitViewUndoElement::RedoInternal()
{
  pqMultiView::Index index;
  index.setFromString(this->XMLElement->GetAttribute("index"));

  int orientation;
  this->XMLElement->GetScalarAttribute("orientation", &orientation);

  double percent;
  this->XMLElement->GetScalarAttribute("percent", &percent);

  pqMultiView* manager = qobject_cast<pqMultiView*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_MANAGER"));
  if (!manager)
    {
    vtkErrorMacro("Failed to locate the multi view manager. "
      << "MULTIVIEW_MANAGER must be registered with application core.");
    return 0;
    }

  manager->splitWidget(
    manager->widgetOfIndex(index), 
    (orientation==0x01)? Qt::Horizontal : Qt::Vertical, 
    (float)percent);
  return 1;
}

//-----------------------------------------------------------------------------
int pqSplitViewUndoElement::UndoInternal()
{
  pqMultiView::Index index;
  index.setFromString(this->XMLElement->GetAttribute("child_index"));

  pqMultiView* manager = qobject_cast<pqMultiView*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_MANAGER"));
  if (!manager)
    {
    vtkErrorMacro("Failed to locate the multi view manager. "
      << "MULTIVIEW_MANAGER must be registered with application core.");
    return 0;
    }

  manager->removeWidget(manager->widgetOfIndex(index));
  return 1;
}

//-----------------------------------------------------------------------------
bool pqSplitViewUndoElement::CanLoadState(vtkPVXMLElement* elem)
{
  return (elem && elem->GetName() && strcmp(elem->GetName(), "SplitView") == 0);
}

//-----------------------------------------------------------------------------
void pqSplitViewUndoElement::SplitView(
  const pqMultiView::Index& index, Qt::Orientation orientation, float percent,
  const pqMultiView::Index& childIndex)
{
  vtkPVXMLElement* elem = vtkPVXMLElement::New();
  elem->SetName("SplitView");

  elem->AddAttribute("index", index.getString().toAscii().data());
  elem->AddAttribute("child_index", childIndex.getString().toAscii().data());
  elem->AddAttribute("orientation", orientation);
  elem->AddAttribute("percent", static_cast<double>(percent));

  this->SetXMLElement(elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
void pqSplitViewUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
