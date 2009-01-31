/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkStringToTimePoint.h,v $

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
// .NAME vtkStringToTimePoint - Converts a string array to a integral time array
//
// .SECTION Description
//
// vtkStringToTimePoint is a filter for converting a string array
// into a datetime, time or date array.  The input strings must
// conform to one of the ISO8601 formats defined in vtkTimePointUtility.
//
// The input array specified by SetInputArrayToProcess(...)
// indicates the array to process.  This array must be of type
// vtkStringArray.
//
// The output array will be of type vtkTypeUInt64Array.

#ifndef __vtkStringToTimePoint_h
#define __vtkStringToTimePoint_h

#include "vtkDataObjectAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkStringToTimePoint : public vtkDataObjectAlgorithm
{
public:
  static vtkStringToTimePoint* New();
  vtkTypeRevisionMacro(vtkStringToTimePoint,vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The name of the output array.
  // If this is not specified, the name will be the same as the input
  // array name with either " [to datetime]", " [to date]", or " [to time]"
  // appended.
  vtkSetStringMacro(OutputArrayName);
  vtkGetStringMacro(OutputArrayName);

  // Description:
  // This is required to capture REQUEST_DATA_OBJECT requests.
  virtual int ProcessRequest(vtkInformation* request, 
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector);

protected:
  vtkStringToTimePoint();
  ~vtkStringToTimePoint();

  // Description:
  // Creates the same output type as the input type.
  virtual int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector);

  char* OutputArrayName;

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);
    
private:
  vtkStringToTimePoint(const vtkStringToTimePoint&); // Not implemented
  void operator=(const vtkStringToTimePoint&);   // Not implemented
};

#endif

