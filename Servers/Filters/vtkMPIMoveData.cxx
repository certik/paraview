/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkMPIMoveData.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMPIMoveData.h"

#include "vtkAppendFilter.h"
#include "vtkAppendPolyData.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDataSetReader.h"
#include "vtkDataSetWriter.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMPIMToNSocketConnection.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineFilter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkSocketCommunicator.h"
#include "vtkSocketController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkUnstructuredGrid.h"

#ifdef VTK_USE_MPI
#include "vtkMPICommunicator.h"
#include "vtkAllToNRedistributePolyData.h"
#endif

vtkCxxRevisionMacro(vtkMPIMoveData, "$Revision: 1.17 $");
vtkStandardNewMacro(vtkMPIMoveData);

vtkCxxSetObjectMacro(vtkMPIMoveData,Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkMPIMoveData,ClientDataServerSocketController, vtkSocketController);
vtkCxxSetObjectMacro(vtkMPIMoveData,MPIMToNSocketConnection, vtkMPIMToNSocketConnection);

//-----------------------------------------------------------------------------
vtkMPIMoveData::vtkMPIMoveData()
{
  this->Controller = 0;
  this->ClientDataServerSocketController = 0;
  this->MPIMToNSocketConnection = 0;

  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->MoveMode = vtkMPIMoveData::PASS_THROUGH;
  // This tells which server/client this object is on.
  this->Server = -1;

  // This is set on the data server and render server when we are running
  // with a render server.
  this->MPIMToNSocketConnection = 0;

  this->NumberOfBuffers = 0;
  this->BufferLengths = 0;
  this->BufferOffsets = 0;
  this->Buffers = 0;
  this->BufferTotalLength = 9;

  this->OutputDataType = VTK_POLY_DATA;

  this->UpdateNumberOfPieces = 0;
  this->UpdatePiece = 0;

  this->DeliverOutlineToClient = 0;
}

//-----------------------------------------------------------------------------
vtkMPIMoveData::~vtkMPIMoveData()
{
  this->SetController(0);
  this->SetClientDataServerSocketController(0);
  this->SetMPIMToNSocketConnection(0);
  this->ClearBuffer();
}

//----------------------------------------------------------------------------
int vtkMPIMoveData::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//-----------------------------------------------------------------------------
int vtkMPIMoveData::RequestDataObject(vtkInformation*,
                                      vtkInformationVector**,
                                      vtkInformationVector* outputVector)
{
  vtkDataObject* output = 
    outputVector->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT());
  
  vtkDataObject* outputCopy = 0;
  if (this->OutputDataType == VTK_POLY_DATA)
    {
    if (output && output->IsA("vtkPolyData"))
      {
      return 1;
      }
    outputCopy = vtkPolyData::New();
    }
  else if (this->OutputDataType == VTK_UNSTRUCTURED_GRID)
    {
    if (output && output->IsA("vtkUnstructuredGrid"))
      {
      return 1;
      }
    outputCopy = vtkUnstructuredGrid::New();
    }
  else if (this->OutputDataType == VTK_IMAGE_DATA)
    {
    if (output && output->IsA("vtkImageData"))
      {
      return 1;
      }
    outputCopy = vtkImageData::New();
    }
  else
    {
    vtkErrorMacro("Unrecognized output type: " << this->OutputDataType
                  << ". Cannot create output.");
    return 0;
    }

  outputCopy->SetPipelineInformation(outputVector->GetInformationObject(0));
  outputCopy->Delete();
  return 1;
}

//-----------------------------------------------------------------------------
int vtkMPIMoveData::RequestInformation(vtkInformation*,
                                       vtkInformationVector** inputVector,
                                       vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (inputVector[0]->GetNumberOfInformationObjects() > 0)
    {
    outInfo->Set(
      vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
      inputVector[0]->GetInformationObject(0)->Get(
        vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES()));
    return 1;
    }

  outInfo->Set(
    vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);

  return 1;
}

// This filter  is going to replace the many variations of collection fitlers.
// It handles collection and duplication.
// It handles poly data and unstructured grid.
// It handles rendering on the data server and render server.


// Pass through, No render server. (Distributed rendering on data server).
// Data server copy input to output.

