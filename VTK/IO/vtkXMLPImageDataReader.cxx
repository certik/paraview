/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkXMLPImageDataReader.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPImageDataReader.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLImageDataReader.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkCxxRevisionMacro(vtkXMLPImageDataReader, "$Revision: 1.10 $");
vtkStandardNewMacro(vtkXMLPImageDataReader);

//----------------------------------------------------------------------------
vtkXMLPImageDataReader::vtkXMLPImageDataReader()
{
  vtkImageData *output = vtkImageData::New();
  this->SetOutput(output);
  // Releasing data for pipeline parallism.
  // Filters will know it is empty. 
  output->ReleaseData();
  output->Delete();
}

//----------------------------------------------------------------------------
vtkXMLPImageDataReader::~vtkXMLPImageDataReader()
{
}

//----------------------------------------------------------------------------
void vtkXMLPImageDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkXMLPImageDataReader::SetOutput(vtkImageData *output)
{
  this->GetExecutive()->SetOutputData(0, output);
}

//----------------------------------------------------------------------------
vtkImageData* vtkXMLPImageDataReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkImageData* vtkXMLPImageDataReader::GetOutput(int idx)
{
  return vtkImageData::SafeDownCast( this->GetOutputDataObject(idx) );
}

//----------------------------------------------------------------------------
vtkImageData* vtkXMLPImageDataReader::GetPieceInput(int index)
{
  vtkXMLImageDataReader* reader =
    static_cast<vtkXMLImageDataReader*>(this->PieceReaders[index]);
  return reader->GetOutput();
}

//----------------------------------------------------------------------------
const char* vtkXMLPImageDataReader::GetDataSetName()
{
  return "PImageData";
}

//----------------------------------------------------------------------------
void vtkXMLPImageDataReader::SetOutputExtent(int* extent)
{
  this->GetOutput()->SetExtent(extent);
}

//----------------------------------------------------------------------------
void vtkXMLPImageDataReader::GetPieceInputExtent(int index, int* extent)
{
  this->GetPieceInput(index)->GetExtent(extent);
}

//----------------------------------------------------------------------------
int vtkXMLPImageDataReader::ReadPrimaryElement(vtkXMLDataElement* ePrimary)
{
  if(!this->Superclass::ReadPrimaryElement(ePrimary)) { return 0; }
  
  // Get the image's origin.
  if(ePrimary->GetVectorAttribute("Origin", 3, this->Origin) != 3)
    {
    this->Origin[0] = 0;
    this->Origin[1] = 0;
    this->Origin[2] = 0;
    }
  
  // Get the image's spacing.
  if(ePrimary->GetVectorAttribute("Spacing", 3, this->Spacing) != 3)
    {
    this->Spacing[0] = 1;
    this->Spacing[1] = 1;
    this->Spacing[2] = 1;
    }
  
  return 1;
}

//----------------------------------------------------------------------------
// Note that any changes (add or removing information) made to this method
// should be replicated in CopyOutputInformation
void vtkXMLPImageDataReader::SetupOutputInformation(vtkInformation *outInfo)
{
  this->Superclass::SetupOutputInformation(outInfo);
  
  outInfo->Set(vtkDataObject::ORIGIN(),  this->Origin, 3);
  outInfo->Set(vtkDataObject::SPACING(), this->Spacing, 3);
}


//----------------------------------------------------------------------------
void vtkXMLPImageDataReader::CopyOutputInformation(vtkInformation *outInfo, int port)
  {
  this->Superclass::CopyOutputInformation(outInfo, port);
  vtkInformation *localInfo = this->GetExecutive()->GetOutputInformation( port );

  if ( localInfo->Has(vtkDataObject::ORIGIN()) )
    {
    outInfo->CopyEntry( localInfo, vtkDataObject::ORIGIN() );
    }
  if ( localInfo->Has(vtkDataObject::SPACING()) )
    {
    outInfo->CopyEntry( localInfo, vtkDataObject::SPACING() );
    }
  }


//----------------------------------------------------------------------------
vtkXMLDataReader* vtkXMLPImageDataReader::CreatePieceReader()
{
  return vtkXMLImageDataReader::New();
}



//----------------------------------------------------------------------------
int vtkXMLPImageDataReader::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  return 1;
}

