/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMInputArrayDomain.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMInputArrayDomain.h"

#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMInputArrayDomain);
vtkCxxRevisionMacro(vtkSMInputArrayDomain, "$Revision: 1.14 $");

//---------------------------------------------------------------------------
static const char* const vtkSMInputArrayDomainAttributeTypes[] = {
  "point",
  "cell",
  "any"
};

//---------------------------------------------------------------------------
vtkSMInputArrayDomain::vtkSMInputArrayDomain()
{
  this->AttributeType = vtkSMInputArrayDomain::ANY;
  this->NumberOfComponents = 0;
}

//---------------------------------------------------------------------------
vtkSMInputArrayDomain::~vtkSMInputArrayDomain()
{
}

//---------------------------------------------------------------------------
int vtkSMInputArrayDomain::IsInDomain(vtkSMProperty* property)
{
  if (this->IsOptional)
    {
    return 1;
    }

  if (!property)
    {
    return 0;
    }

  unsigned int i;

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(property);
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(property);
  if (pp)
    {
    unsigned int numProxs = pp->GetNumberOfUncheckedProxies();
    for (i=0; i<numProxs; i++)
      {
      if (!this->IsInDomain( 
            vtkSMSourceProxy::SafeDownCast(pp->GetUncheckedProxy(i)),
            (ip? ip->GetUncheckedOutputPortForConnection(i):0)) )
        {
        return 0;
        }
      }
    return 1;
    }

  return 0;
}

