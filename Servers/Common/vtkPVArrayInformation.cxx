/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVArrayInformation.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
cxx     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVArrayInformation.h"

#include "vtkClientServerStream.h"
#include "vtkDataArray.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVArrayInformation);
vtkCxxRevisionMacro(vtkPVArrayInformation, "$Revision: 1.9 $");

//----------------------------------------------------------------------------
vtkPVArrayInformation::vtkPVArrayInformation()
{
  this->Name = 0;
  this->Ranges = 0;
  this->Initialize();
}

//----------------------------------------------------------------------------
vtkPVArrayInformation::~vtkPVArrayInformation()
{
  this->Initialize();
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::Initialize()
{
  this->SetName(0);
  this->DataType = VTK_VOID;
  this->NumberOfComponents = 0;
  this->NumberOfTuples = 0;
  if(this->Ranges)
    {
    delete [] this->Ranges;
    this->Ranges = 0;
    }
  this->IsPartial = 0;
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  int num, idx;
  vtkIndent i2 = indent.GetNextIndent();

  this->Superclass::PrintSelf(os,indent);
  if (this->Name)
    {
    os << indent << "Name: " << this->Name << endl;
    }
  os << indent << "DataType: " << this->DataType << endl;
  os << indent << "NumberOfComponents: " << this->NumberOfComponents << endl;
  os << indent << "NumberOfTuples: " << this->NumberOfTuples << endl;
  os << indent << "IsPartial: " << this->IsPartial << endl;

  os << indent << "Ranges :" << endl;
  num = this->NumberOfComponents;
  if (num > 1)
    {
    ++num;
    }
  for (idx = 0; idx < num; ++idx)
    {
    os << i2 << this->Ranges[2*idx] << ", " << this->Ranges[2*idx+1] << endl;
    }
}


//----------------------------------------------------------------------------
void vtkPVArrayInformation::SetNumberOfComponents(int numComps)
{
  if (this->NumberOfComponents == numComps)
    {
    return;
    }
  if (this->Ranges)
    {
    delete [] this->Ranges;
    this->Ranges = NULL;
    }
  this->NumberOfComponents = numComps;
  if (numComps <= 0)
    {
    this->NumberOfComponents = 0;
    return;
    }
  if (numComps > 1)
    { // Extra range for vector magnitude (first in array).
    numComps = numComps + 1;
    }

  int idx;
  this->Ranges = new double[numComps*2];
  for (idx = 0; idx < numComps; ++idx)
    {
    this->Ranges[2*idx] = VTK_DOUBLE_MAX;
    this->Ranges[2*idx+1] = -VTK_DOUBLE_MAX;
    }
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::SetComponentRange(int comp, double min, double max)
{
  if (comp >= this->NumberOfComponents || this->NumberOfComponents <= 0)
    {
    vtkErrorMacro("Bad component");
    }
  if (this->NumberOfComponents > 1)
    { // Shift over vector mag range.
    ++comp;
    }
  if (comp < 0)
    { // anything less than 0 just defaults to the vector mag.
    comp = 0;
    }
  this->Ranges[comp*2] = min;
  this->Ranges[comp*2 + 1] = max;
}

//----------------------------------------------------------------------------
double* vtkPVArrayInformation::GetComponentRange(int comp)
{
  if (comp >= this->NumberOfComponents || this->NumberOfComponents <= 0)
    {
    vtkErrorMacro("Bad component");
    return NULL;
    }
  if (this->NumberOfComponents > 1)
    { // Shift over vector mag range.
    ++comp;
    }
  if (comp < 0)
    { // anything less than 0 just defaults to the vector mag.
    comp = 0;
    }
  return this->Ranges + comp*2;
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::GetComponentRange(int comp, double *range)
{
  double *ptr;

  ptr = this->GetComponentRange(comp);

  if (ptr == NULL)
    {
    range[0] = VTK_DOUBLE_MAX;
    range[1] = -VTK_DOUBLE_MAX;
    return;
    }

  range[0] = ptr[0];
  range[1] = ptr[1];
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::GetDataTypeRange(double range[2])
{
  int dataType = this->GetDataType();
  switch (dataType)
    {
    case VTK_BIT:
      range[0] = VTK_BIT_MAX;
      range[1] = VTK_BIT_MAX;
      break;
    case VTK_UNSIGNED_CHAR:
      range[0] = VTK_UNSIGNED_CHAR_MIN;
      range[1] = VTK_UNSIGNED_CHAR_MAX;
      break;
    case VTK_CHAR:
      range[0] = VTK_CHAR_MIN;
      range[1] = VTK_CHAR_MAX;
      break;
    case VTK_UNSIGNED_SHORT:
      range[0] = VTK_UNSIGNED_SHORT_MIN;
      range[1] = VTK_UNSIGNED_SHORT_MAX;
      break;
    case VTK_SHORT:
      range[0] = VTK_SHORT_MIN;
      range[1] = VTK_SHORT_MAX;
      break;
    case VTK_UNSIGNED_INT:
      range[0] = VTK_UNSIGNED_INT_MIN;
      range[1] = VTK_UNSIGNED_INT_MAX;
      break;
    case VTK_INT:
      range[0] = VTK_INT_MIN;
      range[1] = VTK_INT_MAX;
      break;
    case VTK_UNSIGNED_LONG:
      range[0] = VTK_UNSIGNED_LONG_MIN;
      range[1] = VTK_UNSIGNED_LONG_MAX;
      break;
    case VTK_LONG:
      range[0] = VTK_LONG_MIN;
      range[1] = VTK_LONG_MAX;
      break;
    case VTK_FLOAT:
      range[0] = VTK_FLOAT_MIN;
      range[1] = VTK_FLOAT_MAX;
      break;
    case VTK_DOUBLE:
      range[0] = VTK_DOUBLE_MIN;
      range[1] = VTK_DOUBLE_MAX;
      break;
    default:
      // Default value:
      range[0] = 0;
      range[1] = 1;
      break;
    }
}
//----------------------------------------------------------------------------
void vtkPVArrayInformation::AddRanges(vtkPVArrayInformation *info)
{
  double *range;
  double *ptr = this->Ranges;
  int idx;

  if (this->NumberOfComponents != info->GetNumberOfComponents())
    {
    vtkErrorMacro("Component mismatch.");
    }

  if (this->NumberOfComponents > 1)
    {
    range = info->GetComponentRange(-1);
    if (range[0] < ptr[0])
      {
      ptr[0] = range[0];
      }
    if (range[1] > ptr[1])
      {
      ptr[1] = range[1];
      }
    ptr += 2;
    }

  for (idx = 0; idx < this->NumberOfComponents; ++idx)
    {
    range = info->GetComponentRange(idx);
    if (range[0] < ptr[0])
      {
      ptr[0] = range[0];
      }
    if (range[1] > ptr[1])
      {
      ptr[1] = range[1];
      }
    ptr += 2;
    }

  this->NumberOfTuples += info->GetNumberOfTuples();
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::DeepCopy(vtkPVArrayInformation *info)
{
  int num, idx;

  this->SetName(info->GetName());
  this->DataType = info->GetDataType();
  this->SetNumberOfComponents(info->GetNumberOfComponents());
  this->SetNumberOfTuples(info->GetNumberOfTuples());

  num = 2*this->NumberOfComponents;
  if (this->NumberOfComponents > 1)
    {
    num += 2;
    }
  for (idx = 0; idx < num; ++idx)
    {
    this->Ranges[idx] = info->Ranges[idx];
    }
}

//----------------------------------------------------------------------------
int vtkPVArrayInformation::Compare(vtkPVArrayInformation *info)
{
  if (info == NULL)
    {
    return 0;
    }
  if (strcmp(info->GetName(), this->Name) == 0 &&
      info->GetNumberOfComponents() == this->NumberOfComponents)
    {
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::CopyFromObject(vtkObject* obj)
{
  if (!obj)
    {
    this->Initialize();
    }

  vtkAbstractArray* const array = vtkAbstractArray::SafeDownCast(obj);
  if(!array)
    {
    vtkErrorMacro("Cannot downcast to abstract array.");
    this->Initialize();
    return;
    }

  this->SetName(array->GetName());
  this->DataType = array->GetDataType();
  this->SetNumberOfComponents(array->GetNumberOfComponents());
  this->SetNumberOfTuples(array->GetNumberOfTuples());
  
  if(vtkDataArray* const data_array = vtkDataArray::SafeDownCast(obj))
    {
    double range[2];
    double *ptr;
    int idx;

    ptr = this->Ranges;
    if (this->NumberOfComponents > 1)
      {
      // First store range of vector magnitude.
      data_array->GetRange(range, -1);
      *ptr++ = range[0];
      *ptr++ = range[1];
      }
    for (idx = 0; idx < this->NumberOfComponents; ++idx)
      {
      data_array->GetRange(range, idx);
      *ptr++ = range[0];
      *ptr++ = range[1];
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::AddInformation(vtkPVInformation* info)
{
  if (!info)
    {
    return;
    }

  vtkPVArrayInformation* aInfo = vtkPVArrayInformation::SafeDownCast(info);
  if (!aInfo)
    {
    vtkErrorMacro("Could not downcast info to array info.");
    return;
    }
  if (aInfo->GetNumberOfComponents() > 0)
    {
    if (this->NumberOfComponents == 0)
      {
      // If this object is uninitialized, copy.
      this->DeepCopy(aInfo);
      }
    else
      {
      // Leave everything but ranges as original, add ranges.
      this->AddRanges(aInfo);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply;

  // Array name, data type, and number of components.
  *css << this->Name;
  *css << this->DataType;
  *css << this->NumberOfTuples;
  *css << this->NumberOfComponents;

  // Range of each component.
  int num = this->NumberOfComponents;
  if(this->NumberOfComponents > 1)
    {
    // First range is range of vector magnitude.
    ++num;
    }
  for(int i=0; i < num; ++i)
    {
    *css << vtkClientServerStream::InsertArray(this->Ranges + 2*i, 2);
    }

  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVArrayInformation::CopyFromStream(const vtkClientServerStream* css)
{
  // Array name.
  const char* name = 0;
  if(!css->GetArgument(0, 0, &name))
    {
    vtkErrorMacro("Error parsing array name from message.");
    return;
    }
  this->SetName(name);

  // Data type.
  if(!css->GetArgument(0, 1, &this->DataType))
    {
    vtkErrorMacro("Error parsing array data type from message.");
    return;
    }

  // Number of tuples.
  int num;
  if(!css->GetArgument(0, 2, &num))
    {
    vtkErrorMacro("Error parsing number of tuples from message.");
    return;
    }
  this->SetNumberOfTuples(num);

  // Number of components.
  if(!css->GetArgument(0, 3, &num))
    {
    vtkErrorMacro("Error parsing number of components from message.");
    return;
    }
  this->SetNumberOfComponents(num);

  if (num > 1)
    {
    num++;
    }

  // Range of each component.
  for(int i=0; i < num; ++i)
    {
    if(!css->GetArgument(0, 4+i, this->Ranges + 2*i, 2))
      {
      vtkErrorMacro("Error parsing range of component.");
      return;
      }
    }
}
