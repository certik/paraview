/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkProbeFilter.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProbeFilter.h"

#include "vtkCellData.h"
#include "vtkCell.h"
#include "vtkCharArray.h"
#include "vtkIdTypeArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkProbeFilter, "$Revision: 1.91.2.1 $");
vtkStandardNewMacro(vtkProbeFilter);

class vtkProbeFilter::vtkVectorOfArrays : 
  public vtkstd::vector<vtkDataArray*>
{
};

//----------------------------------------------------------------------------
vtkProbeFilter::vtkProbeFilter()
{
  this->SpatialMatch = 0;
  this->ValidPoints = vtkIdTypeArray::New();
  this->MaskPoints = vtkCharArray::New();
  this->MaskPoints->SetNumberOfComponents(1);
  this->SetNumberOfInputPorts(2);
  this->ValidPointMaskArrayName = 0;
  this->SetValidPointMaskArrayName("vtkValidPointMask");
  this->CellArrays = new vtkVectorOfArrays();
  this->NumberOfValidPoints = 0;
}

//----------------------------------------------------------------------------
vtkProbeFilter::~vtkProbeFilter()
{
  this->MaskPoints->Delete();
  this->MaskPoints = 0;
  this->ValidPoints->Delete();
  this->ValidPoints = NULL;
  this->SetValidPointMaskArrayName(0);
  delete this->CellArrays;
}

//----------------------------------------------------------------------------
void vtkProbeFilter::SetSourceConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}
 
//----------------------------------------------------------------------------
void vtkProbeFilter::SetSource(vtkDataObject *input)
{
  this->SetInput(1, input);
}

//----------------------------------------------------------------------------
vtkDataObject *vtkProbeFilter::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
    {
    return NULL;
    }
  
  return this->GetExecutive()->GetInputData(1, 0);
}

//----------------------------------------------------------------------------
int vtkProbeFilter::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *source = vtkDataSet::SafeDownCast(
    sourceInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!source)
    {
    return 0;
    }

  this->Probe(input, source, output);
  return 1;
}

//----------------------------------------------------------------------------
// * input -- dataset probed with
// * source -- dataset probed into
// * output - output.
void vtkProbeFilter::InitializeForProbing(vtkDataSet* input, vtkDataSet* source,
  vtkDataSet* output)
{
  vtkIdType numPts = input->GetNumberOfPoints();

  // Initialize valid points/mask points arrays.
  this->NumberOfValidPoints = 0;
  this->ValidPoints->Allocate(numPts);
  this->MaskPoints->SetNumberOfTuples(numPts);
  this->MaskPoints->FillComponent(0, 0);
  this->MaskPoints->SetName(this->ValidPointMaskArrayName? 
    this->ValidPointMaskArrayName: "vtkValidPointMask");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  vtkPointData *inPD, *outPD;
  vtkCellData* inCD;

  inPD = source->GetPointData();
  outPD = output->GetPointData();
  inCD = source->GetCellData();

  // Allocate storage for output PointData
  // All input PD is passed to output as PD. Those arrays in input CD that are
  // not present in output PD will be passed as output PD.
  outPD = output->GetPointData();
  outPD->InterpolateAllocate(inPD, numPts, numPts);

  this->CellArrays->clear();
  int numCellArrays = inCD->GetNumberOfArrays();
  for (int cc=0; cc < numCellArrays; cc++)
    {
    vtkDataArray* inArray = inCD->GetArray(cc);
    if (inArray && inArray->GetName() && !outPD->GetArray(inArray->GetName()))
      {
      vtkDataArray* newArray = inArray->NewInstance();
      newArray->SetName(inArray->GetName());
      newArray->SetNumberOfComponents(inArray->GetNumberOfComponents());
      newArray->Allocate(numPts);
      outPD->AddArray(newArray);
      this->CellArrays->push_back(newArray);
      newArray->Delete();
      }
    else
      {
      this->CellArrays->push_back(0);
      }
    }

  outPD->AddArray(this->MaskPoints);

  // BUG FIX: JB.
  // Output gets setup from input, but when output is imagedata, scalartype
  // depends on source scalartype not input scalartype
  if (output->IsA("vtkImageData"))
    {
    vtkImageData *out = (vtkImageData*)output;
    vtkDataArray *s = outPD->GetScalars();
    if (s)
      {
      out->SetScalarType(s->GetDataType());
      out->SetNumberOfScalarComponents(s->GetNumberOfComponents());
      }
    }
}

//----------------------------------------------------------------------------
void vtkProbeFilter::Probe(vtkDataSet *input, vtkDataSet *source,
                           vtkDataSet *output)
{
  this->InitializeForProbing(input, source, output);
  this->ProbeEmptyPoints(input, source, output);
}

