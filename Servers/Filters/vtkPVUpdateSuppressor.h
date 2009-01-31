/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVUpdateSuppressor.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVUpdateSuppressor - prevents propagation of update
// .SECTION Description  I am also going to have this object manage
// flip books (geometry cache).

#ifndef __vtkPVUpdateSuppressor_h
#define __vtkPVUpdateSuppressor_h

#include "vtkDataObjectAlgorithm.h"

class vtkCacheSizeKeeper;
class vtkPVUpdateSuppressorCacheMap;

class VTK_EXPORT vtkPVUpdateSuppressor : public vtkDataObjectAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkPVUpdateSuppressor,vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with user-specified implicit function.
  static vtkPVUpdateSuppressor *New();

  // Description:
  // Methods for saving, clearing and updating flip books.
  // Cache update will update and save cache or just use previous cache.
  // "idx" is the time index, "total" is the number of time steps.
  void RemoveAllCaches();

  // Description:
  // Force update with caching, cacheTime is the key used to save/restore the
  // cached data.
  void CacheUpdate(double cacheTime);

  // Description:
  // Force update on the input.
  void ForceUpdate();

  // Description:
  // Set number of pieces and piece on the data.
  // This causes the filter to ingore the request from the output.
  // It is here because the user may not have celled update on the output
  // before calling force update (it is an easy fix).
  vtkSetMacro(UpdatePiece, int);
  vtkGetMacro(UpdatePiece, int);
  vtkSetMacro(UpdateNumberOfPieces, int);
  vtkGetMacro(UpdateNumberOfPieces, int);

  // Description:
  // Get/Set if the update suppressor is enabled. If the update suppressor 
  // is not enabled, it won't supress any updates. Enabled by default.
  void SetEnabled(int);
  vtkGetMacro(Enabled, int);

  // Description:
  // Get/Set the update time that is sent up the pipeline.
  void SetUpdateTime(double utime);
  vtkGetMacro(UpdateTime, double);

  // Description:
  // Get/Set the cache size keeper. The update suppressor
  // reports its cache size to this keeper, if any.
  void SetCacheSizeKeeper(vtkCacheSizeKeeper*);
  vtkGetObjectMacro(CacheSizeKeeper, vtkCacheSizeKeeper);

protected:
  vtkPVUpdateSuppressor();
  ~vtkPVUpdateSuppressor();

  int RequestDataObject(vtkInformation* request, vtkInformationVector **inputVector,
    vtkInformationVector *outputVector);
  int RequestData(vtkInformation* request, vtkInformationVector **inputVector,
    vtkInformationVector *outputVector);
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  int UpdatePiece;
  int UpdateNumberOfPieces;
  double UpdateTime;

  bool UpdateTimeInitialized;

  int Enabled;

  vtkCacheSizeKeeper* CacheSizeKeeper;
  vtkTimeStamp PipelineUpdateTime;

  int SaveCacheOnCacheUpdate;

  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();

  // This can be removed when streaming is removed.
  int PreviousUpdateWasBlockedByStreaming;  

  vtkPVUpdateSuppressorCacheMap* Cache;
private:
  vtkPVUpdateSuppressor(const vtkPVUpdateSuppressor&);  // Not implemented.
  void operator=(const vtkPVUpdateSuppressor&);  // Not implemented.
};

#endif
