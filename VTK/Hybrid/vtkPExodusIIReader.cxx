/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPExodusIIReader.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkPExodusIIReader.h"
#include "vtkExodusReader.h"

#ifndef MERGE_CELLS
#ifndef APPEND
#define APPEND
#endif
#endif

#ifdef APPEND
#include "vtkAppendFilter.h"
#else
#include "vtkMergeCells.h"
#endif
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkExodusModel.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkCommand.h"

#include "netcdf.h"
#include "exodusII.h"
#include <sys/stat.h>
#include <ctype.h>
#include <vtkstd/vector>

#undef DBG_PEXOIIRDR
#define vtkPExodusIIReaderMAXPATHLEN 2048

static const int objTypes[] = {
  vtkExodusIIReader::EDGE_BLOCK,
  vtkExodusIIReader::FACE_BLOCK,
  vtkExodusIIReader::ELEM_BLOCK,
  vtkExodusIIReader::NODE_SET,
  vtkExodusIIReader::EDGE_SET,
  vtkExodusIIReader::FACE_SET,
  vtkExodusIIReader::SIDE_SET,
  vtkExodusIIReader::ELEM_SET,
  vtkExodusIIReader::NODE_MAP,
  vtkExodusIIReader::EDGE_MAP,
  vtkExodusIIReader::FACE_MAP,
  vtkExodusIIReader::ELEM_MAP
};
static const int numObjTypes = sizeof(objTypes)/sizeof(objTypes[0]);

static const int objResultTypes[] = {
  vtkExodusIIReader::NODAL,
  vtkExodusIIReader::EDGE_BLOCK,
  vtkExodusIIReader::FACE_BLOCK,
  vtkExodusIIReader::ELEM_BLOCK,
  vtkExodusIIReader::NODE_SET,
  vtkExodusIIReader::EDGE_SET,
  vtkExodusIIReader::FACE_SET,
  vtkExodusIIReader::SIDE_SET,
  vtkExodusIIReader::ELEM_SET,
  vtkExodusIIReader::GLOBAL
};
static const int numObjResultTypes = sizeof(objResultTypes)/sizeof(objResultTypes[0]);

static const int objAttribTypes[] = {
  vtkExodusIIReader::EDGE_BLOCK,
  vtkExodusIIReader::FACE_BLOCK,
  vtkExodusIIReader::ELEM_BLOCK
};
static const int numObjAttribTypes = sizeof(objAttribTypes)/sizeof(objAttribTypes[0]);


vtkCxxRevisionMacro(vtkPExodusIIReader, "$Revision: 1.13.4.1 $");
vtkStandardNewMacro(vtkPExodusIIReader);

class vtkPExodusIIReaderUpdateProgress : public vtkCommand
{
public:
  vtkTypeMacro(vtkPExodusIIReaderUpdateProgress, vtkCommand)
  static vtkPExodusIIReaderUpdateProgress* New()
  {
    return new vtkPExodusIIReaderUpdateProgress;
  }
  void SetReader(vtkPExodusIIReader* r)
  {
    Reader = r;
  }
  void SetIndex(int i)
  {
    Index = i;
  }
protected:

  vtkPExodusIIReaderUpdateProgress()
  {
    Reader = NULL;
    Index = 0;
  }
  ~vtkPExodusIIReaderUpdateProgress(){}

  void Execute(vtkObject*, unsigned long event, void* callData)
  {
    if(event == vtkCommand::ProgressEvent)
    {
      double num = Reader->GetNumberOfFileNames();
      if(num == 0)
        num = Reader->GetNumberOfFiles();
      double* progress = static_cast<double*>(callData);
      double newProgress = *progress/num + Index/num;
      Reader->UpdateProgress(newProgress);
    }
  }

  vtkPExodusIIReader* Reader;
  int Index;
};


//----------------------------------------------------------------------------
// Description:
// Instantiate object with NULL filename.
vtkPExodusIIReader::vtkPExodusIIReader()
{
  this->FilePattern   = 0;
  this->CurrentFilePattern   = 0;
  this->FilePrefix    = 0;
  this->CurrentFilePrefix    = 0;
  this->FileRange[0]  = -1;
  this->FileRange[1]  = -1;
  this->CurrentFileRange[0]  = 0;
  this->CurrentFileRange[1]  = 0;
  this->NumberOfFiles = 1;
  this->FileNames = NULL;
  this->NumberOfFileNames = 0;
  this->MultiFileName = new char[vtkPExodusIIReaderMAXPATHLEN];
  this->GenerateFileIdArray = 0;
  this->XMLFileName=NULL;
  this->LastCommonTimeStep = -1;
}
 
