/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkTemporalInterpolator.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*================================================
  Created by Ken Martin, 
  leveraging code written by John Biddiscombe
  ================================================*/

#include "vtkTemporalInterpolator.h"

#include "vtkTemporalDataSet.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkPointSet.h"
#include "vtkPointData.h"
#include "vtkCellData.h"

#include "vtkstd/algorithm"
#include "vtkstd/vector"

vtkCxxRevisionMacro(vtkTemporalInterpolator, "$Revision: 1.10 $");
vtkStandardNewMacro(vtkTemporalInterpolator);

//----------------------------------------------------------------------------
vtkTemporalInterpolator::vtkTemporalInterpolator()
{
  this->DiscreteTimeStepInterval = 0.0; // non value
}

//----------------------------------------------------------------------------
vtkTemporalInterpolator::~vtkTemporalInterpolator()
{
}

//----------------------------------------------------------------------------
void vtkTemporalInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "DiscreteTimeStepInterval: "
     << this->DiscreteTimeStepInterval << "\n";
}
/*
//----------------------------------------------------------------------------
int vtkTemporalInterpolator::FillInputPortInformation(
  int port, 
  vtkInformation* info)
{
  if (port==0) {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTemporalDataSet");
  }
  return 1;
}
//----------------------------------------------------------------------------
int vtkTemporalInterpolator::RequestDataObject(
  vtkInformation* vtkNotUsed(reqInfo), 
  vtkInformationVector** inputVector , 
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (!inInfo)
    {
    return 0;
    }
  
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  if (input)
    {
    // for each output
    for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
      {
      vtkInformation* outInfo = outputVector->GetInformationObject(i);
      vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());
    
      if (!output || !output->IsA(input->GetClassName())) 
        {
        vtkDataObject* newOutput = input->NewInstance();
        newOutput->SetPipelineInformation(outInfo);
        newOutput->Delete();
        this->GetOutputPortInformation(0)->Set(
          vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
        }
      }
    return 1;
    }
  return 0;
}
*/
//----------------------------------------------------------------------------
// Change the information
int vtkTemporalInterpolator::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  double *inTimes;
  int numTimes;
  double outRange[2];

  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
    inTimes =
      inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    numTimes =
      inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    outRange[0] = inTimes[0];
    outRange[1] = inTimes[numTimes-1];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
                 outRange,2);
    // we throw out the discrete entries and our output is considered to be
    // continuous
    if (this->DiscreteTimeStepInterval<=0.0) 
      {
      // unset the time steps if they are set
      if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
        {
        outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
        }
      }
    else 
      {
      //
      // We know the input has got N time steps, 
      // how many output steps are we capable of
      // producing, and what are they.
      //
      if (numTimes>1) 
        {
        int NumberOfOutputTimeSteps = 1 + 
          static_cast<int>(0.5+((outRange[1]-outRange[0])/this->DiscreteTimeStepInterval));

        // Generate list of new output time step values
        vtkstd::vector<double> OutputTimeValues;
        for (int i=0; i<NumberOfOutputTimeSteps; i++) 
          {
          OutputTimeValues.push_back(
            (double)(i)*this->DiscreteTimeStepInterval + outRange[0]);
          }
        outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), 
          &OutputTimeValues[0], NumberOfOutputTimeSteps);
        }
      else 
        { 
        vtkErrorMacro(<<"Not enough input time steps for interpolation");
        return 0;
        }
      }
    }

  return 1;
}


