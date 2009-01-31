/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkTable.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkTable - A table, which contains similar-typed columns of data
//
// .SECTION Description
// vtkTable is a basic data structure for storing columns of data.
// Internally, columns are stored in a vtkFieldData structure.
// However, using the vtkTable API additionally ensures that every column
// has the same number of entries, and provides row access (using vtkVariantArray)
// and single entry access (using vtkVariant).
//
// .SECTION Caveats
// You should use the vtkTable API to change the table data.  Performing
// vtkFieldData operations on the object returned by GetFieldData() may
// yield unexpected results.  vtkTable does allow the user to set the field
// data using SetFieldData(); the number of rows in the table is determined
// by the number of tuples in the first array (it is assumed that all arrays
// are the same length).
//
// .SECTION Thanks
// Thanks to Patricia Crossno, Ken Moreland, Andrew Wilson and Brian Wylie from
// Sandia National Laboratories for their help in developing this class API.

#ifndef __vtkTable_h
#define __vtkTable_h

#include "vtkDataObject.h"

class vtkAbstractArray;
class vtkVariant;
class vtkVariantArray;

class VTK_FILTERING_EXPORT vtkTable : public vtkDataObject
{
public:
  static vtkTable* New();
  vtkTypeRevisionMacro(vtkTable, vtkDataObject);
  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Return what type of dataset this is.
  int GetDataObjectType() {return VTK_TABLE;}

  // Description:
  // Sets the field data for the table.
  virtual void SetFieldData(vtkFieldData* data);

  //
  // Row functions
  //

  // Description:
  // Get the number of rows in the table.
  vtkIdType GetNumberOfRows();

  // Description:
  // Get a row of the table as a vtkVariantArray which has one entry for each column.
  vtkVariantArray* GetRow(vtkIdType row);

  // Description:
  // Set a row of the table with a vtkVariantArray which has one entry for each column.
  void SetRow(vtkIdType row, vtkVariantArray* values);

  // Description:
  // Insert a blank row at the end of the table.
  vtkIdType InsertNextBlankRow();

  // Description:
  // Insert a row specified by a vtkVariantArray.  The number of entries in the array
  // should match the number of columns in the table.
  vtkIdType InsertNextRow(vtkVariantArray* arr);

  // Description:
  // Delete a row from the table.  Rows below the deleted row are shifted up.
  void RemoveRow(vtkIdType row);

  //
  // Column functions
  //

  // Description:
  // Get the number of columns in the table.
  vtkIdType GetNumberOfColumns();

  // Description:
  // Get the name of a column of the table.
  const char* GetColumnName(vtkIdType col);

  // Description:
  // Get a column of the table by its name.
  vtkAbstractArray* GetColumnByName(const char* name);

  // Description:
  // Get a column of the table by its column index.
  vtkAbstractArray* GetColumn(vtkIdType col);

  // Description:
  // Add a column to the table.
  void AddColumn(vtkAbstractArray* arr);

  // Description:
  // Remove a column from the table by its name.
  void RemoveColumnByName(const char* name);

  // Description:
  // Remove a column from the table by its column index.
  void RemoveColumn(vtkIdType col);

  //
  // Table single entry functions
  //

  //BTX
  // Description:
  // Retrieve a value in the table by row and column index as a variant.
  vtkVariant GetValue(vtkIdType row, vtkIdType col);

  // Description:
  // Retrieve a value in the table by row index and column name as a variant.
  vtkVariant GetValueByName(vtkIdType row, const char* col);

  // Description:
  // Set a value in the table by row and column index as a variant.
  void SetValue(vtkIdType row, vtkIdType col, vtkVariant value);

  // Description:
  // Set a value in the table by row index and column name as a variant.
  void SetValueByName(vtkIdType row, const char* col, vtkVariant value);
  //ETX
  
  // Description:
  // Initialize to an empty table.
  virtual void Initialize();

  // Description:
  // Retrieve the table from vtkInformation.
  static vtkTable* GetData(vtkInformation* info);
  static vtkTable* GetData(vtkInformationVector* v, int i=0);

  virtual void ShallowCopy(vtkDataObject* src);

protected:
  vtkTable();
  ~vtkTable() {}
private:
  vtkTable(const vtkTable&); // Not implemented
  void operator=(const vtkTable&); // Not implemented

  vtkIdType Rows;
};

#endif