//----------------------------------------------------------------------------
void vtkProbeFilter::ProbeEmptyPoints(vtkDataSet *input, vtkDataSet *source,
                                      vtkDataSet *output)
{
  vtkIdType ptId, numPts;
  double x[3], tol2;
  vtkCell *cell;
  vtkPointData *pd, *outPD;
  vtkCellData* cd;
  int subId;
  double pcoords[3], *weights;
  double fastweights[256];

  vtkDebugMacro(<<"Probing data");

  pd = source->GetPointData();
  cd = source->GetCellData();
  int numCellArrays = cd->GetNumberOfArrays();

  // lets use a stack allocated array if possible for performance reasons
  int mcs = source->GetMaxCellSize();
  if (mcs<=256)
    {
    weights = fastweights;
    }
  else
    {
    weights = new double[mcs];
    }

  numPts = input->GetNumberOfPoints();
  outPD = output->GetPointData();

  char* maskArray = this->MaskPoints->GetPointer(0);

  // Use tolerance as a function of size of source data
  //
  tol2 = source->GetLength();
  tol2 = tol2 ? tol2*tol2 / 1000.0 : 0.001;

  // Loop over all input points, interpolating source data
  //
  int abort=0;
  vtkIdType progressInterval=numPts/20 + 1;
  for (ptId=0; ptId < numPts && !abort; ptId++)
    {
    if ( !(ptId % progressInterval) )
      {
      this->UpdateProgress((double)ptId/numPts);
      abort = GetAbortExecute();
      }

    if (maskArray[ptId] == static_cast<char>(1))
      {
      // skip points which have already been probed with success.
      // This is helpful for multigroup dataset probing.
      continue;
      }

    // Get the xyz coordinate of the point in the input dataset
    input->GetPoint(ptId, x);

    // Find the cell that contains xyz and get it
    vtkIdType cellId = source->FindCell(x,NULL,-1,tol2,subId,pcoords,weights);
    if (cellId >= 0)
      {
      cell = source->GetCell(cellId);
      }
    else
      {
      cell = 0;
      }
    if (cell)
      {
      // Interpolate the point data
      outPD->InterpolatePoint(pd,ptId,cell->PointIds,weights);
      this->ValidPoints->InsertNextValue(ptId);
      this->NumberOfValidPoints++;
      for (int i=0; i<numCellArrays; i++)
        {
        vtkDataArray* inArray = cd->GetArray(i);
        if ((*this->CellArrays)[i])
          {
          outPD->CopyTuple(inArray, (*this->CellArrays)[i], cellId, ptId);
          }
        }
      maskArray[ptId] = static_cast<char>(1);
      }
    else
      {
      outPD->NullPoint(ptId);
      }
    }

  if (mcs>256)
    {
    delete [] weights;
    }
}

//----------------------------------------------------------------------------
int vtkProbeFilter::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  outInfo->CopyEntry(sourceInfo, 
                     vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->CopyEntry(sourceInfo, 
                     vtkStreamingDemandDrivenPipeline::TIME_RANGE());

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
               6);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               inInfo->Get(
                 vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES()));

  // Special case for ParaView.
  if (this->SpatialMatch == 2)
    {
    outInfo->Set(
      vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
      sourceInfo->Get(
        vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES()));
    }
  
  if (this->SpatialMatch == 1)
    {
    int m1 = 
      inInfo->Get(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES());
    int m2 = 
      sourceInfo->Get(
        vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES());
    if (m1 < 0 && m2 < 0)
      {
      outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
                   -1);
      }
    else
      {
      if (m1 < -1)
        {
        m1 = VTK_LARGE_INTEGER;
        }
      if (m2 < -1)
        {
        m2 = VTK_LARGE_INTEGER;
        }
      if (m2 < m1)
        {
        m1 = m2;
        }
      outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
                   m1);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkProbeFilter::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *sourceInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  int usePiece = 0;

  // What ever happend to CopyUpdateExtent in vtkDataObject?
  // Copying both piece and extent could be bad.  Setting the piece
  // of a structured data set will affect the extent.
  vtkDataObject* output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  if (output &&
      (!strcmp(output->GetClassName(), "vtkUnstructuredGrid") ||
       !strcmp(output->GetClassName(), "vtkPolyData")))
    {
    usePiece = 1;
    }
  
  inInfo->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
  
  if ( ! this->SpatialMatch)
    {
    sourceInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    sourceInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    sourceInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
    }
  else if (this->SpatialMatch == 1)
    {
    if (usePiece)
      {
      // Request an extra ghost level because the probe
      // gets external values with computation prescision problems.
      // I think the probe should be changed to have an epsilon ...
      sourceInfo->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
      sourceInfo->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
      sourceInfo->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS())+1);
      }
    else
      {
      sourceInfo->Set(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()), 6);
      }
    }
  
  if (usePiece)
    {
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
    }
  else
    {
    inInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()), 6);
    }
  
  // Use the whole input in all processes, and use the requested update
  // extent of the output to divide up the source.
  if (this->SpatialMatch == 2)
    {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
    sourceInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    sourceInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    sourceInfo->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkProbeFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkDataObject *source = this->GetSource();

  this->Superclass::PrintSelf(os,indent);
  os << indent << "Source: " << source << "\n";
  os << indent << "SpatialMatch: " << ( this->SpatialMatch ? "On" : "Off" ) << "\n";
  os << indent << "ValidPointMaskArrayName: " << (this->ValidPointMaskArrayName?
    this->ValidPointMaskArrayName : "vtkValidPointMask") << "\n";
  os << indent << "ValidPoints: " << this->ValidPoints << "\n";
}
