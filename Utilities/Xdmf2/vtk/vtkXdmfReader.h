/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkXdmfReader.h,v $
  Language:  C++
  Date:      $Date: 2007/08/30 17:27:03 $
  Version:   $Revision: 1.6 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXdmfReader - read eXtensible Data Model and Format files
// .SECTION Description
// vtkXdmfReader is a source object that reads XDMF data.
// The output of this reader is a vtkMultiGroupDataSet with one group for
// every enabled grid in the domain.
// The superclass of this class, vtkDataReader, provides many methods for
// controlling the reading of the data file, see vtkDataReader for more
// information.
// .SECTION Caveats
// uses the XDMF API
// .SECTION See Also
// vtkDataReader

#ifndef __vtkXdmfReader_h
#define __vtkXdmfReader_h

#include "vtkDataReader.h"

class vtkDataObject;
class vtkDataArraySelection;
class vtkCallbackCommand;
class vtkMultiProcessController;
class vtkXdmfReaderInternal;

//BTX
class XdmfDOM;
//ETX

class VTK_EXPORT vtkXdmfReader : public vtkDataReader
{
public:
  static vtkXdmfReader* New();
  vtkTypeRevisionMacro(vtkXdmfReader, vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this reader.
  vtkDataObject *GetOutput();
  vtkDataObject *GetOutput(int idx);

  // ATTRIBUTES ///////////////////////////////////////////////////////////////

  // Description:
  // Get the data array selection tables used to configure which data
  // arrays are loaded by the reader.
  vtkGetObjectMacro(PointDataArraySelection, vtkDataArraySelection);
  vtkGetObjectMacro(CellDataArraySelection, vtkDataArraySelection);
  
  // Description:  
  // Get the number of point or cell arrays available in the input.
  int GetNumberOfPointArrays();
  int GetNumberOfCellArrays();
  
  // Description:
  // Get the name of the point or cell array with the given index in
  // the input.
  const char* GetPointArrayName(int index);
  const char* GetCellArrayName(int index);

  // Description:
  // Get/Set whether the point or cell array with the given name is to
  // be read.
  int GetPointArrayStatus(const char* name);
  int GetCellArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);  
  void SetCellArrayStatus(const char* name, int status);  

  // Description:
  // Set whether the all point or cell arrays are to
  // be read.
  void EnableAllArrays();
  void DisableAllArrays();

  // PARAMETERS ///////////////////////////////////////////////////////////////
  
  // Description:
  // Get the number of Parameters
  int GetNumberOfParameters();

  // Description:
  // Get Parameter Type
  int GetParameterType(int index);
  int GetParameterType(const char *Name);
  const char *GetParameterTypeAsString(int index);
  const char *GetParameterTypeAsString(const char *Name);

  // Description:
  // Get start, stride, count
  int GetParameterRange(int index, int Shape[3]);
  int GetParameterRange(const char *Name, int Shape[3]);
  const char *GetParameterRangeAsString(int index);
  const char *GetParameterRangeAsString(const char *Name);

  // Description:
  // Get Parameter Name
  const char *GetParameterName(int index);

  // Description:
  // Set/Get Parameter Current Index
  int SetParameterIndex(const char *Name, int CurrentIndex); 
  int SetParameterIndex(int ParameterIndex, int CurrentIndex); 
  int GetParameterIndex(const char *Name);
  int GetParameterIndex(int index);

  // Description:
  // Get Length of Parameter
  int GetParameterLength(const char *Name);
  int GetParameterLength(int index);

  // Description:
  // Get the Current Value of the Parameter
  const char *GetParameterValue(int index);
  const char *GetParameterValue(const char *Name);

  // DOMAINS ///////////////////////////////////////////////////////////////
  // Description:
  // Get number of domains.
  int GetNumberOfDomains();

  // Description:
  // Get/Set the current domain name.
  virtual void SetDomainName(const char*);
  vtkGetStringMacro(DomainName);

  // Get the name of domain at index.
  const char* GetDomainName(int idx);

  // GRIDS ///////////////////////////////////////////////////////////////////
  // Description:
  // Get number of grids in the current domain.
  int GetNumberOfGrids();

  // Description:
  // Get/Set the current grid name.
  void SetGridName(const char*);

  // Description:
  // Get the name of grid at index.
  const char* GetGridName(int idx);
  int GetGridIndex(const char* name);

  // Description:
  // Enable grids.
  void EnableGrid(const char* name);
  void EnableGrid(int idx);
  void EnableAllGrids();

  // Description:
  // Disable grids
  void DisableGrid(const char* name);
  void DisableGrid(int idx);
  void DisableAllGrids();
  void RemoveAllGrids();

  // Description:
  // Get current enable/disable of the grid
  int GetGridSetting(const char* name);
  int GetGridSetting(int idx);

  // STRIDE ///////////////////////////////////////////////////////////////////
  // Description:
  // Set / get stride
  void SetStride(int x, int y, int z);
  void SetStride(int xyz[3])
    {
    this->SetStride(xyz[0], xyz[1], xyz[2]);
    }
  vtkGetVector3Macro(Stride, int);

  // MISCELANEOUS /////////////////////////////////////////////////////////////
  // Description:
  // Get the Low Level XdmfDOM
  const char *GetXdmfDOMHandle();

  // Description:
  // Get the Low Level XdmfGrid
  //Disable for now
  //const char *GetXdmfGridHandle(int idx);

  // Description:
  // Determine if the file can be readed with this reader.
  virtual int CanReadFile(const char* fname);
  
  // Description:
  // Set the controller used to coordinate parallel reading.
  void SetController(vtkMultiProcessController* controller);
  
  // Return the controller used to coordinate parallel reading. By default,
  // it is the global controller.
  vtkGetObjectMacro(Controller,vtkMultiProcessController);
  
protected:
  vtkXdmfReader();
  ~vtkXdmfReader();   

  virtual int ProcessRequest(vtkInformation *request,
                             vtkInformationVector **inputVector,
                             vtkInformationVector *outputVector);
  
  virtual int RequestDataObject(vtkInformationVector *outputVector);
  
  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);
  virtual int FillOutputPortInformation(int port, vtkInformation *info);

  void UpdateUniformGrid(void *GridNode, char *CollectionName);
  void UpdateNonUniformGrid(void *GridNode, char *CollectionName);
  void UpdateGrids();

  vtkXdmfReaderInternal* Internals;

  char* DomainName;

  char* GridName;
  int GridsModified;
  int NumberOfEnabledActualGrids;

  int Stride[3];

  int OutputsInitialized;
  XdmfDOM         *DOM;
  vtkMultiProcessController *Controller;

  // Array selection helpers /////////////////////////////////////////////////
  static void SelectionModifiedCallback(vtkObject* caller, unsigned long eid,
                                        void* clientdata, void* calldata);

  vtkDataArraySelection* PointDataArraySelection;
  vtkDataArraySelection* CellDataArraySelection;

  vtkCallbackCommand* SelectionObserver;
  
private:
  vtkXdmfReader(const vtkXdmfReader&); // Not implemented
  void operator=(const vtkXdmfReader&); // Not implemented  
};

#endif //__vtkXdmfReader_h
