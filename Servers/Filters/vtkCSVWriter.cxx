/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkCSVWriter.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCSVWriter.h"

#include "vtkAlgorithm.h"
#include "vtkArrayIteratorIncludes.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkPolyLineToRectilinearGridFilter.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkCSVWriter);
vtkCxxRevisionMacro(vtkCSVWriter, "$Revision: 1.5.10.1 $");
//-----------------------------------------------------------------------------
vtkCSVWriter::vtkCSVWriter()
{
  this->StringDelimiter = 0;
  this->FieldDelimiter = 0;
  this->UseStringDelimiter = true;
  this->SetStringDelimiter("\"");
  this->SetFieldDelimiter(",");
  this->Stream = 0;
  this->FileName = 0;
}

//-----------------------------------------------------------------------------
vtkCSVWriter::~vtkCSVWriter()
{
  this->SetStringDelimiter(0);
  this->SetFieldDelimiter(0);
  this->SetFileName(0);
  delete this->Stream;
}

//-----------------------------------------------------------------------------
int vtkCSVWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
bool vtkCSVWriter::OpenFile()
{
  if ( !this->FileName )
    {
    vtkErrorMacro(<< "No FileName specified! Can't write!");
    this->SetErrorCode(vtkErrorCode::NoFileNameError);
    return false;
    }

  vtkDebugMacro(<<"Opening file for writing...");

  ofstream *fptr = new ofstream(this->FileName, ios::out);

  if (fptr->fail())
    {
    vtkErrorMacro(<< "Unable to open file: "<< this->FileName);
    this->SetErrorCode(vtkErrorCode::CannotOpenFileError);
    delete fptr;
    return false;
    }

  this->Stream = fptr;
  return true;
}

//-----------------------------------------------------------------------------
template <class iterT>
void vtkCSVWriterGetDataString(
  iterT* iter, vtkIdType tupleIndex, ofstream* stream, vtkCSVWriter* writer)
{
  int numComps = iter->GetNumberOfComponents();
  vtkIdType index = tupleIndex* numComps;
  for (int cc=0; cc < numComps; cc++)
    {
    if ((index+cc) < iter->GetNumberOfValues())
      {
      (*stream) << writer->GetFieldDelimiter()
        << iter->GetValue(index+cc);
      }
    else
      {
      (*stream) << writer->GetFieldDelimiter();
      }
    }
}

//-----------------------------------------------------------------------------
VTK_TEMPLATE_SPECIALIZE
void vtkCSVWriterGetDataString(
  vtkArrayIteratorTemplate<vtkStdString>* iter, vtkIdType tupleIndex, 
  ofstream* stream, vtkCSVWriter* writer)
{
  int numComps = iter->GetNumberOfComponents();
  vtkIdType index = tupleIndex* numComps;
  for (int cc=0; cc < numComps; cc++)
    {
    if ((index+cc) < iter->GetNumberOfValues())
      {
      (*stream) << writer->GetFieldDelimiter()
        << writer->GetString(iter->GetValue(index+cc));
      }
    else
      {
      (*stream) << writer->GetFieldDelimiter();
      }
    }
}

//-----------------------------------------------------------------------------
vtkStdString vtkCSVWriter::GetString(vtkStdString string)
{
  if (this->UseStringDelimiter && this->StringDelimiter)
    {
    vtkStdString temp = this->StringDelimiter;
    temp += string + this->StringDelimiter; 
    return temp;
    }
  return string;
}

//-----------------------------------------------------------------------------
void vtkCSVWriter::WriteData()
{
  vtkRectilinearGrid* rg = vtkRectilinearGrid::SafeDownCast(this->GetInput());
  if (rg)
    {
    this->WriteRectilinearGridData(rg);
    }
  else
    {
    vtkDataSet* ds = vtkDataSet::SafeDownCast(this->GetInput());
    if (ds)
      {
      static const char* const pdInfoStr =
        "vtkCSVWriter: input data type needs to be of type vtkPolyData";
      vtkPointSet* ps = vtkPointSet::SafeDownCast(ds);
      if (ps)
        {
        vtkPolyData* pd = vtkPolyData::SafeDownCast(ps);
        if (pd)
          {
          vtkPolyData* clone = vtkPolyData::New();
          clone->ShallowCopy(pd);

          static const char* const infoStr =
            "input data type is a vtkPolyData."
            " Converting via vtkPolyLineToRectilinearGridFilter";
          vtkDebugMacro(<< infoStr);
          vtkPolyLineToRectilinearGridFilter* p2rgf =
            vtkPolyLineToRectilinearGridFilter::New();
          p2rgf->SetInput(clone);

          p2rgf->Update();
          this->WriteRectilinearGridData(p2rgf->GetOutput());

          p2rgf->Delete();
          clone->Delete();
          }
        else
          {
          vtkDebugMacro(<< pdInfoStr);
          vtkWarningMacro(<< pdInfoStr);
          }
        }
      else
        {
        vtkDebugMacro(<< pdInfoStr);
        vtkWarningMacro(<< pdInfoStr);
        }
      }
    else
      {
      vtkErrorMacro(<< "Bad Ju Ju! The input to vtkCSVWriter must be a vtkDataSet");
      }
    }
}