//----------------------------------------------------------------------------
vtkPExodusIIReader::~vtkPExodusIIReader()
{
  this->SetFilePattern(0);
  this->SetFilePrefix(0);

  // If we've allocated filenames then delete them
  if ( this->FileNames ) 
    {
    for (int i=0; i<this->NumberOfFileNames; i++)
      {
      if ( this->FileNames[i] )
        {
        delete [] this->FileNames[i];
        }
      }
      delete [] this->FileNames;
    }

  // Delete all the readers we may have
  for ( int reader_idx = this->ReaderList.size() - 1; reader_idx >= 0; --reader_idx )
    {
    this->ReaderList[reader_idx]->Delete();
    this->ReaderList.pop_back();
    }

  if ( this->CurrentFilePrefix )
    {
    delete [] this->CurrentFilePrefix;
    delete [] this->CurrentFilePattern;
    }

  delete [] this->MultiFileName;
}

//----------------------------------------------------------------------------
int vtkPExodusIIReader::RequestInformation(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // Setting maximum number of pieces to -1 indicates to the
  // upstream consumer that I can provide the same number of pieces
  // as there are number of processors
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),
               -1);

  int newName = this->GetMetadataMTime() < this->FileNameMTime;

  int newPattern = 
    ((this->FilePattern &&
     !vtkExodusReader::StringsEqual(this->FilePattern, this->CurrentFilePattern)) ||
    (this->FilePrefix &&
     !vtkExodusReader::StringsEqual(this->FilePrefix, this->CurrentFilePrefix)) ||
    (this->FilePattern && 
     ((this->FileRange[0] != this->CurrentFileRange[0]) ||
      (this->FileRange[1] != this->CurrentFileRange[1]))));

  // setting filename for the first time builds the prefix/pattern
  // if one clears the prefix/pattern, but the filename stays the same,
  // we should rebuild the prefix/pattern
  int rebuildPattern = newPattern && this->FilePattern[0] == '\0' &&
                       this->FilePrefix[0] == '\0';

  int sanity = ((this->FilePattern && this->FilePrefix) || this->FileName);

  if ( ! sanity )
    {
    vtkErrorMacro(<< "Must SetFilePattern AND SetFilePrefix, or SetFileName(s)");
    return 0;
    }

  if ( newPattern && !rebuildPattern )
    {
    char *nm = 
      new char[strlen(this->FilePattern) + strlen(this->FilePrefix) + 20];  
    sprintf(nm, this->FilePattern, this->FilePrefix, this->FileRange[0]);
    this->Superclass::SetFileName(nm);
    delete [] nm;
    }
  else if ( newName || rebuildPattern )
    {
    if ( this->NumberOfFileNames == 1 )
      {
      // A singleton file may actually be a hint to look for
      // a series of files with the same base name.  Must compute
      // this now for ParaView.

      this->DeterminePattern( this->FileNames[0] );
      }
    }

  int mmd = this->ExodusModelMetadata;
  this->SetExodusModelMetadata( 0 );    // turn off for now

  // Read in info based on this->FileName
  if ( ! this->Superclass::RequestInformation( request, inputVector, outputVector ) )
    {
    return 0;
    }

  // Check whether we have been given a certain timestep to stop at. If so,
  // override the output time keys with the actual range that ALL readers can read.
  // If files are still being written to, some files might be on different timesteps
  // than others.
  if(this->LastCommonTimeStep >= 0)
    {
    double *times = outInfo->Get( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
    int numTimes = outInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
    numTimes = this->LastCommonTimeStep+1 < numTimes ? this->LastCommonTimeStep+1 : numTimes;
    vtkstd::vector<double> commonTimes;
    commonTimes.insert(commonTimes.begin(),times,times+numTimes);
    double timeRange[2];
    timeRange[1] = commonTimes[numTimes-1];
    timeRange[0] = commonTimes[0];

    outInfo->Set( vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2 );
    outInfo->Set( vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &commonTimes[0], numTimes );
    }

  this->SetExodusModelMetadata( mmd ); // turn it back, will compute in RequestData 

  if ( this->CurrentFilePrefix )
    {
    delete [] this->CurrentFilePrefix;
    this->CurrentFilePrefix = NULL;
    delete [] this->CurrentFilePattern;
    this->CurrentFilePattern = NULL;
    this->CurrentFileRange[0] = 0;
    this->CurrentFileRange[1] = 0;
    }
  if ( this->FilePrefix )
    {
    this->CurrentFilePrefix = vtkExodusReader::StrDupWithNew( this->FilePrefix );
    this->CurrentFilePattern = vtkExodusReader::StrDupWithNew( this->FilePattern );
    this->CurrentFileRange[0] = this->FileRange[0];
    this->CurrentFileRange[1] = this->FileRange[1];
    }

  return 1;
}


