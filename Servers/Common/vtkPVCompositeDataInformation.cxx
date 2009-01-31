/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVCompositeDataInformation.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCompositeDataInformation.h"

#include "vtkClientServerStream.h"
#include "vtkMultiGroupDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkTimerLog.h"

#include "vtkSmartPointer.h"
#include <vtkstd/vector>

#include "vtkAlgorithmOutput.h"
#include "vtkAlgorithm.h"

vtkStandardNewMacro(vtkPVCompositeDataInformation);
vtkCxxRevisionMacro(vtkPVCompositeDataInformation, "$Revision: 1.9 $");

struct vtkPVCompositeDataInformationInternals
{
  typedef 
  vtkstd::vector<vtkSmartPointer<vtkPVDataInformation> > 
  GroupDataInformationType;

  typedef 
  vtkstd::vector<GroupDataInformationType> DataInformationType;

  DataInformationType DataInformation;
};

//----------------------------------------------------------------------------
vtkPVCompositeDataInformation::vtkPVCompositeDataInformation()
{
  this->Internal = new vtkPVCompositeDataInformationInternals;
  this->DataIsComposite = 0;
  this->DataIsHierarchical = 0;
}

//----------------------------------------------------------------------------
vtkPVCompositeDataInformation::~vtkPVCompositeDataInformation()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "DataIsComposite: " << this->DataIsComposite << endl;
  os << indent << "DataIsHierarchical: " << this->DataIsHierarchical << endl;
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataInformation::Initialize()
{
  this->DataIsComposite = 0;
  this->Internal->DataInformation.clear();
}

//----------------------------------------------------------------------------
unsigned int vtkPVCompositeDataInformation::GetNumberOfGroups()
{
  return this->Internal->DataInformation.size();
}