//-----------------------------------------------------------------------------
void vtkCSVWriter::WriteRectilinearGridData(vtkRectilinearGrid* rectilinearGrid)
{
  vtkIdType numPoints = rectilinearGrid->GetNumberOfPoints();
  vtkPointData* pd = rectilinearGrid->GetPointData();
  vtkCellData* cd = rectilinearGrid->GetCellData();
  int dims[3];
  int non_unit_dims = 0;
  int cc;
  rectilinearGrid->GetDimensions(dims);
  for (cc=0; cc < 3; cc++)
    {
    non_unit_dims += (dims[cc] > 1)?  1:  0;
    }
  if (non_unit_dims > 1)
    {
    vtkErrorMacro("The rectilinear grid must be 1D to be save as CSV.");
    return;
    }

  if (!this->OpenFile())
    {
    return;
    }

  // Write headers:
  (*this->Stream) << this->GetString("X::Point")
    << this->FieldDelimiter
    << this->GetString("Y::Point")
    << this->FieldDelimiter
    << this->GetString("Z::Point");

  vtkstd::vector<vtkSmartPointer<vtkArrayIterator> > pointIters;
  vtkstd::vector<vtkSmartPointer<vtkArrayIterator> > cellIters;

  int pdNumArrays = pd->GetNumberOfArrays();
  for (cc=0; cc < pdNumArrays; cc++)
    {
    vtkAbstractArray* array = pd->GetAbstractArray(cc);
    for (int comp=0; comp < array->GetNumberOfComponents(); comp++)
      {
      (*this->Stream) << this->FieldDelimiter
        << this->GetString(vtkStdString(array->GetName())+"::PointData");
      }
    vtkArrayIterator* iter = array->NewIterator();
    pointIters.push_back(iter);
    iter->Delete();
    }

  int cdNumArrays = cd->GetNumberOfArrays();
  for (cc=0; cc < cdNumArrays; cc++)
    {
    vtkAbstractArray* array = cd->GetAbstractArray(cc);
    for (int comp=0; comp < array->GetNumberOfComponents(); comp++)
      {
      (*this->Stream) << this->FieldDelimiter
        << this->GetString(vtkStdString(array->GetName())+"::CellData");
      }
    vtkArrayIterator* iter = array->NewIterator();
    cellIters.push_back(iter);
    iter->Delete();
    }
  (*this->Stream) << "\n";

  for (vtkIdType index=0; index < numPoints; index++)
    {
    double point[3];
    rectilinearGrid->GetPoint(index, point);
    (*this->Stream)
      << point[0] << this->FieldDelimiter
      << point[1] << this->FieldDelimiter
      << point[2];
    vtkstd::vector<vtkSmartPointer<vtkArrayIterator> >::iterator iter;
    for (iter = pointIters.begin(); iter != pointIters.end(); ++iter)
      {
      switch ((*iter)->GetDataType())
        {
        vtkArrayIteratorTemplateMacro(
          vtkCSVWriterGetDataString(static_cast<VTK_TT*>(iter->GetPointer()),
            index, this->Stream, this));
        }
      }

    for (iter = cellIters.begin(); iter != cellIters.end(); ++iter)
      {
      switch ((*iter)->GetDataType())
        {
        vtkArrayIteratorTemplateMacro(
          vtkCSVWriterGetDataString(static_cast<VTK_TT*>(iter->GetPointer()),
            index, this->Stream, this));
        }
      }

    (*this->Stream) << "\n";
    }

  this->Stream->close();
}

//-----------------------------------------------------------------------------
void vtkCSVWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldDelimiter: " << (this->FieldDelimiter ? 
    this->FieldDelimiter : "(none)") << endl;
  os << indent << "StringDelimiter: " << (this->StringDelimiter ?
    this->StringDelimiter : "(none)") << endl;
  os << indent << "UseStringDelimiter: " << this->UseStringDelimiter << endl;
  os << indent << "FileName: " << (this->FileName? this->FileName : "none") 
    << endl;
}