//----------------------------------------------------------------------------
int vtkPExodusIIReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  int fileIndex;
  int processNumber;
  int numProcessors;
  int min, max, idx;
  unsigned int reader_idx;

  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  // get the ouptut
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // The whole notion of pieces for this reader is really
  // just a division of files between processors
  processNumber =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numProcessors =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  int numFiles = this->NumberOfFileNames;
  int start = 0;

  if ( numFiles <= 1 )
    {
    start = this->FileRange[0];   // use prefix/pattern/range
    numFiles = this->NumberOfFiles;
    }

  // Someone has requested a file that is above the number
  // of pieces I have. That may have been caused by having
  // more processors than files. So I'm going to create an
  // empty unstructured grid that contains all the meta
  // information but has 0 cells
  if ( processNumber >= numFiles )
    {
#ifdef DBG_PEXOIIRDR
    vtkWarningMacro("Creating empty grid for processor: " << processNumber);
#endif
    this->SetUpEmptyGrid();
    return 1;
    }

  // Divide the files evenly between processors
  int num_files_per_process = numFiles / numProcessors;

  // This if/else logic is for when you don't have a nice even division of files
  // Each process computes which sequence of files it needs to read in
  int left_over_files = numFiles - (num_files_per_process*numProcessors);
  if ( processNumber < left_over_files )
    {
    min = (num_files_per_process+1) * processNumber + start;
    max = min + (num_files_per_process+1) - 1;
    }
  else
    {
    min = num_files_per_process * processNumber + left_over_files + start;
    max = min + num_files_per_process - 1;
    }
#ifdef DBG_PEXOIIRDR
  vtkWarningMacro("Processor: " << processNumber << " reading files: " << min <<" " <<max);
#endif

#ifdef DBG_PEXOIIRDR
  vtkWarningMacro("Parallel read for processor: " << processNumber);
#endif

  // We are going to read in the files one by one and then
  // append them together. So now we make sure that we have 
  // the correct number of serial exodus readers and we create 
  // our append object that puts the 'pieces' together
  unsigned int numMyFiles = max - min + 1;

  int totalCells = 0;
  int totalPoints = 0;
#ifdef APPEND
  vtkAppendFilter *append = vtkAppendFilter::New();
#else
  int totalSets = 0;
  vtkUnstructuredGrid** mergeGrid = new vtkUnstructuredGrid*[numMyFiles];
  memset( mergeGrid, 0, sizeof(vtkUnstructuredGrid*) );
#endif
  vtkFieldData *fieldData = vtkFieldData::New();

  if ( this->ExodusModelMetadata )
    {
    this->NewExodusModel();
    }

  if ( ReaderList.size() < numMyFiles )
    {
    for ( reader_idx = this->ReaderList.size(); reader_idx < numMyFiles; ++reader_idx )
      {
      vtkExodusIIReader* er = vtkExodusIIReader::New();
      vtkPExodusIIReaderUpdateProgress* progress = vtkPExodusIIReaderUpdateProgress::New();
      progress->SetReader( this );
      progress->SetIndex( reader_idx );
      er->AddObserver( vtkCommand::ProgressEvent, progress );
      progress->Delete();

      this->ReaderList.push_back( er );
      }
    }
  else if ( this->ReaderList.size() > numMyFiles )
    {
    for ( reader_idx = this->ReaderList.size() - 1; reader_idx >= numMyFiles; --reader_idx )
      {
      this->ReaderList[reader_idx]->Delete();
      ReaderList.pop_back();
      }
    }

  // If this is the first execution, we need to initialize the arrays
  // that store the number of points/cells output by each reader
  if(this->NumberOfCellsPerFile.size()==0)
    {
    this->NumberOfCellsPerFile.resize(max-min+1,0);
    }
  if(this->NumberOfPointsPerFile.size()==0)
    {
    this->NumberOfPointsPerFile.resize(max-min+1,0);
    }

#ifdef DBG_PEXOIIRDR
  cout << "\n\n ************************************* Parallel master reader dump\n";
  this->Dump();
