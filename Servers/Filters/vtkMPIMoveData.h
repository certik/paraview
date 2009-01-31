/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkMPIMoveData.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMPIMoveData - Moves/redistributes data between processes.
// .SECTION Description
// This class combines all the duplicate and collection requirements
// into one filter. It can move polydata and unstructured grid between
// processes. It can redistributed polydata from M to N processors.

#ifndef __vtkMPIMoveData_h
#define __vtkMPIMoveData_h

#include "vtkDataSetAlgorithm.h"

class vtkMultiProcessController;
class vtkSocketController;
class vtkMPIMToNSocketConnection;
class vtkDataSet;
class vtkIndent;

class VTK_EXPORT vtkMPIMoveData : public vtkDataSetAlgorithm
{
public:
  static vtkMPIMoveData *New();
  vtkTypeRevisionMacro(vtkMPIMoveData, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Objects for communication.
  // The controller is an MPI controller used to communicate
  // between processes within one server (render or data).
  // The client-data server socket controller is set on the client
  // and data server and is used to communicate between the two.
  // MPIMToNSocetConnection is set on the data server and render server 
  // when we are running with a render server.  It has multiple
  // sockets which are used to send data from the data server to the 
  // render server.
  // ClientDataServerController==0  => One MPI program.
  // MPIMToNSocketConnection==0 => Client-DataServer.
  // MPIMToNSocketConnection==1 => Client-DataServer-RenderServer.
  void SetController(vtkMultiProcessController* controller);
  void SetClientDataServerSocketController(vtkSocketController* sdc);
  void SetMPIMToNSocketConnection(vtkMPIMToNSocketConnection* sc);
  
  // Description:
  // Tell the object on which client/server it resides.
  // Whether the sockets are set helps determine which servers are running.
  void SetServerToClient(){this->Server=vtkMPIMoveData::CLIENT;}
  void SetServerToDataServer(){this->Server=vtkMPIMoveData::DATA_SERVER;}
  void SetServerToRenderServer(){this->Server=vtkMPIMoveData::RENDER_SERVER;}
  vtkSetClampMacro(Server, int, vtkMPIMoveData::CLIENT, vtkMPIMoveData::RENDER_SERVER);

  // Description:
  // Specify how the data is to be redistributed.
  void SetMoveModeToPassThrough(){this->MoveMode=vtkMPIMoveData::PASS_THROUGH;}
  void SetMoveModeToCollect(){this->MoveMode=vtkMPIMoveData::COLLECT;}
  void SetMoveModeToClone(){this->MoveMode=vtkMPIMoveData::CLONE;}
  vtkSetClampMacro(MoveMode, int, vtkMPIMoveData::PASS_THROUGH, vtkMPIMoveData::CLONE);

  // Description:
  // Controls the output type. This is required because processes receiving
  // data cannot know their output type in RequestDataObject without
  // communicating with other processes. Since communicating with other
  // processes in RequestDataObject is dangerous (can cause deadlock because
  // it may happen out-of-sync), the application has to set the output
  // type. The default is VTK_POLY_DATA. Currently, only VTK_UNSTRUCTURED_GRID
  // and VTK_POLY_DATA are supported. Make sure to call this before any
  // pipeline updates occur.
  vtkSetMacro(OutputDataType, int);
  vtkGetMacro(OutputDataType, int);

  // Description:
  // Legacy API for ParaView 1.4
  void SetPassThrough(int v) 
    {if(v){this->SetMoveModeToPassThrough();} else {this->SetMoveModeToClone();}}
  void SetSocketController(vtkSocketController* c) {this->SetClientDataServerSocketController(c);}

  // Description:
  // Sometimes, the data may be too huge to deliver to the client. In that case,
  // the client can request that only the outline for the data may be delivered
  // to the client. This is supported only for vtkPolyData.
  // Off by default.
  vtkSetMacro(DeliverOutlineToClient, int);
  vtkGetMacro(DeliverOutlineToClient, int);

//BTX
  enum MoveModes {
    PASS_THROUGH=0,
    COLLECT=1,
    CLONE=2
  };
//ETX
protected:
  vtkMPIMoveData();
  ~vtkMPIMoveData();

  virtual int RequestDataObject(vtkInformation* request, 
                           vtkInformationVector** inputVector, 
                           vtkInformationVector* outputVector);
  virtual int RequestInformation(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector);
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  vtkMultiProcessController* Controller;
  vtkSocketController* ClientDataServerSocketController;
  vtkMPIMToNSocketConnection* MPIMToNSocketConnection;

  void DataServerAllToN(vtkDataSet* inData, vtkDataSet* outData, int n);
  void DataServerGatherAll(vtkDataSet* input, vtkDataSet* output);
  void DataServerGatherToZero(vtkDataSet* input, vtkDataSet* output);
  void DataServerSendToRenderServer(vtkDataSet* output);
  void RenderServerReceiveFromDataServer(vtkDataSet* output);
  void DataServerZeroSendToRenderServerZero(vtkDataSet* data);
  void RenderServerZeroReceiveFromDataServerZero(vtkDataSet* data);
  void RenderServerZeroBroadcast(vtkDataSet* data);
  void DataServerSendToClient(vtkDataSet* output);
  void ClientReceiveFromDataServer(vtkDataSet* output);

  int        NumberOfBuffers;
  vtkIdType* BufferLengths;
  vtkIdType* BufferOffsets;
  char*      Buffers;
  vtkIdType  BufferTotalLength;

  void ClearBuffer();
  void MarshalDataToBuffer(vtkDataSet* data);
  void ReconstructDataFromBuffer(vtkDataSet* data);

  int MoveMode;
  int Server;

//BTX
  enum Servers {
    CLIENT=0,
    DATA_SERVER=1,
    RENDER_SERVER=2
  };
//ETX

  int OutputDataType;
  int DeliverOutlineToClient;

private:
  int UpdateNumberOfPieces;
  int UpdatePiece;

  vtkMPIMoveData(const vtkMPIMoveData&); // Not implemented
  void operator=(const vtkMPIMoveData&); // Not implemented
};

#endif

