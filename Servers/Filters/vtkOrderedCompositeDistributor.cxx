/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkOrderedCompositeDistributor.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2005 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkOrderedCompositeDistributor.h"

#include "vtkBSPCuts.h"
#include "vtkCallbackCommand.h"
#include "vtkDataSet.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDistributedDataFilter.h"
#include "vtkGarbageCollector.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPKdTree.h"
#include "vtkPolyData.h"
#include "vtkUnstructuredGrid.h"

//-----------------------------------------------------------------------------

static void D3UpdateProgress(vtkObject *_D3, unsigned long,
                             void *_distributor, void *)
{
  vtkDistributedDataFilter *D3
    = reinterpret_cast<vtkDistributedDataFilter *>(_D3);
  vtkOrderedCompositeDistributor *distributor
    = reinterpret_cast<vtkOrderedCompositeDistributor *>(_distributor);

  distributor->SetProgressText(D3->GetProgressText());
  distributor->UpdateProgress(D3->GetProgress() * 0.9);
}

//-----------------------------------------------------------------------------

vtkCxxRevisionMacro(vtkOrderedCompositeDistributor, "$Revision: 1.10 $");
vtkStandardNewMacro(vtkOrderedCompositeDistributor);

vtkCxxSetObjectMacro(vtkOrderedCompositeDistributor, PKdTree, vtkPKdTree);
vtkCxxSetObjectMacro(vtkOrderedCompositeDistributor, Controller,
                     vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkOrderedCompositeDistributor, D3,
                     vtkDistributedDataFilter);
vtkCxxSetObjectMacro(vtkOrderedCompositeDistributor, ToPolyData,
                     vtkDataSetSurfaceFilter);

//-----------------------------------------------------------------------------

vtkOrderedCompositeDistributor::vtkOrderedCompositeDistributor()
{
  this->PKdTree = NULL;
  this->Controller = NULL;

  this->D3 = NULL;
  this->ToPolyData = NULL;

  this->PassThrough = 0;

  this->OutputType = NULL;

  this->LastInput = NULL;
  this->LastOutput = NULL;
  this->LastCuts = vtkBSPCuts::New();
}

vtkOrderedCompositeDistributor::~vtkOrderedCompositeDistributor()
{
  this->SetPKdTree(NULL);
  this->SetController(NULL);

  this->SetD3(NULL);
  this->SetToPolyData(NULL);

  this->SetOutputType(NULL);

  if (this->LastOutput) this->LastOutput->Delete();
  if (this->LastCuts) this->LastCuts->Delete();
}

void vtkOrderedCompositeDistributor::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PKdTree: " << this->PKdTree << endl;
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "PassThrough: " << this->PassThrough << endl;
  os << indent << "OutputType: " << 
    (this->OutputType? this->OutputType : "(none)") << endl;
  os << indent << "D3: " << this->D3 << endl;
  os << indent << "ToPolyData" << this->ToPolyData << endl;
}

//-----------------------------------------------------------------------------

void vtkOrderedCompositeDistributor::ReportReferences(
                                                 vtkGarbageCollector *collector)
{
  this->Superclass::ReportReferences(collector);

  vtkGarbageCollectorReport(collector, this->D3, "D3");
  vtkGarbageCollectorReport(collector, this->ToPolyData, "ToPolyData");
  vtkGarbageCollectorReport(collector, this->PKdTree, "PKdTree");
  vtkGarbageCollectorReport(collector, this->Controller, "Controller");
}

//-----------------------------------------------------------------------------

int vtkOrderedCompositeDistributor::FillInputPortInformation(
                                                 int port, vtkInformation *info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//-----------------------------------------------------------------------------

int vtkOrderedCompositeDistributor::RequestDataObject(
                                             vtkInformation* request,
                                             vtkInformationVector** inputVector,
                                             vtkInformationVector* outputVector)
{
  if (!this->OutputType || this->OutputType[0] == '\0')
    {
    return this->Superclass::RequestDataObject(request,
                                               inputVector, outputVector);
    }


  // for each output
  for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
    {
    vtkInformation* info = outputVector->GetInformationObject(i);
    vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());

    if (!output || !output->IsA(this->OutputType)) 
      {
      output = vtkDataObjectTypes::NewDataObject(this->OutputType);
      if (!output)
        {
        return 0;
        }
      output->SetPipelineInformation(info);
      output->Delete();
      this->GetOutputPortInformation(0)->Set(vtkDataObject::DATA_EXTENT_TYPE(),
                                             output->GetExtentType());
      }
    }
  return 1;
}

