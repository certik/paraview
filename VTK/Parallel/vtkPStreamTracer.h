/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPStreamTracer.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPStreamTracer - Abstract superclass for parallel streamline generators
// .SECTION Description
// This class implements some necessary functionality used by distributed
// and parallel streamline generators. Note that all processes must have
// access to the WHOLE seed source, i.e. the source must be identical
// on all processes.
// .SECTION See Also
// vtkStreamTracer vtkDistributedStreamTracer vtkMPIStreamTracer

#ifndef __vtkPStreamTracer_h
#define __vtkPStreamTracer_h

#include "vtkStreamTracer.h"

#include "vtkSmartPointer.h" // This is a leaf node. No need to
#include <vtkstd/vector>     // use PIMPL to avoid compile time penalty.

class vtkInterpolatedVelocityField;
class vtkMultiProcessController;

class VTK_PARALLEL_EXPORT vtkPStreamTracer : public vtkStreamTracer
{
public:
  vtkTypeRevisionMacro(vtkPStreamTracer,vtkStreamTracer);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the controller use in compositing (set to
  // the global controller by default)
  // If not using the default, this must be called before any
  // other methods.
  virtual void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

protected:

  vtkPStreamTracer();
  ~vtkPStreamTracer();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  vtkMultiProcessController* Controller;

  vtkInterpolatedVelocityField* Interpolator;
  void SetInterpolator(vtkInterpolatedVelocityField*);

  // See the implementation for comments
  void SendCellPoint(vtkPolyData* data,
                     vtkIdType streamId, 
                     vtkIdType idx, 
                     int sendToId);
  void ReceiveCellPoint(vtkPolyData* tomod, int streamId, vtkIdType idx);
  void SendFirstPoints(vtkPolyData *output);
  void ReceiveLastPoints(vtkPolyData *output);
  void MoveToNextSend(vtkPolyData *output);

  virtual void ParallelIntegrate() = 0;

  vtkDataArray* Seeds;
  vtkIdList* SeedIds;
  vtkIntArray* IntegrationDirections;

  int EmptyData;

//BTX
  typedef vtkstd::vector< vtkSmartPointer<vtkPolyData> > TmpOutputsType;
//ETX

  TmpOutputsType TmpOutputs;

private:
  vtkPStreamTracer(const vtkPStreamTracer&);  // Not implemented.
  void operator=(const vtkPStreamTracer&);  // Not implemented.
};


#endif