//----------------------------------------------------------------------------
int vtkTemporalInterpolator::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  
  vtkTemporalDataSet *inData = vtkTemporalDataSet::SafeDownCast
    (inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkTemporalDataSet *outData = vtkTemporalDataSet::SafeDownCast
    (outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!inData || !outData)
    {
    return 1;
    }

  // get the input times
  double *inTimes = inData->GetInformation()
    ->Get(vtkDataObject::DATA_TIME_STEPS());
  int numInTimes = inData->GetInformation()
    ->Length(vtkDataObject::DATA_TIME_STEPS());

  // get the requested update times
  double *upTimes =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
  int numUpTimes = 
    outInfo->Length(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());

  // for each targeted output time
  int upIdx;
  for (upIdx = 0; upIdx < numUpTimes; ++upIdx)
    {
    // below the range
    vtkDataObject *out1;
    if (upTimes[upIdx] <= inTimes[0])
      {
      // pass the lowest data
      vtkDebugMacro(<<"Interpolation time below/== range : " << inTimes[0]);
      vtkDataObject *in1 = inData->GetDataSet(0,0);
      out1 = in1->NewInstance();
      out1->ShallowCopy(in1);
      outData->SetDataSet(upIdx,0,out1);
//      outData->ShallowCopy(out1);
      out1->Delete();
      }
    // above the range?
    else if (upTimes[upIdx] >= inTimes[numInTimes-1])
      {
      // pass the highest data
      vtkDebugMacro(<<"Interpolation time above/== range : " << inTimes[numInTimes-1] << " of " << numInTimes);
      vtkDataObject *in1 = inData->GetDataSet(numInTimes-1,0);
      out1 = in1->NewInstance();
      out1->ShallowCopy(in1);
      outData->SetDataSet(upIdx,0,out1);
//      outData->ShallowCopy(out1);
      out1->Delete();
      }
    // in the middle, interpolate
    else
      {
      int i = 0;
      while (upTimes[upIdx] > inTimes[i])
        {
        ++i;
        }
      // was there an exact time match? If so shallow copy
      if (upTimes[upIdx] == inTimes[i])
        {
        // pass the match
        vtkDebugMacro(<<"Interpolation time " << inTimes[i]);
        vtkDataObject *in1 = inData->GetDataSet(i,0);
        out1 = in1->NewInstance();
        out1->ShallowCopy(in1);
        outData->SetDataSet(upIdx,0,out1);
//        outData->ShallowCopy(out1);
        out1->Delete();
        }
      else
        {
        // interpolate i-1 and i
        vtkDataObject *in1 = inData->GetDataSet(i-1,0);
        vtkDataObject *in2 = inData->GetDataSet(i,0);
        double ratio = (upTimes[upIdx]-inTimes[i-1])/(inTimes[i] - inTimes[i-1]);
        vtkDebugMacro(<<"Interpolation times " << inTimes[i-1] << "->" << inTimes[i] 
          << " : " << upTimes[upIdx] << " Interpolation ratio " << ratio );
        out1 = this->InterpolateDataObject(in1,in2,ratio);
        outData->SetDataSet(upIdx,0,out1);
//        outData->ShallowCopy(out1);
        out1->Delete();
        }
      out1->GetInformation()->Set(vtkDataObject::DATA_TIME_STEPS(), 
                                 &upTimes[upIdx], 1);
      }
    }

  // set the resulting times
  outData->GetInformation()->Set(vtkDataObject::DATA_TIME_STEPS(), 
                                 upTimes, numUpTimes);

  return 1;
}

//----------------------------------------------------------------------------
int vtkTemporalInterpolator::RequestUpdateExtent (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  // find the required input time steps and request them
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS()))
    {
    // get the update times
    double *upTimes =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());
    int numUpTimes = 
      outInfo->Length(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS());

    // get the available input times
    double *inTimes =
      inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    int numInTimes =
      inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    // only if the input is not continuous should we do anything
    if (inTimes)
      {
      bool *inTimesToUse;
      inTimesToUse = new bool [numInTimes];
      int i;
      for (i = 0; i < numInTimes; ++i)
        {
        inTimesToUse[i] = false;
        }
      
      // for each requested time mark the required input times
      int u;
      i = 0;
      for (u = 0; u < numUpTimes; ++u)
        {
        // below the range
        if (upTimes[u] <= inTimes[0])
          {
          inTimesToUse[0] = true;
          }
        // above the range?
        else if (upTimes[u] >= inTimes[numInTimes-1])
          {
          inTimesToUse[numInTimes-1] = true;
          }
        // in the middle
        else
          {
          while (upTimes[u] > inTimes[i])
            {
            ++i;
            }
          inTimesToUse[i] = true;
          inTimesToUse[i-1] = true;
          }
        }
      
      // how many input times do we need?
      int numInUpTimes = 0;
      for (i = 0; i < numInTimes; ++i)
        {
        if (inTimesToUse[i])
          {
          numInUpTimes++;
          }
        }
      
      double *inUpTimes = new double [numInUpTimes];
      u = 0;
      for (i = 0; i < numInTimes; ++i)
        {
        if (inTimesToUse[i])
          {
          inUpTimes[u] = inTimes[i];
          u++;
          }
        }
      
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(),
                  inUpTimes,numInUpTimes);
      vtkDebugMacro(<<"Requesting " << numInUpTimes << " times ");
      
      delete [] inUpTimes;
      delete [] inTimesToUse;
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
bool vtkTemporalInterpolator
::VerifyArrays(vtkDataArray **arrays, int N)
{
  vtkIdType Nt = arrays[0]->GetNumberOfTuples();
  vtkIdType Nc = arrays[0]->GetNumberOfComponents();
  for (int i=1; i<N; ++i) 
    {
    if (arrays[i]->GetNumberOfTuples()!=Nt) 
      {
      return false;
      }
    if (arrays[i]->GetNumberOfComponents()!=Nc) 
      {
      return false;
      }
    }
  return true;
}

