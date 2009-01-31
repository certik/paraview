/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVDataInformation.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDataInformation.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkByteSwap.h"
#include "vtkCellData.h"
#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkGenericDataSet.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkRectilinearGrid.h"
#include "vtkSelection.h"
#include "vtkSource.h"
#include "vtkStructuredGrid.h"
#include "vtkTable.h"
#include "vtkUniformGrid.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkPVDataInformation);
vtkCxxRevisionMacro(vtkPVDataInformation, "$Revision: 1.38 $");

//----------------------------------------------------------------------------
vtkPVDataInformation::vtkPVDataInformation()
{
  this->CompositeDataSetType = -1;
  this->DataSetType = -1;
  this->NumberOfPoints = 0;
  this->NumberOfCells = 0;
  this->NumberOfRows = 0;
  this->MemorySize = 0;
  this->PolygonCount = 0;
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;
  this->Extent[0] = this->Extent[2] = this->Extent[4] = VTK_LARGE_INTEGER;
  this->Extent[1] = this->Extent[3] = this->Extent[5] = -VTK_LARGE_INTEGER;
  this->PointDataInformation = vtkPVDataSetAttributesInformation::New();
  this->CellDataInformation = vtkPVDataSetAttributesInformation::New();
  this->FieldDataInformation = vtkPVDataSetAttributesInformation::New();

  this->CompositeDataInformation = vtkPVCompositeDataInformation::New();

  this->PointArrayInformation = vtkPVArrayInformation::New();
  
  this->Name = 0;
  this->DataClassName = 0;
  this->CompositeDataClassName = 0;
  this->NumberOfDataSets = 0;
  this->NameSetToDefault = 0;
}