#endif // DBG_PEXOIIRDR
  // This constructs the filenames
  for ( fileIndex = min, reader_idx=0; fileIndex <= max; ++fileIndex, ++reader_idx )
    {
    int fileId = -1;

    if ( this->NumberOfFileNames > 1 )
      {
      strcpy( this->MultiFileName, this->FileNames[fileIndex] );
      if ( this->GenerateFileIdArray )
        {
        fileId = vtkPExodusIIReader::DetermineFileId( this->FileNames[fileIndex] );
        }
      }
    else if ( this->FilePattern )
      {
      sprintf( this->MultiFileName, this->FilePattern, this->FilePrefix, fileIndex );
      if ( this->GenerateFileIdArray )
        {
        fileId = fileIndex;
        }
      }
    else
      {
      // hmmm.... shouldn't get here
      vtkErrorMacro("Some weird problem with filename/filepattern");
      return 0;
      }

    if ( outInfo->Has( vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS() ) )
      { // Get the requested time step. We only support requests of a single time step in this reader right now
      double* requestedTimeSteps = outInfo->Get( vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS() );

      // Save the time value in the output data information.
      int length = outInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
      double* steps = outInfo->Get( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );

      if ( ! this->GetHasModeShapes() )
        {
        // find the highest time step with a time value that is smaller than the requested time.
        int timeStep = 0;
        while (timeStep < length - 1 && steps[timeStep] < requestedTimeSteps[0])
          {
          timeStep++;
          }
        this->TimeStep = timeStep;
        this->ReaderList[reader_idx]->SetTimeStep( this->TimeStep );
        output->GetInformation()->Set( vtkDataObject::DATA_TIME_STEPS(), steps + timeStep, 1 );
        }
      else
        {
        // Let the metadata know the time value so that the Metadata->RequestData call below will generate
        // the animated mode shape properly.
        this->SetModeShapeTime( requestedTimeSteps[0] );
        this->ReaderList[reader_idx]->SetTimeStep( this->TimeStep );
        this->ReaderList[reader_idx]->SetModeShapeTime( requestedTimeSteps[0] );
        output->GetInformation()->Set( vtkDataObject::DATA_TIME_STEPS(), requestedTimeSteps, 1 );
        //output->GetInformation()->Remove( vtkDataObject::DATA_TIME_STEPS() );
        }
      }
    else
      {
      this->ReaderList[reader_idx]->SetTimeStep( this->TimeStep );
      }

    this->ReaderList[reader_idx]->SetGenerateObjectIdCellArray( this->GetGenerateObjectIdCellArray() );
    this->ReaderList[reader_idx]->SetGenerateGlobalElementIdArray( this->GetGenerateGlobalElementIdArray() );
    this->ReaderList[reader_idx]->SetGenerateGlobalNodeIdArray( this->GetGenerateGlobalNodeIdArray() );
    this->ReaderList[reader_idx]->SetApplyDisplacements( this->GetApplyDisplacements() );
    this->ReaderList[reader_idx]->SetDisplacementMagnitude( this->GetDisplacementMagnitude() );
    this->ReaderList[reader_idx]->SetHasModeShapes( this->GetHasModeShapes() );
    this->ReaderList[reader_idx]->SetEdgeFieldDecorations( this->GetEdgeFieldDecorations() );
    this->ReaderList[reader_idx]->SetFaceFieldDecorations( this->GetFaceFieldDecorations() );

    this->ReaderList[reader_idx]->SetExodusModelMetadata( this->ExodusModelMetadata );
    // For now, this *must* come last before the UpdateInformation() call because its MTime is compared to the metadata's MTime,
    // which is modified by the calls above.
    this->ReaderList[reader_idx]->SetFileName( this->MultiFileName );
    //this->ReaderList[reader_idx]->PackExodusModelOntoOutputOff();

    this->ReaderList[reader_idx]->UpdateInformation();
#ifdef DBG_PEXOIIRDR
    cout << "\n\n ************************************* Reader " << reader_idx << " dump\n";
    this->ReaderList[reader_idx]->Dump();
#endif // DBG_PEXOIIRDR

    int typ;
    for ( typ = 0; typ < numObjTypes; ++typ )
      {
      int nObj = this->ReaderList[reader_idx]->GetNumberOfObjects( objTypes[typ] );
      for ( idx = 0; idx < nObj; ++idx )
        {
        this->ReaderList[reader_idx]->SetObjectStatus( objTypes[typ], idx, this->GetObjectStatus( objTypes[typ], idx ) );
        }
      }

    for ( typ = 0; typ < numObjAttribTypes; ++typ )
      {
      int nObj = this->ReaderList[reader_idx]->GetNumberOfObjects( objAttribTypes[typ] );
      for ( idx = 0; idx < nObj; ++idx )
        {
        int nObjAtt = this->GetNumberOfObjectAttributes( objAttribTypes[typ], idx );
        for ( int aidx = 0; aidx < nObjAtt; ++aidx )
          {
          this->ReaderList[reader_idx]->SetObjectAttributeStatus( objAttribTypes[typ], idx, aidx,
            this->GetObjectAttributeStatus( objAttribTypes[typ], idx, aidx ) );
          }
        }
      }

    for ( typ = 0; typ < numObjResultTypes; ++typ )
      {
      int nObjArr = this->GetNumberOfObjectArrays( objResultTypes[typ] );
      for ( idx = 0; idx < nObjArr; ++idx )
        {
        this->ReaderList[reader_idx]->SetObjectArrayStatus(
          objResultTypes[typ], idx, this->GetObjectArrayStatus( objResultTypes[typ], idx ) );
        }
      }

    // Look for fast-path keys and propagate to sub-reader.
    // All keys must be present for the fast-path to work.
    if ( outInfo->Has( vtkStreamingDemandDrivenPipeline::FAST_PATH_OBJECT_TYPE()) && 
         outInfo->Has( vtkStreamingDemandDrivenPipeline::FAST_PATH_OBJECT_ID()) && 
         outInfo->Has( vtkStreamingDemandDrivenPipeline::FAST_PATH_ID_TYPE()))
      {
      const char *objectType = outInfo->Get(
            vtkStreamingDemandDrivenPipeline::FAST_PATH_OBJECT_TYPE());
      vtkIdType objectId = outInfo->Get(
            vtkStreamingDemandDrivenPipeline::FAST_PATH_OBJECT_ID());
      const char *idType = outInfo->Get(
            vtkStreamingDemandDrivenPipeline::FAST_PATH_ID_TYPE());

      // If the id is a VTK index, check whether it resides in this file,
      // if not, set objectId to -1. The reason we pass the keys to the reader
      // either way is to make sure the sub-reader gets re-executed. Because 
      // even if the id doesn't belong to the file, the
      // previous id from the previous fast-path request may have and we want
      // to make sure that its output field data does not include old temporal
      // arrays.
      if(strcmp(idType,"INDEX")==0)
        {
        if(strcmp(objectType,"POINT")==0)
          {
          if(objectId < totalPoints || 
             objectId >= totalPoints + this->NumberOfPointsPerFile[reader_idx])
            {
            objectId = -1;
            }
          else
            {
            objectId = objectId - totalPoints;
            }
          }
        else if(strcmp(objectType,"CELL")==0)
          {
          if(objectId < totalCells || 
             objectId >= totalCells + this->NumberOfCellsPerFile[reader_idx])
            {
            objectId = -1;
            }
          else
            {
            objectId = objectId - totalCells;
            }
          }
        }

      this->ReaderList[reader_idx]->SetFastPathObjectType(objectType);
      this->ReaderList[reader_idx]->SetFastPathObjectId(objectId);
      this->ReaderList[reader_idx]->SetFastPathIdType(idType);
      }

    this->ReaderList[reader_idx]->Update();

    vtkUnstructuredGrid* subgrid = vtkUnstructuredGrid::New();
    subgrid->ShallowCopy( this->ReaderList[reader_idx]->GetOutput() );

    int ncells = subgrid->GetNumberOfCells();

    if ( (ncells > 0) && this->GenerateFileIdArray )
      {
      vtkIntArray *ia = vtkIntArray::New();
      ia->SetNumberOfValues(ncells);
      for ( idx = 0; idx < ncells; ++idx )
        {
        ia->SetValue( idx, fileId );
        }
      ia->SetName("vtkFileId");
      subgrid->GetCellData()->AddArray( ia );
      ia->Delete();
      }

    // Don't append if you don't have any cells
    if ( ncells != 0 )
      {
      if ( this->ExodusModelMetadata )
        {
        vtkExodusModel* em = this->ReaderList[reader_idx]->GetExodusModel();
        if ( em )
          {
          this->ExodusModel->MergeExodusModel( em );
          }
        }

    // Store field data arrays of each reader to be added to the appended
    // output later:
    vtkFieldData *ifd = subgrid->GetFieldData();
    for(vtkIdType fidx = 0; fidx < ifd->GetNumberOfArrays(); fidx++)
      {
      vtkAbstractArray *farray = ifd->GetAbstractArray(fidx);

      // A special case:
      // The exodus writer needs the set of element block ids in order to write
      // out in parallel. So here we add it to the field data:
      vtkIntArray *iprevArray = vtkIntArray::SafeDownCast(fieldData->GetArray(farray->GetName()));
      vtkIntArray *ifarray = vtkIntArray::SafeDownCast(farray);
      if(iprevArray && ifarray && strcmp(farray->GetName(),"ElementBlockIds")==0)
        {
        // If there is already an array in our field data object of this name,
        // append its contents to the existing one
        for(vtkIdType fidx2=0; fidx2<ifarray->GetNumberOfTuples(); fidx2++)
          {
          iprevArray->InsertNextValue(ifarray->GetValue(fidx2));
          }
        }
      else
        {
        fieldData->AddArray(farray);
        }
      }

    totalCells += ncells;
    totalPoints += subgrid->GetNumberOfPoints();
    this->NumberOfCellsPerFile[reader_idx] = ncells;
    this->NumberOfPointsPerFile[reader_idx] = subgrid->GetNumberOfPoints();

#ifdef APPEND
      append->AddInput(subgrid);
      subgrid->Delete();
#else
      totalSets++;
      mergeGrid[reader_idx] = subgrid;
#endif
      }
    }