// Passthrough, Yes RenderServer (Distributed rendering on render server).
// Data server MtoN
// Move data from N data server processes to N render server processes.

// Duplicate, No render server. (Tile rendering on data server and client).
// GatherAll on data server.
// Data server process 0 sends data to client.

// Duplicate, Yes RenderServer (Tile rendering on rendering server and client).
// GatherToZero on data server.
// Data server process 0 sends to client
// Data server process 0 sends to render server 0
// Render server process 0 broad casts to all render server processes.

// Collect, render server: yes or no. (client rendering).
// GatherToZero on data server.
// Data server process 0 sends data to client.

//-----------------------------------------------------------------------------
// We should avoid marshalling more than once.
int vtkMPIMoveData::RequestData(vtkInformation*,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkDataSet* input = 0;
  vtkDataSet* output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (inputVector[0]->GetNumberOfInformationObjects() > 0)
    {
    input = vtkDataSet::SafeDownCast(
      inputVector[0]->GetInformationObject(0)->Get(
        vtkDataObject::DATA_OBJECT()));
    }

  // We do not yet collect image data but we pretend to handle it.
  // This is here to prevent errors on the client side during structured
  // volume rendering.
  if (this->OutputDataType == VTK_IMAGE_DATA)
    {
    if (input)
      {
      output->ShallowCopy(input);
      }
    return 1;
    }

  this->UpdatePiece = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  this->UpdateNumberOfPieces = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  // This case deals with everything running as one MPI group
  // Client, Data and render server are all the same program.
  // This covers single process mode too, although this filter 
  // is unnecessary in that mode and really should not be put in.
  if (this->MPIMToNSocketConnection == 0 &&
      this->ClientDataServerSocketController == 0)
    {
    // Clone in this mode is used for plots and picking.
    if (this->MoveMode == vtkMPIMoveData::CLONE)
      {
      this->DataServerGatherAll(input, output);
      return 1;
      }
    // Collect mode for rendering on node 0.
    if (this->MoveMode == vtkMPIMoveData::COLLECT)
      {
      this->DataServerGatherToZero(input, output);
      return 1;
      }
    // PassThrough mode for compositing.
    if (this->MoveMode == vtkMPIMoveData::PASS_THROUGH)
      {
      output->ShallowCopy(input);
      return 1;
      }
    vtkErrorMacro("MoveMode not set.");
    return 0;
    }    

  // PassThrough with no RenderServer. (Distributed rendering on data server).
  // Data server copy input to output. 
  // Render server and client will not have an input.
  if (this->MoveMode == vtkMPIMoveData::PASS_THROUGH && 
      this->MPIMToNSocketConnection == 0)
    {
    if (input)
      {
      output->ShallowCopy(input);
      }
    return 1;
    }

  // Passthrough and RenderServer (Distributed rendering on render server).
  // Data server MtoN
  // Move data from N data server processes to N render server processes.
  if (this->MoveMode == vtkMPIMoveData::PASS_THROUGH && 
      this->MPIMToNSocketConnection)
    {
    if (this->Server == vtkMPIMoveData::DATA_SERVER)
      {
      this->DataServerAllToN(input,output, 
                      this->MPIMToNSocketConnection->GetNumberOfConnections());
      this->DataServerSendToRenderServer(output);
      output->Initialize();
      return 1;
      }
    if (this->Server == vtkMPIMoveData::RENDER_SERVER)
      {
      this->RenderServerReceiveFromDataServer(output);
      return 1;
      }
    // Client does nothing.
    return 1;
    }

  // Duplicate with no RenderServer.(Tile rendering on data server and client).
  // GatherAll on data server.
  // Data server process 0 sends data to client.
  if (this->MoveMode == vtkMPIMoveData::CLONE && 
      this->MPIMToNSocketConnection ==0)
    {
    if (this->Server == vtkMPIMoveData::DATA_SERVER)
      {
      this->DataServerGatherAll(input, output);
      this->DataServerSendToClient(output);
      return 1;
      }
    if (this->Server == vtkMPIMoveData::CLIENT)
      {
      this->ClientReceiveFromDataServer(output);
      return 1;
      }
    }

  // Duplicate and RenderServer(Tile rendering on rendering server and client).
  // GatherToZero on data server.
  // Data server process 0 sends to client
  // Data server process 0 sends to render server 0
  // Render server process 0 broad casts to all render server processes.
  if (this->MoveMode == vtkMPIMoveData::CLONE && 
      this->MPIMToNSocketConnection)
    {
    if (this->Server == vtkMPIMoveData::DATA_SERVER)
      {
      this->DataServerGatherToZero(input, output);
      this->DataServerSendToClient(output);
      this->DataServerZeroSendToRenderServerZero(output);
      return 1;
      }
    if (this->Server == vtkMPIMoveData::CLIENT)
      {
      this->ClientReceiveFromDataServer(output);
      return 1;
      }
    if (this->Server == vtkMPIMoveData::RENDER_SERVER)
      {
      this->RenderServerZeroReceiveFromDataServerZero(output);
      this->RenderServerZeroBroadcast(output);
      }
    }
  
  // Collect and data server or render server (client rendering).
  // GatherToZero on data server.
  // Data server process 0 sends data to client.
  if (this->MoveMode == vtkMPIMoveData::COLLECT)
    {
    if (this->Server == vtkMPIMoveData::DATA_SERVER)
      {
      this->DataServerGatherToZero(input, output);
      this->DataServerSendToClient(output);
      return 1;
      }
    if (this->Server == vtkMPIMoveData::CLIENT)
      {
      this->ClientReceiveFromDataServer(output);
      return 1;
      }
    // Render server does nothing
    return 1;
    }
  return 1;
}