//-----------------------------------------------------------------------------

int vtkOrderedCompositeDistributor::RequestData(
                                            vtkInformation *vtkNotUsed(request),
                                            vtkInformationVector **inputVector,
                                            vtkInformationVector *outputVector)
{
  // Get the info objects.
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  if (!inInfo || !outInfo)
    {
    // Ignore request.
    return 1;
    }

  // Get the input and output.
  vtkDataSet *input = vtkDataSet::SafeDownCast(
                                     inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
                                    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!output || !input)
    {
    // Ignore request.
    return 1;
    }

  if (this->PassThrough)
    {
    // Don't do anything to the data.
    output->ShallowCopy(input);
    return 1;
    }

  vtkBSPCuts *cuts = this->PKdTree->GetCuts();
  if (cuts == NULL)
    {
    // No partitioning has been defined.  Just pass the data through.
    output->ShallowCopy(input);
    return 1;
    }

  // Check to see if update needs to be done (parallel operation), and pass
  // through old data if not.
  int needUpdate = 0;
  if (   (this->LastInput != input)
      || (this->LastUpdate < input->GetMTime())
      || !this->LastCuts->Equals(cuts) )
    {
    needUpdate = 1;
    }

  const int COLLECT_NEED_UPDATE = 25234;
  const int BROADCAST_NEED_UPDATE = 25235;
  if (this->Controller->GetLocalProcessId() == 0)
    {
    int i, numproc;
    numproc = this->Controller->GetNumberOfProcesses();
    for (i = 1; i < numproc; i++)
      {
      int remoteNeedUpdate;
      this->Controller->Receive(&remoteNeedUpdate, 1, i, COLLECT_NEED_UPDATE);
      needUpdate |= remoteNeedUpdate;
      }
    for (i = 1; i < numproc; i++)
      {
      this->Controller->Send(&needUpdate, 1, i, BROADCAST_NEED_UPDATE);
      }
    }
  else
    {
    this->Controller->Send(&needUpdate, 1, 0, COLLECT_NEED_UPDATE);
    this->Controller->Receive(&needUpdate, 1, 0, BROADCAST_NEED_UPDATE);
    }

  if (!needUpdate)
    {
    output->ShallowCopy(this->LastOutput);
    return 1;
    }

  this->UpdateProgress(0.01);

  if (this->D3 == NULL)
    {
    this->D3 = vtkDistributedDataFilter::New();
    }

  vtkCallbackCommand *cbc = vtkCallbackCommand::New();
  cbc->SetClientData(this);
  cbc->SetCallback(D3UpdateProgress);
  this->D3->AddObserver(vtkCommand::ProgressEvent, cbc);

  this->D3->SetBoundaryModeToSplitBoundaryCells();
  this->D3->SetInput(input);
  this->D3->SetCuts(cuts);
  this->D3->SetController(this->Controller);
  this->D3->Modified();
  this->D3->Update();

  this->D3->RemoveObserver(cbc);
  cbc->Delete();

  if (output->IsA("vtkUnstructuredGrid"))
    {
    output->ShallowCopy(this->D3->GetOutput());
    }
  else if (output->IsA("vtkPolyData"))
    {
    if (this->ToPolyData == NULL)
      {
      this->ToPolyData = vtkDataSetSurfaceFilter::New();
      }
    this->ToPolyData->SetInputConnection(0, this->D3->GetOutputPort(0));
    this->ToPolyData->Update();

    output->ShallowCopy(this->ToPolyData->GetOutput());
    }
  else
    {
    vtkErrorMacro(<< "vtkOrderedCompositeDistributor used with unsupported "
                  << "type.");
    return 0;
    }

  this->LastUpdate.Modified();
  this->LastInput = input;
  this->LastCuts->CreateCuts(cuts->GetKdNodeTree());

  if (this->LastOutput && !this->LastOutput->IsA(output->GetClassName()))
    {
    this->LastOutput->Delete();
    this->LastOutput = NULL;
    }
  if (!this->LastOutput)
    {
    this->LastOutput = output->NewInstance();
    }
  this->LastOutput->ShallowCopy(output);

  return 1;
}