#ifdef APPEND
  // Append complains/barfs if you update it without any inputs
  if ( append->GetInput() != NULL ) 
    {
    append->Update();
    output->ShallowCopy(append->GetOutput());
    // vtkAppendFilter does not handle field data so we must do so separately:
    output->GetFieldData()->ShallowCopy(fieldData);
    }

  // I've copied fieldData's output to the 'output' so delete fieldData
  fieldData->Delete();
  fieldData = NULL;
  
  // I've copied append's output to the 'output' so delete append
  append->Delete();
  append = NULL;

  if ( this->PackExodusModelOntoOutput )
    {
    // The metadata is written to field arrays and attached
    // to the output unstructured grid.  (vtkMergeCells does this
    // itself, so we only have to do this for vtkAppendFilter.)

    if ( this->ExodusModel ) 
      {
      vtkModelMetadata::RemoveMetadata(output);
      this->ExodusModel->GetModelMetadata()->Pack(output);
      }
    }
#else

  // Idea: Modify vtkMergeCells to save point Id and cell Id
  // maps (these could be compressed quite a bit), and then
  // to MergeFieldArraysOnly(int idx, vtkDataSet *set) later
  // on if only field arrays have changed.  Would save a major
  // amount of time, and also leave clues to downstream filters
  // like D3 that geometry has not changed.

  vtkMergeCells *mc = vtkMergeCells::New();
  mc->SetUnstructuredGrid(output);
  mc->SetTotalNumberOfDataSets(totalSets);
  mc->SetTotalNumberOfPoints(totalPoints);
  mc->SetTotalNumberOfCells(totalCells);

  if ( this->GetGenerateGlobalNodeIdArray() )
    {
    // merge duplicate points using global IDs
    mc->SetGlobalIdArrayName( this->GetGlobalNodeIdArrayName() );
    }
  else
    {
    // don't bother trying to merge duplicate points with a locator
    mc->MergeDuplicatePointsOff();
    }

  for ( reader_idx=0; reader_idx < numMyFiles; ++reader_idx )
    {
    if ( mergeGrid[reader_idx]->GetNumberOfCells() != 0 )
      {
      mc->MergeDataSet(mergeGrid[reader_idx]);

      mergeGrid[reader_idx]->Delete();
      }
    }

  delete [] mergeGrid;

  mc->Finish();
  mc->Delete();
