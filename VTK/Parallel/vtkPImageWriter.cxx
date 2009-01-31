/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPImageWriter.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPImageWriter.h"

#include "vtkObjectFactory.h"
#include "vtkPipelineSize.h"
#include "vtkImageData.h"

#define vtkPIWCloseFile \
    if (file && fileOpenedHere) \
      { \
      this->WriteFileTrailer(file,cache); \
      file->close(); \
      delete file; \
      file = NULL; \
      } \

vtkCxxRevisionMacro(vtkPImageWriter, "$Revision: 1.8 $");
vtkStandardNewMacro(vtkPImageWriter);

#ifdef write
#undef write
#endif

#ifdef close
#undef close
#endif


//----------------------------------------------------------------------------
vtkPImageWriter::vtkPImageWriter()
{
  // Set a default memory limit of a gigabyte
  this->MemoryLimit = 1000000; 

  this->SizeEstimator = vtkPipelineSize::New();
}



//----------------------------------------------------------------------------
vtkPImageWriter::~vtkPImageWriter()
{
  if (this->SizeEstimator)
    {
    this->SizeEstimator->Delete();
    }
}


//----------------------------------------------------------------------------
void vtkPImageWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "MemoryLimit: " << this->MemoryLimit << "\n";
}


//----------------------------------------------------------------------------
// Breaks region into pieces with correct dimensionality.
void vtkPImageWriter::RecursiveWrite(int axis, vtkImageData *cache,
                                    ofstream *file)
{
  int             min, max, mid;
  vtkImageData    *data;
  int             fileOpenedHere = 0;
  int             *ext;
  unsigned long   inputMemorySize;

  // if we need to open another slice, do it
  if (!file && (axis + 1) == this->FileDimensionality)
    {
    // determine the name
    if (this->FileName)
      {
      sprintf(this->InternalFileName,"%s",this->FileName);
      }
    else 
      {
      if (this->FilePrefix)
        {
        sprintf(this->InternalFileName, this->FilePattern, 
                this->FilePrefix, this->FileNumber);
        }
      else
        {
        sprintf(this->InternalFileName, this->FilePattern,this->FileNumber);
        }
      }
    // Open the file
#ifdef _WIN32
    file = new ofstream(this->InternalFileName, ios::out | ios::binary);
#else
    file = new ofstream(this->InternalFileName, ios::out);
#endif
    fileOpenedHere = 1;
    if (file->fail())
      {
      vtkErrorMacro("RecursiveWrite: Could not open file " << 
                    this->InternalFileName);
      delete file;
      return;
      }

    // Subclasses can write a header with this method call.
    this->WriteFileHeader(file, cache);
    ++this->FileNumber;
    }
  
  // Propagate the update extent so we can determine pipeline size
  this->GetInput()->PropagateUpdateExtent();

  // Now we can ask how big the pipeline will be
  inputMemorySize = this->SizeEstimator->GetEstimatedSize(this,0,0);

  // will the current request fit into memory
  // if so the just get the data and write it out
  if ( inputMemorySize < this->MemoryLimit )
    {
    ext = cache->GetUpdateExtent();
    vtkDebugMacro("Getting input extent: " << ext[0] << ", " << ext[1] << ", " << ext[2] << ", " << ext[3] << ", " << ext[4] << ", " << ext[5] << endl);
    cache->Update();
    data = cache;
    this->RecursiveWrite(axis,cache,data,file);
    vtkPIWCloseFile;
    return;
    }

  // if the current request did not fit into memory
  // the we will split the current axis
  this->GetInput()->GetAxisUpdateExtent(axis, min, max);
  
  vtkDebugMacro("Axes: " << axis << "(" << min << ", " << max 
        << "), UpdateMemory: " << inputMemorySize 
        << ", Limit: " << this->MemoryLimit << endl);
  
  if (min == max)
    {
    if (axis > 0)
      {
      this->RecursiveWrite(axis - 1,cache, file);
      }
    else
      {
      vtkWarningMacro("MemoryLimit too small for one pixel of information!!");
      }
    vtkPIWCloseFile;
    return;
    }
  
  mid = (min + max) / 2;

  // if it is the y axis then flip by default
  if (axis == 1 && !this->FileLowerLeft)
    {
    // first half
    cache->SetAxisUpdateExtent(axis, mid+1, max);
    this->RecursiveWrite(axis,cache,file);
    
    // second half
    cache->SetAxisUpdateExtent(axis, min, mid);
    this->RecursiveWrite(axis,cache,file);
    }
  else
    {
    // first half
    cache->SetAxisUpdateExtent(axis, min, mid);
    this->RecursiveWrite(axis,cache,file);
    
    // second half
    cache->SetAxisUpdateExtent(axis, mid+1, max);
    this->RecursiveWrite(axis,cache,file);
    }
    
  // restore original extent
  cache->SetAxisUpdateExtent(axis, min, max);

  // if we opened the file here, then we need to close it up
  vtkPIWCloseFile;
}