//-----------------------------------------------------------------------------
// Use LANL filter to redistribute the data.
// We will marshal more than once, but that is OK.
void vtkMPIMoveData::DataServerAllToN(vtkDataSet* inData, 
                                      vtkDataSet* outData, int n)
{
  vtkMultiProcessController* controller = this->Controller;
  vtkPolyData* input = vtkPolyData::SafeDownCast(inData);
  vtkPolyData* output = vtkPolyData::SafeDownCast(outData);
  int m;

  if (controller == 0)
    {
    vtkErrorMacro("Missing controller.");
    return;
    }

  m = this->Controller->GetNumberOfProcesses();
  if (n > m)
    {
    vtkWarningMacro("Too many render servers.");
    n = m;
    }
  if (input == 0 || output == 0)
    {
    vtkErrorMacro("All to N only works for poly data currently.");
    return;
    }

  if (n == m)
    {
    output->ShallowCopy(input);
    }

  // Perform the M to N operation.
#ifdef VTK_USE_MPI
  vtkPolyData* tmp;
   vtkAllToNRedistributePolyData* AllToN = NULL;
   vtkPolyData* inputCopy = vtkPolyData::New();
   inputCopy->ShallowCopy(input);
   AllToN = vtkAllToNRedistributePolyData::New();
   AllToN->SetController(controller);
   AllToN->SetNumberOfProcesses(n);
   AllToN->SetInput(inputCopy);
   inputCopy->Delete();
   tmp = AllToN->GetOutput();
   tmp->SetUpdateNumberOfPieces(this->UpdateNumberOfPieces);
   tmp->SetUpdatePiece(this->UpdatePiece);
   tmp->Update();
   output->ShallowCopy(tmp);
   AllToN->Delete();
   AllToN= 0;
#endif
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::DataServerGatherAll(vtkDataSet* input, 
                                         vtkDataSet* output)
{
  int numProcs= this->Controller->GetNumberOfProcesses();

  if (numProcs <= 1)
    {
    output->ShallowCopy(input);
    return;
    }

#ifdef VTK_USE_MPI
  int idx;
  vtkMPICommunicator* com = vtkMPICommunicator::SafeDownCast(
                                         this->Controller->GetCommunicator()); 

  if (com == 0)
    {
    vtkErrorMacro("MPICommunicator neededfor this operation.");
    return;
    }
  this->ClearBuffer();
  this->MarshalDataToBuffer(input);

  // Save a copy of the buffer so we can receive into the buffer.
  // We will be responsiblefor deleting the buffer.
  // This assumes one buffer. MashalData will produce only one buffer
  // One data set, one buffer.
  vtkIdType inBufferLength = this->BufferTotalLength;
  char *inBuffer = this->Buffers;
  this->Buffers = NULL;
  this->ClearBuffer();

  // Allocate arrays used by the AllGatherV call.
  this->BufferLengths = new vtkIdType[numProcs];
  this->BufferOffsets = new vtkIdType[numProcs];
  
  // Compute the degenerate input offsets and lengths.
  // Broadcast our size to all other processes.
  com->AllGather(&inBufferLength, this->BufferLengths, 1);

  // Compute the displacements.
  this->BufferTotalLength = 0;
  for (idx = 0; idx < numProcs; ++idx)
    {
    this->BufferOffsets[idx] = this->BufferTotalLength;
    this->BufferTotalLength += this->BufferLengths[idx];
    }
  // Gather the marshaled data sets from all procs.
  this->NumberOfBuffers = numProcs;
  this->Buffers = new char[this->BufferTotalLength];
  com->AllGatherV(inBuffer, this->Buffers, inBufferLength, 
                  this->BufferLengths, this->BufferOffsets);

  this->ReconstructDataFromBuffer(output);
  
  //int fixme; // Do not clear buffers here
  this->ClearBuffer();
#endif
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::DataServerGatherToZero(vtkDataSet* input, 
                                            vtkDataSet* output)
{
  int numProcs= this->Controller->GetNumberOfProcesses();
  if (numProcs == 1)
    {
    output->ShallowCopy(input);
    return;
    }

    vtkTimerLog::MarkStartEvent("Dataserver gathering to 0");

#ifdef VTK_USE_MPI
  int idx;
  int myId= this->Controller->GetLocalProcessId();
  vtkMPICommunicator* com = vtkMPICommunicator::SafeDownCast(
                                         this->Controller->GetCommunicator()); 

  if (com == 0)
    {
    vtkErrorMacro("MPICommunicator neededfor this operation.");
    return;
    }
  this->ClearBuffer();
  this->MarshalDataToBuffer(input);

  // Save a copy of the buffer so we can receive into the buffer.
  // We will be responsiblefor deleting the buffer.
  // This assumes one buffer. MashalData will produce only one buffer
  // One data set, one buffer.
  vtkIdType inBufferLength = this->BufferTotalLength;
  char *inBuffer = this->Buffers;
  this->Buffers = NULL;
  this->ClearBuffer();

  if (myId == 0)
    {
    // Allocate arrays used by the AllGatherV call.
    this->BufferLengths = new vtkIdType[numProcs];
    this->BufferOffsets = new vtkIdType[numProcs];
    }

  // Compute the degenerate input offsets and lengths.
  // Broadcast our size to process 0.
  com->Gather(&inBufferLength, this->BufferLengths, 1, 0);

  // Compute the displacements.
  this->BufferTotalLength = 0;
  if (myId == 0)
    {
    for (idx = 0; idx < numProcs; ++idx)
      {
      this->BufferOffsets[idx] = this->BufferTotalLength;
      this->BufferTotalLength += this->BufferLengths[idx];
      }
    // Gather the marshaled data sets to 0.
    this->Buffers = new char[this->BufferTotalLength];
    }
  com->GatherV(inBuffer, this->Buffers, inBufferLength, 
                  this->BufferLengths, this->BufferOffsets, 0);
  this->NumberOfBuffers = numProcs;

  if (myId == 0)
    {
    this->ReconstructDataFromBuffer(output);
    }

  //int fixme; // Do not clear buffers here
  this->ClearBuffer();

  delete [] inBuffer;
  inBuffer = NULL;
#endif

  vtkTimerLog::MarkEndEvent("Dataserver gathering to 0");
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::DataServerSendToRenderServer(vtkDataSet* output)
{
  vtkSocketCommunicator* com = 
      this->MPIMToNSocketConnection->GetSocketCommunicator();

  if (com == 0)
    {
    // Some data server may not have sockets because there are more data
    // processes than render server processes.
    return;
    }

  //int fixme;
  // We might be able to eliminate this marshal.
  this->ClearBuffer();
  this->MarshalDataToBuffer(output);

  com->Send(&(this->NumberOfBuffers), 1, 1, 23480);
  com->Send(this->BufferLengths, this->NumberOfBuffers, 1, 23481);
  com->Send(this->Buffers, this->BufferTotalLength, 1, 23482);
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::RenderServerReceiveFromDataServer(vtkDataSet* output)
{
  vtkSocketCommunicator* com = 
      this->MPIMToNSocketConnection->GetSocketCommunicator();

  if (com == 0)
    {
    vtkErrorMacro("All render server processes should have sockets.");
    return;
    }

  this->ClearBuffer();
  com->Receive(&(this->NumberOfBuffers), 1, 1, 23480);
  this->BufferLengths = new vtkIdType[this->NumberOfBuffers];
  com->Receive(this->BufferLengths, this->NumberOfBuffers, 1, 23481);
  // Compute additional buffer information.
  this->BufferOffsets = new vtkIdType[this->NumberOfBuffers];
  this->BufferTotalLength = 0;
  for (int idx = 0; idx < this->NumberOfBuffers; ++idx)
    {
    this->BufferOffsets[idx] = this->BufferTotalLength;
    this->BufferTotalLength += this->BufferLengths[idx];
    }
  this->Buffers = new char[this->BufferTotalLength];
  com->Receive(this->Buffers, this->BufferTotalLength, 1, 23482);

  //int fixme;  // Can we avoid this?
  this->ReconstructDataFromBuffer(output);
  this->ClearBuffer();
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::DataServerZeroSendToRenderServerZero(vtkDataSet* data)
{
  int myId = this->Controller->GetLocalProcessId();

  if (myId == 0)
    {
    vtkSocketCommunicator* com = 
        this->MPIMToNSocketConnection->GetSocketCommunicator();

    if (com == 0)
      {
      // Proc 0 (at least) should have a communicator.
      vtkErrorMacro("Missing socket connection.");
      return;
      }

    //int fixme;
    // We might be able to eliminate this marshal.
    this->ClearBuffer();
    this->MarshalDataToBuffer(data);
    com->Send(&(this->NumberOfBuffers), 1, 1, 23480);
    com->Send(this->BufferLengths, this->NumberOfBuffers, 1, 23481);
    com->Send(this->Buffers, this->BufferTotalLength, 1, 23482);
    this->ClearBuffer();
    }
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::RenderServerZeroReceiveFromDataServerZero(vtkDataSet* data)
{
  int myId = this->Controller->GetLocalProcessId();

  if (myId == 0)
    {
    vtkSocketCommunicator* com = 
        this->MPIMToNSocketConnection->GetSocketCommunicator();

    if (com == 0)
      {
      vtkErrorMacro("All render server processes should have sockets.");
      return;
      }

    this->ClearBuffer();
    com->Receive(&(this->NumberOfBuffers), 1, 1, 23480);
    this->BufferLengths = new vtkIdType[this->NumberOfBuffers];
    com->Receive(this->BufferLengths, this->NumberOfBuffers, 1, 23481);
    // Compute additional buffer information.
    this->BufferOffsets = new vtkIdType[this->NumberOfBuffers];
    this->BufferTotalLength = 0;
    for (int idx = 0; idx < this->NumberOfBuffers; ++idx)
      {
      this->BufferOffsets[idx] = this->BufferTotalLength;
      this->BufferTotalLength += this->BufferLengths[idx];
      }
    this->Buffers = new char[this->BufferTotalLength];
    com->Receive(this->Buffers, this->BufferTotalLength, 1, 23482);

    //int fixme;  // Can we avoid this?
    this->ReconstructDataFromBuffer(data);
    this->ClearBuffer();
    }
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::DataServerSendToClient(vtkDataSet* output)
{
  int myId = this->Controller->GetLocalProcessId();

  if (myId == 0)
    {
    vtkTimerLog::MarkStartEvent("Dataserver sending to client");

    vtkSmartPointer<vtkDataSet> tosend = output;
    if (this->DeliverOutlineToClient)
      {
      // reduce data using outline filter.
      if (output->IsA("vtkPolyData"))
        {
        vtkDataSet* clone = output->NewInstance();
        clone->ShallowCopy(output);

        vtkOutlineFilter* filter = vtkOutlineFilter::New();
        filter->SetInput(clone);
        filter->Update();
        tosend = filter->GetOutput();
        filter->Delete();
        clone->Delete();
        }
      else
        {
        vtkErrorMacro("DeliverOutlineToClient can only be used for vtkPolyData.");
        }
      }

    this->ClearBuffer();
    this->MarshalDataToBuffer(tosend);
    this->ClientDataServerSocketController->Send(
                                     &(this->NumberOfBuffers), 1, 1, 23490);
    this->ClientDataServerSocketController->Send(this->BufferLengths, 
                                     this->NumberOfBuffers, 1, 23491);
    this->ClientDataServerSocketController->Send(this->Buffers, 
                                     this->BufferTotalLength, 1, 23492);
    this->ClearBuffer();
    vtkTimerLog::MarkEndEvent("Dataserver sending to client");
    }
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::ClientReceiveFromDataServer(vtkDataSet* output)
{
  vtkCommunicator* com = 0;
  com = this->ClientDataServerSocketController->GetCommunicator();
  if (com == 0)
    {
    vtkErrorMacro("Missing socket controler on cleint.");
    return;
    }

  this->ClearBuffer();
  com->Receive(&(this->NumberOfBuffers), 1, 1, 23490);
  this->BufferLengths = new vtkIdType[this->NumberOfBuffers];
  com->Receive(this->BufferLengths, this->NumberOfBuffers, 
                                  1, 23491);
  // Compute additional buffer information.
  this->BufferOffsets = new vtkIdType[this->NumberOfBuffers];
  this->BufferTotalLength = 0;
  for (int idx = 0; idx < this->NumberOfBuffers; ++idx)
    {
    this->BufferOffsets[idx] = this->BufferTotalLength;
    this->BufferTotalLength += this->BufferLengths[idx];
    }
  this->Buffers = new char[this->BufferTotalLength];
  com->Receive(this->Buffers, this->BufferTotalLength, 
                                  1, 23492);
  this->ReconstructDataFromBuffer(output);
  this->ClearBuffer();
}


//-----------------------------------------------------------------------------
void vtkMPIMoveData::RenderServerZeroBroadcast(vtkDataSet* data)
{
  (void)data; // shut up warning
  int numProcs= this->Controller->GetNumberOfProcesses();
  if (numProcs <= 1)
    {
    return;
    }

#ifdef VTK_USE_MPI
  int myId= this->Controller->GetLocalProcessId();

  vtkMPICommunicator* com = vtkMPICommunicator::SafeDownCast(
                                         this->Controller->GetCommunicator()); 

  if (com == 0)
    {
    vtkErrorMacro("MPICommunicator neededfor this operation.");
    return;
    }

  int bufferLength = 0;
  if (myId == 0)
    {
    this->ClearBuffer();
    this->MarshalDataToBuffer(data);
    bufferLength = this->BufferLengths[0];
    }
  
  // Broadcast the size of the buffer.
  com->Broadcast(&bufferLength, 1, 0);

  // Allocate buffers for all receiving nodes.
  if (myId != 0)
    {
    this->NumberOfBuffers = 1;
    this->BufferLengths = new vtkIdType[1];
    this->BufferLengths[0] = bufferLength;
    this->BufferOffsets = new vtkIdType[1];
    this->BufferOffsets[0] = 0;
    this->BufferTotalLength = this->BufferLengths[0];
    this->Buffers = new char[bufferLength];
    }

  // Broadcast the buffer.
  com->Broadcast(this->Buffers, bufferLength, 0);

  // Reconstruct the output on nodes other than 0.
  if (myId != 0)
    {
    this->ReconstructDataFromBuffer(data);
    }
  
  this->ClearBuffer();
#endif
}




//-----------------------------------------------------------------------------
void vtkMPIMoveData::ClearBuffer()
{
  this->NumberOfBuffers = 0;
  if (this->BufferLengths)
    {
    delete [] this->BufferLengths;
    this->BufferLengths = 0;
    }
  if (this->BufferOffsets)
    {
    delete [] this->BufferOffsets;
    this->BufferOffsets = 0;
    }
  if (this->Buffers)
    {
    delete [] this->Buffers;
    this->Buffers = 0;
    }
  this->BufferTotalLength = 0;
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::MarshalDataToBuffer(vtkDataSet* data)
{
  // Protect from empty data.
  if (data->GetNumberOfPoints() == 0)
    {
    this->NumberOfBuffers = 0;
    }

  // Copy input to isolate reader from the pipeline.
  vtkDataSet* d = data->NewInstance();
  d->CopyStructure(data);
  d->GetPointData()->PassData(data->GetPointData());
  d->GetCellData()->PassData(data->GetCellData());
  // Marshal with writer.
  vtkDataSetWriter *writer = vtkDataSetWriter::New();
  writer->SetFileTypeToBinary();
  writer->WriteToOutputStringOn();
  writer->SetInput(d);
  writer->Write();
  // Get string.
  this->NumberOfBuffers = 1;
  this->BufferLengths = new vtkIdType[1];
  this->BufferLengths[0] = writer->GetOutputStringLength();
  this->BufferOffsets = new vtkIdType[1];
  this->BufferOffsets[0] = 0;
  this->BufferTotalLength = this->BufferLengths[0];
  this->Buffers = writer->RegisterAndGetOutputString();

  d->Delete();
  d = 0;
  writer->Delete();
  writer = 0;
}


//-----------------------------------------------------------------------------
void vtkMPIMoveData::ReconstructDataFromBuffer(vtkDataSet* data)
{
  if (this->NumberOfBuffers == 0 || this->Buffers == 0)
    {
    data->Initialize();
    return;
    }

  // PolyData and Unstructured grid need different append filters.
  vtkAppendPolyData* appendPd = NULL;
  vtkAppendFilter*   appendUg = NULL;
  if (this->NumberOfBuffers > 1)
    {
    if (data->IsA("vtkPolyData"))
      {
      appendPd = vtkAppendPolyData::New();
      }
    else if (data->IsA("vtkUnstructuredGrid"))
      {
      appendUg = vtkAppendFilter::New();
      }
    else
      {
      vtkErrorMacro("This filter only handles unstructured data.");
      return;
      }
    }

  for (int idx = 0; idx < this->NumberOfBuffers; ++idx)
    {
    // Setup a reader.
    vtkDataSetReader *reader = vtkDataSetReader::New();
    reader->ReadFromInputStringOn();
    vtkCharArray* mystring = vtkCharArray::New();
    mystring->SetArray(this->Buffers+this->BufferOffsets[idx], 
                       this->BufferLengths[idx], 1);
    reader->SetInputArray(mystring);
    reader->Modified(); // For append loop
    reader->GetOutput()->Update();
    if (appendPd)
      { 
      appendPd->AddInput(reader->GetPolyDataOutput());
      }
    else if (appendUg)
      { 
      appendUg->AddInput(reader->GetUnstructuredGridOutput());
      }
    else
      {
      vtkDataSet* out = reader->GetOutput();
      data->CopyStructure(out);
      data->GetPointData()->PassData(out->GetPointData());
      data->GetCellData()->PassData(out->GetCellData());
      }
    mystring->Delete();
    mystring = 0;
    reader->Delete();
    reader = NULL;
    }

  if (appendPd)
    {
    vtkDataSet* out = appendPd->GetOutput();
    out->Update();
    data->CopyStructure(out);
    data->GetPointData()->PassData(out->GetPointData());
    data->GetCellData()->PassData(out->GetCellData());
    appendPd->Delete();
    appendPd = NULL;
    }
  if (appendUg)
    {
    vtkDataSet* out = appendUg->GetOutput();
    out->Update();
    data->CopyStructure(out);
    data->GetPointData()->PassData(out->GetPointData());
    data->GetCellData()->PassData(out->GetCellData());
    appendUg->Delete();
    appendUg = NULL;
    }
}

//-----------------------------------------------------------------------------
void vtkMPIMoveData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "NumberOfBuffers: " << this->NumberOfBuffers << endl;
  os << indent << "Server: " << this->Server << endl;
  os << indent << "MoveMode: " << this->MoveMode << endl;
  os << indent << "DeliverOutlineToClient : " 
    << this->DeliverOutlineToClient << endl;
  os << indent << "OutputDataType: ";
  if (this->OutputDataType == VTK_POLY_DATA)
    {
    os << "VTK_POLY_DATA";
    }
  else if (this->OutputDataType == VTK_UNSTRUCTURED_GRID)
    {
    os << "VTK_UNSTRUCTURED_GRID";
    }
  else
    {
    os << "Unrecognized output type " << this->OutputDataType;
    }
  os << endl;
  //os << indent << "MToN
}

