/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkHierarchicalBoxDataSet.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalBoxDataSet - hierarchical dataset of vtkUniformGrids
// .SECTION Description
// vtkHierarchicalBoxDataSet is a concrete implementation of
// vtkHierarchicalDataSet. The dataset type is restricted to
// vtkUniformGrid. Each dataset has an associated vtkAMRBox that represents
// it's region (similar to extent) in space.
// .SECTION Warning
// To compute the cellId of a cell within a vtkUniformGrid with AMRBox=box, 
// you should not use vtkUniformGrid::ComputeCellId( {x,y,z} ) but instead
// use the following pseudo code:
// for (int i=0; i<3; i++)
//   {
//   cellDims[i] = box.HiCorner[i] - box.LoCorner[i] + 1;
//   }
// vtkIdType cellId =
//   (z-box.LoCorner[2])*cellDims[0]*cellDims[1] +
//   (y-box.LoCorner[1])*cellDims[0] +
//   (x-box.LoCorner[0]);


#ifndef __vtkHierarchicalBoxDataSet_h
#define __vtkHierarchicalBoxDataSet_h

#include "vtkHierarchicalDataSet.h"

//BTX
struct vtkHierarchicalBoxDataSetInternal;
//ETX
class vtkDataObject;
class vtkInformationIdTypeKey;
class vtkUniformGrid;
class vtkAMRBox;

class VTK_FILTERING_EXPORT vtkHierarchicalBoxDataSet : public vtkHierarchicalDataSet
{
public:
  static vtkHierarchicalBoxDataSet *New();

  vtkTypeRevisionMacro(vtkHierarchicalBoxDataSet,vtkHierarchicalDataSet);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return class name of data type (see vtkType.h for definitions).
  virtual int GetDataObjectType() {return VTK_HIERARCHICAL_BOX_DATA_SET;}

//BTX
  // Description:
  // Set the dataset pointer for a given node. This method does
  // not remove the existing parent/child links. It only replaces
  // the dataset pointer.
  void SetDataSet(unsigned int level, unsigned int id, 
                  vtkAMRBox& box, vtkUniformGrid* dataSet);
  void SetDataSet(unsigned int level, unsigned int id, vtkDataObject* dataSet)
    {
    this->Superclass::SetDataSet(level, id, dataSet);
    }

  // Description:
  // Get a dataset given a level and an id.
  vtkUniformGrid* GetDataSet(unsigned int level,
                             unsigned int id,
                             vtkAMRBox& box);
//ETX
  vtkDataObject* GetDataSet(unsigned int level, unsigned int id)
    { return this->Superclass::GetDataSet(level, id); }

  vtkDataObject* GetDataSet(vtkInformation* index)
    { return this->Superclass::GetDataSet(index); }

  // Description:
  // Sets the refinement of a given level. The spacing at level
  // level+1 is defined as spacing(level+1) = spacing(level)/refRatio(level).
  // Note that currently, this is not enforced by this class however
  // some algorithms might not function properly if the spacing in
  // the blocks (vtkUniformGrid) does not match the one described
  // by the refinement ratio.
  void SetRefinementRatio(unsigned int level, int refRatio);

  // Description:
  // Returns the refinement of a given level.
  int GetRefinementRatio(unsigned int level);

  // Description:
  // Blank lower level cells if they are overlapped by higher
  // level ones.
  void GenerateVisibilityArrays();

  // Description:
  // Shallow and Deep copy.
  virtual void ShallowCopy(vtkDataObject *src);
  virtual void DeepCopy(vtkDataObject *src);

  static vtkInformationIntegerVectorKey* BOX();
  static vtkInformationIdTypeKey* NUMBER_OF_BLANKED_POINTS();

  // Description:
  // Returns the total number of points of all blocks. This will
  // iterate over all blocks and call GetNumberOfPoints() so it
  // might be expensive. Does not include the number of blanked
  // points.
  virtual vtkIdType GetNumberOfPoints();

  //BTX
  // Description:
  // Retrieve an instance of this class from an information object.
  static vtkHierarchicalBoxDataSet* GetData(vtkInformation* info);
  static vtkHierarchicalBoxDataSet* GetData(vtkInformationVector* v, int i=0);
  //ETX

  // Description:
  // Copy the cached scalar range into range.
  virtual void GetScalarRange(double range[2]);
  
  // Description:
  // Return the cached range.
  virtual double *GetScalarRange();
  
protected:
  vtkHierarchicalBoxDataSet();
  ~vtkHierarchicalBoxDataSet();

  // Description:
  // Compute the range of the scalars and cache it into ScalarRange
  // only if the cache became invalid (ScalarRangeComputeTime).
  virtual void ComputeScalarRange();
  
  vtkHierarchicalBoxDataSetInternal* BoxInternal;
  
  // Cached scalar range
  double ScalarRange[2];
  // Time at which scalar range is computed
  vtkTimeStamp ScalarRangeComputeTime;

private:
  vtkHierarchicalBoxDataSet(const vtkHierarchicalBoxDataSet&);  // Not implemented.
  void operator=(const vtkHierarchicalBoxDataSet&);  // Not implemented.
};

#endif

