/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMExtractSelectionsProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMExtractSelectionsProxy.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSelection.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"

#include <vtkstd/vector>

class vtkSMExtractSelectionsProxy::vtkInternal
{
public:

  typedef vtkstd::vector<vtkIdType> IdVectorType;
  IdVectorType Indices;
  IdVectorType GlobalsIDs;
};

vtkStandardNewMacro(vtkSMExtractSelectionsProxy);
vtkCxxRevisionMacro(vtkSMExtractSelectionsProxy, "$Revision: 1.1 $");
//-----------------------------------------------------------------------------
vtkSMExtractSelectionsProxy::vtkSMExtractSelectionsProxy()
{
  this->UseGlobalIDs = 0;
  this->SelectionFieldType = vtkSelection::CELL;
  this->Internal = new vtkInternal();
}

//-----------------------------------------------------------------------------
vtkSMExtractSelectionsProxy::~vtkSMExtractSelectionsProxy()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vtkSMExtractSelectionsProxy::AddIndex(vtkIdType piece, vtkIdType id)
{
  this->Internal->Indices.push_back(piece);
  this->Internal->Indices.push_back(id);
}

//-----------------------------------------------------------------------------
void vtkSMExtractSelectionsProxy::RemoveAllIndices()
{
  this->Internal->Indices.clear();
}

//-----------------------------------------------------------------------------
void vtkSMExtractSelectionsProxy::AddGlobalID(vtkIdType id)
{
  // piece number is not used for global ids.
  this->Internal->GlobalsIDs.push_back(-1);
  this->Internal->GlobalsIDs.push_back(id);
}

//-----------------------------------------------------------------------------
void vtkSMExtractSelectionsProxy::RemoveAllGlobalIDs()
{
  this->Internal->GlobalsIDs.clear();
}

//-----------------------------------------------------------------------------
void vtkSMExtractSelectionsProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  if (!this->ObjectsCreated)
    {
    return;
    }

  vtkSMSourceProxy* selectionSource = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("SelectionSource"));
  if (!selectionSource)
    {
    vtkErrorMacro("Missing subproxy: SelectionSource");
    return;
    }
  
  this->AddInput(selectionSource, "SetSelectionConnection");
}

//-----------------------------------------------------------------------------
void vtkSMExtractSelectionsProxy::UpdateVTKObjects()
{
  this->Superclass::UpdateVTKObjects();

  vtkSMProxy* selectionSource = this->GetSubProxy("SelectionSource");
  if (!selectionSource)
    {
    vtkErrorMacro("Missing subproxy: SelectionSource");
    return;
    }

  vtkSMIdTypeVectorProperty* idvp = vtkSMIdTypeVectorProperty::SafeDownCast(
    selectionSource->GetProperty("IDs"));
  if (this->UseGlobalIDs)
    {
    idvp->SetNumberOfElements(this->Internal->GlobalsIDs.size());
    if (this->Internal->GlobalsIDs.size() > 0)
      {
      idvp->SetElements(&this->Internal->GlobalsIDs[0]);
      }
    }
  else
    {
    idvp->SetNumberOfElements(this->Internal->Indices.size());
    if (this->Internal->Indices.size() > 0)
      {
      idvp->SetElements(&this->Internal->Indices[0]);
      }
    }

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSource->GetProperty("FieldType"));
  ivp->SetElement(0, this->SelectionFieldType);

  ivp = vtkSMIntVectorProperty::SafeDownCast(
    selectionSource->GetProperty("ContentType"));
  if (this->UseGlobalIDs)
    {
    ivp->SetElement(0, vtkSelection::GLOBALIDS);
    }
  else
    {
    ivp->SetElement(0, vtkSelection::INDICES);
    }
  selectionSource->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMExtractSelectionsProxy::CopySelectionSource(
  vtkSMSourceProxy* selSource)
{
  vtkSMSourceProxy* selectionSource = 
    vtkSMSourceProxy::SafeDownCast(this->GetSubProxy("SelectionSource"));
  if (!selectionSource)
    {
    vtkErrorMacro("Missing subproxy: SelectionSource");
    return;
    }
  
  if(selSource)
    {
    selectionSource->Copy(selSource, 
      0, vtkSMProxy::COPY_PROXY_PROPERTY_VALUES_BY_CLONING);
    }
  else
    {
    //this->AddInput(0, "SetSelectionConnection");
    }
  selectionSource->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
int vtkSMExtractSelectionsProxy::ReadXMLAttributes(
  vtkSMProxyManager* pm, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(pm, element))
    {
    return 0;
    }
  const char* type = element->GetAttribute("selection_field_type");
  if (type && strcmp(type,"POINT") == 0)
    {
    this->SelectionFieldType  = vtkSelection::POINT;
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMExtractSelectionsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseGlobalIDs: " << this->UseGlobalIDs << endl;
  os << indent << "SelectionFieldType: " << this->SelectionFieldType << endl;
}
