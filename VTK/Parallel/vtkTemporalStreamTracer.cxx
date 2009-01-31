/*=========================================================================

Program:   Visualization Toolkit
Module:    $RCSfile: vtkTemporalStreamTracer.cxx,v $

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTemporalStreamTracer.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkExecutive.h"
#include "vtkGenericCell.h"
#include "vtkIdList.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPolyData.h"
#include "vtkPolyLine.h"
#include "vtkRungeKutta2.h"
#include "vtkRungeKutta4.h"
#include "vtkRungeKutta45.h"
#include "vtkSmartPointer.h"
#include "vtkTemporalInterpolatedVelocityField.h"
#include "vtkTemporalDataSet.h"
#include "vtkOutputWindow.h"
#include "vtkAbstractParticleWriter.h"

#include "vtkToolkits.h" // For VTK_USE_MPI 

#undef JB_H5PART_PARTICLE_OUTPUT
#ifdef JB_H5PART_PARTICLE_OUTPUT
  #include "vtkH5PartWriter.h"
#endif

#ifdef VTK_USE_MPI
  #include "vtkMPIController.h"
#endif

#include <functional>
#include <algorithm>

using namespace vtkTemporalStreamTracerNamespace;

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkTemporalStreamTracer, "$Revision: 1.23 $");
vtkStandardNewMacro(vtkTemporalStreamTracer);
vtkCxxSetObjectMacro(vtkTemporalStreamTracer, Controller, vtkMultiProcessController);
vtkCxxSetObjectMacro(vtkTemporalStreamTracer, ParticleWriter, vtkAbstractParticleWriter);
//---------------------------------------------------------------------------
// @Todo - use MPI to ensure Ids are really unique across processors
vtkIdType vtkTemporalStreamTracer::UniqueIdCounter = 0;
//---------------------------------------------------------------------------
vtkTemporalStreamTracer::vtkTemporalStreamTracer()
{
  this->IntegrationDirection        = FORWARD;
  this->TimeStep                    = 0;
  this->ActualTimeStep              = 0;
  this->NumberOfInputTimeSteps      = 0;
  this->ForceReinjectionEveryNSteps = 1;
  this->ReinjectionCounter          = 0;
  this->ReinjectionFlag             = 0;
  this->UpdatePiece                 = 0;
  this->UpdateNumPieces             = 0;
  this->EnableSource1               = 1;
  this->EnableSource2               = 1;
  this->AllFixedGeometry            = 0;
  this->NoFixedGeometry             = 1;
  this->ComputeVorticity            = 1;
  this->IgnorePipelineTime          = 0;
  this->ParticleWriter              = NULL;
  this->ParticleFileName            = NULL;
  this->EnableParticleWriting       = false;
  //
  this->MaxCellSize           = 0;
  this->NumberOfParticles     = 0;
  this->TimeStepResolution    = 1.0;
  this->EarliestTime          = -1E6;
  // we are not actually using these for now
  this->MaximumPropagation.Unit         = TIME_UNIT;
  this->MaximumPropagation.Interval     = 1.0;
  this->MinimumIntegrationStep.Unit     = TIME_UNIT;
  this->MinimumIntegrationStep.Interval = 1.0E-2;
  this->MaximumIntegrationStep.Unit     = TIME_UNIT;
  this->MaximumIntegrationStep.Interval = 1.0;
  this->InitialIntegrationStep.Unit     = TIME_UNIT;
  this->InitialIntegrationStep.Interval = 0.5;
  //
  this->SetNumberOfInputPorts(3);
  this->Controller = NULL;
  this->SetController(vtkMultiProcessController::GetGlobalController());
#ifdef JB_H5PART_PARTICLE_OUTPUT
//  vtkDebugMacro(<<"Setting vtkH5PartWriter");
//  vtkH5PartWriter *writer = vtkH5PartWriter::New();
//  vtkDebugMacro(<<"Setting vtkXMLParticleWriter");
//  vtkXMLParticleWriter *writer = vtkXMLParticleWriter::New();
//  this->SetParticleWriter(writer);
//  writer->Delete();
#endif
}
//---------------------------------------------------------------------------
vtkTemporalStreamTracer::~vtkTemporalStreamTracer()
{
  this->SetController(NULL);
  this->SetParticleWriter(NULL);
  if (this->ParticleFileName)
  {
    delete []this->ParticleFileName;
    this->ParticleFileName = NULL;
  }
}
//----------------------------------------------------------------------------
int vtkTemporalStreamTracer::FillInputPortInformation(
  int port, 
  vtkInformation* info)
{
  // port 0 must be temporal data, but port 1 can be any dataset
  if (port==0) {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTemporalDataSet");
  }
  else if (port==1) {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  else if (port==2) {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  }
  return 1;
}
//----------------------------------------------------------------------------
void vtkTemporalStreamTracer::SetSource2Connection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(2, algOutput);
}
//----------------------------------------------------------------------------
void vtkTemporalStreamTracer::SetSource2(vtkDataSet *source)
{
  this->SetInput(2, source);
}
//----------------------------------------------------------------------------
vtkDataSet *vtkTemporalStreamTracer::GetSource2()
{
  if (this->GetNumberOfInputConnections(2) < 1)
    {
    return 0;
    }
  return vtkDataSet::SafeDownCast(
    this->GetExecutive()->GetInputData(2, 0));
}
//----------------------------------------------------------------------------
int vtkTemporalStreamTracer::ProcessRequest(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    return this->RequestInformation(request, inputVector, outputVector);       
    }
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
      return this->RequestUpdateExtent(request, inputVector, outputVector);
    }
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    return this->RequestData(request, inputVector, outputVector);       
    }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}
//----------------------------------------------------------------------------
int vtkTemporalStreamTracer::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo  = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
    {
    this->NumberOfInputTimeSteps = inInfo->Length( 
      vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
    vtkDebugMacro(<<"vtkTemporalStreamTracer "
      "inputVector TIME_STEPS " << this->NumberOfInputTimeSteps);
    //
    // Get list of input time step values
    this->InputTimeValues.resize(this->NumberOfInputTimeSteps);
    inInfo->Get( vtkStreamingDemandDrivenPipeline::TIME_STEPS(), 
      &this->InputTimeValues[0] );
    if (this->NumberOfInputTimeSteps==1) 
      { 
      vtkErrorMacro(<<"Not enough input time steps for particle integration");
      return 0;
      }
    //
    // We only output T-1 time steps   
    //
    this->OutputTimeValues.resize(this->NumberOfInputTimeSteps-1);
    this->OutputTimeValues.clear();
    this->OutputTimeValues.insert(
      this->OutputTimeValues.begin(), 
      this->InputTimeValues.begin()+1, this->InputTimeValues.end());
  }
  else 
    {
    vtkErrorMacro(<<"Input information has no TIME_STEPS set");
    return 0;
    }
  
  outInfo->Set(
    vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), 
               &this->OutputTimeValues[0], this->OutputTimeValues.size());

  return 1;
}
//----------------------------------------------------------------------------
class WithinTolerance: public vtkstd::binary_function<double, double, bool>
{
public:
    result_type operator()(first_argument_type a, second_argument_type b) const
    {
      bool result = (fabs(a-b)<=(a*1E-6));
      return (result_type)result;
    }
};
//----------------------------------------------------------------------------
int vtkTemporalStreamTracer::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation  *inInfo = inputVector[0]->GetInformationObject(0);

  //
  // The output has requested a time value, what times must we ask from our input
  //
  double requestedTimeValue;

  if (this->IgnorePipelineTime || !outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS())) 
    {
    //
    // ideally we want the output information to be requesting a time step,
    // but since it isn't we must use the SetTimeStep value as a Time request
    //
    requestedTimeValue = this->OutputTimeValues[this->TimeStep];
    this->ActualTimeStep = this->TimeStep;

    vtkDebugMacro(<< "SetTimeStep       : requestedTimeValue " 
      << requestedTimeValue << " ActualTimeStep " << this->ActualTimeStep);
    } 
  else 
    {
    //
    // Get the requested time step. 
    //
    double *requestedTimeValues = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    requestedTimeValue = requestedTimeValues[0];
    this->ActualTimeStep = vtkstd::find_if(
      this->OutputTimeValues.begin(), 
      this->OutputTimeValues.end(), 
      vtkstd::bind2nd( WithinTolerance( ), requestedTimeValue )) 
      - this->OutputTimeValues.begin();
    if (this->ActualTimeStep>=this->OutputTimeValues.size())
    {
      this->ActualTimeStep = 0;
    }
    vtkDebugMacro(<< "UPDATE_TIME_STEPS : requestedTimeValue " 
      << requestedTimeValue << " ActualTimeStep " << this->ActualTimeStep);
    }

  if (this->ActualTimeStep<this->OutputTimeValues.size()) 
    {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(), 
      &this->InputTimeValues[this->ActualTimeStep], 2);
    // our timestep T is timestep T+1 in the source
    // so output inputTimeSteps[T], inputTimeSteps[T+1]
    vtkDebugMacro(<< "requested 2 time values : " 
      << this->InputTimeValues[this->ActualTimeStep] << " "  
      << this->InputTimeValues[this->ActualTimeStep+1]);
    }
  else
  {
    vtkDebugMacro(<< "UPDATE_TIME_STEPS : Error getting requested time step");
    return 0;
  }

  return 1;
}  
//---------------------------------------------------------------------------
int vtkTemporalStreamTracer::InitializeInterpolator()
{
  //
  // When Multiblock arrays are processed, some may be empty
  // if the first is empty, we won't find the correct vector name
  // so scan until we get one
  //
  vtkCompositeDataIterator* I = this->InputDataT[0]->NewIterator();
  vtkSmartPointer<vtkCompositeDataIterator> iterP(I);
  I->Delete();
  iterP->GoToFirstItem();
  char *vecname = NULL;
  while (!iterP->IsDoneWithTraversal())
    {
    vtkDataArray *vectors = this->GetInputArrayToProcess(
      0,iterP->GetCurrentDataObject());
    if (vectors)
      {
      vecname = vectors->GetName();
      }
    iterP->GoToNextItem();
    }
  if (!vecname)
    {
    vtkDebugMacro(<< "Couldn't find vector array " << vecname);
    return VTK_ERROR;
    }

  vtkDebugMacro(<< "Interpolator using array " << vecname);
  this->Interpolator->SelectVectors(vecname);
 
  this->AllFixedGeometry = 1;
  this->NoFixedGeometry  = 1;

  int numInputs[2] = {0, 0};
  this->DataReferenceT[0] = this->DataReferenceT[1] = 0;
  for (int i=0; i<2; i++) {
    this->CachedBounds[i].clear();
    this->GeometryFixed[i].clear();
    vtkCompositeDataIterator* iter = this->InputDataT[i]->NewIterator();
    vtkSmartPointer<vtkCompositeDataIterator> anotherIterP(iter);
    iter->Delete();  

    // Add all the inputs ( except source, of course ) which
    // have the appropriate vectors and compute the maximum
    // Cell size.
    anotherIterP->GoToFirstItem();
    while (!anotherIterP->IsDoneWithTraversal())
      {
      vtkDataSet* inp = vtkDataSet::SafeDownCast(anotherIterP->GetCurrentDataObject());
      if (inp)
        {
        if (!inp->GetPointData()->GetVectors(vecname))
          {
          vtkDebugMacro(
            "One of the input blocks does not contain a velocity vector.");
          }
        else 
          {
          int cellSize = inp->GetMaxCellSize();
          if ( cellSize > this->MaxCellSize )
            {
            this->MaxCellSize = cellSize;
            }
          this->Interpolator->AddDataSetAtTime(i, this->CurrentTimeSteps[i], inp);
          if (!this->DataReferenceT[i]) {
            this->DataReferenceT[i] = inp;
          }

          //
          // store the bounding boxes of each dataset for faster points testing
          //
          bounds bbox;
          inp->ComputeBounds();
          inp->GetBounds(&bbox.b[0]);
          this->CachedBounds[i].push_back(bbox);
          if (inp->GetInformation()->Has(vtkDataObject::DATA_GEOMETRY_UNMODIFIED()))
            {
            this->GeometryFixed[i].push_back(1);
            this->NoFixedGeometry = 0;
            }
          else {
            this->GeometryFixed[i].push_back(0);
            this->AllFixedGeometry = 0;
            }
          //
          numInputs[i]++;
          }
        }
      anotherIterP->GoToNextItem();
      }
  }
  if (numInputs[0]==0 || numInputs[1]==0)
    {
    vtkDebugMacro("Not enough inputs have been found. Can not execute." << numInputs[0] << " " << numInputs[1]);
    return VTK_ERROR;
    }
  if (numInputs[0] != numInputs[1])
    {
    vtkDebugMacro("The number of datasets is different between time steps " << numInputs[0] << " " << numInputs[1]);
    return VTK_ERROR;
    }
  return VTK_OK;
}
//---------------------------------------------------------------------------
int vtkTemporalStreamTracer::SetupInputs(vtkInformation* inInfo, 
                                 vtkInformation* vtkNotUsed(outInfo))
{
  this->InputData = NULL;
  vtkTemporalDataSet *td = vtkTemporalDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!td)
    {
    vtkDebugMacro(<<"Not a temporal data input " << 
      inInfo->Get(vtkDataObject::DATA_OBJECT())->GetClassName());
    return 0;
    }
  if (td->GetNumberOfGroups()<2)
    {
    vtkDebugMacro(<<"Input didn't have 2 timesteps/groups");
    return 1;
    }

  vtkDataObject *input[2];
  input[0] = td->GetDataSet(0,0);
  input[1] = td->GetDataSet(1,0);
  for (int i=0; i<2; i++) 
    {
    vtkCompositeDataSet *hdInput = vtkCompositeDataSet::SafeDownCast(input[i]);
    vtkDataSet* dsInput = vtkDataSet::SafeDownCast(input[i]);
    if (hdInput) 
      {
      this->InputDataT[i] = hdInput;
      this->InputDataT[i]->Register(this);
      }
    else if (dsInput)
      {
      vtkDataSet* copy = dsInput->NewInstance();
      copy->ShallowCopy(dsInput);
      vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::New();
      this->InputDataT[i] = mb;
      mb->SetNumberOfBlocks(1);
      mb->SetNumberOfDataSets(0, 1);
      mb->SetDataSet(0, 0, copy);
      copy->Delete();
      }
    else
      {
      vtkErrorMacro("This filter cannot handle input of type: "
                    << (input[i]?input[i]->GetClassName():"(none)"));
      return 0;
      }
    }
  return 1;
}
//---------------------------------------------------------------------------
bool vtkTemporalStreamTracer::InsideBounds(double point[]) 
{
  double delta[3] = { 0.0, 0.0, 0.0 };
  for (int t=0; t<2; ++t) {
    for (unsigned int i=0; i<(this->CachedBounds[t].size()); ++i) {
      if (vtkMath::PointIsWithinBounds(point, 
        &((this->CachedBounds[t])[i].b[0]), delta)) return true;
    }
  }
  return false;
}
//---------------------------------------------------------------------------
void vtkTemporalStreamTracer::InjectSeeds(
  vtkDataSet *source, int sourceID, int injectionID, 
  ParticleList *inputlist,
  ParticleList &candidates, 
  ParticleList *outofdomain)
{
  int numSeedsNew = 0, successful = 0;
  int valid = candidates.size();
  int outofdom = outofdomain ? outofdomain->size() : 0;
  if (source) {
    numSeedsNew = source->GetNumberOfPoints();
  } 
  else if (inputlist) {
    numSeedsNew = inputlist->size();
  }
  if (numSeedsNew==0) return;
  //
  candidates.resize(valid + numSeedsNew);
  if (outofdomain) outofdomain->resize(outofdom + numSeedsNew);
  //
  // Test to see if they are inside our zone, add them to candidates
  //
  double *pos;
  for (int i=0; i<numSeedsNew; i++) {
    if (source) pos = source->GetPoint(i);
    else pos = &((*inputlist)[i].CurrentPosition.x[0]);
    //
    int ID;
    if (!this->InsideBounds(pos)) ID=ID_OUTSIDE_ALL;
    else {
      // if the point is valid, then this will set the cache ids and datasets
      ID = this->Interpolator->TestPoint(pos);
    }
    //
    if (ID!=ID_INSIDE_ALL) {
      if (outofdomain) {
        ParticleInformation &info = (*outofdomain)[outofdom];
        if (source) {
          info.Counter          = 0;
          info.Index            = 0;
          info.Wrap             = 0;
          info.CachedDataSet[0] = 0;
          info.CachedDataSet[1] = 0;
          info.CachedCellId[0]  =-1;
          info.CachedCellId[1]  =-1;
          info.SourceID         = sourceID;
          info.InjectedPointId  = injectionID + i;
          info.UniqueParticleId = vtkTemporalStreamTracer::UniqueIdCounter++;
          info.rotation         = 0.0;
          info.angularVel       = 0.0;
          info.time             = 0.0;
          memcpy(&(info.CurrentPosition.x[0]), pos, sizeof(Position));
          // if it was injected, then set the time
          info.CurrentPosition.x[3] = this->CurrentTimeSteps[0];
        }
        else {
          memcpy(&info, &((*inputlist)[i]), sizeof(ParticleInformation));
          info.Counter          = 0;
          info.Index            = 0;
          info.Wrap             = 0;
          info.CachedDataSet[0] = 0;
          info.CachedDataSet[1] = 0;
          info.CachedCellId[0]  =-1;
          info.CachedCellId[1]  =-1;
        }
      }
      outofdom++;
    }
    else {
      ParticleInformation &info = candidates[valid];
      // get the cached ids and datasets from earlier TestPoint call
      if (source) {
        this->Interpolator->GetCachedCellIds(info.CachedCellId, info.CachedDataSet);
        info.Counter          = 0;
        info.Index            = 0;
        info.Wrap             = 0;
        info.SourceID         = sourceID;
        info.InjectedPointId  = injectionID + i;
        info.UniqueParticleId = vtkTemporalStreamTracer::UniqueIdCounter++;
        info.rotation         = 0.0;
        info.angularVel       = 0.0;
        info.time             = 0.0;
        memcpy(&(info.CurrentPosition.x[0]), pos, sizeof(Position));
        // if it was injected, then set the time
        info.CurrentPosition.x[3] = this->CurrentTimeSteps[0];
      }
      else {
        memcpy(&info, &(*inputlist)[i], sizeof(ParticleInformation));
        this->Interpolator->GetCachedCellIds(info.CachedCellId, info.CachedDataSet);
        info.Counter          = 0;
        info.Index            = 0;
        info.Wrap             = 0;
      }
      valid++;
      successful++;
    }
  }
  candidates.resize(valid);
  if (outofdomain) outofdomain->resize(outofdom);
  vtkDebugMacro(<< "Tested " << numSeedsNew << " Good " << successful << " Total " << valid);
}
//---------------------------------------------------------------------------
#ifdef VTK_USE_MPI
void vtkTemporalStreamTracer::TransmitReceiveParticles(
  ParticleList &outofdomain, ParticleList &received, bool removeself)
{
  vtkMPICommunicator* com = vtkMPICommunicator::SafeDownCast(
    this->Controller->GetCommunicator()); 
  if (com == 0) {
    vtkErrorMacro("MPICommunicator neededfor this operation.");
    return;
  }
  // 
  // We must allocate buffers for all processor particles
  // 
  vtkIdType OurParticles = outofdomain.size();
  int TotalParticles = 0;
  // setup arrays used by the AllGatherV call.
  vtkstd::vector<vtkIdType> recvLengths(this->UpdateNumPieces, 0);
  vtkstd::vector<vtkIdType> recvOffsets(this->UpdateNumPieces, 0);
  // Broadcast and receive size to/from all other processes.
  com->AllGather(&OurParticles, &recvLengths[0], 1);
  // Compute the displacements.
  const int TypeSize = sizeof(ParticleInformation);
  for (int i=0; i<this->UpdateNumPieces; ++i)
  {
    //  << i << ": " << recvLengths[i] << "   ";
    recvOffsets[i] = TotalParticles*TypeSize;
    TotalParticles += recvLengths[i];
    recvLengths[i] *= TypeSize;
  }
  //  << '\n';
  // Allocate the space for all particles
  received.resize(TotalParticles);
  if (TotalParticles==0) return;
  // Gather the data from all procs.
  char *sendbuf = (char*) ((outofdomain.size()>0) ? &(outofdomain[0]) : NULL);
  char *recvbuf = (char*) (&(received[0]));
  com->AllGatherV(sendbuf, recvbuf, 
    OurParticles*TypeSize, &recvLengths[0], &recvOffsets[0]);
  // Now all particles from all processors are in one big array
  // remove any from ourself that we have already tested
  if (removeself) {
    vtkstd::vector<ParticleInformation>::iterator first = 
      received.begin() + recvOffsets[this->UpdatePiece]/TypeSize;
    vtkstd::vector<ParticleInformation>::iterator last = 
      first + recvLengths[this->UpdatePiece]/TypeSize;
    received.erase(first, last);
  }
}
#else // VTK_USE_MPI
void vtkTemporalStreamTracer::TransmitReceiveParticles(
  ParticleList &, ParticleList &, bool )
{
}
#endif // VTK_USE_MPI
//---------------------------------------------------------------------------
void vtkTemporalStreamTracer::UpdateSeeds(ParticleList &candidates)
{
  int numSeedsNew = candidates.size();
  // 
  for (int i=0; i<numSeedsNew; i++) {
    // allocate a new particle on the list and get a reference to it
    ParticleInformation Info;
    this->ParticleHistories.push_back(Info);
    ParticleInformation &info = ParticleHistories.back();
    //
    memcpy(&info, &candidates[i], sizeof(ParticleInformation));
  }
  this->NumberOfParticles = ParticleHistories.size();
}
//---------------------------------------------------------------------------
int vtkTemporalStreamTracer::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
#ifdef VTK_USE_MPI
  this->Controller->Barrier();
#endif

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  this->UpdatePiece =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  this->UpdateNumPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  if (!this->SetupInputs(inInfo, outInfo))
    {
    vtkDebugMacro(<<"SetupInputs failed");
    return 0;
    }

  vtkDataSet *source1 = NULL;
  vtkDataSet *source2 = NULL;
  // To get around problems when running in parallel, we only
  // accept source objects from process zero for now: @ToDO
  vtkInformation *sourceInfo1 = inputVector[1]->GetInformationObject(0);
  if (sourceInfo1 && this->EnableSource1) 
    {
    source1 = vtkDataSet::SafeDownCast(
      sourceInfo1->Get(vtkDataObject::DATA_OBJECT()));
    }

  vtkInformation *sourceInfo2 = inputVector[2]->GetInformationObject(0);
  if (sourceInfo2 && this->EnableSource2) 
    {
    source2 = vtkDataSet::SafeDownCast(
      sourceInfo2->Get(vtkDataObject::DATA_OBJECT()));
    }

  if (this->IntegrationDirection != FORWARD) 
  {
    vtkErrorMacro(<<"We can only handle forward time particle tracking at the moment");
    return 1;
  }

  if (this->MaximumPropagation.Unit != TIME_UNIT)
  {
    vtkErrorMacro(<<"We can only handle TIME_UNIT propagation steps at the moment");
    return 1;
  }

  //
  // Get the timestep information for this instant
  //
  vtkTemporalDataSet* td = vtkTemporalDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkInformation *doInfo = td->GetInformation();
  vtkstd::vector<double> timesteps;
  if (doInfo->Has(vtkDataObject::DATA_TIME_STEPS()))
  {
    int NumberOfDataTimeSteps = doInfo->Length(vtkDataObject::DATA_TIME_STEPS());
    if (NumberOfDataTimeSteps<2) {
      vtkErrorMacro(<<"Not enough time steps in input data");
      return 1;
    }
    timesteps.resize(NumberOfDataTimeSteps);
    doInfo->Get(vtkDataObject::DATA_TIME_STEPS(), &timesteps[0]);
  }
  else {
    return 0;
  }
  this->CurrentTimeSteps[0] = timesteps[0];//*this->TimeStepResolution;
  this->CurrentTimeSteps[1] = timesteps[1];//*this->TimeStepResolution;

  //
  // Make sure the input datasets are OK and copy the actual datasets into
  // the velocity field interpolator objects. Watch out, because the
  // input datasets (almost certainly) change every time step, 
  // we have to reset everything to avoid bad cache tests inside the Interpolators
  // @ToDo, add possibility of fixed geometry caching some interesting stuff
  //
  this->Interpolator = vtkSmartPointer<vtkTemporalInterpolatedVelocityField>::New();
  if (this->InitializeInterpolator() != VTK_OK)
  {
    if (this->InputDataT[0]) this->InputDataT[0]->UnRegister(this);
    if (this->InputDataT[1]) this->InputDataT[1]->UnRegister(this);
    return 1;
  }

  // if we know that all datasets have the DATA_GEOMETRY_UNMODIFIED
  // key set, then make the most of caching in the interpolator
  this->Interpolator->SetGeometryFixed(this->AllFixedGeometry);

  //
  // Make sure the Particle Positions are initialized with Seed particles
  //
  this->ReinjectionFlag = 0;
  if (this->ForceReinjectionEveryNSteps>0) {
    if ((this->ActualTimeStep%this->ForceReinjectionEveryNSteps)==0) 
    {
      this->ReinjectionFlag = 1;
    }
  }
  //
  // If T=0 reset everything to allow us to setup stuff then start an animation
  // with a clean slate
  //
  if (this->ActualTimeStep==0) {
    this->ParticleHistories.clear();
    this->EarliestTime    = -1E6;
    this->ReinjectionFlag = 1;
    this->UniqueIdCounter = 0;
  }
  else if (this->CurrentTimeSteps[0]<this->EarliestTime) {
    //
    // We don't want to go back in time, so just reuse whatever we have
    //
//    this->GenerateOutputLines(output);
    vtkDebugMacro("skipping particle tracking because we have seen this timestep before");
    outInfo->Set(vtkDataObject::DATA_TIME_STEPS(), 
      &this->InputTimeValues[this->ActualTimeStep], 1);
    if (this->InputDataT[0]) this->InputDataT[0]->UnRegister(this);
    if (this->InputDataT[1]) this->InputDataT[1]->UnRegister(this);
    return 1;
  }
  this->EarliestTime = (this->CurrentTimeSteps[0]>this->EarliestTime)
    ? this->CurrentTimeSteps[0] : this->EarliestTime;
  //
  //
  //
  if ((source1 && source1->GetMTime()>this->ParticleInjectionTime) || 
      (source2 && source2->GetMTime()>this->ParticleInjectionTime)) 
  {
//    this->ReinjectionFlag = 1;
  }
  //
  // Lists for seed particles
  //
  ParticleList candidates;
  ParticleList outofdomain;
  ParticleList received;
  //
  if (this->ReinjectionFlag) {
    int injectionId = source1 ? source1->GetNumberOfPoints() : 0;
    if (source1 /*&& this->UpdatePiece==0*/) 
      this->InjectSeeds(source1, 1,           0, NULL, candidates, &outofdomain);
    if (source2 /*&& this->UpdatePiece==0*/) 
      this->InjectSeeds(source2, 2, injectionId, NULL, candidates, &outofdomain);
    this->ParticleInjectionTime.Modified();
    //
    // Any injected particles have been classified as "in" or "out", so now
    // send the "out" ones to other processes and collect any they might have sent to us.
    //
    if (this->UpdateNumPieces>1) {
      this->TransmitReceiveParticles(outofdomain, received, true);
      // don't want the ones that we sent away
      outofdomain.clear();
      // classify all the ones we received
      this->InjectSeeds(NULL, 0, 0, &received, candidates, NULL);
      // free up unwanted memory
      received.clear();
    }
    // Now update our main list with the ones we are keeping
    this->UpdateSeeds(candidates);
    // free up unwanted memory
    candidates.clear();
  }

  //
  // Setup some variables
  //
  vtkSmartPointer<vtkInitialValueProblemSolver> integrator = this->GetIntegrator()->NewInstance();
  integrator->SetFunctionSet(this->Interpolator);
  integrator->Delete();

  //
  // setup all our output arrays
  //
  vtkPolyData *output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  this->ParticleTime        = vtkSmartPointer<vtkFloatArray>::New();
  this->ParticleIds         = vtkSmartPointer<vtkIntArray>::New();
  this->ParticleSourceIds   = vtkSmartPointer<vtkIntArray>::New();
  this->InjectedPointIds    = vtkSmartPointer<vtkIntArray>::New();
  this->ParticleVorticity   = vtkSmartPointer<vtkFloatArray>::New();
  this->ParticleRotation    = vtkSmartPointer<vtkFloatArray>::New();
  this->ParticleAngularVel  = vtkSmartPointer<vtkFloatArray>::New();
  this->cellVectors         = vtkSmartPointer<vtkDoubleArray>::New();
  this->ParticleCells       = vtkSmartPointer<vtkCellArray>::New();
  this->OutputCoordinates   = vtkSmartPointer<vtkPoints>::New();
  this->OutputPointData     = output->GetPointData();
  this->OutputPointDataT1   = vtkSmartPointer<vtkPointData>::New();
  this->OutputPointDataT2   = vtkSmartPointer<vtkPointData>::New();
  this->OutputPointData->Initialize();
  this->OutputPointDataT1->InterpolateAllocate(this->DataReferenceT[0]->GetPointData());
  this->OutputPointDataT2->InterpolateAllocate(this->DataReferenceT[1]->GetPointData());
  this->OutputPointData->InterpolateAllocate(this->DataReferenceT[1]->GetPointData());
  //
  this->ParticleTime->SetName("ParticleTime");
  this->ParticleIds->SetName("ParticleId");
  this->ParticleSourceIds->SetName("SourceId");
  this->InjectedPointIds->SetName("InjectedPointId");

  if (this->ComputeVorticity)
    {
    this->cellVectors->SetNumberOfComponents(3);
    this->cellVectors->Allocate(3*VTK_CELL_SIZE);   
    this->ParticleVorticity->SetName("Vorticity");
    this->ParticleRotation->SetName("Rotation");
    this->ParticleAngularVel->SetName("AngularVelocity");
    }

  output->SetPoints(this->OutputCoordinates);
  output->SetVerts(this->ParticleCells);

  //
  // Perform 2 passes
  // Pass 0 : Particles created by a source in this process 
  // or received from a source in another process are integrated.
  //
  // Pass 1 : Particles that were sent in mid integration from another process
  // are added in and their integration continued here. In actual fact, the process
  // should be repeated until all particles are finished, but the chances of 
  // a particle stepping inside and out again through a single domain 
  // in one time step are small (hopefully!)

  this->MPISendList.clear();
  int Number = this->ParticleHistories.size();
  ParticleIterator it_first=this->ParticleHistories.begin();
  ParticleIterator  it_last=this->ParticleHistories.end();
  for (int pass=0; pass<2; pass++) {
    vtkDebugMacro(<<"Beginning Pass " << pass << " with " << Number << " Particles");
    // Traverse the list - but keep the next iterator handy because if a particle
    // passes out of the domain, the iterator will be deleted.
    for (ParticleIterator it=it_first; it!=it_last;)
    {
      ParticleIterator next = it;
      next++;
      //
      if (pass==1) {
        vtkDebugMacro(<<"In Pass " << pass << " with Particle " << it->UniqueParticleId);
      }
      this->IntegrateParticle(it, this->CurrentTimeSteps[0], 
        this->CurrentTimeSteps[1], integrator);
      //
      if (this->GetAbortExecute()) {
        break;
      }
      it = next;
    }
    // particles will have been deleted so now mark the new iterator positions
    // ready for the second pass after new particles are added    
    if (Number>0) {
      it_first = --this->ParticleHistories.end();
    }
    // Send and receive any particles which exited/entered the domain
    if (this->UpdateNumPieces>1 && pass==0) {
      // the Particle lists will grow if any are received
      // so we must be very careful with our iterators
      vtkDebugMacro(<<"Pass 0 about to Transmit receive");
      this->TransmitReceiveParticles(this->MPISendList, received, true);
      // don't want the ones that we sent away
      this->MPISendList.clear();
      // classify all the ones we received
      this->InjectSeeds(NULL, 0, 0, &received, candidates, NULL);
      received.clear();
      // Now update our main list with the ones we are keeping
      this->UpdateSeeds(candidates);
      it_last = this->ParticleHistories.end();
      if (Number>0) it_first++;
      else it_first = this->ParticleHistories.begin();
      // free up unwanted memory
      Number = candidates.size();
      candidates.clear();
    }
  }
  if (this->MPISendList.size()>0) {
    vtkDebugMacro(<<"MPISendList not empty " << this->MPISendList.size());
  }

  //
  // We must only add these scalar arrays at the end because the 
  // existing scalars on the input get interpolated during iteration
  // over the particles
  // 
  this->OutputPointData->AddArray(this->ParticleIds);
  this->OutputPointData->AddArray(this->ParticleSourceIds);
  this->OutputPointData->AddArray(this->InjectedPointIds);
  this->OutputPointData->AddArray(this->ParticleTime);
  if (this->ComputeVorticity)
    {
    this->OutputPointData->AddArray(this->ParticleVorticity);
    this->OutputPointData->AddArray(this->ParticleRotation);
    this->OutputPointData->AddArray(this->ParticleAngularVel);
    }
  //
  outInfo->Set(vtkDataObject::DATA_TIME_STEPS(), 
    &this->InputTimeValues[this->ActualTimeStep], 1);
  //
  if (this->InputDataT[0]) this->InputDataT[0]->UnRegister(this);
  if (this->InputDataT[1]) this->InputDataT[1]->UnRegister(this);

  //
  // Write Particles out if necessary
  //
  // NB. We don't want our writer to trigger any updates, 
  // so shallow copy the output
  if (this->ParticleWriter && this->EnableParticleWriting) {
    vtkDebugMacro(<< "Waiting to write particle data for step " << this->ActualTimeStep);
    #ifdef VTK_USE_MPI
//      this->Controller->Barrier();
    #endif
    vtkSmartPointer<vtkPolyData> polys = vtkSmartPointer<vtkPolyData>::New();
    polys->ShallowCopy(output);
    this->ParticleWriter->SetFileName(this->ParticleFileName);
    this->ParticleWriter->SetTimeStep(this->ActualTimeStep);
    this->ParticleWriter->SetTimeValue(this->CurrentTimeSteps[1]);
    this->ParticleWriter->SetInput(polys);
    this->ParticleWriter->Write();
    this->ParticleWriter->CloseFile();
    this->ParticleWriter->SetInput(NULL);
  }
  return 1;
}
//---------------------------------------------------------------------------
void vtkTemporalStreamTracer::GenerateOutputLines(vtkPolyData *vtkNotUsed(output))
{
/*
  vtkDebugMacro(<<"GenerateOutputLines");
  //
  // Now create this->GenericCell array for POLY_LINE representation
  //
  // init our local variables for the cell array generation
  this->ParticleCells     = vtkSmartPointer<vtkCellArray>::New();
  this->OutputCoordinates = vtkSmartPointer<vtkPoints>::New();
  this->ParticleIds = vtkSmartPointer<vtkFloatArray>::New();
  this->ParticleSourceIds = vtkSmartPointer<vtkFloatArray>::New();
  this->InjectedPointIds = vtkSmartPointer<vtkFloatArray>::New();
  this->ParticleIds->SetName("ParticleId");
  this->ParticleSourceIds->SetName("SourceId");
  this->InjectedPointIds->SetName("InjectedPointId");
  vtkIdType tempId; // only need 1 now we have removed trails
  vtkIdType Np = this->ParticleHistories.size();
  vtkIdType *cells = this->ParticleCells->WritePointer(Np, Np*2);
  //
  vtkIdType index = 0;
  for (ParticleIterator 
    it=this->ParticleHistories.begin();
    it!=this->ParticleHistories.end(); ++it, ++index)
  {
    ParticleInformation &info = (*it);
    // create Point Id's 
    double *coord = &info.CurrentPosition.x[0];
    tempId = this->OutputCoordinates->InsertNextPoint(coord);
    ParticleIds->InsertNextTuple1(info.UniqueParticleId);
    ParticleSourceIds->InsertNextTuple1(info.SourceID);
    InjectedPointIds->InsertNextTuple1(info.InjectedPointId);
    cells[index*2]   = 1;
    cells[index*2+1] = tempId;
  }

//  vtkSmartPointer<vtkCellArray> verts = vtkSmartPointer<vtkCellArray>::New();
//  for (int i=0; i<this->MysteryCoordinates.size(); i++) {
//    vtkIdType pointId = this->ParticleCoordinates->InsertNextPoint(&this->MysteryCoordinates[i].x[0]);
//    verts->InsertNextCell(1, &pointId);
//  }

  output->Modified();
  output->GetCellData()->Initialize();
  output->GetPointData()->Initialize();
  // Add point data scalars here
  output->GetPointData()->AddArray(ParticleIds);
  output->GetPointData()->AddArray(ParticleSourceIds);
  output->GetPointData()->AddArray(InjectedPointIds);
  //
  output->SetPoints(this->OutputCoordinates);
//  if (this->MaxTrackLength>1) {
//    output->SetLines(this->ParticleCells);
//  }
//  else {
    output->SetVerts(this->ParticleCells);
//  }

*/
}
//---------------------------------------------------------------------------
void vtkTemporalStreamTracer::IntegrateParticle(
  ParticleIterator &it, 
  double currenttime, double terminationtime,
  vtkInitialValueProblemSolver* integrator)
{
  double epsilon = (terminationtime-currenttime)/100.0;
  double velocity[3], point1[4], point2[4] = {0.0, 0.0, 0.0, 0.0};
  double minStep=0, maxStep=0;
  double stepWanted, stepTaken;
  
  ParticleInformation &info = (*it);
  // Get the Initial point {x,y,z,t}
  memcpy(point1, &info.CurrentPosition, sizeof(Position));

  if (point1[3]<(currenttime-epsilon) || point1[3]>(terminationtime+epsilon)) {
    vtkDebugMacro(<< "Bad particle time : expected (" 
      << this->CurrentTimeSteps[0] << "-" << this->CurrentTimeSteps[1] 
      << ") got " << point1[3]);
  }

  IntervalInformation delT;
  delT.Unit     = TIME_UNIT;
  delT.Interval = (terminationtime-currenttime)*this->InitialIntegrationStep.Interval;
  epsilon = delT.Interval*1E-3;

  //
  // begin interpolation between available time values, if the particle has 
  // a cached cell ID and dataset - try to use it, 
  // if AllFixedGeometry is true, caching has already been turned on
  // if NoFixedGeometry is true, caching is already turned off
  // if some geometry is fixed, we must test the cached information
  // to see if we can cache between these time steps
  //
  int fixedgeometry = this->AllFixedGeometry;
  if (!this->NoFixedGeometry)
  {
    if (this->GeometryFixed[0][info.CachedDataSet[0]] &&
        this->GeometryFixed[1][info.CachedDataSet[1]]) 
    {
      this->Interpolator->SetGeometryFixed(1);
    }
    else 
    {
      this->Interpolator->SetGeometryFixed(0);
      fixedgeometry = 0;
    }
  }
  this->Interpolator->SetCachedCellIds(info.CachedCellId, info.CachedDataSet);

  bool ok = true;
  while (point1[3] < (terminationtime-epsilon)) {
    //
    // Here beginneth the real work
    //
    double error = 0;

    // If, with the next step, propagation will be larger than
    // max, reduce it so that it is (approximately) equal to max.
    stepWanted = delT.Interval;
    if ( (point1[3] + stepWanted) > terminationtime )
      {
      stepWanted = terminationtime - point1[3];
      maxStep = stepWanted;
      }
    this->LastUsedTimeStep = stepWanted;
          
    // Calculate the next step using the integrator provided.
    // If the next point is out of bounds, send it to another process
    if (integrator->ComputeNextStep(
          point1, point2, point1[3], stepWanted, 
          stepTaken, minStep, maxStep, 
          this->MaximumError, error) != 0)
    {
      vtkDebugMacro(<< "INTEGRATE_FAILED   : Sending Particle " << info.UniqueParticleId << " Time " << point1[3]);
      this->DoParticleSendTasks(info, point1, this->LastUsedTimeStep);
      this->ParticleHistories.erase(it);
      ok = false;
      break;
    }

    // increment the particle time
    point2[3] = point1[3] + stepTaken;

    // Point is valid. Insert it.
    memcpy(&info.CurrentPosition, point2, sizeof(Position));
    memcpy(point1, point2, sizeof(Position));

    // If the solver is adaptive and the next time step (delT.Interval)
    // that the solver wants to use is smaller than minStep or larger 
    // than maxStep, re-adjust it. This has to be done every step
    // because minStep and maxStep can change depending on the Cell
    // size (unless it is specified in time units)
    if (integrator->IsAdaptive())
      {
        // code removed. Put it back when this is stable
      }
  }

  if (ok) {
    // The integration succeeded, but check the computed final position 
    // is actually inside the domain (the intermediate steps taken inside 
    // the integrator were ok, but the final step may just pass out)
    if (!this->Interpolator->FunctionValues(&info.CurrentPosition.x[0], velocity) )
    {
      vtkDebugMacro(<< "INTEGRATE_OVERSHOT : Sending Particle " 
        << info.UniqueParticleId << " Time " << info.CurrentPosition.x[3]);
      this->AddParticleToMPISendList(info);
      this->ParticleHistories.erase(it);
      ok = false;
    }
  }

  //
  // We got this far without error :
  // Insert the point into the output
  // Create any new scalars and interpolate existing ones
  // Cache cell ids and datasets
  //
  if (ok) {
    //
    // Get the last Cell Ids and dataset indices
    //
    this->Interpolator->GetCachedCellIds(info.CachedCellId, info.CachedDataSet);
    if (fixedgeometry) {
      // and now advance forward one time step
      info.CachedCellId[0]  = info.CachedCellId[1];
      info.CachedDataSet[0] = info.CachedDataSet[1];
    }
    //
    // Now generate the output geometry and scalars
    //
    // Note that this is slightly overkill. In principle we always integrate
    // the particle until it reaches Time2 - so we don't need to do any 
    // interpolation of the scalars between T1 and T2, just fetch the values
    // of the spatially interpolated scalars from T2.
    // We'll leave this here for now as it is instructive to see if it 
    // works properly
    //
    vtkDebugMacro(<< "Adding Particle to output : " << info.UniqueParticleId
      << " Time " << info.CurrentPosition.x[3]);

    // insert the coordinate
    double    *coord = &info.CurrentPosition.x[0];
    vtkIdType tempId = this->OutputCoordinates->InsertNextPoint(coord);
    // create the cell
    this->ParticleCells->InsertNextCell(1, &tempId);
    // set the easy scalars for this particle
    this->ParticleIds->InsertNextTuple1(info.UniqueParticleId);
    this->ParticleSourceIds->InsertNextTuple1(info.SourceID);
    this->InjectedPointIds->InsertNextTuple1(info.InjectedPointId);
    this->ParticleTime->InsertNextTuple1(info.CurrentPosition.x[3]);
    //
    // Interpolate all existing point attributes
    //
    this->Interpolator->InterpolatePointAtT(
      1, this->OutputPointData, tempId);
/*
    bool ok = this->Interpolator->InterpolatePoint(
      this->OutputPointDataT1, this->OutputPointDataT2, tempId);
    if (ok) {
      this->OutputPointData->InterpolateTime(
        this->OutputPointDataT1, this->OutputPointDataT2, 
        tempId, this->Interpolator->GetCurrentWeight());
    }
    else {
      vtkDebugMacro(<<"Point interpolation failed");
    }
*/
    //
    // Compute vorticity 
    //
    if (this->ComputeVorticity)
    {
      vtkGenericCell *cell;
      double pcoords[3], vorticity[3], weights[256];
      double rotation, omega, speed = vtkMath::Norm(velocity);
      this->Interpolator->GetVorticityData(
        1, pcoords, weights, cell, this->cellVectors
      );
      vtkStreamTracer::CalculateVorticity(cell, pcoords, cellVectors, vorticity);
      this->ParticleVorticity->InsertNextTuple(vorticity);
      // local rotation = vorticity . unit tangent ( i.e. velocity/speed )
      if (speed != 0.0) {
        omega = vtkMath::Dot(vorticity, velocity);
        omega /= speed;
        omega *= this->RotationScale;
      }
      else {
        omega = 0.0;
      }     
      vtkIdType index = this->ParticleAngularVel->InsertNextValue(omega);
      if (index>0) {     
        rotation     = info.rotation + (info.angularVel + omega)/2 * (info.CurrentPosition.x[3] - info.time);
      } 
      else {
        rotation     = 0.0;
      }
      this->ParticleRotation->InsertNextValue(rotation);
      info.rotation   = rotation;
      info.angularVel = omega;
      info.time       = info.CurrentPosition.x[3];
    }
  }
  else this->Interpolator->ClearCache();

  double eps = (this->CurrentTimeSteps[1]-this->CurrentTimeSteps[0])/100;
  if (point1[3]<(this->CurrentTimeSteps[0]-eps) || point1[3]>(this->CurrentTimeSteps[1]+eps)) {
    vtkDebugMacro(<< "Unexpected time ending IntegrateParticle - expected (" 
      << this->CurrentTimeSteps[0] << "-" << this->CurrentTimeSteps[1] << ") got " << point1[3]);  
  }
}
//---------------------------------------------------------------------------
bool vtkTemporalStreamTracer::DoParticleSendTasks(ParticleInformation &info, double point1[4], double velocity[3], double delT)
{
  // Get the most approximate theoretical next point
  for (int v=0; v<3; v++) info.CurrentPosition.x[v] = point1[v] + velocity[v]*delT;
  info.CurrentPosition.x[3] = point1[3] + delT;
  //
//  vtkDebugMacro(<< "DoParticleSendTasks : Sending Particle " << "XXX" << " Time " << point2[3]);
  this->AddParticleToMPISendList(info);
  return 1;

}
//---------------------------------------------------------------------------
bool vtkTemporalStreamTracer::DoParticleSendTasks(ParticleInformation &info, double point1[4], double delT)
{
  double velocity[3];
  if ( !this->Interpolator->FunctionValues(point1, velocity) ) {
    vtkDebugMacro(<< "FunctionValues(point1, velocity) : OUT_OF_DOMAIN " << info.UniqueParticleId << '\n');
    this->Interpolator->GetLastGoodVelocity(velocity);
    return this->DoParticleSendTasks(info, point1, velocity, delT);
  }
  else {
    return this->DoParticleSendTasks(info, point1, velocity, delT);
  }
}
//---------------------------------------------------------------------------
void vtkTemporalStreamTracer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "TimeStepResolution: " << this->TimeStepResolution << endl;
  os << indent << "Controller: " << this->Controller << endl;
  os << indent << "ParticleWriter: " << this->ParticleWriter << endl;
  os << indent << "ParticleFileName: " << 
    (this->ParticleFileName ? this->ParticleFileName : "None") << endl;
  os << indent << "EnableSource1: " << this->EnableSource1 << endl;
  os << indent << "EnableSource2: " << this->EnableSource2 << endl;
  os << indent << "TimeStep: " << this->TimeStep << endl;
  os << indent << "ForceReinjectionEveryNSteps: "
     << this->ForceReinjectionEveryNSteps << endl;
  os << indent << "EnableParticleWriting: " << this->EnableParticleWriting << endl;
  os << indent << "IgnorePipelineTime: " << this->IgnorePipelineTime << endl;
}
//---------------------------------------------------------------------------
bool vtkTemporalStreamTracer::ComputeDomainExitLocation(
  double pos[4], double p2[4], double intersection[4],
  vtkGenericCell *cell)
{
  double t, pcoords[3];
  int subId;
  if (cell->IntersectWithLine(pos, p2, 1E-3, t, intersection, pcoords, subId)==0) {
    vtkDebugMacro(<< "No cell/domain exit was found");
    return 0;
  }
  else {
    // We found an intersection on the edge of the cell. 
    // Shift it by a small amount to ensure that it crosses over the edge 
    // into the adjoining cell.
    for (int i=0; i<3; i++) intersection[i] = pos[i] + (t+0.01)*(p2[i]-pos[i]);
    // intersection stored, compute T for intersection
    intersection[3] = pos[3] + (t+0.01)*(p2[3]-pos[3]);
    return 1;    
  }
}
//---------------------------------------------------------------------------
void vtkTemporalStreamTracer::AddParticleToMPISendList(ParticleInformation &info)
{
  double eps = (this->CurrentTimeSteps[1]-this->CurrentTimeSteps[0])/100;
  if (info.CurrentPosition.x[3]<(this->CurrentTimeSteps[0]-eps) || 
      info.CurrentPosition.x[3]>(this->CurrentTimeSteps[1]+eps)) {
    vtkDebugMacro(<< "Unexpected time value in MPISendList - expected (" 
      << this->CurrentTimeSteps[0] << "-" << this->CurrentTimeSteps[1] << ") got " 
      << info.CurrentPosition.x[3]);
  }
#ifdef VTK_USE_MPI
  int size = this->MPISendList.size();
  this->MPISendList.resize(size+1);
  memcpy(&(this->MPISendList[size]), &info, sizeof(ParticleInformation));
#endif
}
//---------------------------------------------------------------------------
/*
  // Now try to compute the trajectory exit location from the cell on the edge
  if (this->Interpolator->GetLastValidCellId(0)!=-1) {
    vtkDataSet *hitdata = this->Interpolator->GetLastDataSet(0);
    hitdata->GetCell(this->Interpolator->GetLastValidCellId(0), this->GenericCell);
    double intersection[4];
    if (this->ComputeDomainExitLocation(point1, point2, intersection, this->GenericCell))
    {
      vtkDebugMacro(<< "DoParticleSendTasks : Sending Particle " << particleId << " Time " << intersection[3]);
      this->AddParticleToMPISendList(intersection, velocity);
      vtkIdType nextPoint = ParticleCoordinates->InsertNextPoint(point2);
      this->ParticleHistories[particleId].push_back(nextPoint);
      this->LiveParticleIds[particleId] = 0;
      return 1;
    }
    else {
      vtkDebugMacro(<< "Domain-Exit aborted : Domain Intersection failed" << particleId );
      this->LiveParticleIds[particleId] = 0;
      return 0;
    }
  }
  else {
    vtkDebugMacro(<< "Domain-Exit aborted : Couldn't copy cell from earlier test" << particleId );
    this->LiveParticleIds[particleId] = 0;
    return 0;
  }
*/
