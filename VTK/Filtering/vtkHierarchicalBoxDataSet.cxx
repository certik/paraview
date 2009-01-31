/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkHierarchicalBoxDataSet.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalBoxDataSet.h"

#include "vtkHierarchicalBoxDataSetInternal.h"

#include "vtkMultiGroupDataInformation.h"
#include "vtkInformation.h"
#include "vtkInformationIdTypeKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"

vtkCxxRevisionMacro(vtkHierarchicalBoxDataSet, "$Revision: 1.17 $");
vtkStandardNewMacro(vtkHierarchicalBoxDataSet);

vtkInformationKeyMacro(vtkHierarchicalBoxDataSet,BOX,IntegerVector);
vtkInformationKeyMacro(vtkHierarchicalBoxDataSet,NUMBER_OF_BLANKED_POINTS,IdType);

typedef vtkstd::vector<vtkAMRBox> vtkAMRBoxList;
//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSet::vtkHierarchicalBoxDataSet()
{
  this->BoxInternal = new vtkHierarchicalBoxDataSetInternal;
  this->ScalarRange[0]=VTK_DOUBLE_MAX;
  this->ScalarRange[1]=VTK_DOUBLE_MIN;
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSet::~vtkHierarchicalBoxDataSet()
{
  delete this->BoxInternal;
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::SetDataSet(
  unsigned int level, unsigned int id, vtkAMRBox& box, vtkUniformGrid* dataSet)
{
  this->Superclass::SetDataSet(level, id, dataSet);

  vtkInformation* info =
    this->MultiGroupDataInformation->GetInformation(level, id);
  if (info)
    {
    info->Set(BOX(),
              box.LoCorner[0], box.LoCorner[1], box.LoCorner[2],
              box.HiCorner[0], box.HiCorner[1], box.HiCorner[2]);
    }
}

//----------------------------------------------------------------------------
vtkUniformGrid* vtkHierarchicalBoxDataSet::GetDataSet(unsigned int level,
                                                      unsigned int id,
                                                      vtkAMRBox& box)
{
  if (this->Internal->DataSets.size() <= level)
    {
    return 0;
    }

  vtkMultiGroupDataSetInternal::GroupDataSetsType& ldataSets =
    this->Internal->DataSets[level];
  if (ldataSets.size() <= id)
    {
    return 0;
    }

  if (!ldataSets[id])
    {
    return 0;
    }

  vtkInformation* info =
    this->MultiGroupDataInformation->GetInformation(level, id);
  if (info)
    {
    int* boxVec = info->Get(BOX());
    if (boxVec)
      {
      vtkAMRBoxInitialize<3>(box.LoCorner, box.HiCorner,
                             boxVec      , boxVec+3);
      }
    }
  return static_cast<vtkUniformGrid*>(ldataSets[id].GetPointer());
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::SetRefinementRatio(unsigned int level,
                                                   int ratio)
{
  if (level >= this->BoxInternal->RefinementRatios.size())
    {
    this->BoxInternal->RefinementRatios.resize(level+1);
    }
  this->BoxInternal->RefinementRatios[level] = ratio;
}

//----------------------------------------------------------------------------
int vtkHierarchicalBoxDataSet::GetRefinementRatio(unsigned int level)
{
  if (level >= this->BoxInternal->RefinementRatios.size())
    {
    return 0;
    }
  return this->BoxInternal->RefinementRatios[level];
}

//----------------------------------------------------------------------------
int vtkHierarchicalBoxDataSetIsInBoxes(vtkAMRBoxList& boxes,
                                       int i, int j, int k)
{
  vtkAMRBoxList::iterator it;
  for(it = boxes.begin(); it != boxes.end(); it++)
    {
    if (it->DoesContainCell(i, j, k))
      {
      return 1;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::GenerateVisibilityArrays()
{
  if (!this->MultiGroupDataInformation)
    {
    vtkErrorMacro("No information about data layout is specified. "
                  "Cannot generate visibility arrays");
    return;
    }

  unsigned int numLevels = this->GetNumberOfLevels();

  for (unsigned int levelIdx=0; levelIdx<numLevels; levelIdx++)
    {
    // Copy boxes of higher level and coarsen to this level
    vtkAMRBoxList boxes;
    unsigned int numDataSets = this->GetNumberOfDataSets(levelIdx+1);
    unsigned int dataSetIdx;
    if (levelIdx < numLevels - 1)
      {
      for (dataSetIdx=0; dataSetIdx<numDataSets; dataSetIdx++)
        {
        if (!this->MultiGroupDataInformation->HasInformation(
              levelIdx+1, dataSetIdx))
          {
          continue;
          }
        vtkInformation* info =
          this->MultiGroupDataInformation->GetInformation(
            levelIdx+1,dataSetIdx);
        int* boxVec = info->Get(BOX());
        vtkAMRBox coarsebox(3, boxVec, boxVec+3);
        if (this->BoxInternal->RefinementRatios.size() <= levelIdx)
          {
          continue;
          }
        coarsebox.Coarsen(this->BoxInternal->RefinementRatios[levelIdx]);
        boxes.push_back(coarsebox);
        }
      }

    numDataSets = this->GetNumberOfDataSets(levelIdx);
    for (dataSetIdx=0; dataSetIdx<numDataSets; dataSetIdx++)
      {
      vtkAMRBox box;
      vtkUniformGrid* grid = this->GetDataSet(levelIdx, dataSetIdx, box);

      if (grid)
        {
        int i;
        int cellDims[3];
        for (i=0; i<3; i++)
          {
          cellDims[i] = box.HiCorner[i] - box.LoCorner[i] + 1;
          }
        vtkUnsignedCharArray* vis = vtkUnsignedCharArray::New();
        vtkIdType numCells = box.GetNumberOfCells();
        vis->SetNumberOfTuples(numCells);
        for (i=0; i<numCells; i++)
          {
          vis->SetValue(i, 1);
          }
        vtkIdType numBlankedPts = 0;
        for (int iz=box.LoCorner[2]; iz<=box.HiCorner[2]; iz++)
          {
          for (int iy=box.LoCorner[1]; iy<=box.HiCorner[1]; iy++)
            {
            for (int ix=box.LoCorner[0]; ix<=box.HiCorner[0]; ix++)
              {
              // Blank if cell is covered by a box of higher level
              if (vtkHierarchicalBoxDataSetIsInBoxes(boxes, ix, iy, iz))
                {
                vtkIdType id =
                  (iz-box.LoCorner[2])*cellDims[0]*cellDims[1] +
                  (iy-box.LoCorner[1])*cellDims[0] +
                  (ix-box.LoCorner[0]);
                vis->SetValue(id, 0);
                numBlankedPts++;
                }
              }
            }
          }
        grid->SetCellVisibilityArray(vis);
        vis->Delete();
        if (this->MultiGroupDataInformation->HasInformation(
              levelIdx, dataSetIdx))
          {
          vtkInformation* infotmp =
            this->MultiGroupDataInformation->GetInformation(
              levelIdx,dataSetIdx);
          infotmp->Set(NUMBER_OF_BLANKED_POINTS(), numBlankedPts);
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkHierarchicalBoxDataSet::GetNumberOfPoints()
{
  vtkIdType numPts = 0;

  unsigned int numLevels = this->GetNumberOfLevels();
  for (unsigned int level=0; level<numLevels; level++)
    {
    unsigned int numDataSets = this->GetNumberOfDataSets(level);
    for (unsigned int dataIdx=0; dataIdx<numDataSets; dataIdx++)
      {
      vtkIdType numBlankedPts = 0;
      vtkInformation* blockInfo =
        this->MultiGroupDataInformation->GetInformation(level, dataIdx);
      if (blockInfo)
        {
        if (blockInfo->Has(
              vtkHierarchicalBoxDataSet::NUMBER_OF_BLANKED_POINTS()))
          {
          numBlankedPts = blockInfo->Get(NUMBER_OF_BLANKED_POINTS());
          }
        }
      vtkDataSet* ds = vtkDataSet::SafeDownCast(
        this->GetDataSet(level, dataIdx));
      if (ds)
        {
        numPts += ds->GetNumberOfPoints() - numBlankedPts;
        }
      }
    }

  return numPts;
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::ShallowCopy(vtkDataObject *src)
{
  if (src == this)
    {
    return;
    }
  this->InitializeDataSets();
  this->Modified();

  vtkHierarchicalBoxDataSet* from =
    vtkHierarchicalBoxDataSet::SafeDownCast(src);
  if (from)
    {
    // If the source is a vtkHierarchicalBoxDataSet, do not call
    // superclass' ShallowCopy, instead skip to vtkCompositeDataSet's
    // constructor
    this->vtkCompositeDataSet::ShallowCopy(src);

    unsigned int numLevels = from->GetNumberOfLevels();
    this->SetNumberOfLevels(numLevels);
    for (unsigned int i=0; i<numLevels; i++)
      {
      unsigned int numDataSets = from->GetNumberOfDataSets(i);
      this->SetNumberOfDataSets(i, numDataSets);
      for (unsigned int j=0; j<numDataSets; j++)
        {
        vtkAMRBox box;
        vtkUniformGrid* grid = from->GetDataSet(i, j, box);
        this->SetDataSet(i, j, box, grid);
        }
      }
    }
  else
    {
    this->Superclass::ShallowCopy(src);
    }
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::DeepCopy(vtkDataObject *src)
{
  if (src == this)
    {
    return;
    }
  this->InitializeDataSets();
  this->Modified();

  vtkHierarchicalBoxDataSet* from =
    vtkHierarchicalBoxDataSet::SafeDownCast(src);
  if (from)
    {
    // If the source is a vtkHierarchicalBoxDataSet, do not call
    // superclass' DeepCopy, instead skip to vtkCompositeDataSet's
    // constructor
    this->vtkCompositeDataSet::ShallowCopy(src);

    unsigned int numLevels = from->GetNumberOfLevels();
    this->SetNumberOfLevels(numLevels);
    for (unsigned int i=0; i<numLevels; i++)
      {
      unsigned int numDataSets = from->GetNumberOfDataSets(i);
      this->SetNumberOfDataSets(i, numDataSets);
      for (unsigned int j=0; j<numDataSets; j++)
        {
        vtkAMRBox box;
        vtkUniformGrid* ds = from->GetDataSet(i, j, box);
        if (ds)
          {
          vtkUniformGrid* copy = ds->NewInstance();
          copy->DeepCopy(ds);
          this->SetDataSet(i, j, box, copy);
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkHierarchicalBoxDataSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  unsigned int numLevels = this->GetNumberOfLevels();
  os << indent << "Number of levels: " <<  numLevels << endl;
  for (unsigned int i=0; i<numLevels; i++)
    {
    unsigned int numDataSets = this->GetNumberOfDataSets(i);
    os << indent << "Level " << i << " number of datasets: " << numDataSets
       << endl;
    for (unsigned j=0; j<numDataSets; j++)
      {
      os << indent << "DataSet(" << i << "," << j << "):";
      vtkDataObject* dobj = this->GetDataSet(i, j);
      if (dobj)
        {
        os << endl;
        dobj->PrintSelf(os, indent.GetNextIndent());
        }
      else
        {
        os << "(none)" << endl;
        }
      }
    }
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSet* vtkHierarchicalBoxDataSet::GetData(
  vtkInformation* info)
{
  return
    info?vtkHierarchicalBoxDataSet::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkHierarchicalBoxDataSet* vtkHierarchicalBoxDataSet::GetData(
  vtkInformationVector* v, int i)
{
  return vtkHierarchicalBoxDataSet::GetData(v->GetInformationObject(i));
}

//----------------------------------------------------------------------------
// Description:
// Copy the cached scalar range into range.
void vtkHierarchicalBoxDataSet::GetScalarRange(double range[2])
{
  this->ComputeScalarRange();
  range[0]=this->ScalarRange[0];
  range[1]=this->ScalarRange[1];
}
  
//----------------------------------------------------------------------------
// Description:
// Return the cached range.
double *vtkHierarchicalBoxDataSet::GetScalarRange()
{
  this->ComputeScalarRange();
  return this->ScalarRange;
}

//----------------------------------------------------------------------------
// Description:
// Compute the range of the scalars and cache it into ScalarRange
// only if the cache became invalid (ScalarRangeComputeTime).
void vtkHierarchicalBoxDataSet::ComputeScalarRange()
{
  if ( this->GetMTime() > this->ScalarRangeComputeTime )
    {
    double dataSetRange[2];
    this->ScalarRange[0]=VTK_DOUBLE_MAX;
    this->ScalarRange[1]=VTK_DOUBLE_MIN;
    unsigned int level=0;
    unsigned int levels=this->GetNumberOfLevels();
    while(level<levels)
      {
      unsigned int dataset=0;
      unsigned int datasets=this->GetNumberOfDataSets(level);
      while(dataset<datasets)
        {
        vtkUniformGrid *ug;
        ug=static_cast<vtkUniformGrid *>(this->GetDataSet(level,dataset));
        ug->GetScalarRange(dataSetRange);
        if(dataSetRange[0]<this->ScalarRange[0])
          {
          this->ScalarRange[0]=dataSetRange[0];
          }
        if(dataSetRange[1]>this->ScalarRange[1])
          {
          this->ScalarRange[1]=dataSetRange[1];
          }
        ++dataset;
        }
      ++level;
      }
    this->ScalarRangeComputeTime.Modified();
    }
}
