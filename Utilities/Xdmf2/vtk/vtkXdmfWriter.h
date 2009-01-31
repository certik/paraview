/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkXdmfWriter.h,v $
  Language:  C++
  Date:      $Date: 2007/07/10 17:55:41 $
  Version:   $Revision: 1.1 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXdmfWriter - write eXtensible Data Model and Format files
// .SECTION Description
// vtkXdmfWriter is a process object that writes XDMF data.  The input to this
// writer is a single vtkDataSet object.
// .SECTION Caveats
// used the XDMF API
// .SECTION See Also
// vtkDataReader
#ifndef _vtkXdmfWriter_h
#define _vtkXdmfWriter_h

#include "vtkProcessObject.h"

class vtkDataSet;
class vtkPoints;
class vtkCellArray;
class vtkDataArray;
class vtkDataSetCollection;

class VTK_EXPORT vtkXdmfWriter : public vtkProcessObject
{
public:
  static vtkXdmfWriter *New();
  vtkTypeRevisionMacro(vtkXdmfWriter,vtkProcessObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set or get the AllLight flag. If set, all data will be written as light
  // data (inlined).
  vtkSetClampMacro(AllLight, int, 0, 1);
  vtkBooleanMacro(AllLight, int);
  vtkGetMacro(AllLight, int);

  // Description:
  // Make all data heavy, including rectilinear grid arrays.
  vtkSetClampMacro(AllHeavy, int, 0, 1);
  vtkBooleanMacro(AllHeavy, int);
  vtkGetMacro(AllHeavy, int);

  // Description:
  // Set or get the file name of the xdmf file.
  virtual void SetFileName(const char* fname);
  virtual const char* GetFileName();

  // Description:
  // Set or get the grid name of the dataset.
  vtkSetStringMacro(GridName);
  vtkGetStringMacro(GridName);

  // Description:
  // Set or ger the domain name.
  vtkSetStringMacro(DomainName);
  vtkGetStringMacro(DomainName);

  // Description:
  // Collection name defines collection grids belong to
  vtkSetStringMacro(CollectionName);
  vtkGetStringMacro(CollectionName);

  // Description:
  // If GridOnly is set, only the grid will be written and all the header
  // information will be ignored
  vtkSetClampMacro(GridOnly, int, 0, 1);
  vtkBooleanMacro(GridOnly, int);
  vtkGetMacro(GridOnly, int);

  // Description:
  // Set or get the name of the heavy data file name.
  virtual void SetHeavyDataSetName( const char *name);
  virtual const char* GetHeavyDataSetName();

  // Description:
  // Set the input data set.
  virtual void SetInput(vtkDataSet* ds);

  // Description:
  // Write the XDMF file.
  void Write();

  // Description:
  // Add a dataset to the list of data to append.
  void AddInput(vtkDataObject *in);

  // Description:
  // Get any input of this filter.
  vtkDataObject *GetInput(int idx);
  vtkDataObject *GetInput() 
    {return this->GetInput( 0 );}
  
  // Description:
  // Remove a dataset from the list of data to append.
  void RemoveInput(vtkDataObject *in);

  // Description:
  // Returns a copy of the input array.  Modifications to this list
  // will not be reflected in the actual inputs.
  vtkDataSetCollection *GetInputList();

  // Description:
  // Indent xml 
  void Indent(ostream& ost);
  void IncrementIndent() { this->CurrIndent ++; }
  void DecrementIndent() { this->CurrIndent --; }

  // Description:
  // Generate hdf5 name for array
  const char* GenerateHDF5ArrayName(const char* gridName, const char* arrayName);

protected:
  vtkXdmfWriter();
  ~vtkXdmfWriter();

  void WriteAttributes( ostream& ost, vtkDataSet* ds, const char* gridName );
  void StartTopology( ostream& ost, int cellType, vtkIdType numVert, vtkIdType numCells );
  void StartTopology( ostream& ost, const char* toptype, int rank, int *dims );
  void EndTopology( ostream& ost );
  void StartGeometry( ostream& ost, const char* type );
  void EndGeometry( ostream& ost );
  virtual int WriteHead( ostream& ost );
  virtual int WriteTail( ostream& ost );
  virtual int WriteGrid( ostream& ost, const char* name, vtkDataSet* ds, 
    void* mapofcells = 0, const void *celltype = 0 );
  virtual int WriteCellArray( ostream& ost, vtkDataSet* Cells, const char* gridName, 
    void* mapofcells, const void *celltype );
  virtual int WritePoints( ostream& ost, vtkPoints *Points, vtkDataSet* dataSet, const char* gridName );
  virtual int WriteDataArray( ostream& ost, vtkDataArray* array, vtkDataSet* ds,
    int dims[3], const char* Name, const char* Center, int type, const char* gridName,
    int active, int cellData = 0 );
  virtual int WriteVTKArray( ostream& ost, vtkDataArray* array, vtkDataSet* dataSet,
    int dims[3], int *extents, const char* name, const char* dataName, const char* gridName, int alllight,
    int cellData = 0);

  vtkSetStringMacro(HeavyDataSetNameString);
  char    *HeavyDataSetNameString;

  vtkSetStringMacro(FileNameString);
  char    *FileNameString;
  char    *GridName;
  char    *DomainName;
  char    *CollectionName;
  
  int    AllLight;
  int    AllHeavy;

  int CurrIndent;

  int GridOnly;

  vtkSetStringMacro(HDF5ArrayName);
  char* HDF5ArrayName;

  // list of data sets to append together.
  // Here as a convenience.  It is a copy of the input array.
  vtkDataSetCollection *InputList;

private:
  vtkXdmfWriter(const vtkXdmfWriter&); // Not implemented
  void operator=(const vtkXdmfWriter&); // Not implemented
};

#endif /* _vtkXdmfWriter_h */