//----------------------------------------------------------------------------
vtkDataObject *vtkTemporalInterpolator
::InterpolateDataObject( vtkDataObject *in1, vtkDataObject *in2, double ratio)
{
  if (vtkDataSet::SafeDownCast(in1)) 
    {
    //
    // if we have reached the Leaf/DataSet level, we can interpolate directly
    //
    vtkDataSet *inds1 = vtkDataSet::SafeDownCast(in1);
    vtkDataSet *inds2 = vtkDataSet::SafeDownCast(in2);
    return this->InterpolateDataSet(inds1, inds2, ratio);
    }
  else if (vtkMultiGroupDataSet::SafeDownCast(in1)) 
    {
    vtkMultiGroupDataSet *mgds[2];
    mgds[0] = vtkMultiGroupDataSet::SafeDownCast(in1);
    mgds[1] = vtkMultiGroupDataSet::SafeDownCast(in2);

    //
    // We need to loop over blocks etc and build up a new dataset
    //
    vtkMultiGroupDataSet *output = mgds[0]->NewInstance();
    int numGroups = mgds[0]->GetNumberOfGroups();
    output->SetNumberOfGroups(numGroups);
    //
    for (int g=0; g<numGroups; ++g) 
      {
      int numDataSets = mgds[0]->GetNumberOfDataSets(g);
      output->SetNumberOfDataSets(g,numDataSets);
      for (int d=0; d<numDataSets; ++d) 
        {
        // These multigroup dataset can have null data, it's bad, but
        // we'll just skip the rest of that bundle
        vtkDataObject *dataobj1 = mgds[0]->GetDataSet(g,d);
        vtkDataObject *dataobj2 = mgds[1]->GetDataSet(g,d);
        if (!dataobj1 || !dataobj2) 
          {
          vtkWarningMacro
            (
             "The MultiGroup datasets were not identical in structure : Group " 
             << g << " Dataset " << d << " was skipped");
          continue;
          }
        vtkDataObject *result = 
          this->InterpolateDataObject(dataobj1, dataobj2, ratio);
        if (result) 
          {
          output->SetDataSet(g, d, result); 
          result->Delete();
          }
        else 
          {
          vtkErrorMacro(<<"Unexpected error during interpolation");
          // need to clear up memory we may have allocated and lost :(
          return NULL;
          }
        }
      }
    return output;
    }
  else 
    {
    vtkErrorMacro("We cannot yet interpolate this type of dataset");
    return NULL;
    }
}