//----------------------------------------------------------------------------
unsigned int vtkPVCompositeDataInformation::GetNumberOfDataSets(
  unsigned int level)
{
  if (level >= this->GetNumberOfGroups())
    {
    return 0;
    }

  vtkPVCompositeDataInformationInternals::GroupDataInformationType& ldata = 
    this->Internal->DataInformation[level];

  return ldata.size();
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkPVCompositeDataInformation::GetDataInformation(
  unsigned int level, unsigned int idx)
{
  if (level >= this->GetNumberOfGroups())
    {
    return 0;
    }

  vtkPVCompositeDataInformationInternals::GroupDataInformationType& ldata = 
    this->Internal->DataInformation[level];

  if (idx >= ldata.size())
    {
    return 0;
    }

  return ldata[idx];
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataInformation::CopyFromObject(vtkObject* object)
{
  this->Initialize();

  vtkMultiGroupDataSet* hds = 
    vtkMultiGroupDataSet::SafeDownCast(object);
  if (!hds)
    {
    return;
    }

  if (hds->IsA("vtkHierarchicalDataSet"))
    {
    this->DataIsHierarchical = 1;
    }

//  vtkTimerLog::MarkStartEvent("Copying information from composite data");

  this->DataIsComposite = 1;

  unsigned int numGroups = hds->GetNumberOfGroups();
  this->Internal->DataInformation.resize(numGroups);
  for (unsigned int i=0; i<numGroups; i++)
    {
    vtkPVCompositeDataInformationInternals::GroupDataInformationType& ldata = 
      this->Internal->DataInformation[i];
    unsigned int numDataSets = hds->GetNumberOfDataSets(i);
    ldata.resize(numDataSets);
    // If data is a vtkHierarchicalDataSet or sub-class, do not get the
    // information for sub-datasets. There may be a lot of them.
    // Temprorarily disabling this. When an AMR dataset gets converted to
    // a multi-group dataset due to filter application, this was causing
    // large amount meta-data delivery to the client.
    /*
    if (!this->DataIsHierarchical)
      {
      vtkTimerLog::MarkStartEvent("Copying information from hierarchical data");
      for (unsigned int j=0; j<numDataSets; j++)
        {
        vtkDataObject* dobj = hds->GetDataSet(i, j);
        if (dobj)
          {
          vtkPVDataInformation* dataInf = vtkPVDataInformation::New();
          if (dobj->IsA("vtkCompositeDataSet"))
            {
            dataInf->CopyFromCompositeDataSet(
              static_cast<vtkCompositeDataSet*>(dobj), 0);
            }
          else
            {
            dataInf->CopyFromObject(dobj);
            }
          ldata[j] = dataInf;
          dataInf->Delete();
          }
        }
      vtkTimerLog::MarkEndEvent("Copying information from hierarchical data");
      }
    */
    }

//  vtkTimerLog::MarkEndEvent("Copying information from composite data");
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataInformation::AddInformation(vtkPVInformation* pvi)
{
  vtkPVCompositeDataInformation *info;

  info = vtkPVCompositeDataInformation::SafeDownCast(pvi);
  if (info == NULL)
    {
    vtkErrorMacro("Cound not cast object to data information.");
    return;
    }

  this->DataIsComposite = info->GetDataIsComposite();
  this->DataIsHierarchical = info->GetDataIsHierarchical();

  unsigned int otherNumGroups = info->Internal->DataInformation.size();
  unsigned int numGroups = this->Internal->DataInformation.size();
  if ( otherNumGroups > numGroups )
    {
    numGroups = otherNumGroups;
    this->Internal->DataInformation.resize(numGroups);
    }

  for (unsigned int i=0; i < otherNumGroups; i++)
    {
    vtkPVCompositeDataInformationInternals::GroupDataInformationType& 
      otherldata = info->Internal->DataInformation[i];
    vtkPVCompositeDataInformationInternals::GroupDataInformationType& ldata = 
      this->Internal->DataInformation[i];
    unsigned otherNumDataSets = otherldata.size();
    unsigned numDataSets = ldata.size();
    if (otherNumDataSets > numDataSets)
      {
      numDataSets = otherNumDataSets;
      ldata.resize(numDataSets);
      }
    // If data is a vtkHierarchicalDataSet or sub-class, do not get the
    // information for sub-datasets. There may be a lot of them.
    if (!this->DataIsHierarchical)
      {
      for (unsigned int j=0; j < otherNumDataSets; j++)
        {
        vtkPVDataInformation* otherInfo = otherldata[j];
        vtkPVDataInformation* localInfo = ldata[j];
        if (otherInfo)
          {
          if (localInfo)
            {
            localInfo->AddInformation(otherInfo);
            }
          else
            {
            vtkPVDataInformation* dinf = vtkPVDataInformation::New();
            dinf->AddInformation(otherInfo);
            ldata[j] = dinf;
            dinf->Delete();
            }
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataInformation::CopyToStream(
  vtkClientServerStream* css)
{
  unsigned int i, j;

//  vtkTimerLog::MarkStartEvent("Copying composite information to stream");
  css->Reset();
  *css << vtkClientServerStream::Reply;
  *css << this->DataIsComposite;
  *css << this->DataIsHierarchical;
  if (!this->DataIsComposite)
    {
//    vtkTimerLog::MarkEndEvent("Copying composite information to stream");
    *css << vtkClientServerStream::End;
    return;
    }
  unsigned int numGroups = this->Internal->DataInformation.size();
  *css << numGroups;
  for(i=0; i<numGroups; i++)
    {
    vtkPVCompositeDataInformationInternals::GroupDataInformationType& ldata = 
      this->Internal->DataInformation[i];
    *css << ldata.size();
    }

  // If data is a vtkHierarchicalDataSet or sub-class, do not get the
  // information for sub-datasets. There may be a lot of them.
  if (!this->DataIsHierarchical)
    {
    vtkClientServerStream dcss;
    size_t length;
    const unsigned char* data;
    
    for(i=0; i<numGroups; i++)
      {
      vtkPVCompositeDataInformationInternals::GroupDataInformationType& ldata = 
        this->Internal->DataInformation[i];
      unsigned int numDataSets = ldata.size();
      for(j=0; j<numDataSets; j++)
        {
        vtkPVDataInformation* dataInf = ldata[j];
        if (dataInf)
          {
          *css << i;
          *css << j;
          dcss.Reset();
          dataInf->CopyToStream(&dcss);
          dcss.GetData(&data, &length);
          *css << vtkClientServerStream::InsertArray(data, length);
          }
        }
      }
    *css << numGroups;
    *css << vtkClientServerStream::End;
    }
//  vtkTimerLog::MarkEndEvent("Copying composite information to stream");
}

//----------------------------------------------------------------------------
void vtkPVCompositeDataInformation::CopyFromStream(
  const vtkClientServerStream* css)
{
  if(!css->GetArgument(0, 0, &this->DataIsComposite))
    {
    vtkErrorMacro("Error parsing data set type.");
    return;
    }
  if (!this->DataIsComposite)
    {
    return;
    }
  if(!css->GetArgument(0, 1, &this->DataIsHierarchical))
    {
    vtkErrorMacro("Error parsing data set type.");
    return;
    }
  unsigned int numGroups;
  if(!css->GetArgument(0, 2, &numGroups))
    {
    vtkErrorMacro("Error parsing data set type.");
    return;
    }
  int msgIdx = 2;
  this->Internal->DataInformation.resize(numGroups);
  for (unsigned int i=0; i<numGroups; i++)
    {
    unsigned int numDataSets;
    msgIdx++;
    if(!css->GetArgument(0, msgIdx, &numDataSets))
      {
      vtkErrorMacro("Error parsing data set type.");
      return;
      }
    vtkPVCompositeDataInformationInternals::GroupDataInformationType& ldata = 
      this->Internal->DataInformation[i];
    ldata.resize(numDataSets);
    }

  // If data is a vtkHierarchicalDataSet or sub-class, do not get the
  // information for sub-datasets. There may be a lot of them.
  if (this->DataIsHierarchical)
    {
    return;
    }

  while (1)
    {
    msgIdx++;
    unsigned int levelIdx, dataSetIdx;
    if(!css->GetArgument(0, msgIdx, &levelIdx))
      {
      vtkErrorMacro("Error parsing data set type.");
      return;
      }
    if (levelIdx >= numGroups)
      {
      break;
      }
    msgIdx++;
    if(!css->GetArgument(0, msgIdx, &dataSetIdx))
      {
      vtkErrorMacro("Error parsing data set type.");
      return;
      }

    vtkTypeUInt32 length;
    vtkstd::vector<unsigned char> data;
    vtkClientServerStream dcss;
    
    msgIdx++;
    // Data information.
    vtkPVDataInformation* dataInf = vtkPVDataInformation::New();
    if(!css->GetArgumentLength(0, msgIdx, &length))
      {
      vtkErrorMacro("Error parsing length of cell data information.");
      dataInf->Delete();
      return;
      }
    data.resize(length);
    if(!css->GetArgument(0, msgIdx, &*data.begin(), length))
      {
      vtkErrorMacro("Error parsing cell data information.");
      dataInf->Delete();
      return;
      }
    dcss.SetData(&*data.begin(), length);
    dataInf->CopyFromStream(&dcss);
    vtkPVCompositeDataInformationInternals::GroupDataInformationType& ldata = 
      this->Internal->DataInformation[levelIdx];
    ldata[dataSetIdx] = dataInf;
    dataInf->Delete();
    }

}
