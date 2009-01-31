/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMEnumerationDomain.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMEnumerationDomain - list of integers with associated strings
// .SECTION Description
// vtkSMEnumerationDomain represents an enumeration of integer values
// with associated descriptive strings.
// Valid XML elements are:
// @verbatim
// * <Entry text="" value=""/> where text is the descriptive
// string and value is the integer value.
// @endverbatim
// .SECTION See Also
// vtkSMDomain 

#ifndef __vtkSMEnumerationDomain_h
#define __vtkSMEnumerationDomain_h

#include "vtkSMDomain.h"

//BTX
struct vtkSMEnumerationDomainInternals;
//ETX

class VTK_EXPORT vtkSMEnumerationDomain : public vtkSMDomain
{
public:
  static vtkSMEnumerationDomain* New();
  vtkTypeRevisionMacro(vtkSMEnumerationDomain, vtkSMDomain);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns true if the value of the propery is in the domain.
  // The propery has to be a vtkSMIntVectorProperty. If all 
  // vector values are in the domain, it returns 1. It returns
  // 0 otherwise.
  virtual int IsInDomain(vtkSMProperty* property);

  // Description:
  // Returns true if the int is in the domain. If value is
  // in domain, it's index is return in idx.
  int IsInDomain(int val, unsigned int& idx);

  // Description:
  // Returns the number of entries in the enumeration.
  unsigned int GetNumberOfEntries();

  // Description:
  // Returns the integer value of an enumeration entry.
  int GetEntryValue(unsigned int idx);

  // Description:
  // Returns the descriptive string of an enumeration entry.
  const char* GetEntryText(unsigned int idx);

//BTX
  // Description:
  // Given an entry text, return the integer value.
  // Valid is set to 1 if text is defined, otherwise 0.
  // If valid=0, return value is undefined.
  int GetEntryValue(const char* text, int& valid);
//ETX

  // Description:
  // Add a new enumeration entry. text cannot be null.
  void AddEntry(const char* text, int value);

  // Description:
  // Clear all entries.
  void RemoveAllEntries();

  // Description:
  // Update self based on the "unchecked" values of all required
  // properties. Overwritten by sub-classes.
  virtual void Update(vtkSMProperty* property);
protected:
  vtkSMEnumerationDomain();
  ~vtkSMEnumerationDomain();

  // Description:
  // Set the appropriate ivars from the xml element. Should
  // be overwritten by subclass if adding ivars.
  virtual int ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element);

  virtual void ChildSaveState(vtkPVXMLElement* domainElement);

  vtkSMEnumerationDomainInternals* EInternals;

private:
  vtkSMEnumerationDomain(const vtkSMEnumerationDomain&); // Not implemented
  void operator=(const vtkSMEnumerationDomain&); // Not implemented
};

#endif