#endif


  // This should not be necessary. If broken, investigate
  // further.
  //this->GetOutput()->SetMaximumNumberOfPieces(-1);
  return 1;
}

void vtkPExodusIIReader::SetUpEmptyGrid()
{
  int idx;
  vtkUnstructuredGrid *output = this->GetOutput();

  // Set up an empty unstructured grid
  output->Allocate(0);

  // Create new points
  vtkPoints *newPoints = vtkPoints::New();
  newPoints->SetNumberOfPoints(0);
  output->SetPoints(newPoints);
  newPoints->Delete();
  newPoints = NULL;

  // Create point and cell arrays
  int typ;
  for ( typ = 0; typ < numObjResultTypes; ++typ )
    {
    int otyp = objResultTypes[typ];
    int nObjArr = this->GetNumberOfObjectArrays( otyp );
    for ( idx = 0; idx < nObjArr; ++idx )
      {
      vtkDoubleArray* da = vtkDoubleArray::New();
      da->SetName( this->GetObjectArrayName( otyp, idx ) );
      da->SetNumberOfComponents( this->GetNumberOfObjectArrayComponents( otyp, idx ) );
      if ( otyp == vtkExodusIIReader::NODAL )
        {
        output->GetPointData()->AddArray( da );
        }
      else
        {
        output->GetCellData()->AddArray( da );
        }
      da->FastDelete();
      }
    }

  for ( typ = 0; typ < numObjAttribTypes; ++typ )
    {
    int otyp = objAttribTypes[typ];
    int nObj = this->GetNumberOfObjects( otyp );
    for ( idx = 0; idx < nObj; ++idx )
      {
      // Attributes are defined per block, not per block type.
      int nObjAtt = this->GetNumberOfObjectAttributes( otyp, idx );
      for ( int aidx = 0; aidx < nObjAtt; ++aidx )
        {
        vtkDoubleArray* da = vtkDoubleArray::New();
        da->SetName( this->GetObjectAttributeName( otyp, idx, aidx ) );
        da->SetNumberOfComponents( 1 );
        // All attributes are cell data
        output->GetCellData()->AddArray( da );
        da->FastDelete();
        }
      }
    }

  if ( this->GetGenerateObjectIdCellArray() )
    {
    vtkIntArray* ia = vtkIntArray::New();
    ia->SetName( this->GetObjectIdArrayName() );
    ia->SetNumberOfComponents( 1 );
    output->GetCellData()->AddArray( ia );
    ia->FastDelete();
    }

  if ( this->GetGenerateGlobalNodeIdArray() )
    {
    vtkIntArray* ia = vtkIntArray::New();
    ia->SetName( this->GetGlobalNodeIdArrayName() );
    ia->SetNumberOfComponents( 1 );
    output->GetPointData()->AddArray( ia );
    ia->FastDelete();
    }

  if ( this->GetGenerateGlobalElementIdArray() )
    {
    vtkIntArray* ia = vtkIntArray::New();
    ia->SetName( this->GetGlobalElementIdArrayName() );
    ia->SetNumberOfComponents( 1 );
    output->GetCellData()->AddArray( ia );
    ia->FastDelete();
    }
}

