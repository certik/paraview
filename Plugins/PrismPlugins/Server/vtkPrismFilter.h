/*=========================================================================

Program:   Visualization Toolkit
Module:    $RCSfile: vtkPrismFilter.h,v $

=========================================================================*/
// .NAME vtkPrismFilter - 
// .SECTION Description

#ifndef __vtkPrismFilter
#define __vtkPrismFilter

#include "vtkAlgorithm.h"

#include "vtkCell.h" // Needed for VTK_CELL_SIZE

class vtkIntArray;

class VTK_EXPORT vtkPrismFilter : public vtkAlgorithm 
{
public:
  vtkTypeRevisionMacro(vtkPrismFilter,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPrismFilter *New();


  // Description:
  // see vtkAlgorithm for details
  virtual int ProcessRequest(vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);
  // Description:
  // Set the filename to read
  void SetFileName(const char* file);
  // Description:
  // Get the filename to read
  const char* GetFileName();

  // Description:
  // Return whether this is a valid file
  int IsValidFile();

  // Description:
  // Get the number of tables in this file
  int GetNumberOfTableIds();

  // Description:
  // Get the ids of the tables in this file
  int* GetTableIds();

  // Description:
  // Returns the table ids in a data array.
  vtkIntArray* GetTableIdsAsArray();

  // Description:
  // Set the table to read in
  void SetTable(int tableId);
  // Description:
  // Get the table to read in
  int GetTable();

  // Description:
  // Get the number of arrays for the table to read
  int GetNumberOfTableArrayNames();

  // Description:
  // Get the number of arrays for the table to read
  int GetNumberOfTableArrays()
  { return this->GetNumberOfTableArrayNames(); }
  // Description:
  // Get the names of arrays for the table to read
  const char* GetTableArrayName(int index);

  // Description:

  void SetTableArrayToProcess(const char* name);
  const char* GetTableArrayNameToProcess();


  // Description:
  // Set whether to read a table array
  void SetTableArrayStatus(const char* name, int flag);
  int GetTableArrayStatus(const char* name);


  void SetXAxisVarName( const char *name );
  void SetYAxisVarName( const char *name );
  void SetZAxisVarName( const char *name );
  const char *GetXAxisVarName();
  const char *GetYAxisVarName(); 
  const char *GetZAxisVarName(); 

protected:
  vtkPrismFilter();
  ~vtkPrismFilter() {}


  //BTX 
  class MyInternal;
  MyInternal* Internal;
  //ETX 

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  // Description:
  // This is called by the superclass.
  // This is the method you should override.
  virtual int RequestUpdateExtent(vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  // see algorithm for more info
  virtual int FillOutputPortInformation(int port, vtkInformation* info);
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // virtual void ExecuteData(vtkDataObject *output);
private:
  int CalculateValues( double *x, double *f );
  vtkPrismFilter(const vtkPrismFilter&);  // Not implemented.
  void operator=(const vtkPrismFilter&);  // Not implemented.
  int RequestSESAMEData(
    vtkInformation *vtkNotUsed(request),
    vtkInformationVector **vtkNotUsed(inputVector),
    vtkInformationVector *outputVector);
  int RequestGeometryData(
    vtkInformation *vtkNotUsed(request),
    vtkInformationVector **vtkNotUsed(inputVector),
    vtkInformationVector *outputVector);
};

#endif
