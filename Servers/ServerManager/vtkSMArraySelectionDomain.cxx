/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMArraySelectionDomain.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMArraySelectionDomain.h"

#include "vtkObjectFactory.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMArraySelectionDomain);
vtkCxxRevisionMacro(vtkSMArraySelectionDomain, "$Revision: 1.5 $");

//---------------------------------------------------------------------------
vtkSMArraySelectionDomain::vtkSMArraySelectionDomain()
{
}

//---------------------------------------------------------------------------
vtkSMArraySelectionDomain::~vtkSMArraySelectionDomain()
{
}

//---------------------------------------------------------------------------
void vtkSMArraySelectionDomain::Update(vtkSMProperty* prop)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if (svp && svp->GetInformationOnly())
    {
    this->RemoveAllStrings();
    this->SetIntDomainMode(vtkSMStringListRangeDomain::BOOLEAN);

    if (svp->GetNumberOfElementsPerCommand() == 1 &&
      svp->GetElementType(0) == vtkSMStringVectorProperty::STRING)
      {
      // Information property does not provide selection information,
      // it simply provides the list of arrays.
      unsigned int numEls = svp->GetNumberOfElements();
      for (unsigned int i=0; i<numEls; i++)
        {
        this->AddString(svp->GetElement(i));
        }
      }
    else
      {
      unsigned int numEls = svp->GetNumberOfElements();
      if (numEls % 2 != 0)
        {
        vtkErrorMacro("The required property seems to have wrong number of "
          "elements. It should be a multiple of 2");
        return;
        }
      for (unsigned int i=0; i<numEls/2; i++)
        {
        this->AddString(svp->GetElement(i*2));
        }
      }
    this->InvokeModified();
    }
}
  
int vtkSMArraySelectionDomain::SetDefaultValues(vtkSMProperty* prop)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if(!svp || this->GetNumberOfRequiredProperties() == 0)
    {
    return this->Superclass::SetDefaultValues(prop);
    }

  // info property has default values
  int numStrings = this->GetNumberOfStrings();
  vtkSMStringVectorProperty* isvp;
  isvp = vtkSMStringVectorProperty::SafeDownCast(prop->GetInformationProperty());
  if(isvp)
    {
    unsigned int numEls = isvp->GetNumberOfElements();
    for (unsigned int i=0; i<numEls; i++)
      {
      svp->SetElement(i, isvp->GetElement(i));
      }
    svp->SetNumberOfElements(numEls);
    }
  
  svp->SetNumberOfElements(numStrings * 2);

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMArraySelectionDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