//----------------------------------------------------------------------------
void vtkPExodusIIReader::SetFileRange(int min, int max)
{
  if ( min == this->FileRange[0] && max == this->FileRange[1] )
    {  
    return;
    }
  this->FileRange[0] = min;
  this->FileRange[1] = max;
  this->NumberOfFiles = max-min+1;
  this->Modified();
}
//----------------------------------------------------------------------------
void vtkPExodusIIReader::SetFileName(const char *name)
{
  this->SetFileNames(1, &name);
}
void vtkPExodusIIReader::SetFileNames(int nfiles, const char **names)
{
  // If I have an old list of filename delete them
  if ( this->FileNames )
    {
    for (int i=0; i<this->NumberOfFileNames; i++)
      {
      if ( this->FileNames[i] )
        {
        delete [] this->FileNames[i];
        }
      }
    delete [] this->FileNames;
    this->FileNames = NULL;
    }

  // Set the number of files
  this->NumberOfFileNames = nfiles;

  // Allocate memory for new filenames
  this->FileNames = new char * [this->NumberOfFileNames];

  // Copy filenames
  for (int i=0; i<nfiles; i++)
    {
    this->FileNames[i] = vtkExodusReader::StrDupWithNew(names[i]);
    }

  vtkExodusIIReader::SetFileName(names[0]);
}

//----------------------------------------------------------------------------
int vtkPExodusIIReader::DetermineFileId( const char* file )
{
  // Assume the file number is the last digits found in the file name.
  int fileId = 0;
  const char *start = file;
  const char *end = file + strlen(file) - 1;
  const char *numString = end;

  if ( ! isdigit( *numString ) )
    {
    while ( numString > start )
      {
      --numString;
      if ( isdigit( *numString ) ) break;
      }

    if ( numString == start )
      {
      if ( isdigit( *numString ) )
        {
        fileId = atoi( numString );
        }
      return fileId;  // no numbers in file name
      }
    }

  while(numString > start)
    {
    --numString;
    if ( ! isdigit( *numString ) ) break;
    }

  if ( (numString == start) && (isdigit(*numString)))
    {
    fileId = atoi(numString);
    }
  else
    {
    fileId = atoi(++numString);
    }

  return fileId;
}