//---------------------------------------------------------------------------
int vtkSMInputArrayDomain::IsInDomain(vtkSMSourceProxy* proxy, 
                                      int outputport/*=0*/)
{
  if (!proxy)
    {
    return 0;
    }

  // Make sure the outputs are created.
  proxy->CreateParts();
  vtkPVDataInformation* info = proxy->GetDataInformation(outputport);
  if (!info)
    {
    return 0;
    }

  if (this->AttributeType == vtkSMInputArrayDomain::POINT ||
      this->AttributeType == vtkSMInputArrayDomain::ANY)
    {
    if (this->AttributeInfoContainsArray(proxy, outputport, 
        info->GetPointDataInformation()))
      {
      return 1;
      }
    }

  if (this->AttributeType == vtkSMInputArrayDomain::CELL||
      this->AttributeType == vtkSMInputArrayDomain::ANY)
    {
    if (this->AttributeInfoContainsArray(proxy, outputport,
        info->GetCellDataInformation()))
      {
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkSMInputArrayDomain::CheckForArray(
  vtkPVArrayInformation* arrayInfo, vtkPVDataSetAttributesInformation* attrInfo)
{
  if (!attrInfo || !arrayInfo)
    {
    return 0;
    }

  int num = attrInfo->GetNumberOfArrays();
  for (int idx = 0; idx < num; ++idx)
    {
    vtkPVArrayInformation* curInfo = attrInfo->GetArrayInformation(idx);
    if (curInfo == arrayInfo)
      {
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkSMInputArrayDomain::IsFieldValid(
  vtkSMSourceProxy* proxy, int outputport, vtkPVArrayInformation* arrayInfo)
{
  return this->IsFieldValid(proxy, outputport, arrayInfo, 0);
}

//----------------------------------------------------------------------------
int vtkSMInputArrayDomain::IsFieldValid(
  vtkSMSourceProxy* proxy, int outputport, 
  vtkPVArrayInformation* arrayInfo, int bypass)
{
  vtkPVDataInformation* info = proxy->GetDataInformation(outputport);
  if (!info)
    {
    return 0;
    }

  int attributeType = this->AttributeType;
  if (!bypass)
    {
    // FieldDataSelection typically is a SelectInputScalars kind of property
    // in which case the attribute type is at index 3 or is a simple
    // type choosing property in which case it's a vtkSMIntVectorProperty.
    vtkSMProperty* pfds = this->GetRequiredProperty("FieldDataSelection");
    vtkSMStringVectorProperty* fds = vtkSMStringVectorProperty::SafeDownCast(
      pfds);
    vtkSMIntVectorProperty* ifds = vtkSMIntVectorProperty::SafeDownCast(pfds);
    if (fds || ifds)
      {
      int val = (fds)? atoi(fds->GetUncheckedElement(3)) :
        ifds->GetUncheckedElement(0);
      if (val == vtkDataObject::FIELD_ASSOCIATION_POINTS)
        {
        attributeType = vtkSMInputArrayDomain::POINT;
        }
      else if (val == vtkDataObject::FIELD_ASSOCIATION_CELLS)
        {
        attributeType = vtkSMInputArrayDomain::CELL;
        }
      }
    }

  int isField = 0;
  if (attributeType == vtkSMInputArrayDomain::POINT ||
      attributeType == vtkSMInputArrayDomain::ANY)
    {
    isField = this->CheckForArray(arrayInfo, info->GetPointDataInformation());
    }

  if (!isField &&
      (attributeType == vtkSMInputArrayDomain::CELL||
       attributeType == vtkSMInputArrayDomain::ANY) )
    {
    isField = this->CheckForArray(arrayInfo, info->GetCellDataInformation());
    }

  if (!isField)
    {
    return 0;
    }

  if (this->NumberOfComponents > 0 && 
      this->NumberOfComponents != arrayInfo->GetNumberOfComponents())
    {
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkSMInputArrayDomain::AttributeInfoContainsArray(
  vtkSMSourceProxy* proxy, 
  int outputport,
  vtkPVDataSetAttributesInformation* attrInfo)
{
  if (!attrInfo)
    {
    return 0;
    }

  int num = attrInfo->GetNumberOfArrays();
  for (int idx = 0; idx < num; ++idx)
    {
    vtkPVArrayInformation* arrayInfo = attrInfo->GetArrayInformation(idx);
    if (this->IsFieldValid(proxy, outputport, arrayInfo, 1))
      {
      return 1;
      }
    }

  return 0;
}

//---------------------------------------------------------------------------
void vtkSMInputArrayDomain::ChildSaveState(vtkPVXMLElement* domainElement)
{
  this->Superclass::ChildSaveState(domainElement);

  vtkPVXMLElement* inputArrayElem = vtkPVXMLElement::New();
  inputArrayElem->SetName("InputArray");
  inputArrayElem->AddAttribute("attribute_type", 
                               this->GetAttributeTypeAsString());
  inputArrayElem->AddAttribute("number_of_components", 
                               this->GetNumberOfComponents());
  domainElement->AddNestedElement(inputArrayElem);
  inputArrayElem->Delete();

}

//---------------------------------------------------------------------------
int vtkSMInputArrayDomain::ReadXMLAttributes(
  vtkSMProperty* prop, vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(prop, element);

  const char* attribute_type = element->GetAttribute("attribute_type");
  if (attribute_type)
    {
    if (strcmp(attribute_type, "cell") == 0)
      {
      this->SetAttributeType(vtkSMInputArrayDomain::CELL);
      }
    else if (strcmp(attribute_type, "point") == 0)
      {
      this->SetAttributeType(
        static_cast<unsigned char>(vtkSMInputArrayDomain::POINT));
      }
    else if (strcmp(attribute_type, "any") == 0)
      {
      this->SetAttributeType(
        static_cast<unsigned char>(vtkSMInputArrayDomain::ANY));
      }
    else
      {
      vtkErrorMacro("Unrecognize attribute type.");
      return 0;
      }
    }

  int numComponents;
  if (element->GetScalarAttribute("number_of_components", &numComponents))
    {
    this->SetNumberOfComponents(numComponents);
    }

  return 1;
}

//---------------------------------------------------------------------------
const char* vtkSMInputArrayDomain::GetAttributeTypeAsString()
{
  return vtkSMInputArrayDomainAttributeTypes[this->AttributeType];
}

//---------------------------------------------------------------------------
void vtkSMInputArrayDomain::SetAttributeType(const char* type)
{
  if ( ! type )
    {
    vtkErrorMacro("No type specified");
    return;
    }
  unsigned char cc;
  for ( cc = 0; cc < vtkSMInputArrayDomain::LAST_ATTRIBUTE_TYPE; cc ++ )
    {
    if ( strcmp(type, vtkSMInputArrayDomainAttributeTypes[cc]) == 0 )
      {
      this->SetAttributeType(cc);
      return;
      }
    }
  vtkErrorMacro("No such attribute type: " << type);
}

//---------------------------------------------------------------------------
void vtkSMInputArrayDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "NumberOfComponents: " << this->NumberOfComponents << endl;
  os << indent << "AttributeType: " << this->AttributeType << endl;
}