//----------------------------------------------------------------------------
vtkDataSet *vtkTemporalInterpolator
::InterpolateDataSet(vtkDataSet *in1, vtkDataSet *in2, double ratio)
{
  vtkDataSet *input[2];
  input[0] = in1;
  input[1] = in2;

  //
  vtkDataSet *output = input[0]->NewInstance();
  output->CopyStructure(input[0]);
  //
  // Interpolate points if the dataset is a vtkPointSet
  //
  vtkPointSet *inPointSet1 = vtkPointSet::SafeDownCast(input[0]);
  vtkPointSet *inPointSet2 = vtkPointSet::SafeDownCast(input[1]);
  vtkPointSet *outPointSet = vtkPointSet::SafeDownCast(output);
  if (inPointSet1 && inPointSet2) 
    {
    vtkDataArray *arrays[2];
    arrays[0] = inPointSet1->GetPoints()->GetData();
    arrays[1] = inPointSet2->GetPoints()->GetData();

    // allocate double for output if input is double - otherwise float
    // do a quick check to see if all arrays have the same number of tuples
    if (!this->VerifyArrays(arrays, 2)) 
      {
      vtkWarningMacro
        ("Interpolation aborted for points because the number of "
         "tuples/components in each time step are different");
      }
    vtkDataArray *outarray = 
      this->InterpolateDataArray(
        ratio, arrays,arrays[0]->GetNumberOfTuples());

    // Do not shallow copy points from either input, because otherwise when
    // we set the actual point coordinate data we overwrite the original
    // we must instantiate a new points object 
    // (ie we override the copystrucure above)
    vtkPoints *inpoints = inPointSet1->GetPoints();
    vtkPoints *outpoints = inpoints->NewInstance();
    outPointSet->SetPoints(outpoints);
    if (vtkDoubleArray::SafeDownCast(outarray)) 
      {
      outpoints->SetDataTypeToDouble();
      }
    else 
      {
      outpoints->SetDataTypeToFloat();
      }
    outpoints->SetNumberOfPoints(arrays[0]->GetNumberOfTuples());
    outpoints->SetData(outarray);
    outpoints->Delete();
    outarray->Delete();
    }
  //
  // Interpolate pointdata if present
  //
  output->GetPointData()->ShallowCopy(input[0]->GetPointData());
  for (int s=0; s < input[0]->GetPointData()->GetNumberOfArrays(); ++s) 
    {
    vtkstd::vector<vtkDataArray*> arrays;
    char *scalarname = NULL;
    for (int i=0; i<2; ++i) 
      {
      //
      // On some data, the scalar arrays are consistent but ordered
      // differently on each time step, so we will fetch them by name if
      // possible.
      //
      if (i==0 || (scalarname==NULL)) 
        {
        vtkDataArray *dataarray = input[i]->GetPointData()->GetArray(s);
        scalarname = dataarray->GetName();
        arrays.push_back(dataarray);
        }
      else 
        {
        vtkDataArray *dataarray = 
          input[i]->GetPointData()->GetArray(scalarname);
        arrays.push_back(dataarray);
        }
      }
    // do a quick check to see if all arrays have the same number of tuples
    if (!this->VerifyArrays(&arrays[0], 2)) 
      {
      vtkWarningMacro(<<"Interpolation aborted for array " 
        << (scalarname ? scalarname : "(unnamed array)") 
        << " because the number of tuples/components"
        << " in each time step are different");
      }
    // allocate double for output if input is double - otherwise float
    vtkDataArray *outarray = 
      this->InterpolateDataArray(ratio, &arrays[0],
                                 arrays[0]->GetNumberOfTuples());
    output->GetPointData()->AddArray(outarray);
    outarray->Delete();
    }
  //
  // Interpolate celldata if present
  //
  output->GetCellData()->ShallowCopy(input[0]->GetCellData());
  for (int s=0; s<input[0]->GetCellData()->GetNumberOfArrays(); ++s) 
    {
    // copy the structure
    vtkstd::vector<vtkDataArray*> arrays;
    char *scalarname = NULL;
    for (int i=0; i<2; ++i) 
      {
      //
      // On some data, the scalar arrays are consistent but ordered
      // differently on each time step, so we will fetch them by name if
      // possible.
      //
      if (i==0 || (scalarname==NULL)) 
        {
        vtkDataArray *dataarray = input[i]->GetCellData()->GetArray(s);
        scalarname = dataarray->GetName();
        arrays.push_back(dataarray);
        }
      else 
        {
        vtkDataArray *dataarray = 
          input[i]->GetCellData()->GetArray(scalarname);
        arrays.push_back(dataarray);
        }
      }
    // do a quick check to see if all arrays have the same number of tuples
    if (!this->VerifyArrays(&arrays[0], 2)) 
      {
      vtkWarningMacro(<<"Interpolation aborted for array " 
                      << (scalarname ? scalarname : "(unnamed array)") 
                      << " because the number of tuples/components"
                      << " in each time step are different");
      }
    // allocate double for output if input is double - otherwise float
    vtkDataArray *outarray = 
      this->InterpolateDataArray(ratio, &arrays[0],
                                 arrays[0]->GetNumberOfTuples());
    output->GetCellData()->AddArray(outarray);
    outarray->Delete();
    }
  return output;
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
void vtkTemporalInterpolatorExecute(vtkTemporalInterpolator *,
                                    double ratio,
                                    vtkDataArray *output,
                                    vtkDataArray **arrays,
                                    int numComp,
                                    int numTuple,
                                    T *)
{
  T *outData = static_cast<T*>(output->GetVoidPointer(0));
  T *inData1 = static_cast<T*>(arrays[0]->GetVoidPointer(0));
  T *inData2 = static_cast<T*>(arrays[1]->GetVoidPointer(0));

  double oneMinusRatio = 1.0 - ratio;

  unsigned long idx;
  for (idx = 0; idx < static_cast<unsigned long>(numTuple*numComp); ++idx)
    {
    *outData = static_cast<T>((*inData1)*oneMinusRatio + (*inData2)*ratio);
    outData++;
    inData1++;
    inData2++;
  }
}


//----------------------------------------------------------------------------
vtkDataArray *vtkTemporalInterpolator
::InterpolateDataArray(double ratio, vtkDataArray **arrays, vtkIdType N)
{
  //
  // Create the output
  //
  vtkAbstractArray *aa = arrays[0]->CreateArray(arrays[0]->GetDataType());
  vtkDataArray *output = vtkDataArray::SafeDownCast(aa);
  
  int Nc = arrays[0]->GetNumberOfComponents();

  //
  // initialize the output
  //
  output->SetNumberOfComponents(Nc);
  output->SetNumberOfTuples(N);
  output->SetName(arrays[0]->GetName());

  // now do the interpolation
  switch (arrays[0]->GetDataType())
    {
    vtkTemplateMacro(vtkTemporalInterpolatorExecute
                     (this, ratio, output, arrays, Nc, N,
                      static_cast<VTK_TT *>(0)));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return 0;
    }

  return output;
}