int vtkPExodusIIReader::DeterminePattern( const char* file )
{
  char* prefix = vtkExodusReader::StrDupWithNew(file);
  int slen = strlen( file );
  char pattern[20] = "%s";
  int scount = 0;
  int cc = 0;
  int res =0;
  int min=0, max=0;

  
  // Check for specific extensions
  // If .ex2 or .ex2v2 is present do not look for a numbered sequence.
  char* ex2 = strstr(prefix, ".ex2");
  char* ex2v2 = strstr(prefix, ".ex2v2");
  if ( ex2 || ex2v2 )
    {
    // Set my info
    this->SetFilePattern( pattern );
    this->SetFilePrefix( prefix );
    this->SetFileRange( min, max );
    delete [] prefix;
    return VTK_OK;
    }

  char* ex2v3 = strstr(prefix, ".ex2v3");
  // Find minimum of range, if any
  for ( cc = ex2v3 ? ex2v3 - prefix - 1 : slen - 1; cc >= 0; --cc )
    {
    if ( prefix[cc] >= '0' && prefix[cc] <= '9' )
      {
      prefix[cc] = 0;
      scount ++;
      }
    else if ( prefix[cc] == '.' )
      {
      prefix[cc] = 0;
      break;
      }
    else
      {
      break;
      }
    }

  // Determine the pattern
  if ( scount > 0 )
    {
    res = sscanf( file + (ex2v3 ? ex2v3 - prefix - scount : slen - scount), "%d", &min);
    if ( res )
      {
      if ( ex2v3 )
        {
        sprintf( pattern, "%%s.%%0%ii%s", scount, file + (ex2v3 - prefix) );
        }
      else
        {
        sprintf( pattern, "%%s.%%0%ii", scount );
        }
      }
    }

  // Count up the files
  char buffer[1024];
  struct stat fs;
  
  // First go up every 100
  for ( cc = min + 100; res; cc += 100 )
    {
    sprintf( buffer, pattern, prefix, cc );

    // Stat returns -1 if file NOT found
    if ( stat( buffer, &fs ) == -1 )
      break;

    }
  // Okay if I'm here than stat has failed so -100 on my cc
  cc = cc - 100;
  for (cc = cc + 1; res; ++cc )
    {
    sprintf( buffer, pattern, prefix, cc );

    // Stat returns -1 if file NOT found
    if ( stat( buffer, &fs ) == -1)
      break;

    }
  // Okay if I'm here than stat has failed so -1 on my cc
  max = cc - 1;

  // Second, go down every 100
  // We can't assume that we're starting at 0 because the file selector
  // will pick up every file that ends in .ex2v3... not just the first one.
  for ( cc = min - 100; res; cc -= 100 )
    {
    if ( cc < 0 )
      break;

    sprintf( buffer, pattern, prefix, cc );

    // Stat returns -1 if file NOT found
    if ( stat( buffer, &fs ) == -1 )
      break;

    }

  cc += 100;
  // Okay if I'm here than stat has failed so -100 on my cc
  for (cc = cc - 1; res; --cc )
    {
    if ( cc < 0 )
      break;

    sprintf( buffer, pattern, prefix, cc );

    // Stat returns -1 if file NOT found
    if ( stat( buffer, &fs ) == -1)
      break;

    }
  min = cc + 1;

  // If the user did not specify a range before this, 
  // than set the range to the min and max
  if ( (this->FileRange[0] == -1) && (this->FileRange[1] == -1) )
    {
    this->SetFileRange( min, max );
    }

   // Set my info
  this->SetFilePattern( pattern );
  this->SetFilePrefix( prefix );
  delete [] prefix;

  return VTK_OK;
}

void vtkPExodusIIReader::SetGenerateFileIdArray(int flag)
{
  this->GenerateFileIdArray = flag;
  this->Modified();
}

 
//----------------------------------------------------------------------------
void vtkPExodusIIReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkExodusIIReader::PrintSelf(os,indent);

  if ( this->FilePattern )
    {
    os << indent << "FilePattern: " << this->FilePattern << endl;
    }
  else
    {
    os << indent << "FilePattern: NULL\n";
    }

  if ( this->FilePattern )
    {
    os << indent << "FilePrefix: " << this->FilePrefix << endl;
    }
  else
    {
    os << indent << "FilePrefix: NULL\n";
    }

  os << indent << "FileRange: " 
     << this->FileRange[0] << " " << this->FileRange[1] << endl;

  os << indent << "GenerateFileIdArray: " << this->GenerateFileIdArray << endl;
  os << indent << "NumberOfFiles: " << this->NumberOfFiles << endl;
}

int vtkPExodusIIReader::GetTotalNumberOfElements()
{
  int total = 0;
  for(int id=ReaderList.size()-1; id >= 0; --id)
    {
    total += this->ReaderList[id]->GetTotalNumberOfElements();
    }
  return total;
}

int vtkPExodusIIReader::GetTotalNumberOfNodes()
{
  int total = 0;
  for(int id=ReaderList.size()-1; id >= 0; --id)
    {
    total += this->ReaderList[id]->GetTotalNumberOfNodes();
    }
  return total;
}

void vtkPExodusIIReader::UpdateTimeInformation()
{
  // Before we start, make sure that we have readers to read (i.e. that 
  // RequestData() has been called. 
  if(this->ReaderList.size() == 0)
    {
    return;
    }

  int lastTimeStep = VTK_INT_MAX;
  int numTimeSteps = 0;
  for ( unsigned int reader_idx=0; reader_idx<this->ReaderList.size(); ++reader_idx )
    {
    vtkExodusIIReader *reader = this->ReaderList[reader_idx];

    // In order to get an up-to-date number of timesteps, update the reader's
    // time information first
    reader->UpdateTimeInformation();
    numTimeSteps = reader->GetNumberOfTimeSteps();

    // if this reader's last time step is less than the one we have, use it instead
    lastTimeStep = numTimeSteps-1 < lastTimeStep ? numTimeSteps-1 : lastTimeStep;
    }

  this->LastCommonTimeStep = lastTimeStep;

  this->Superclass::UpdateTimeInformation();
  this->Modified();
  this->UpdateInformation();
}