//----------------------------------------------------------------------------
vtkPVDataInformation::~vtkPVDataInformation()
{
  this->PointDataInformation->Delete();
  this->PointDataInformation = NULL;
  this->CellDataInformation->Delete();
  this->CellDataInformation = NULL;
  this->FieldDataInformation->Delete();
  this->FieldDataInformation = NULL;
  this->CompositeDataInformation->Delete();
  this->CompositeDataInformation = NULL;
  this->PointArrayInformation->Delete();
  this->PointArrayInformation = NULL;
  
  this->SetName(0);
  this->SetDataClassName(0);
  this->SetCompositeDataClassName(0);
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkIndent i2 = indent.GetNextIndent();
  this->Superclass::PrintSelf(os,indent);

  os << indent << "DataSetType: " << this->DataSetType << endl;
  os << indent << "CompositeDataSetType: " << this->CompositeDataSetType << endl;
  os << indent << "NumberOfPoints: " << this->NumberOfPoints << endl;
  os << indent << "NumberOfRows: " << this->NumberOfRows << endl;
  os << indent << "NumberOfCells: " << this->NumberOfCells << endl;
  os << indent << "NumberOfDataSets: " << this->NumberOfDataSets  << endl;
  os << indent << "MemorySize: " << this->MemorySize << endl;
  os << indent << "PolygonCount: " << this->PolygonCount << endl;
  os << indent << "Bounds: " << this->Bounds[0] << ", " << this->Bounds[1]
     << ", " << this->Bounds[2] << ", " << this->Bounds[3]
     << ", " << this->Bounds[4] << ", " << this->Bounds[5] << endl;
  os << indent << "Extent: " << this->Extent[0] << ", " << this->Extent[1]
     << ", " << this->Extent[2] << ", " << this->Extent[3]
     << ", " << this->Extent[4] << ", " << this->Extent[5] << endl;
  os << indent << "PointDataInformation " << endl;
  this->PointDataInformation->PrintSelf(os, i2);
  os << indent << "CellDataInformation " << endl;
  this->CellDataInformation->PrintSelf(os, i2);
  os << indent << "FieldDataInformation " << endl;
  this->FieldDataInformation->PrintSelf(os, i2);
  os << indent << "CompositeDataInformation " << endl;
  this->CompositeDataInformation->PrintSelf(os, i2);
  os << indent << "PointArrayInformation " << endl;
  this->PointArrayInformation->PrintSelf(os, i2);

  if (this->Name)
    {
    os << indent << "Name: " << this->Name << endl;
    }
  else
    {
    os << indent << "Name: NULL\n";
    }

  os << indent << "DataClassName: " 
     << (this->DataClassName?this->DataClassName:"(none)") << endl;
  os << indent << "CompositeDataClassName: " 
     << (this->CompositeDataClassName?this->CompositeDataClassName:"(none)") << endl;
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::Initialize()
{
  this->DataSetType = -1;
  this->CompositeDataSetType = -1;
  this->NumberOfPoints = 0;
  this->NumberOfCells = 0;
  this->NumberOfRows = 0;
  this->NumberOfDataSets = 0;
  this->MemorySize = 0;
  this->PolygonCount = 0;
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_DOUBLE_MAX;
  this->Extent[0] = this->Extent[2] = this->Extent[4] = VTK_LARGE_INTEGER;
  this->Extent[1] = this->Extent[3] = this->Extent[5] = -VTK_LARGE_INTEGER;
  this->PointDataInformation->Initialize();
  this->CellDataInformation->Initialize();
  this->FieldDataInformation->Initialize();
  this->CompositeDataInformation->Initialize();
  this->PointArrayInformation->Initialize();
  
  this->SetName(0);
  this->SetDataClassName(0);
  this->SetCompositeDataClassName(0);
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::DeepCopy(vtkPVDataInformation *dataInfo)
{
  int idx;
  double *bounds;
  int *ext;

  this->DataSetType = dataInfo->GetDataSetType();
  this->CompositeDataSetType = dataInfo->GetCompositeDataSetType();
  this->SetDataClassName(dataInfo->GetDataClassName());
  this->SetCompositeDataClassName(dataInfo->GetCompositeDataClassName());

  this->NumberOfDataSets = dataInfo->NumberOfDataSets;

  this->NumberOfPoints = dataInfo->GetNumberOfPoints();
  this->NumberOfCells = dataInfo->GetNumberOfCells();
  this->NumberOfRows = dataInfo->GetNumberOfRows();
  this->MemorySize = dataInfo->GetMemorySize();
  this->PolygonCount = dataInfo->GetPolygonCount();

  bounds = dataInfo->GetBounds();
  for (idx = 0; idx < 6; ++idx)
    {
    this->Bounds[idx] = bounds[idx];
    }
  ext = dataInfo->GetExtent();
  for (idx = 0; idx < 6; ++idx)
    {
    this->Extent[idx] = ext[idx];
    }

  // Copy attribute information.
  this->PointDataInformation->DeepCopy(dataInfo->GetPointDataInformation());
  this->CellDataInformation->DeepCopy(dataInfo->GetCellDataInformation());
  this->FieldDataInformation->DeepCopy(dataInfo->GetFieldDataInformation());
  this->CompositeDataInformation->AddInformation(
    dataInfo->GetCompositeDataInformation());
  this->PointArrayInformation->AddInformation(
    dataInfo->GetPointArrayInformation());

  this->SetName(dataInfo->GetName());
}

//----------------------------------------------------------------------------
int vtkPVDataInformation::AddFromCompositeDataSet(vtkCompositeDataSet* data)
{
  int numDataSets = 0;
  vtkCompositeDataIterator* iter = data->NewIterator();
  iter->GoToFirstItem();
  while (!iter->IsDoneWithTraversal())
    {
    numDataSets++;
    vtkDataObject* dobj = iter->GetCurrentDataObject();
    vtkPVDataInformation* dinf = vtkPVDataInformation::New();
    dinf->CopyFromObject(dobj);
    dinf->SetDataClassName(dobj->GetClassName());
    dinf->DataSetType = dobj->GetDataObjectType();
    this->AddInformation(dinf, 1);
    dinf->Delete();

    iter->GoToNextItem();
    }
  iter->Delete();

  return numDataSets;
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromCompositeDataSet(vtkCompositeDataSet* data,
                                                    int recurse)
{
  this->Initialize();

  int numDataSets = this->AddFromCompositeDataSet(data);

  if (recurse)
    {
    this->CompositeDataInformation->CopyFromObject(data);
    }
  this->SetCompositeDataClassName(data->GetClassName());
  this->CompositeDataSetType = data->GetDataObjectType();
  this->NumberOfDataSets = numDataSets;
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromDataSet(vtkDataSet* data)
{
  int idx;
  double *bds;
  int *ext = NULL;

  this->SetDataClassName(data->GetClassName());
  this->DataSetType = data->GetDataObjectType();

  this->NumberOfDataSets = 1;

  // Look for a name stored in Field Data.
  vtkDataArray *nameArray = data->GetFieldData()->GetArray("Name");
  if (nameArray)
    {
    char* str = static_cast<char*>(nameArray->GetVoidPointer(0));
    this->SetName(str);
    }


  switch ( this->DataSetType )
    {
    case VTK_IMAGE_DATA:
      ext = static_cast<vtkImageData*>(data)->GetExtent();
      break;
    case VTK_STRUCTURED_GRID:
      ext = static_cast<vtkStructuredGrid*>(data)->GetExtent();
      break;
    case VTK_RECTILINEAR_GRID:
      ext = static_cast<vtkRectilinearGrid*>(data)->GetExtent();
      break;
    case VTK_UNIFORM_GRID:
      ext = static_cast<vtkUniformGrid*>(data)->GetExtent();
      break;
    case VTK_UNSTRUCTURED_GRID:
    case VTK_POLY_DATA:
      this->PolygonCount = data->GetNumberOfCells();
      break;
    }
  if (ext)
    {
    for (idx = 0; idx < 6; ++idx)
      {
      this->Extent[idx] = ext[idx];
      }
    }

  this->NumberOfPoints = data->GetNumberOfPoints();
  if (!this->NumberOfPoints)
    {
    return;
    }

  // We do not want to get the number of dual cells from an octree
  // because this triggers generation of connectivity arrays.
  if (data->GetDataObjectType() != VTK_HYPER_OCTREE)
    {
    this->NumberOfCells = data->GetNumberOfCells();
    }
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  ofstream *tmpFile = pm->GetLogFile();
  if (tmpFile)
    {
    if (data->GetSource())
      {
      *tmpFile << "output of " << data->GetSource()->GetClassName()
               << " contains\n";
      }
    else if (data->GetProducerPort())
      {
      *tmpFile << "output of "
               << data->GetProducerPort()->GetProducer()->GetClassName()
               << " contains\n";
      }
    *tmpFile << "\t" << this->NumberOfPoints << " points" << endl;
    *tmpFile << "\t" << this->NumberOfCells << " cells" << endl;
    }
  
  bds = data->GetBounds();
  for (idx = 0; idx < 6; ++idx)
    {
    this->Bounds[idx] = bds[idx];
    }
  this->MemorySize = data->GetActualMemorySize();

  vtkPointSet* ps = vtkPointSet::SafeDownCast(data);
  if (ps && ps->GetPoints())
    {
    this->PointArrayInformation->CopyFromObject(ps->GetPoints()->GetData());
    }

  // Copy Point Data information
  this->PointDataInformation->CopyFromDataSetAttributes(data->GetPointData());

  // Copy Cell Data information
  this->CellDataInformation->CopyFromDataSetAttributes(data->GetCellData());

  // Copy Field Data information, if any
  vtkFieldData *fd = data->GetFieldData();
  if(fd && fd->GetNumberOfArrays()>0)
    {
    this->FieldDataInformation->CopyFromFieldData(fd);
    }
}
//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromGenericDataSet(vtkGenericDataSet *data)
{
  int idx;
  double *bds;

  this->SetDataClassName(data->GetClassName());
  this->DataSetType = data->GetDataObjectType();

  this->NumberOfDataSets = 1;

  // Look for a name stored in Field Data.
  vtkDataArray *nameArray = data->GetFieldData()->GetArray("Name");
  if (nameArray)
    {
    char* str = static_cast<char*>(nameArray->GetVoidPointer(0));
    this->SetName(str);
    }

  this->NumberOfPoints = data->GetNumberOfPoints();
  if (!this->NumberOfPoints)
    {
    return;
    }
  // We do not want to get the number of dual cells from an octree
  // because this triggers generation of connectivity arrays.
  if (data->GetDataObjectType() != VTK_HYPER_OCTREE)
    {
    this->NumberOfCells = data->GetNumberOfCells();
    }
  bds = data->GetBounds();
  for (idx = 0; idx < 6; ++idx)
    {
    this->Bounds[idx] = bds[idx];
    }
  this->MemorySize = data->GetActualMemorySize();
  switch ( this->DataSetType )
    {
    case VTK_POLY_DATA:
      this->PolygonCount = data->GetNumberOfCells();
      break;
    }

  // Copy Point Data information
  this->PointDataInformation->CopyFromGenericAttributesOnPoints(
    data->GetAttributes());

  // Copy Cell Data information
  this->CellDataInformation->CopyFromGenericAttributesOnCells(
    data->GetAttributes());
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromSelection(vtkSelection* data)
{
  this->SetDataClassName(data->GetClassName());
  this->DataSetType = data->GetDataObjectType();
  this->NumberOfDataSets = 1;
  this->Bounds[0] = this->Bounds[1] = this->Bounds[2] 
    = this->Bounds[3] = this->Bounds[4] = this->Bounds[5] = 0;

  this->MemorySize = data->GetActualMemorySize();
  this->NumberOfCells = 0;
  this->NumberOfPoints = 0;

  // Copy Point Data information
  this->PointDataInformation->CopyFromFieldData(data->GetFieldData());
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromTable(vtkTable* data)
{
  this->SetDataClassName(data->GetClassName());
  this->DataSetType = data->GetDataObjectType();
  this->NumberOfDataSets = 1;
  this->Bounds[0] = this->Bounds[1] = this->Bounds[2] 
    = this->Bounds[3] = this->Bounds[4] = this->Bounds[5] = 0;

  this->MemorySize = data->GetActualMemorySize();
  this->NumberOfCells = data->GetNumberOfRows() * data->GetNumberOfColumns();
  this->NumberOfPoints = 0;
  this->NumberOfRows = data->GetNumberOfRows();

  // Copy Point Data information
  this->PointDataInformation->CopyFromFieldData(data->GetFieldData());
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromObject(vtkObject* object)
{
  vtkDataObject* dobj = vtkDataObject::SafeDownCast(object);

  // Handle the case where the a vtkAlgorithmOutput is passed instead of
  // the data object. vtkSMPart uses vtkAlgorithmOutput.
  if (!dobj)
    {
    vtkAlgorithmOutput* algOutput = vtkAlgorithmOutput::SafeDownCast(object);
    if (algOutput && algOutput->GetProducer())
      {
      dobj = algOutput->GetProducer()->GetOutputDataObject(
        algOutput->GetIndex());
      }
    }

  if (!dobj)
    {
    vtkErrorMacro("Could not cast object to a known data set: " 
                  << (object?object->GetClassName():"(null"));
    return;
    }

  vtkCompositeDataSet* cds = vtkCompositeDataSet::SafeDownCast(dobj);
  if (cds)
    {
    this->CopyFromCompositeDataSet(cds);
    return;
    }

  vtkDataSet* ds = vtkDataSet::SafeDownCast(dobj);
  if (ds)
    {
    this->CopyFromDataSet(ds);
    return;
    }

  vtkGenericDataSet* ads = vtkGenericDataSet::SafeDownCast(dobj);
  if (ads)
    {
    this->CopyFromGenericDataSet(ads);
    return;
    }

  vtkTable* table = vtkTable::SafeDownCast(dobj);
  if (table)
    {
    this->CopyFromTable(table);
    return;
    }

  vtkSelection* selection = vtkSelection::SafeDownCast(dobj);
  if (selection)
    {
    this->CopyFromSelection(selection);
    return;
    }

  vtkErrorMacro("Could not cast object to a known data set: " 
                << (dobj?dobj->GetClassName():"(null"));
}
//----------------------------------------------------------------------------
void vtkPVDataInformation::AddInformation(vtkPVInformation* pvi)
{
  this->AddInformation(pvi, 0);
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::AddInformation(
  vtkPVInformation* pvi, int addingParts)
{
  vtkPVDataInformation *info;
  int             i,j;
  double*         bounds;
  int*            ext;

  info = vtkPVDataInformation::SafeDownCast(pvi);
  if (info == NULL)
    {
    vtkErrorMacro("Cound not cast object to data information.");
    return;
    }

  this->SetCompositeDataClassName(info->GetCompositeDataClassName());
  this->CompositeDataSetType = info->CompositeDataSetType;

  this->CompositeDataInformation->AddInformation(
    info->CompositeDataInformation);

  if (info->NumberOfDataSets == 0)
    {
    return;
    }

  if (this->NumberOfPoints == 0 && 
      this->NumberOfCells == 0 && 
      this->NumberOfDataSets == 0)
    { // Just copy the other array information.
    this->DeepCopy(info);
    return;
    }

  // For data set, lets pick the common super class.
  // This supports Heterogeneous collections.
  // We need a new classification: Structured.
  // This would allow extracting grid from mixed structured collections.
  if (this->DataSetType != info->GetDataSetType())
    { // IsTypeOf method will not work here.  Must be done manually.
    if (this->DataSetType == VTK_IMAGE_DATA ||
        this->DataSetType == VTK_RECTILINEAR_GRID ||
        this->DataSetType == VTK_DATA_SET ||
        info->GetDataSetType() == VTK_IMAGE_DATA ||
        info->GetDataSetType() == VTK_RECTILINEAR_GRID ||
        info->GetDataSetType() == VTK_DATA_SET)
      {
      this->DataSetType = VTK_DATA_SET;
      this->SetDataClassName("vtkDataSet");
      }
    else
      {
      if(this->DataSetType == VTK_GENERIC_DATA_SET ||
         info->GetDataSetType() == VTK_GENERIC_DATA_SET )
        {
        this->DataSetType = VTK_GENERIC_DATA_SET;
        this->SetDataClassName("vtkGenericDataSet");
        }
      else
        {
        this->DataSetType = VTK_POINT_SET;
        this->SetDataClassName("vtkPointSet");
        }
      }
    }

  // Empty data set? Ignore bounds, extent and array info.
  if (info->GetNumberOfCells() == 0 && info->GetNumberOfPoints() == 0)
    {
    return;
    }

  // First the easy stuff.
  this->NumberOfPoints += info->GetNumberOfPoints();
  this->NumberOfCells += info->GetNumberOfCells();
  this->MemorySize += info->GetMemorySize();
  this->NumberOfRows += info->GetNumberOfRows();
  
  switch ( this->DataSetType )
    {
    case VTK_POLY_DATA:
      this->PolygonCount += info->GetNumberOfCells();
      break;
    }
  if (addingParts)
    {
    // Adding data information of parts
    this->NumberOfDataSets += info->GetNumberOfDataSets();
    }
  else 
    {
    // Adding data information of 1 part across processors
    if (this->GetCompositeDataClassName())
      {
      // Composite data blocks are not distributed across processors.
      // Simply add their number.
      this->NumberOfDataSets += info->GetNumberOfDataSets();
      }
    else
      {
      // Simple data blocks are distributed across processors, use
      // the largest number (actually, NumberOfDataSets should always
      // be 1 since the data information is for a part)
      if (this->NumberOfDataSets < info->GetNumberOfDataSets())
        {
        this->NumberOfDataSets = info->GetNumberOfDataSets();
        }
      }
    }

  // Bounds are only a little harder.
  bounds = info->GetBounds();
  ext = info->GetExtent();
  for (i = 0; i < 3; ++i)
    {
    j = i*2;
    if (bounds[j] < this->Bounds[j])
      {
      this->Bounds[j] = bounds[j];
      }
    if (ext[j] < this->Extent[j])
      {
      this->Extent[j] = ext[j];
      }
    ++j;
    if (bounds[j] > this->Bounds[j])
      {
      this->Bounds[j] = bounds[j];
      }
    if (ext[j] > this->Extent[j])
      {
      this->Extent[j] = ext[j];
      }
    }


  // Now for the messy part, all of the arrays.
  this->PointArrayInformation->AddInformation(info->GetPointArrayInformation());
  this->PointDataInformation->AddInformation(info->GetPointDataInformation());
  this->CellDataInformation->AddInformation(info->GetCellDataInformation());
  this->FieldDataInformation->AddInformation(info->GetFieldDataInformation());
//  this->GenericAttributesInformation->AddInformation(info->GetGenericAttributesInformation());

  if (this->Name == NULL)
    {
    this->SetName(info->GetName());
    }
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::SetName(const char* name)
{
  if (!this->Name && !name) { return;}
  if ( this->Name && name && (!strcmp(this->Name, name))) 
    { 
    return;
    }
  if (this->Name) { delete [] this->Name; }
  if (name) 
    { 
    this->Name = new char[strlen(name)+1];
    strcpy(this->Name, name);
    } 
  else 
    { 
    this->Name = 0; 
    } 
  this->Modified(); 
  this->NameSetToDefault = 0;
}

//----------------------------------------------------------------------------
const char* vtkPVDataInformation::GetName()
{
  if (!this->Name || this->Name[0] == '\0' || this->NameSetToDefault)
    {
    if (this->Name)
      {
      delete[] this->Name;
      }
    char str[256];
    if (this->GetDataSetType() == VTK_POLY_DATA)
      {
      long nc = this->GetNumberOfCells();
      sprintf(str, "Polygonal: %ld cells", nc);
      }
    else if (this->GetDataSetType() == VTK_UNSTRUCTURED_GRID)
      {
      long nc = this->GetNumberOfCells();
      sprintf(str, "Unstructured Grid: %ld cells", nc);
      }
    else if (this->GetDataSetType() == VTK_IMAGE_DATA)
      {
      int *ext = this->GetExtent();
      if (ext)
        {
        sprintf(str, "Uniform Rectilinear: extent (%d, %d) (%d, %d) (%d, %d)", 
                ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
        }
      else
        {
        sprintf(str, "Uniform Rectilinear: invalid extent");
        }
      }
    else if (this->GetDataSetType() == VTK_UNIFORM_GRID)
      {
      int *ext = this->GetExtent();
      sprintf(str, 
              "Uniform Rectilinear (with blanking): "
              "extent (%d, %d) (%d, %d) (%d, %d)", 
              ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
      }
    else if (this->GetDataSetType() == VTK_RECTILINEAR_GRID)
      {
      int *ext = this->GetExtent();
      if (ext)
        {
        sprintf(str, 
                "Nonuniform Rectilinear: extent (%d, %d) (%d, %d) (%d, %d)", 
                ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
        }
      else
        {
        sprintf(str, "Nonuniform Rectilinear: invalid extent");
        }
      }
    else if (this->GetDataSetType() == VTK_STRUCTURED_GRID)
      {
      int *ext = this->GetExtent();
      if (ext)
        {
        sprintf(str, "Curvilinear: extent (%d, %d) (%d, %d) (%d, %d)", 
                ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
        }
      else
        {
        sprintf(str, "Curvilinear: invalid extent");
        }
      }
    else
      {
      sprintf(str, "Part of unknown type");
      }
    this->Name = new char[256];
    strncpy(this->Name, str, 256);
    this->NameSetToDefault = 1;
    }
  return this->Name;
}


//----------------------------------------------------------------------------
const char* vtkPVDataInformation::GetPrettyDataTypeString()
{
  int dataType = this->DataSetType;
  if (this->CompositeDataSetType >= 0)
    {
    dataType = this->CompositeDataSetType;
    }

  switch(dataType)
    {
    case VTK_POLY_DATA:
      return "Polygonal Mesh";
    case VTK_STRUCTURED_POINTS:
      return "Image (Uniform Rectilinear Grid)";
    case VTK_STRUCTURED_GRID:
      return "Structured (Curvilinear) Grid";
    case VTK_RECTILINEAR_GRID:
      return "Rectilinear Grid";
    case VTK_UNSTRUCTURED_GRID:
      return "Unstructured Grid";
    case VTK_PIECEWISE_FUNCTION:
      return "Piecewise function";
    case VTK_IMAGE_DATA:
      return "Image (Uniform Rectilinear Grid)";
    case VTK_DATA_OBJECT:
      return "Data Object";
    case VTK_DATA_SET:
      return "Data Set";
    case VTK_POINT_SET:
      return "Point Set";
    case VTK_UNIFORM_GRID:
      return "Image (Uniform Rectilinear Grid) with blanking";
    case VTK_COMPOSITE_DATA_SET:
      return "Composite Dataset";
    case VTK_MULTIGROUP_DATA_SET:
      return "Multi-group Dataset";
    case VTK_MULTIBLOCK_DATA_SET:
      return "Multi-block Dataset";
    case VTK_HIERARCHICAL_DATA_SET:
      return "Hierarchical DataSet";
    case VTK_HIERARCHICAL_BOX_DATA_SET:
      return "AMR Dataset";
    case VTK_GENERIC_DATA_SET:
      return "Generic Dataset";
    case VTK_HYPER_OCTREE:
      return "Hyper-octree";
    case VTK_TEMPORAL_DATA_SET:
      return "Temporal Dataset";
    case VTK_TABLE:
      return "Table";
    case VTK_GRAPH:
      return "Graph";
    case VTK_TREE:
      return "Tree";
    }
  
  return "UnknownType";
}

//----------------------------------------------------------------------------
const char* vtkPVDataInformation::GetDataSetTypeAsString()
{
  switch(this->DataSetType)
    {
    case VTK_POLY_DATA:
      return "vtkPolyData";
    case VTK_STRUCTURED_POINTS:
      return "vtkStructuredPoints";
    case VTK_STRUCTURED_GRID:
      return "vtkStructuredGrid";
    case VTK_RECTILINEAR_GRID:
      return "vtkRectilinearGrid";
    case VTK_UNSTRUCTURED_GRID:
      return "vtkUnstructuredGrid";
    case VTK_PIECEWISE_FUNCTION:
      return "vtkPiecewiseFunction";
    case VTK_IMAGE_DATA:
      return "vtkImageData";
    case VTK_DATA_OBJECT:
      return "vtkDataObject";
    case VTK_DATA_SET:
      return "vtkDataSet";
    case VTK_POINT_SET:
      return "vtkPointSet";
    case VTK_UNIFORM_GRID:
      return "vtkUniformGrid";
    case VTK_COMPOSITE_DATA_SET:
      return "vtkCompositeDataSet";
    case VTK_MULTIGROUP_DATA_SET:
      return "vtkMultigroupDataSet";
    case VTK_MULTIBLOCK_DATA_SET:
      return "vtkMultiblockDataSet";
    case VTK_HIERARCHICAL_DATA_SET:
      return "vtkHierarchicalDataSet";
    case VTK_HIERARCHICAL_BOX_DATA_SET:
      return "vtkHierarchicalBoxDataSet";
    case VTK_GENERIC_DATA_SET:
      return "vtkGenericDataSet";
    case VTK_HYPER_OCTREE:
      return "vtkHyperOctree";
    case VTK_TEMPORAL_DATA_SET:
      return "vtkTemporalDataSet";
    case VTK_TABLE:
      return "vtkTable";
    case VTK_GRAPH:
      return "vtkGraph";
    case VTK_TREE:
      return "vtkTree";
    }
  
  return "UnknownType";
}

//----------------------------------------------------------------------------
// Need to do this manually.
int vtkPVDataInformation::DataSetTypeIsA(const char* type)
{
  if (strcmp(type, "vtkDataObject") == 0)
    { // Every type is of type vtkDataObject.
    return 1;
    }
  if (strcmp(type, "vtkDataSet") == 0)
    { // Every type is of type vtkDataObject.
    if (this->DataSetType == VTK_POLY_DATA ||
        this->DataSetType == VTK_STRUCTURED_GRID ||
        this->DataSetType == VTK_UNSTRUCTURED_GRID ||
        this->DataSetType == VTK_IMAGE_DATA ||
        this->DataSetType == VTK_RECTILINEAR_GRID ||
        this->DataSetType == VTK_UNSTRUCTURED_GRID ||
        this->DataSetType == VTK_STRUCTURED_POINTS)
      {
      return 1;
      }
    }
  if (strcmp(type, this->GetDataSetTypeAsString()) == 0)
    { // If class names are the same, then they are of the same type.
    return 1;
    }
  if (strcmp(type, "vtkPointSet") == 0)
    {
    if (this->DataSetType == VTK_POLY_DATA ||
        this->DataSetType == VTK_STRUCTURED_GRID ||
        this->DataSetType == VTK_UNSTRUCTURED_GRID)
      {
      return 1;
      }
    }
  if (strcmp(type, "vtkStructuredData") == 0)
    {
    if (this->DataSetType == VTK_IMAGE_DATA ||
        this->DataSetType == VTK_STRUCTURED_GRID ||
        this->DataSetType == VTK_RECTILINEAR_GRID)
      {
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyToStream(vtkClientServerStream* css)
{
  css->Reset();
  *css << vtkClientServerStream::Reply;
  *css << this->Name
       << this->DataClassName
       << this->DataSetType
       << this->NumberOfDataSets
       << this->NumberOfPoints
       << this->NumberOfCells
       << this->NumberOfRows
       << this->MemorySize
       << this->PolygonCount
       << vtkClientServerStream::InsertArray(this->Bounds, 6)
       << vtkClientServerStream::InsertArray(this->Extent, 6);

  size_t length;
  const unsigned char* data;
  vtkClientServerStream dcss;

  this->PointArrayInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, length);

  dcss.Reset();

  this->PointDataInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, length);

  dcss.Reset();

  this->CellDataInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, length);

  *css << this->CompositeDataClassName;
  *css << this->CompositeDataSetType;

  dcss.Reset();

  this->CompositeDataInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, length);

  dcss.Reset();

  this->FieldDataInformation->CopyToStream(&dcss);
  dcss.GetData(&data, &length);
  *css << vtkClientServerStream::InsertArray(data, length);

  *css << vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
void vtkPVDataInformation::CopyFromStream(const vtkClientServerStream* css)
{
  const char* name = 0;
  if(!css->GetArgument(0, 0, &name))
    {
    vtkErrorMacro("Error parsing name of data.");
    return;
    }
  this->SetName(name);
  const char* dataclassname = 0;
  if(!css->GetArgument(0, 1, &dataclassname))
    {
    vtkErrorMacro("Error parsing class name of data.");
    return;
    }
  this->SetDataClassName(dataclassname);
  if(!css->GetArgument(0, 2, &this->DataSetType))
    {
    vtkErrorMacro("Error parsing data set type.");
    return;
    }
  if(!css->GetArgument(0, 3, &this->NumberOfDataSets))
    {
    vtkErrorMacro("Error parsing number of datasets.");
    return;
    }
  if(!css->GetArgument(0, 4, &this->NumberOfPoints))
    {
    vtkErrorMacro("Error parsing number of points.");
    return;
    }
  if(!css->GetArgument(0, 5, &this->NumberOfCells))
    {
    vtkErrorMacro("Error parsing number of cells.");
    return;
    }
  if(!css->GetArgument(0, 6, &this->NumberOfRows))
    {
    vtkErrorMacro("Error parsing number of cells.");
    return;
    }
  if(!css->GetArgument(0, 7, &this->MemorySize))
    {
    vtkErrorMacro("Error parsing memory size.");
    return;
    }
  if(!css->GetArgument(0, 8, &this->PolygonCount))
    {
    vtkErrorMacro("Error parsing memory size.");
    return;
    }
  if(!css->GetArgument(0, 9, this->Bounds, 6))
    {
    vtkErrorMacro("Error parsing bounds.");
    return;
    }
  if(!css->GetArgument(0, 10, this->Extent, 6))
    {
    vtkErrorMacro("Error parsing extent.");
    return;
    }

  vtkTypeUInt32 length;
  vtkstd::vector<unsigned char> data;
  vtkClientServerStream dcss;

  // Point array information.
  if(!css->GetArgumentLength(0, 11, &length))
    {
    vtkErrorMacro("Error parsing length of point data information.");
    return;
    }
  data.resize(length);
  if(!css->GetArgument(0, 11, &*data.begin(), length))
    {
    vtkErrorMacro("Error parsing point data information.");
    return;
    }
  dcss.SetData(&*data.begin(), length);
  this->PointArrayInformation->CopyFromStream(&dcss);

  // Point data array information.
  if(!css->GetArgumentLength(0, 12, &length))
    {
    vtkErrorMacro("Error parsing length of point data information.");
    return;
    }
  data.resize(length);
  if(!css->GetArgument(0, 12, &*data.begin(), length))
    {
    vtkErrorMacro("Error parsing point data information.");
    return;
    }
  dcss.SetData(&*data.begin(), length);
  this->PointDataInformation->CopyFromStream(&dcss);

  // Cell data array information.
  if(!css->GetArgumentLength(0, 13, &length))
    {
    vtkErrorMacro("Error parsing length of cell data information.");
    return;
    }
  data.resize(length);
  if(!css->GetArgument(0, 13, &*data.begin(), length))
    {
    vtkErrorMacro("Error parsing cell data information.");
    return;
    }
  dcss.SetData(&*data.begin(), length);
  this->CellDataInformation->CopyFromStream(&dcss);

  const char* compositedataclassname = 0;
  if(!css->GetArgument(0, 14, &compositedataclassname))
    {
    vtkErrorMacro("Error parsing class name of data.");
    return;
    }
  this->SetCompositeDataClassName(compositedataclassname);

  if(!css->GetArgument(0, 15, &this->CompositeDataSetType))
    {
    vtkErrorMacro("Error parsing data set type.");
    return;
    }

  // Composite data information.
  if(!css->GetArgumentLength(0, 16, &length))
    {
    vtkErrorMacro("Error parsing length of cell data information.");
    return;
    }
  data.resize(length);
  if(!css->GetArgument(0, 16, &*data.begin(), length))
    {
    vtkErrorMacro("Error parsing cell data information.");
    return;
    }
  dcss.SetData(&*data.begin(), length);
  if (dcss.GetNumberOfMessages() > 0)
    {
    this->CompositeDataInformation->CopyFromStream(&dcss);
    }
  else
    {
    this->CompositeDataInformation->Initialize();
    }

  // Field data array information.
  if(!css->GetArgumentLength(0, 17, &length))
    {
    vtkErrorMacro("Error parsing length of field data information.");
    return;
    }

  data.resize(length);
  if(!css->GetArgument(0, 17, &*data.begin(), length))
    {
    vtkErrorMacro("Error parsing field data information.");
    return;
    }
  dcss.SetData(&*data.begin(), length);
  this->FieldDataInformation->CopyFromStream(&dcss);
}
