/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkXdmfReader.cxx,v $
  Language:  C++
  Date:      $Date: 2007/08/31 18:34:49 $
  Version:   $Revision: 1.27 $


  Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen  
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
  of any contributors may be used to endorse or promote products derived
  from this software without specific prior written permission.

  * Modified source versions must be plainly marked as such, and must not be
  misrepresented as being the original software.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  =========================================================================*/
#include "vtkXdmfReader.h"

#include "vtkCallbackCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredGrid.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXdmfDataArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkCellArray.h"
#include "vtkCharArray.h"
#include "vtkXMLParser.h"
#include "vtkImageData.h"
#include "vtkUniformGrid.h"
#include "vtkMultiGroupDataInformation.h"
#include "vtkMultiGroupDataSet.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkMultiProcessController.h"

#include "XdmfArray.h"
#include "XdmfAttribute.h"
#include "XdmfDOM.h"
#include "XdmfDataDesc.h"
#include "XdmfDataItem.h"
#include "XdmfGrid.h"
#include "XdmfTopology.h"
#include "XdmfGeometry.h"

#include <sys/stat.h>
#include <vtkstd/set>
#include <vtkstd/map>
#include <vtkstd/string>
#include <vtkstd/vector>
#include <assert.h>

#define USE_IMAGE_DATA // otherwise uniformgrid

vtkStandardNewMacro(vtkXdmfReader);
vtkCxxRevisionMacro(vtkXdmfReader, "$Revision: 1.27 $");

vtkCxxSetObjectMacro(vtkXdmfReader,Controller,vtkMultiProcessController);

#if defined(_WIN32) && (defined(_MSC_VER) || defined(__BORLANDC__))
#  include <direct.h>
#  define GETCWD _getcwd
#else
#include <unistd.h>
#  define GETCWD getcwd
#endif

#define vtkMAX(x, y) (((x)>(y))?(x):(y))
#define vtkMIN(x, y) (((x)<(y))?(x):(y))

#define PRINT_EXTENT(x) "[" << (x)[0] << " " << (x)[1] << " " << (x)[2] << " " << (x)[3] << " " << (x)[4] << " " << (x)[5] << "]" 

//============================================================================
class vtkXdmfReaderGrid
{
public:
  vtkXdmfReaderGrid() : XMGrid(0), DataDescription(0) {}
  ~vtkXdmfReaderGrid()
    {
      delete this->XMGrid;
    }

  XdmfGrid       *XMGrid;
  XdmfDataDesc   *DataDescription;
  vtkstd::string Name;
  int Level;
};

//============================================================================
class vtkXdmfReaderGridCollection
{
public:
  vtkXdmfReaderGrid* GetXdmfGrid(const char* gridName,
                                 int level);

  typedef vtkstd::map<vtkstd::string, vtkXdmfReaderGrid*> SetOfGrids;
  SetOfGrids Grids;
  // Update number of levels and number of datasets.
  void UpdateCounts();
  
  // Return the last number of levels computed by UpdateCounts.
  int GetNumberOfLevels()
    {
      return this->NumberOfLevels;
    }
  // Return the last number of dataset for a given level
  // computed by UpdateCounts.
  int GetNumberOfDataSets(int level)
    {
      assert("pre: valid_level" && level>=0 && level<GetNumberOfLevels());
      return this->NumberOfDataSets[level];
    }
  
protected:
  int NumberOfLevels;
  vtkstd::vector<int> NumberOfDataSets;
};

//----------------------------------------------------------------------------
vtkXdmfReaderGrid* vtkXdmfReaderGridCollection::GetXdmfGrid(
  const char *gridName,
  int level)
{
  //find the grid with that name or make one up
  vtkXdmfReaderGridCollection::SetOfGrids::iterator it = this->Grids.find(gridName);
  if ( it == this->Grids.end() || it->second == 0 )
    {
    this->Grids[gridName] = new vtkXdmfReaderGrid;
    }
  //sets its level
  this->Grids[gridName]->Level=level;
  return this->Grids[gridName];
}

//----------------------------------------------------------------------------
void vtkXdmfReaderGridCollection::UpdateCounts()
{
  // Update the number of levels.
  vtkXdmfReaderGridCollection::SetOfGrids::iterator it;
  vtkXdmfReaderGridCollection::SetOfGrids::iterator itEnd;
  
  //find maximum level of any of my grids
  int maxLevel=0;
  it=this->Grids.begin();
  itEnd=this->Grids.end();
  while(it!=itEnd)
    {
    int level=it->second->Level;
    if(level>maxLevel)
      {
      maxLevel=level;
      }
    ++it;
    }
  this->NumberOfLevels=maxLevel+1;
  this->NumberOfDataSets.resize(this->NumberOfLevels);

  //clear the number of datasets at each level array
  int l=this->NumberOfLevels;
  int i=0;
  while(i<l)
    {
    this->NumberOfDataSets[i]=0;
    ++i;
    }
  
  //count up the number of datasets at each level and save in the array
  it=this->Grids.begin();
  while(it!=itEnd)
    {
    int level=it->second->Level;
    ++this->NumberOfDataSets[level];
    ++it;
    }
}

//============================================================================
class vtkXdmfReaderActualGrid
{
public:
  vtkXdmfReaderActualGrid() : Enabled(0), Grid(0), Collection(0) {}
  ~vtkXdmfReaderActualGrid() 
  {
  /*
  if (this->Collection)
    {
    vtkXdmfReaderGridCollection::SetOfGrids::iterator it = this->Collection->Grids.begin();
    while ( it != this->Collection->Grids.end() )
      {
      if (it->second)
        {
        delete it->second;
        it->second = NULL;
        }
      it++;
      }
    };
  */
  }

  int Enabled;
  vtkXdmfReaderGrid* Grid;
  vtkXdmfReaderGridCollection* Collection;
};

//============================================================================
class vtkXdmfReaderInternal
{
public:
  vtkXdmfReaderInternal()
    {
      this->DataItem = 0;
      this->ArrayConverter = vtkXdmfDataArray::New();
      this->NumberOfEnabledActualGrids=0;
    }
  ~vtkXdmfReaderInternal()
    {
      if ( this->DataItem )
        {
        // cout << "....Deleting DataItem " << this->DataItem << endl;
        delete this->DataItem;
        // cout << "....Back from Deleting DataItem " << this->DataItem << endl;
        this->DataItem = 0;
        }
      this->ArrayConverter->Delete();
      this->ArrayConverter = 0;
    }

  typedef vtkstd::vector<vtkstd::string> StringListType;
  StringListType DomainList;
  XdmfXmlNode DomainPtr;

  int RequestSingleGridData(const char* currentGridName,
                            vtkXdmfReaderGrid *grid,
                            vtkInformation *outInfo,
                            vtkDataObject *output,
                            int isSubBlock);
  
  int RequestActualGridData(const char* currentGridName,
                            vtkXdmfReaderActualGrid* currentActualGrid,
                            int outputGrid,
                            int numberOfGrids,
                            vtkInformationVector *outputVector);
  
  // outInfo is null in the  multi-block case.
  int RequestSingleGridInformation(vtkXdmfReaderGrid *grid,
                                   vtkInformation *outInfo);

  int RequestActualGridInformation(vtkXdmfReaderActualGrid* currentActualGrid,
                                   int outputGrid,
                                   int numberOfGrids,
                                   vtkInformationVector* outputVector);
  
  typedef vtkstd::map<vtkstd::string,vtkXdmfReaderActualGrid> MapOfActualGrids;
  MapOfActualGrids ActualGrids;
  int NumberOfEnabledActualGrids;
  
  vtkXdmfReaderGridCollection* GetCollection(const char* collectionName);
  vtkXdmfReaderActualGrid* GetGrid(const char* gridName);
  vtkXdmfReaderActualGrid* GetGrid(int idx);
  vtkXdmfReaderGrid *GetXdmfGrid(const char *gridName,
                                 const char *collectionName,
                                 const char *levelName);

  vtkXdmfReader* Reader;
  XdmfDataItem *DataItem;

  // For converting arrays from XDMF to VTK format
  vtkXdmfDataArray *ArrayConverter;
};

//----------------------------------------------------------------------------
vtkXdmfReaderGridCollection* vtkXdmfReaderInternal::GetCollection(
  const char* collectionName)
{
  if ( !collectionName )
    {
    return 0;
    }

  vtkXdmfReaderActualGrid* actualGrid = &this->ActualGrids[collectionName];

  if ( !actualGrid->Collection )
    {
    if ( actualGrid->Grid )
      {
      cerr << "Trying to create collection with the same name as an existing grid" << endl;
      return 0;
      }
    actualGrid->Collection = new vtkXdmfReaderGridCollection;
    }

  return actualGrid->Collection;
}

//----------------------------------------------------------------------------
vtkXdmfReaderGrid* vtkXdmfReaderInternal::GetXdmfGrid(
  const char *gridName,
  const char *collectionName,
  const char *levelName)
{
  if ( !gridName )
    {
    return 0;
    }

  if ( collectionName )
    {
    vtkXdmfReaderGridCollection *collection=this->GetCollection(collectionName);
    if ( !collection )
      {
      return 0;
      }
    int level;
    if(levelName==0)
      {
      level=0;
      }
    else
      {
      char *tmp=new char[strlen(levelName)+1];
      strcpy(tmp,levelName);
      istrstream s(tmp,strlen(tmp));
      s>>level;
      if(level<0)
        {
        cerr << "Expect a positive Level value, level=" << level <<endl;
        delete[] tmp;
        return 0;
        }
      else
        {
        delete[] tmp;
        }
      }
    return collection->GetXdmfGrid(gridName,level);
    }

  vtkXdmfReaderActualGrid* grid = this->GetGrid(gridName);
  if ( grid->Collection )
    {
    cerr << "Trying to create a grid with the same name as an existing collection" << endl;
    return 0;
    }
  grid->Grid = new vtkXdmfReaderGrid; 
  return grid->Grid;
}

//----------------------------------------------------------------------------
vtkXdmfReaderActualGrid* vtkXdmfReaderInternal::GetGrid(const char* gridName)
{
  return &this->ActualGrids[gridName];
}

//----------------------------------------------------------------------------
vtkXdmfReaderActualGrid* vtkXdmfReaderInternal::GetGrid(int idx)
{
  if ( idx < 0 )
    {
    return 0;
    }
  vtkXdmfReaderInternal::MapOfActualGrids::iterator it;
  int cnt = 0;
  for ( it = this->ActualGrids.begin();
        it != this->ActualGrids.end();
        ++ it )
    {
    if ( cnt == idx )
      {
      return &it->second;
      }
    cnt ++;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkXdmfReaderInternal::RequestActualGridData(
  const char* vtkNotUsed(currentGridName),
  vtkXdmfReaderActualGrid* currentActualGrid,
  int outputGrid,
  int vtkNotUsed(numberOfGrids),
  vtkInformationVector *outputVector)
{
  vtkInformation *info=outputVector->GetInformationObject(0);
  int procId=info->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()); 
  int nbProcs=info->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkMultiGroupDataSet *mgd=vtkMultiGroupDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (!currentActualGrid->Collection)
    {
    return 0; //work around an undo/redo bug
    }

  unsigned int numberOfDataSets=currentActualGrid->Collection->Grids.size();
    
  currentActualGrid->Collection->UpdateCounts();
  int levels=currentActualGrid->Collection->GetNumberOfLevels();
  // mgd->SetNumberOfLevels(levels);
  // int levels = 1;
  
  int level=0;
  mgd->SetNumberOfDataSets(outputGrid, currentActualGrid->Collection->GetNumberOfDataSets(level));
  while(level<levels)
    {
    // mgd->SetNumberOfDataSets(level,currentActualGrid->Collection->GetNumberOfDataSets(level));
    ++level;
    }
  
  vtkXdmfReaderGridCollection::SetOfGrids::iterator gridIt;
  vtkXdmfReaderGridCollection::SetOfGrids::iterator gridItEnd;
    
  int numBlocksPerProcess=numberOfDataSets/nbProcs;
  int leftOverBlocks=numberOfDataSets-(numBlocksPerProcess*nbProcs);
    
  int blockStart;
  int blockEnd;
    
  if(procId<leftOverBlocks)
    {
    blockStart=(numBlocksPerProcess+1)*procId;
    blockEnd=blockStart+(numBlocksPerProcess+1)-1;
    }
  else
    {
    blockStart=numBlocksPerProcess*procId+leftOverBlocks;
    blockEnd=blockStart+numBlocksPerProcess-1;
    }
    
  gridIt=currentActualGrid->Collection->Grids.begin();
  gridItEnd=currentActualGrid->Collection->Grids.end();
  int result=1;
    
  int datasetIdx=0;
    
  vtkMultiGroupDataInformation *compInfo =
    vtkMultiGroupDataInformation::SafeDownCast(
      info->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION()));
    
  // currentIndex in each level.
  vtkstd::vector<int> currentIndex(levels);
  level=0;
  while(level<levels)
    {
    currentIndex[level]=0;
    ++level;
    }
    
  while(gridIt != gridItEnd && result)
    {
    vtkXdmfReaderGrid *subgrid=gridIt->second;
    level=subgrid->Level;
    int index=currentIndex[level];
    if(datasetIdx<blockStart || datasetIdx>blockEnd)
      {
      // mgd->SetDataSet(level,index,0); // empty, on another processor
      mgd->SetDataSet(outputGrid, index, 0); // empty, on another processor
      }
    else
      {
      XdmfGrid *xdmfGrid=subgrid->XMGrid;
      if( xdmfGrid->GetTopology()->GetClass() == XDMF_UNSTRUCTURED ) 
        {
        vtkUnstructuredGrid *ds=vtkUnstructuredGrid::New();
        ds->SetMaximumNumberOfPieces(1);
        // mgd->SetDataSet(level,index,ds);
        mgd->SetDataSet(outputGrid, index, ds);
        ds->Delete();
        } 
      else if( xdmfGrid->GetTopology()->GetTopologyType() == XDMF_2DSMESH ||
               xdmfGrid->GetTopology()->GetTopologyType() == XDMF_3DSMESH )
        {
        vtkStructuredGrid *ds=vtkStructuredGrid::New();
        // mgd->SetDataSet(level,index,ds);
        mgd->SetDataSet(outputGrid, index, ds);
        ds->Delete();
        }
      else if 
        (xdmfGrid->GetTopology()->GetTopologyType() == XDMF_2DCORECTMESH ||
         xdmfGrid->GetTopology()->GetTopologyType() == XDMF_3DCORECTMESH )
        {
#ifdef USE_IMAGE_DATA
        vtkImageData *ds=vtkImageData::New();
#else
        vtkUniformGrid *ds=vtkUniformGrid::New();
#endif
        //mgd->SetDataSet(level,index,ds);
        mgd->SetDataSet(outputGrid,index,ds);
        ds->Delete();
        }
      else if ( 
        xdmfGrid->GetTopology()->GetTopologyType() == XDMF_2DRECTMESH ||
        xdmfGrid->GetTopology()->GetTopologyType() == XDMF_3DRECTMESH )
        {
        vtkRectilinearGrid *ds=vtkRectilinearGrid::New();
        //mgd->SetDataSet(level,index,ds);
        mgd->SetDataSet(outputGrid,index,ds);
        ds->Delete();
        }
      else
        {
        // Unknown type for this sub grid. 
        return 0;
        }
      vtkDataObject *ds=mgd->GetDataSet(outputGrid,index);
      vtkInformation *subInfo=compInfo->GetInformation(outputGrid,index); 
      result=this->RequestSingleGridData("",gridIt->second,subInfo,ds,1);
      }
    ++currentIndex[level];
    ++gridIt;
    ++datasetIdx;
    this->Reader->UpdateProgress(1.0 * datasetIdx / numberOfDataSets);
    }
  return result;
}

//----------------------------------------------------------------------------
int vtkXdmfReaderInternal::RequestSingleGridData(
  const char* currentGridName,
  vtkXdmfReaderGrid *grid,
  vtkInformation *outInfo,
  vtkDataObject *output,
  int isSubBlock)
{
  int *readerStride = this->Reader->GetStride();
  
  vtkDataArraySelection* pointDataArraySelection = 
    this->Reader->GetPointDataArraySelection();
  vtkDataArraySelection* cellDataArraySelection = 
    this->Reader->GetCellDataArraySelection();
  
  // Handle single grid
  XdmfGrid* xdmfGrid = grid->XMGrid;
  XdmfDOM* xdmfDOM = xdmfGrid->GetDOM();
  
  if ( !grid->DataDescription )
    {
    grid->DataDescription = xdmfGrid->GetTopology()->GetShapeDesc();
    //continue;
    }
  
  vtkDebugWithObjectMacro(this->Reader, 
                          << "Reading Heavy Data for " 
                          << xdmfGrid->GetName());
  xdmfGrid->Update();
  
  // True for all 3d datasets except unstructured grids
  int globalrank = 3;
  switch(xdmfGrid->GetTopology()->GetTopologyType())
    {
    case XDMF_2DSMESH: case XDMF_2DRECTMESH: case XDMF_2DCORECTMESH:
      globalrank = 2;
    }
  if ( xdmfGrid->GetTopology()->GetClass() == XDMF_UNSTRUCTURED )
    {
    globalrank = 1;
    }
  
  
  vtkIdType cc;
  XdmfXmlNode attrNode;
  XdmfXmlNode dataNode;
  XdmfInt64 start[4]  = { 0, 0, 0, 0 };
  XdmfInt64 stride[4] = { 1, 1, 1, 1 };
  XdmfInt64 count[4] = { 0, 0, 0, 0 };
  grid->DataDescription->GetShape(count);

  int upext[6];
  int whext[6];
  
  if( xdmfGrid->GetTopology()->GetClass() != XDMF_UNSTRUCTURED)
    {
    if(isSubBlock)
      {
      // the composite pipeline does not set update extent, so just take
      // the whole extent for as update extent.
      outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), upext);
      }
    else
      {
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), upext);
      }
    
    outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), whext);
    
    start[2] = vtkMAX(0, upext[0]);
    start[1] = vtkMAX(0, upext[2]);
    start[0] = vtkMAX(0, upext[4]);
    
    count[2] = upext[1] - upext[0];
    count[1] = upext[3] - upext[2];
    count[0] = upext[5] - upext[4];
    }
  
  XdmfGeometry  *Geometry = xdmfGrid->GetGeometry();
  
  
  // Read Topology for Unstructured Grid
  if( xdmfGrid->GetTopology()->GetClass() == XDMF_UNSTRUCTURED ) 
    {
    vtkUnstructuredGrid  *vGrid = static_cast<vtkUnstructuredGrid *>(output);
    vtkCellArray                *verts;
    XdmfInt32           vType;
    XdmfInt32           NodesPerElement;
    vtkIdType           NumberOfElements;
    vtkIdType           i, j, index;    
    XdmfInt64           Length, *Connections;
    vtkIdType           *connections;
    int                 *cell_types, *ctp;
    
    vtkDebugWithObjectMacro(
      this->Reader,<< "Unstructured Topology is " 
      << xdmfGrid->GetTopology()->GetTopologyTypeAsString());
    switch ( xdmfGrid->GetTopology()->GetTopologyType() )
      {
      case  XDMF_POLYVERTEX :
        vType = VTK_POLY_VERTEX;
        break;
      case  XDMF_POLYLINE :
        vType = VTK_POLY_LINE;
        break;
      case  XDMF_POLYGON :
        vType = VTK_POLYGON;
        break;
      case  XDMF_TRI :
        vType = VTK_TRIANGLE;
        break;
      case  XDMF_QUAD :
        vType = VTK_QUAD;
        break;
      case  XDMF_TET :
        vType = VTK_TETRA;
        break;
      case  XDMF_PYRAMID :
        vType = VTK_PYRAMID;
        break;
      case  XDMF_WEDGE :
        vType = VTK_WEDGE;
        break;
      case  XDMF_HEX :
        vType = VTK_HEXAHEDRON;
        break;
      case  XDMF_EDGE_3 :
        vType = VTK_QUADRATIC_EDGE ;
        break;
      case  XDMF_TRI_6 :
        vType = VTK_QUADRATIC_TRIANGLE ;
        break;
      case  XDMF_QUAD_8 :
        vType = VTK_QUADRATIC_QUAD ;
        break;
      case  XDMF_TET_10 :
        vType = VTK_QUADRATIC_TETRA ;
        break;
      case  XDMF_PYRAMID_13 :
        vType = VTK_QUADRATIC_PYRAMID ;
        break;
      case  XDMF_WEDGE_15 :
        vType = VTK_QUADRATIC_WEDGE ;
        break;
      case  XDMF_HEX_20 :
        vType = VTK_QUADRATIC_HEXAHEDRON ;
        break;
      case XDMF_MIXED :
        vType = -1;
        break;
      default :
        XdmfErrorMessage("Unknown Topology Type = " 
                         << xdmfGrid->GetTopology()->GetTopologyType());
        return 1;
      }
    if( xdmfGrid->GetTopology()->GetTopologyType() != XDMF_MIXED)
      {
      NodesPerElement = xdmfGrid->GetTopology()->GetNodesPerElement();
      if ( xdmfGrid->GetTopology()->GetConnectivity()->GetRank() == 2 )
        {
        NodesPerElement = 
          xdmfGrid->GetTopology()->GetConnectivity()->GetDimension(1);
        }
    
      /* Create Cell Type Array */
      Length = xdmfGrid->GetTopology()->GetConnectivity()->GetNumberOfElements();
      Connections = new XdmfInt64[ Length ];
      xdmfGrid->GetTopology()->GetConnectivity()->GetValues(
        0, 
        Connections, 
        Length);
      
      NumberOfElements = 
        xdmfGrid->GetTopology()->GetShapeDesc()->GetNumberOfElements();
      ctp = cell_types = new int[ NumberOfElements ];
      
      /* Create Cell Array */
      verts = vtkCellArray::New();
      
      /* Get the pointer */
      connections = verts->WritePointer(
        NumberOfElements,
        NumberOfElements * ( 1 + NodesPerElement ));
      
      /* Connections : N p1 p2 ... pN */
      /* i.e. Triangles : 3 0 1 2    3 3 4 5   3 6 7 8 */
      index = 0;
      for( j = 0 ; j < NumberOfElements; j++ )
        {
        *ctp++ = vType;
        *connections++ = NodesPerElement;
        for( i = 0 ; i < NodesPerElement; i++ )
          {
          *connections++ = Connections[index++];
          }
        }
      } 
    else 
      {
      // Mixed Topology
      /* Create Cell Type Array */
      vtkIdTypeArray *IdArray;
      vtkIdType RealSize;
      
      Length = xdmfGrid->GetTopology()->GetConnectivity()->GetNumberOfElements();
      Connections = new XdmfInt64[ Length ];
      xdmfGrid->GetTopology()->GetConnectivity()->GetValues(
        0, 
        Connections, 
        Length );
      NumberOfElements = 
        xdmfGrid->GetTopology()->GetShapeDesc()->GetNumberOfElements();
      ctp = cell_types = new int[ NumberOfElements ];
      
      /* Create Cell Array */
      verts = vtkCellArray::New();
      
      /* Get the pointer. Make it Big enough ... too big for now */
      // cout << "::::: Length = " << Length << endl;
      connections = verts->WritePointer(
        NumberOfElements,
        Length);
      //   Length);
      /* Connections : N p1 p2 ... pN */
      /* i.e. Triangles : 3 0 1 2    3 3 4 5   3 6 7 8 */
      index = 0;
      for( j = 0 ; j < NumberOfElements; j++ )
        {
        switch ( Connections[index++] )
          {
          case  XDMF_POLYVERTEX :
            vType = VTK_POLY_VERTEX;
            NodesPerElement = Connections[index++];
            break;
          case  XDMF_POLYLINE :
            vType = VTK_POLY_LINE;
            NodesPerElement = Connections[index++];
            break;
          case  XDMF_POLYGON :
            vType = VTK_POLYGON;
            NodesPerElement = Connections[index++];
            break;
          case  XDMF_TRI :
            vType = VTK_TRIANGLE;
            NodesPerElement = 3;
            break;
          case  XDMF_QUAD :
            vType = VTK_QUAD;
            NodesPerElement = 4;
            break;
          case  XDMF_TET :
            vType = VTK_TETRA;
            NodesPerElement = 4;
            break;
          case  XDMF_PYRAMID :
            vType = VTK_PYRAMID;
            NodesPerElement = 5;
            break;
          case  XDMF_WEDGE :
            vType = VTK_WEDGE;
            NodesPerElement = 6;
            break;
          case  XDMF_HEX :
            vType = VTK_HEXAHEDRON;
            NodesPerElement = 8;
            break;
          case  XDMF_EDGE_3 :
            vType = VTK_QUADRATIC_EDGE ;
            NodesPerElement = 3;
            break;
          case  XDMF_TRI_6 :
            vType = VTK_QUADRATIC_TRIANGLE ;
            NodesPerElement = 6;
            break;
          case  XDMF_QUAD_8 :
            vType = VTK_QUADRATIC_QUAD ;
            NodesPerElement = 8;
            break;
          case  XDMF_TET_10 :
            vType = VTK_QUADRATIC_TETRA ;
            NodesPerElement = 10;
            break;
          case  XDMF_PYRAMID_13 :
            vType = VTK_QUADRATIC_PYRAMID ;
            NodesPerElement = 13;
            break;
          case  XDMF_WEDGE_15 :
            vType = VTK_QUADRATIC_WEDGE ;
            NodesPerElement = 15;
            break;
          case  XDMF_HEX_20 :
            vType = VTK_QUADRATIC_HEXAHEDRON ;
            NodesPerElement = 20;
            break;
          default :
            XdmfErrorMessage("Unknown Topology Type");
            return 1;
          }
        *ctp++ = vType;
        *connections++ = NodesPerElement;
        for( i = 0 ; i < NodesPerElement; i++ )
          {
          *connections++ = Connections[index++];
          }
        }
      // Resize the Array to the Proper Size
      IdArray = verts->GetData();
      RealSize = index - 1;
      vtkDebugWithObjectMacro(this->Reader, 
                              "Resizing to " << RealSize << " elements");
      IdArray->Resize(RealSize);
      }

    delete [] Connections;
    vGrid->SetCells(cell_types, verts);
    /* OK, because of reference counting */
    verts->Delete();
    delete [] cell_types;
    vGrid->Modified();
    }  // if( xdmfGrid->GetClass() == XDMF_UNSTRUCTURED ) 
  else if( xdmfGrid->GetTopology()->GetTopologyType() == XDMF_2DSMESH ||
           xdmfGrid->GetTopology()->GetTopologyType() == XDMF_3DSMESH )
    {
    vtkDebugWithObjectMacro(this->Reader, 
                            << "Setting Extents for vtkStructuredGrid");
    vtkStructuredGrid  *vGrid = vtkStructuredGrid::SafeDownCast(output);
    vGrid->SetExtent(upext);    
    } 
  else if (xdmfGrid->GetTopology()->GetTopologyType() == XDMF_2DCORECTMESH ||
           xdmfGrid->GetTopology()->GetTopologyType() == XDMF_3DCORECTMESH ) 
    {
#ifdef USE_IMAGE_DATA
    vtkImageData *idata = vtkImageData::SafeDownCast(output);
#else
    vtkUniformGrid *idata = vtkUniformGrid::SafeDownCast(output);
#endif
    idata->SetExtent(upext);
    }
  else if ( xdmfGrid->GetTopology()->GetTopologyType() == XDMF_2DRECTMESH ||
            xdmfGrid->GetTopology()->GetTopologyType() == XDMF_3DRECTMESH )
    {
    vtkRectilinearGrid *vGrid = vtkRectilinearGrid::SafeDownCast(output);
    vGrid->SetExtent(upext);    
    }
  else
    {
    vtkErrorWithObjectMacro(this->Reader,"Do not understand topology type: " 
                            << xdmfGrid->GetTopology()->GetTopologyType());
    }
  // Read Geometry
  if( ( Geometry->GetGeometryType() == XDMF_GEOMETRY_X_Y_Z ) ||
      ( Geometry->GetGeometryType() == XDMF_GEOMETRY_XYZ ) ||
      ( Geometry->GetGeometryType() == XDMF_GEOMETRY_X_Y ) ||
      ( Geometry->GetGeometryType() == XDMF_GEOMETRY_XY ) )
    {
    XdmfInt64   Length;
    vtkPoints   *Points;
    vtkPointSet *Pointset = vtkPointSet::SafeDownCast(output);
    
    // Special flag, for structured data
    int structured_grid = 0;
    if ( vtkStructuredGrid::SafeDownCast(output) )
      {
      structured_grid = 1;
      }
    
    Points = Pointset->GetPoints();
    if( !Points )
      {
      vtkDebugWithObjectMacro(this->Reader,<<  "Creating vtkPoints" );
      Points = vtkPoints::New();
      Pointset->SetPoints( Points );
      // OK Because of Reference Counting
      Points->Delete();
      }
    
    if( Geometry->GetPoints() )
      {
      if( Points )
        {
        if ( Geometry->GetPoints()->GetNumberType() == XDMF_FLOAT32_TYPE )
          {
          if ( Points->GetData()->GetDataType() != VTK_FLOAT)
            {
            vtkFloatArray* da = vtkFloatArray::New();
            da->SetNumberOfComponents(3);
            Points->SetData(da);
            da->Delete();
            }
          }
        else // means == XDMF_FLOAT64_TYPE
          {
          if ( Points->GetData()->GetDataType() != VTK_DOUBLE )
            {
            vtkDoubleArray* da = vtkDoubleArray::New();
            da->SetNumberOfComponents(3);
            Points->SetData(da);
            da->Delete();
            }
          }
        
        Length = Geometry->GetPoints()->GetNumberOfElements();
        vtkDebugWithObjectMacro(this->Reader, 
                                << "Setting Array of " << (int)Length << " = " 
                                << (int)Geometry->GetNumberOfPoints() 
                                << " Points");
        vtkIdType iskip[3] = { 0, 0, 0 };
        vtkIdType eskip[3] = { 0, 0, 0 };
        int strides_or_extents = 0;
        if ( structured_grid )
          {
          XdmfInt64     ii, jj, kk;
          vtkIdType numpoints = Geometry->GetNumberOfPoints();
          vtkIdType newnumpoints = ((upext[5] - upext[4] + 1) * (upext[3] - upext[2] + 1) * (upext[1] - upext[0] + 1));
          int cnt = 0;
          for (kk = upext[4]; kk <= upext[5]; kk ++ )
            {
            for ( jj = upext[2]; jj <= upext[3]; jj ++ )
              {
              for ( ii = upext[0]; ii <= upext[1]; ii ++ )
                {
                cnt ++;
                }
              }
            }
          newnumpoints = cnt;
          
          Points->SetNumberOfPoints(newnumpoints);
          vtkIdType dims[3];
          dims[0] = whext[1] - whext[0] + 1;
          dims[1] = whext[3] - whext[2] + 1;
          dims[2] = whext[5] - whext[4] + 1;
          iskip[0] = upext[0];
          iskip[1] = upext[2] * dims[0];
          iskip[2] = upext[4] * dims[0] * dims[1];
          eskip[0] = whext[1] - upext[1];
          eskip[1] = (whext[3] - upext[3]) * dims[0];
          eskip[2] = (whext[5] - upext[5]) * dims[0] * dims[1];
          if ( newnumpoints != numpoints )
            {
            strides_or_extents = 1;
            }
          }
        else
          {
          // Unstructured grid
          Points->SetNumberOfPoints( Geometry->GetNumberOfPoints() );
          int kk;
          for ( kk = 0; kk < 6; kk ++ )
            {
            upext[kk] = whext[kk];
            }
          }
        if( Geometry->GetPoints()->GetDataType() == XDMF_FLOAT32_TYPE && 
            !strides_or_extents) 
          {
          Geometry->GetPoints()->GetValues(
            0, 
            vtkFloatArray::SafeDownCast(Points)->GetPointer(0), Length );
          } 
        else if( Geometry->GetPoints()->GetDataType() == XDMF_FLOAT64_TYPE && 
                 !strides_or_extents) 
          {
          Geometry->GetPoints()->GetValues(
            0, 
            vtkDoubleArray::SafeDownCast(Points)->GetPointer(0), Length );
          } 
        else 
          {
          XdmfFloat64   *TmpPp;
          XdmfFloat64   *TmpPoints = new XdmfFloat64[ Length ];
          XdmfInt64     ii, jj, kk;
          
          Geometry->GetPoints()->GetValues( 0, TmpPoints, Length );
          TmpPp = TmpPoints;
          vtkIdType cnt = 0;
          
          if ( strides_or_extents )
            {
            XdmfInt64   Dimensions[3] = { 0, 0, 0 };
            XdmfTopology        *Topology = xdmfGrid->GetTopology();
            Topology->GetShapeDesc()->GetShape( Dimensions );
            cnt = 0;
            for (kk = whext[4]; kk < Dimensions[0]; kk ++ )
              {
              for ( jj = whext[2]; jj < Dimensions[1]; jj ++ )
                {
                for ( ii = whext[0]; ii < Dimensions[2]; ii ++ )
                  {
                  vtkIdType rii = ii / readerStride[0];
                  vtkIdType rjj = jj / readerStride[1];
                  vtkIdType rkk = kk / readerStride[2];
                  vtkIdType mii = ii % readerStride[0];
                  vtkIdType mjj = jj % readerStride[1];
                  vtkIdType mkk = kk % readerStride[2];
                  if ( 
                    rii >= upext[0] && rii <= upext[1] &&
                    rjj >= upext[2] && rjj <= upext[3] &&
                    rkk >= upext[4] && rkk <= upext[5] &&
                    mii == 0 && mjj == 0 && mkk == 0 )
                    {
                    // We are inside the extents
                    Points->SetPoint(cnt, TmpPp);
                    TmpPp += 3;
                    cnt ++;
                    }
                  else
                    {
                    TmpPp += 3;
                    }
                  }
                }
              }
            }
          else
            {
            cnt = 0;
            for( ii = 0 ; ii < Length / 3 ; ii++ )
              {
              Points->SetPoint(cnt, TmpPp);
              TmpPp += 3;
              cnt ++;
              }
            }
          delete [] TmpPoints;
          }
        Points->Modified();
        Pointset->Modified();
        } 
      else 
        {
        XdmfErrorMessage("Base Grid Has No Points");
        return 1;
        }
      } 
    else 
      {
      XdmfErrorMessage("No Points to Set");
      return 1;
      }
    if ( structured_grid )
      {
      stride[2] = readerStride[0];
      stride[1] = readerStride[1];
      stride[0] = readerStride[2];
      }
    }
  else if ( Geometry->GetGeometryType() == XDMF_GEOMETRY_ORIGIN_DXDYDZ )
    {
#ifdef USE_IMAGE_DATA
    vtkImageData *vGrid = vtkImageData::SafeDownCast(output);
#else
    vtkUniformGrid *vGrid = vtkUniformGrid::SafeDownCast(output);
#endif
    XdmfTopology        *Topology = xdmfGrid->GetTopology();
    XdmfInt64   Dimensions[3] = { 0, 0, 0 };
    XdmfFloat64 *origin = Geometry->GetOrigin();
    vGrid->SetOrigin(origin[2], origin[1], origin[0]);
    XdmfFloat64 *spacing = Geometry->GetDxDyDz();
    Topology->GetShapeDesc()->GetShape( Dimensions );
    vGrid->SetDimensions(Dimensions[2], Dimensions[1], Dimensions[0]);
    stride[2] = readerStride[0];
    stride[1] = readerStride[1];
    stride[0] = readerStride[2];
    vGrid->SetSpacing(
      spacing[2]*readerStride[0], 
      spacing[1]*readerStride[1],
      spacing[0]*readerStride[2]);
    }
  else
    {
    // Special Rectilinear and CoRectilinear Geometries
    XdmfTopology        *Topology = xdmfGrid->GetTopology();
    vtkIdType Index;
    vtkRectilinearGrid *vGrid = vtkRectilinearGrid::SafeDownCast(output);
    if ( vGrid )
      {
      vtkDoubleArray      *XCoord, *YCoord, *ZCoord;
      XdmfFloat64 *Origin;
      XdmfInt64   Dimensions[3] = { 0, 0, 0 };
      
      // Make Sure Grid Has Coordinates
      Topology->GetShapeDesc()->GetShape( Dimensions );
      
      XCoord = vtkDoubleArray::New();
      vGrid->SetXCoordinates( XCoord );
      // OK Because of Reference Counting
      XCoord->Delete();   
      XCoord->SetNumberOfValues( count[2]+1 );
      YCoord = vtkDoubleArray::New();
      vGrid->SetYCoordinates( YCoord );
      // OK Because of Reference Counting
      YCoord->Delete();   
      YCoord->SetNumberOfValues( count[1]+1 );
      ZCoord = vtkDoubleArray::New();
      vGrid->SetZCoordinates( ZCoord );
      // OK Because of Reference Counting
      ZCoord->Delete();   
      ZCoord->SetNumberOfValues( count[0]+1 );

      // Build Vectors if nescessary
      if( Geometry->GetGeometryType() == XDMF_GEOMETRY_ORIGIN_DXDYDZ )
        {
        if( !Geometry->GetVectorX() )
          {
          Geometry->SetVectorX( new XdmfArray );
          Geometry->GetVectorX()->SetNumberType( XDMF_FLOAT32_TYPE );
          }
        if( !Geometry->GetVectorY() )
          {
          Geometry->SetVectorY( new XdmfArray );
          Geometry->GetVectorY()->SetNumberType( XDMF_FLOAT32_TYPE );
          }
        if( !Geometry->GetVectorZ() )
          {
          Geometry->SetVectorZ( new XdmfArray );
          Geometry->GetVectorZ()->SetNumberType( XDMF_FLOAT32_TYPE );
          }
        Geometry->GetVectorX()->SetNumberOfElements( Dimensions[2] );
        Geometry->GetVectorY()->SetNumberOfElements( Dimensions[1] );
        Geometry->GetVectorZ()->SetNumberOfElements( Dimensions[0] );
        Origin = Geometry->GetOrigin();
        Geometry->GetVectorX()->Generate(
          Origin[0],
          Origin[0] + ( Geometry->GetDx() * ( Dimensions[2] - 1 ) ) );
        Geometry->GetVectorY()->Generate(
          Origin[1],
          Origin[1] + ( Geometry->GetDy() * ( Dimensions[1] - 1 ) ) );
        Geometry->GetVectorZ()->Generate(
          Origin[2],
          Origin[2] + ( Geometry->GetDz() * ( Dimensions[0] - 1 ) ) );
        }
      int sstart[3];
      sstart[0] = start[0];
      sstart[1] = start[1];
      sstart[2] = start[2];
      
      vtkIdType cstart[3] = { 0, 0, 0 };
      vtkIdType cend[3];
      cstart[0] = vtkMAX(0, sstart[2]);
      cstart[1] = vtkMAX(0, sstart[1]);
      cstart[2] = vtkMAX(0, sstart[0]);
      
      cend[0] = start[2] + count[2]*readerStride[0]+1;
      cend[1] = start[1] + count[1]*readerStride[1]+1;
      cend[2] = start[0] + count[0]*readerStride[2]+1;
      
      vtkDebugWithObjectMacro(this->Reader,
                              << "CStart: " << cstart[0] << ", " 
                              << cstart[1] << ", " 
                              << cstart[2] );
      vtkDebugWithObjectMacro(this->Reader,
                              << "CEnd: " << cend[0] << ", " 
                              << cend[1] << ", " 
                              << cend[2] );
      
      // Set the Points
      for(Index=cstart[0], cc = 0 ; Index < cend[0] ; Index += readerStride[0])
        {
        XCoord->SetValue( cc++, 
                          Geometry->GetVectorX()->GetValueAsFloat64( Index ));
        } 
      for(Index = cstart[1], cc = 0 ; Index < cend[1]; Index+=readerStride[1] )
        {
        YCoord->SetValue( cc++ , 
                          Geometry->GetVectorY()->GetValueAsFloat64( Index ) );
        } 
      for(Index = cstart[2], cc = 0 ; Index < cend[2];Index += readerStride[2])
        {
        ZCoord->SetValue( cc++ , 
                          Geometry->GetVectorZ()->GetValueAsFloat64( Index ) );
        }
      
      stride[2] = readerStride[0];
      stride[1] = readerStride[1];
      stride[0] = readerStride[2];
      
      // vGrid->SetExtent(upext);    
      }
    }
  vtkDataSet *dataSet = static_cast<vtkDataSet *>(output);
  for ( cc = 0; cc < dataSet->GetPointData()->GetNumberOfArrays(); cc ++ )
    {
    dataSet->GetPointData()->RemoveArray(
      dataSet->GetPointData()->GetArrayName(cc));
    }
  int haveActive = 0;
  for( cc = 0 ; cc < xdmfGrid->GetNumberOfAttributes() ; cc++ )
    {
    XdmfInt32 AttributeCenter;
    XdmfInt32 AttributeType;
    int       Components;
    XdmfAttribute       *Attribute;
    
    Attribute = xdmfGrid->GetAttribute( cc );
    const char *name = Attribute->GetName();
    int status = 1;
    AttributeCenter = Attribute->GetAttributeCenter();
    AttributeType = Attribute->GetAttributeType();
    Components = 1;
    switch (AttributeType) 
      {
      case XDMF_ATTRIBUTE_TYPE_TENSOR :
        Components = 9;
        break;
      case XDMF_ATTRIBUTE_TYPE_VECTOR:
        Components = 3;
        break;
      default :
        Components = 1;
        break;
      }
    if (name )
      {
      if ( AttributeCenter == XDMF_ATTRIBUTE_CENTER_GRID || 
           AttributeCenter == XDMF_ATTRIBUTE_CENTER_NODE)
        {
        status = pointDataArraySelection->ArrayIsEnabled(name);
        }
      else
        {
        status = cellDataArraySelection->ArrayIsEnabled(name);
        }
      }
    if ( !status )
      {
      continue;
      }
    status = 1;
    vtkDebugWithObjectMacro(this->Reader,
                            << "Array with name: " 
                            << name << " has status: " << status);
    // attrNode = this->DOM->FindElement("Attribute", cc);
    attrNode = Attribute->GetElement();
    // dataNode = this->DOM->FindElement("DataStructure", 0, attrNode);
    // Find the DataTransform or DataStructure below the <Attribute>
    dataNode = xdmfDOM->FindElement(NULL, 0, attrNode);
    this->DataItem->SetElement(dataNode);
    this->DataItem->UpdateInformation();
    grid->DataDescription = this->DataItem->GetDataDesc();
    
    if ( Attribute && status )
      {
      //Attribute->Update();
      // XdmfDataItem dataItem;
      XdmfArray *values;

      XdmfInt64 realcount[4] = { 0, 0, 0, 0 };
      realcount[0] = count[0];
      realcount[1] = count[1];
      realcount[2] = count[2];
      realcount[3] = count[3];
      if ( AttributeCenter == XDMF_ATTRIBUTE_CENTER_NODE ||
           AttributeCenter == XDMF_ATTRIBUTE_CENTER_GRID )
        {
        // Point count is 1 + cell extent
        realcount[0] ++;
        realcount[1] ++;
        realcount[2] ++;
        }
      /*
        XdmfArray *values = this->FormatMulti->ElementToArray( 
        dataNode, this->Internals->DataDescriptions[currentGrid] );
      */
      vtkDebugWithObjectMacro(this->Reader, "Topology class: " << xdmfGrid->GetTopology()->GetClassAsString());
      if(xdmfGrid->GetTopology()->GetClass() != XDMF_UNSTRUCTURED)
        {
        XdmfDataDesc* ds = grid->DataDescription;
        XdmfInt64 realdims[XDMF_MAX_DIMENSION];
        XdmfInt32 realrank = ds->GetShape(realdims);
        if ( realrank == 4 )
          {
          realcount[3] = realdims[3];
          }
        this->DataItem->GetDataDesc()->SelectHyperSlab(start, stride, realcount);
        vtkDebugWithObjectMacro(this->Reader,
                                << "Dims = " << ds->GetShapeAsString()
                                << "Slab = " << ds->GetHyperSlabAsString());
        // Only works for the structured and rectilinear grid
        vtkDebugWithObjectMacro(this->Reader, << "Preparing to Read :" 
                                << xdmfDOM->Get(dataNode, "CData"));
        this->DataItem->Update();
        values = this->DataItem->GetArray();
        }
      else 
        {
        this->DataItem->Update();
        values = this->DataItem->GetArray();
        }
      this->ArrayConverter->SetVtkArray( NULL );
      if ( values )
        {
        vtkDataArray* vtkValues = this->ArrayConverter->FromXdmfArray(
          values->GetTagName(), 1, globalrank, Components);
        
        vtkDebugWithObjectMacro(this->Reader, << "Reading array: " << name );
        vtkValues->SetName(name);
        
        // Special Cases
        if( AttributeCenter == XDMF_ATTRIBUTE_CENTER_GRID ) 
          {
          // Implement XDMF_ATTRIBUTE_CENTER_GRID as PointData
          XdmfArray *tmpArray = new XdmfArray;
          
          vtkDebugWithObjectMacro(this->Reader, << "Setting Grid Centered Values");
          tmpArray->CopyType( values );
          tmpArray->SetNumberOfElements( dataSet->GetNumberOfPoints() );
          tmpArray->Generate( values->GetValueAsFloat64(0), 
                              values->GetValueAsFloat64(0) );
          vtkValues->Delete();
          this->ArrayConverter->SetVtkArray( NULL );
          vtkValues=this->ArrayConverter->FromXdmfArray(tmpArray->GetTagName(), 1, 1, Components);
          if( !name )
            {
            name = values->GetTagName();
            }
          vtkValues->SetName( name );
          delete tmpArray;
          AttributeCenter = XDMF_ATTRIBUTE_CENTER_NODE;
          }
        switch (AttributeCenter)
          {
          case XDMF_ATTRIBUTE_CENTER_NODE :
            dataSet->GetPointData()->RemoveArray(name);
            dataSet->GetPointData()->AddArray(vtkValues);
            if ( Attribute->GetActive() )
              {
              haveActive = 1;
              switch( AttributeType )
                {
                case XDMF_ATTRIBUTE_TYPE_SCALAR :
                  dataSet->GetPointData()->SetActiveScalars( name );
                  break;
                case XDMF_ATTRIBUTE_TYPE_VECTOR :
                  dataSet->GetPointData()->SetActiveVectors( name );
                  break;
                case XDMF_ATTRIBUTE_TYPE_TENSOR :
                  dataSet->GetPointData()->SetActiveTensors( name );
                  break;
                default :
                  break;
                }
              }
            break;
          case XDMF_ATTRIBUTE_CENTER_CELL :
            dataSet->GetCellData()->RemoveArray(name);
            dataSet->GetCellData()->AddArray(vtkValues);
            if ( Attribute->GetActive() )
              {
              haveActive = 1;
              switch( AttributeType )
                {
                case XDMF_ATTRIBUTE_TYPE_SCALAR :
                  dataSet->GetCellData()->SetActiveScalars( name );
                  break;
                case XDMF_ATTRIBUTE_TYPE_VECTOR :
                  dataSet->GetCellData()->SetActiveVectors( name );
                  break;
                case XDMF_ATTRIBUTE_TYPE_TENSOR :
                  dataSet->GetCellData()->SetActiveTensors( name );
                  break;
                default :
                  break;
                }
              }
            break;
          default : 
            vtkErrorWithObjectMacro(this->Reader,
                                    << "Can't Handle Values at " 
                                    <<Attribute->GetAttributeCenterAsString());
            break;
          }
        if ( vtkValues )
          {
          vtkValues->Delete();
          }
        if ( grid->DataDescription ) 
          {
          // delete grid->DataDescription;
          // grid->DataDescription = 0;
          }
        }
      }
    }
  if ( !haveActive )
    {
    vtkDataSetAttributes* fd = dataSet->GetPointData();
    for ( cc = 0; cc < fd->GetNumberOfArrays(); cc ++ )
      {
      vtkDataArray* ar = fd->GetArray(cc);
      switch ( ar->GetNumberOfComponents() )
        {
        case 1:
          fd->SetActiveScalars(ar->GetName());
          break;
        case 3:
          fd->SetActiveVectors(ar->GetName());
          break;
        case 6:
          fd->SetActiveTensors(ar->GetName());
          break;
        }
      }
    fd = dataSet->GetCellData();
    for ( cc = 0; cc < fd->GetNumberOfArrays(); cc ++ )
      {
      vtkDataArray* ar = fd->GetArray(cc);
      switch ( ar->GetNumberOfComponents() )
        {
        case 1:
          fd->SetActiveScalars(ar->GetName());
          break;
        case 3:
          fd->SetActiveVectors(ar->GetName());
          break;
        case 6:
          fd->SetActiveTensors(ar->GetName());
          break;
        }
      }
    }
  
  const char* name = currentGridName;
  vtkCharArray *nameArray = vtkCharArray::New();
  nameArray->SetName("Name");
  char *str = nameArray->WritePointer(0, strlen(name)+1);
  sprintf(str, "%s", name);
  output->GetFieldData()->AddArray(nameArray);
  nameArray->Delete();
  
  return 1;
}

//-----------------------------------------------------------------------------
int vtkXdmfReaderInternal::RequestActualGridInformation(
  vtkXdmfReaderActualGrid* currentActualGrid,
  int outputGrid,
  int vtkNotUsed(numberOfGrids),
  vtkInformationVector* outputVector)
{
  // Handle single grid
  if ( currentActualGrid->Collection )
    { 
    vtkInformation* info = outputVector->GetInformationObject(0);
      
    vtkMultiGroupDataInformation *compInfo = 
      vtkMultiGroupDataInformation::SafeDownCast(
        info->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION()));

    currentActualGrid->Collection->UpdateCounts();
    int levels=currentActualGrid->Collection->GetNumberOfLevels();
    int level=0;
    while(level<levels)
      {
      ++level;
      }
    level = 0;


    unsigned int numberOfDataSets=currentActualGrid->Collection->Grids.size();
    compInfo->SetNumberOfDataSets(
      outputGrid, currentActualGrid->Collection->GetNumberOfDataSets(level));
        
    if(this->Reader->GetController()==0)
      {
      return 0;
      }

    int procId=this->Reader->GetController()->GetLocalProcessId();
    int nbProcs=this->Reader->GetController()->GetNumberOfProcesses();
    
    int numBlocksPerProcess=numberOfDataSets/nbProcs;
    int leftOverBlocks=numberOfDataSets-(numBlocksPerProcess*nbProcs);
    
    int blockStart;
    int blockEnd;
    
    if(procId<leftOverBlocks)
      {
      blockStart=(numBlocksPerProcess+1)*procId;
      blockEnd=blockStart+(numBlocksPerProcess+1)-1;
      }
    else
      {
      blockStart=numBlocksPerProcess*procId+leftOverBlocks;
      blockEnd=blockStart+numBlocksPerProcess-1;
      }

    vtkXdmfReaderGridCollection::SetOfGrids::iterator gridIt;
    vtkXdmfReaderGridCollection::SetOfGrids::iterator gridItEnd;
    int result=1;
    int datasetIdx=0;
    gridIt=currentActualGrid->Collection->Grids.begin();
    gridItEnd=currentActualGrid->Collection->Grids.end();
    
    // currentIndex in each level.
    vtkstd::vector<int> currentIndex(levels);
    level=0;
    while(level<levels)
      {
      currentIndex[level]=0;
      ++level;
      }
    
    while(gridIt != gridItEnd && result)
      {
      vtkXdmfReaderGrid *subgrid=gridIt->second;
      level=subgrid->Level;
      int index=currentIndex[level];
      
      // the following actually create the info about the block.
      vtkInformation *subInfo=compInfo->GetInformation(outputGrid,index);
      
      if(datasetIdx>=blockStart && datasetIdx<=blockEnd)
        {      
        result=this->RequestSingleGridInformation(subgrid,subInfo);
        }
      ++currentIndex[level];
      ++gridIt;
      ++datasetIdx;
      }
    return result;
    }
// neither a single grid, neither a collection.
  vtkDebugWithObjectMacro(this->Reader, "Grid does not have a collection");
  return 0;
}

//-----------------------------------------------------------------------------
int vtkXdmfReaderInternal::RequestSingleGridInformation(
  vtkXdmfReaderGrid *grid,
  vtkInformation *outInfo)
{
  XdmfInt32    Rank;
  XdmfInt64    Dimensions[ XDMF_MAX_DIMENSION ];
  XdmfInt64    EndExtent[ XDMF_MAX_DIMENSION ];
  vtkDataArraySelection* pointDataArraySelection = 
    this->Reader->GetPointDataArraySelection();
  vtkDataArraySelection* cellDataArraySelection = 
    this->Reader->GetCellDataArraySelection();
  int *readerStride = this->Reader->GetStride();

  
  XdmfGrid* xdmfGrid = grid->XMGrid;
  
  int kk;
  for( kk = 0 ; kk < xdmfGrid->GetNumberOfAttributes() ; kk++ )
    {
    XdmfAttribute       *Attribute;
    Attribute = xdmfGrid->GetAttribute( kk );
    const char *name = Attribute->GetName();
    if (name )
      {
      XdmfInt32 AttributeCenter = Attribute->GetAttributeCenter();
      if ( AttributeCenter == XDMF_ATTRIBUTE_CENTER_GRID || 
           AttributeCenter == XDMF_ATTRIBUTE_CENTER_NODE)
        {
        if ( !pointDataArraySelection->ArrayExists(name) )
          {
          pointDataArraySelection->AddArray(name);
          }
        }
      else
        {
        if ( !cellDataArraySelection->ArrayExists(name) )
          {
          cellDataArraySelection->AddArray(name);
          }
        }
      }
    }
  
  // Revised Initial Setup
  grid->DataDescription = xdmfGrid->GetTopology()->GetShapeDesc();
  Rank = grid->DataDescription->GetShape( Dimensions );
  int i;
  for(i = Rank ; i < XDMF_MAX_DIMENSION ; i++)
    {
    Dimensions[i] = 1;
    }
  // End Extent is Dim - 1
  EndExtent[0] = Dimensions[0] - 1;
  EndExtent[1] = Dimensions[1] - 1;
  EndExtent[2] = Dimensions[2] - 1;
  // vtk Dims are i,j,k XDMF are k,j,i
  EndExtent[0] = vtkMAX(0, EndExtent[0]) / readerStride[2];
  EndExtent[1] = vtkMAX(0, EndExtent[1]) / readerStride[1];
  EndExtent[2] = vtkMAX(0, EndExtent[2]) / readerStride[0];
  vtkDebugWithObjectMacro(this->Reader, << "EndExtents = " 
                          << (vtkIdType)EndExtent[0] << ", " 
                          << (vtkIdType)EndExtent[1] << ", " 
                          << (vtkIdType)EndExtent[2]);
  
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               0, EndExtent[2], 0, EndExtent[1], 0, EndExtent[0]);
  
  vtkDebugWithObjectMacro(this->Reader, << "Grid Type = " << xdmfGrid->GetTopology()->GetTopologyTypeAsString() << " = " << xdmfGrid->GetTopology()->GetTopologyType());
  if( xdmfGrid->GetTopology()->GetClass() != XDMF_UNSTRUCTURED ) 
    {
    if( (xdmfGrid->GetTopology()->GetTopologyType() == XDMF_2DSMESH ) ||
        (xdmfGrid->GetTopology()->GetTopologyType() == XDMF_3DSMESH ) )
      {
      vtkDebugWithObjectMacro(this->Reader, 
                              << "Setting Extents for vtkStructuredGrid");
      outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                   0, EndExtent[2], 0, EndExtent[1], 0, EndExtent[0]);
     
      }
    else if ( xdmfGrid->GetTopology()->GetTopologyType() == XDMF_2DCORECTMESH|| 
              xdmfGrid->GetTopology()->GetTopologyType() == XDMF_3DCORECTMESH )
      {
      outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                   0, EndExtent[2], 0, EndExtent[1], 0, EndExtent[0]);
    
      XdmfGeometry  *Geometry = xdmfGrid->GetGeometry();
      if ( Geometry->GetGeometryType() == XDMF_GEOMETRY_ORIGIN_DXDYDZ )
        { 
        // Update geometry so that origin and spacing are read
        Geometry->Update();
        XdmfFloat64 *origin = Geometry->GetOrigin();
        XdmfFloat64 *spacing = Geometry->GetDxDyDz();
        
        outInfo->Set(vtkDataObject::ORIGIN(), 
                     origin[2], origin[1], origin[0]);
        outInfo->Set(vtkDataObject::SPACING(), 
                     spacing[2], spacing[1], spacing[0]);
        }
      }
    else  if ( xdmfGrid->GetTopology()->GetTopologyType() == XDMF_2DRECTMESH ||
               xdmfGrid->GetTopology()->GetTopologyType() == XDMF_3DRECTMESH )
      {
      vtkDebugWithObjectMacro(this->Reader, 
                              << "Setting Extents for vtkRectilinearGrid");
      outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
                   0, EndExtent[2], 0, EndExtent[1], 0, EndExtent[0]);
      
      }
    else 
      {
      vtkErrorWithObjectMacro(this->Reader,"Unknown topology type: " 
                              << xdmfGrid->GetTopology()->GetTopologyType());
      }
    
    int uExt[6];
    
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), uExt);
    
    vtkDebugWithObjectMacro(this->Reader, << "Update Extents: " << 
                            uExt[0] << ", " <<
                            uExt[1] << ", " <<
                            uExt[2] << ", " <<
                            uExt[3] << ", " <<
                            uExt[4] << ", " <<
                            uExt[5] );
    }
  return 1;
}

//============================================================================
class vtkXdmfReaderTester : public vtkXMLParser
{
public:
  vtkTypeMacro(vtkXdmfReaderTester, vtkXMLParser);
  static vtkXdmfReaderTester* New();
  int TestReadFile()
    {
      this->Valid = 0;
      if(!this->FileName)
        {
        return 0;
        }

      ifstream inFile(this->FileName);
      if(!inFile)
        {
        return 0;
        }

      this->SetStream(&inFile);
      this->Done = 0;

      this->Parse();

      if(this->Done && this->Valid )
        {
        return 1;
        }
      return 0;
    }
  void StartElement(const char* name, const char**)
    {
      this->Done = 1;
      if(strcmp(name, "Xdmf") == 0)
        {
        this->Valid = 1;
        }
    }

protected:
  vtkXdmfReaderTester()
    {
      this->Valid = 0;
      this->Done = 0;
    }

private:
  void ReportStrayAttribute(const char*, const char*, const char*) {}
  void ReportMissingAttribute(const char*, const char*) {}
  void ReportBadAttribute(const char*, const char*, const char*) {}
  void ReportUnknownElement(const char*) {}
  void ReportXmlParseError() {}

  int ParsingComplete() { return this->Done; }
  int Valid;
  int Done;
  vtkXdmfReaderTester(const vtkXdmfReaderTester&); // Not implemented
  void operator=(const vtkXdmfReaderTester&); // Not implemented
};

vtkStandardNewMacro(vtkXdmfReaderTester);

//============================================================================
vtkXdmfReader::vtkXdmfReader()
{
//this->SetNumberOfInputPorts(0);
//this->SetNumberOfOutputPorts(0);
  
  this->Internals = new vtkXdmfReaderInternal;
  this->Internals->Reader = this;

  // I have this flag because I do not want to change the initialization
  // of the output to the generic output.  It might break something;
  this->OutputsInitialized = 0;

  this->DOM = 0;

  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->CellDataArraySelection = vtkDataArraySelection::New();

  // Setup the selection callback to modify this object when an array
  // selection is changed.
  this->SelectionObserver = vtkCallbackCommand::New();
  this->SelectionObserver->SetCallback(
    &vtkXdmfReader::SelectionModifiedCallback);
  this->SelectionObserver->SetClientData(this);
  this->PointDataArraySelection->AddObserver(vtkCommand::ModifiedEvent,
                                             this->SelectionObserver);
  this->CellDataArraySelection->AddObserver(vtkCommand::ModifiedEvent,
                                            this->SelectionObserver);

  this->DomainName = 0;
  this->Internals->DomainPtr = 0;
  this->GridName = 0;
  
  for (int i = 0; i < 3; i ++ )
    {
    this->Stride[i] = 1;
    }

  this->GridsModified = 0;
  
  this->NumberOfEnabledActualGrids=0;
  
  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkXdmfReader::~vtkXdmfReader()
{
  this->CellDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->PointDataArraySelection->RemoveObserver(this->SelectionObserver);
  this->SelectionObserver->Delete();
  this->CellDataArraySelection->Delete();
  this->PointDataArraySelection->Delete();

  this->SetDomainName(0);
  
  vtkXdmfReaderInternal::MapOfActualGrids::iterator actualGridIt;
  for ( actualGridIt = this->Internals->ActualGrids.begin();
        actualGridIt != this->Internals->ActualGrids.end();
        ++ actualGridIt )
    {
    vtkXdmfReaderActualGrid* grid = &actualGridIt->second;
    if ( grid->Grid )
      {
      delete grid->Grid;
      }
    if ( grid->Collection )
      {
      vtkXdmfReaderGridCollection::SetOfGrids::iterator gridIt;
      int i =0;
      for ( gridIt = grid->Collection->Grids.begin();
            gridIt != grid->Collection->Grids.end();
            ++ gridIt )
        {
        delete gridIt->second;
        i++;
        }
      grid->Collection->Grids.clear();
      delete grid->Collection;
      }
    }
  this->Internals->ActualGrids.clear();

  delete this->Internals;

  if ( this->DOM )
    {
    delete this->DOM;
    }

  H5garbage_collect();
  
  this->SetController(0);
}

//----------------------------------------------------------------------------
vtkDataObject *vtkXdmfReader::GetOutput()
{
  return this->GetOutput(0);
}

//----------------------------------------------------------------------------
vtkDataObject *vtkXdmfReader::GetOutput(int idx)
{
  return this->GetExecutive()->GetOutputData(idx);
}


//----------------------------------------------------------------------------
void vtkXdmfReader::SelectionModifiedCallback(vtkObject*, unsigned long,
                                              void* clientdata, void*)
{
  static_cast<vtkXdmfReader*>(clientdata)->Modified();
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetNumberOfPointArrays()
{
  return this->PointDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetNumberOfCellArrays()
{
  return this->CellDataArraySelection->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
const char* vtkXdmfReader::GetPointArrayName(int index)
{
  return this->PointDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
const char* vtkXdmfReader::GetCellArrayName(int index)
{
  return this->CellDataArraySelection->GetArrayName(index);
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetPointArrayStatus(const char* name)
{
  return this->PointDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetCellArrayStatus(const char* name)
{
  return this->CellDataArraySelection->ArrayIsEnabled(name);
}

//----------------------------------------------------------------------------
void vtkXdmfReader::SetPointArrayStatus(const char* name, int status)
{
  vtkDebugMacro("Set point array \"" << name << "\" status to: " << status);
  if(status)
    {
    this->PointDataArraySelection->EnableArray(name);
    }
  else
    {
    this->PointDataArraySelection->DisableArray(name);
    }
}

//----------------------------------------------------------------------------
void vtkXdmfReader::SetCellArrayStatus(const char* name, int status)
{
  vtkDebugMacro("Set cell array \"" << name << "\" status to: " << status);
  if(status)
    {
    this->CellDataArraySelection->EnableArray(name);
    }
  else
    {
    this->CellDataArraySelection->DisableArray(name);
    }
}

//----------------------------------------------------------------------------
void vtkXdmfReader::EnableAllArrays()
{
  vtkDebugMacro("Enable all point and cell arrays");
  this->PointDataArraySelection->EnableAllArrays();
  this->CellDataArraySelection->EnableAllArrays();
}

//----------------------------------------------------------------------------
void vtkXdmfReader::DisableAllArrays()
{
  vtkDebugMacro("Disable all point and cell arrays");
  this->PointDataArraySelection->DisableAllArrays();
  this->CellDataArraySelection->DisableAllArrays();
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetNumberOfParameters()
{
  if(!this->DOM) 
    {
    return(-1);
    }
  // return(this->DOM->FindNumberOfParameters());
  return(0);
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetParameterType(const char *vtkNotUsed(parameterName))
{
  /*
    XdmfParameter *Param;
  
  
    if(!this->DOM) 
    {
    return(0);
    }
    Param = this->DOM->FindParameter(parameterName);
    if(Param) 
    {
    return(Param->GetParameterType());
    } 
    else 
    {
    return(0);
    }
  */
  return(0);
}
//----------------------------------------------------------------------------
const char *vtkXdmfReader::GetParameterTypeAsString(const char *vtkNotUsed(parameterName))
{
  /*
    if (this->GetParameterType(parameterName) == XDMF_PARAMETER_RANGE_TYPE) 
    {
    return("RANGE");
    } 
  */
  return("LIST");
}
//----------------------------------------------------------------------------
int vtkXdmfReader::GetParameterType(int vtkNotUsed(index))
{
  /*
    XdmfParameter *Param;
  
  
    if(!this->DOM) 
    {
    return(0);
    }
    Param = this->DOM->GetParameter(index);
    if(Param) 
    {
    return(Param->GetParameterType());
    } 
    else 
    {
    return(0);
    }
  */
  return(0);
}

//----------------------------------------------------------------------------
const char *vtkXdmfReader::GetParameterTypeAsString(int vtkNotUsed(index))
{
  
  /*
    if (this->GetParameterType(index) == XDMF_PARAMETER_RANGE_TYPE) 
    {
    return("RANGE");
    } 
  */
  return("LIST");
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetParameterRange(int vtkNotUsed(index), int vtkNotUsed(Shape)[3])
{
  /*
    XdmfParameter *Param;
    XdmfArray  *Parray;
  
  
    if(!this->DOM) 
    {
    return(0);
    }
    Param = this->DOM->GetParameter(index);
    if(Param) 
    {
    if( Param->GetParameterType() == XDMF_PARAMETER_RANGE_TYPE )
    {
    Parray = Param->GetArray();
    Shape[0] = Parray->GetValueAsInt64(0);
    Shape[1] = Parray->GetValueAsInt64(1);
    Shape[2] = Parray->GetValueAsInt64(2);
    } else {
    Shape[0] = 0;
    Shape[1] = 1;
    Shape[2] = Param->GetNumberOfElements();
    }
    return(Shape[2]);
    }
  */
  return(0);
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetParameterRange(const char *vtkNotUsed(parameterName), int vtkNotUsed(Shape)[3])
{
  /*
    XdmfParameter *Param;
    XdmfArray  *Parray;
  
    if(!this->DOM) 
    {
    return(0);
    }
    Param = this->DOM->FindParameter(parameterName);
    if(Param) 
    {
    if( Param->GetParameterType() == XDMF_PARAMETER_RANGE_TYPE )
    {
    Parray = Param->GetArray();
    Shape[0] = Parray->GetValueAsInt64(0);
    Shape[1] = Parray->GetValueAsInt64(1);
    Shape[2] = Parray->GetValueAsInt64(2);
    } 
    else 
    {
    Shape[0] = 0;
    Shape[1] = 1;
    Shape[2] = Param->GetNumberOfElements();
    }
    return(Shape[2]);
    }
  */
  return(0);
}

//----------------------------------------------------------------------------
const char *vtkXdmfReader::GetParameterRangeAsString(int vtkNotUsed(index))
{
  /*
    int Range[3];
    ostrstream StringOutput;
  
    if(this->GetParameterRange(index, Range) <= 0)
    {
    return(NULL);
    }
    StringOutput << ICE_64BIT_CAST Range[0] << " ";
    StringOutput << ICE_64BIT_CAST Range[1] << " ";
    StringOutput << ICE_64BIT_CAST Range[2] << ends;
    return(StringOutput.str());
  */
  return(0);
}

//----------------------------------------------------------------------------
const char *vtkXdmfReader::GetParameterRangeAsString(const char *vtkNotUsed(parameterName))
{
  /*
    int Range[3];
    ostrstream StringOutput;
  
    if (this->GetParameterRange(parameterName, Range) <= 0) 
    {
    return(NULL);
    }
    StringOutput << ICE_64BIT_CAST Range[0] << " ";
    StringOutput << ICE_64BIT_CAST Range[1] << " ";
    StringOutput << ICE_64BIT_CAST Range[2] << ends;
    return(StringOutput.str());
  */
  return(0);
}

//----------------------------------------------------------------------------
const char *vtkXdmfReader::GetParameterName(int vtkNotUsed(index))
{
  /*
    XdmfParameter *Param;
  
  
    if(!this->DOM) 
    {
    return(0);
    }
    Param = this->DOM->GetParameter(index);
    if(Param) 
    {
    return(Param->GetParameterName());
    } 
    else 
    {
    return(0);
    }
  */
  return(0);
}

//----------------------------------------------------------------------------
int vtkXdmfReader::SetParameterIndex(int vtkNotUsed(Index), int vtkNotUsed(CurrentIndex)) 
{
  /*
    XdmfParameter *Param;
  
  
    if(!this->DOM) 
    {
    return(0);
    }
    Param = this->DOM->GetParameter(Index);
    if(!Param) 
    {
    return(-1);
    }
    this->Modified();
    return(Param->SetCurrentIndex(CurrentIndex));
  */
  return(0);
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetParameterIndex(int vtkNotUsed(Index)) 
{
  /*
    XdmfParameter *Param;
  
  
    if(!this->DOM) 
    {
    return(0);
    }
    Param = this->DOM->GetParameter(Index);
    if(!Param) 
    {
    return(-1);
    }
    return(Param->GetCurrentIndex());
  */
  return(0);
}

//----------------------------------------------------------------------------
int vtkXdmfReader::SetParameterIndex(const char *vtkNotUsed(ParameterName),
                                     int vtkNotUsed(CurrentIndex)) 
{
  /*
    XdmfParameter *Param;
    int Status=-1;

    if(!this->DOM) 
    {
    return(0);
    }
    for(int i=0 ; i < this->DOM->FindNumberOfParameters() ;  i++)
    {
    Param = this->DOM->GetParameter(i);
    if(!Param) 
    {
    return(-1);
    }
    if(XDMF_WORD_CMP(Param->GetParameterName(), ParameterName))
    {
    Status = Param->SetCurrentIndex(CurrentIndex);
    this->Modified();
    if(Status <= 0 ) 
    {
    return(Status);
    }
    }
    }
    return(Status);
  */
  return(0);
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetParameterIndex(const char *vtkNotUsed(parameterName)) 
{
  /*
    XdmfParameter *Param;
  
  
    if(!this->DOM) 
    {
    return(0);
    }
    Param = this->DOM->FindParameter(parameterName);
    if(!Param) 
    {
    return(-1);
    }
    return(Param->GetCurrentIndex());
  */
  return(0);
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetParameterLength(int vtkNotUsed(index))
{
  /*
    XdmfParameter *Param;
  
  
    if(!this->DOM) 
    {
    return(0);
    }
    Param = this->DOM->GetParameter(index);
    if(Param) 
    {
    return(Param->GetNumberOfElements());
    } 
    else 
    {
    return(0);
    }
  */
  return(0);
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetParameterLength(const char *vtkNotUsed(parameterName))
{
  /*
    XdmfParameter *Param;
  
  
    if(!this->DOM) 
    {
    return(0);
    }
    Param = this->DOM->FindParameter(parameterName);
    if(Param) 
    {
    return(Param->GetNumberOfElements());
    } 
    else 
    {
    return(0);
    }
  */
  return(0);
}

//----------------------------------------------------------------------------
const char *vtkXdmfReader::GetParameterValue(const char *vtkNotUsed(parameterName)) 
{
  /*
    XdmfParameter *Param;
  
  
    if(!this->DOM) 
    {
    return(0);
    }
    Param = this->DOM->FindParameter(parameterName);
    if(!Param) 
    {
    return(0);
    }
    Param->Update();
    return(Param->GetParameterValue());
  */
  return(0);
}

//----------------------------------------------------------------------------
const char *vtkXdmfReader::GetParameterValue(int vtkNotUsed(index)) 
{
  /*
    XdmfParameter *Param;
  
  
    if(!this->DOM) 
    {
    return(0);
    }
    Param = this->DOM->GetParameter(index);
    if(!Param) 
    {
    return(0);
    }
    Param->Update();
    return(Param->GetParameterValue());
  */
  return(0);
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetNumberOfDomains()
{
  return this->Internals->DomainList.size();
}

//----------------------------------------------------------------------------
void vtkXdmfReader::SetDomainName(const char* domain)
{
  if ( this->DomainName == domain )
    {
    return;
    }
  if ( this->DomainName && domain && strcmp(this->DomainName, domain) == 0 )
    {
    return;
    }
  if ( this->DomainName )
    {
    delete [] this->DomainName;
    this->DomainName = 0;
    }
  if ( domain )
    {
    this->DomainName = new char [ strlen(domain) + 1 ];
    strcpy(this->DomainName, domain);
    }
  this->GridsModified = 1;
}

//----------------------------------------------------------------------------
const char* vtkXdmfReader::GetDomainName(int idx)
{
  return this->Internals->DomainList[idx].c_str();
}

//----------------------------------------------------------------------------
void vtkXdmfReader::SetGridName(const char* grid)
{
  this->DisableAllGrids();
  this->EnableGrid(grid);
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetNumberOfGrids()
{
  return this->Internals->ActualGrids.size();
}

//----------------------------------------------------------------------------
const char* vtkXdmfReader::GetGridName(int idx)
{
  if ( idx < 0 )
    {
    return 0;
    }
  vtkXdmfReaderInternal::MapOfActualGrids::iterator it;
  int cnt = 0;
  for ( it = this->Internals->ActualGrids.begin();
        it != this->Internals->ActualGrids.end();
        ++ it )
    {
    if ( cnt == idx )
      {
      return it->first.c_str();
      }
    cnt ++;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetGridIndex(const char* name)
{
  if ( !name )
    {
    return -1;
    }
  vtkXdmfReaderInternal::MapOfActualGrids::iterator it;
  int cnt = 0;
  for ( it = this->Internals->ActualGrids.begin();
        it != this->Internals->ActualGrids.end();
        ++ it )
    {
    if ( it->first == name )
      {
      return cnt;
      }
    cnt ++;
    }

  return -1;
}

//----------------------------------------------------------------------------
void vtkXdmfReader::EnableGrid(const char* name)
{
  vtkXdmfReaderActualGrid* grid = this->Internals->GetGrid(name);
  if ( !grid || 
       (!grid->Grid && !grid->Collection) ) //work around an undo/redo crash
    {
    return;
    }
  if(!grid->Enabled)
    {
    ++this->NumberOfEnabledActualGrids;
    grid->Enabled = 1;
    vtkDebugMacro("Enable grid \"" << name << "\"");
    this->Modified();
    this->UpdateInformation();
    }
}

//----------------------------------------------------------------------------
void vtkXdmfReader::EnableGrid(int idx)
{
  vtkDebugMacro("Enable grid \"" << idx << "\"");
  vtkXdmfReaderActualGrid* grid = this->Internals->GetGrid(idx);
  if ( !grid )
    {
    return;
    }
  
  if(!grid->Enabled)
    {
    ++this->NumberOfEnabledActualGrids;
    grid->Enabled = 1;
    this->PointDataArraySelection->RemoveAllArrays();
    this->CellDataArraySelection->RemoveAllArrays();
    this->Modified();
    this->UpdateInformation();
    }
}

//----------------------------------------------------------------------------
void vtkXdmfReader::EnableAllGrids()
{
  vtkDebugMacro("Enable all grids");
  vtkXdmfReaderInternal::MapOfActualGrids::iterator it;
  int changed=0;
  for ( it = this->Internals->ActualGrids.begin();
        it != this->Internals->ActualGrids.end();
        ++ it )
    {
    if(!it->second.Enabled)
      {
      it->second.Enabled = 1;
      ++this->NumberOfEnabledActualGrids;
      changed=1;
      }
    }
  if(changed)
    {
    this->PointDataArraySelection->RemoveAllArrays();
    this->CellDataArraySelection->RemoveAllArrays();
    this->Modified();
    this->UpdateInformation();
    }
}

//----------------------------------------------------------------------------
void vtkXdmfReader::DisableGrid(const char* name)
{
  vtkDebugMacro("Disable grid \"" << name << "\"");
  vtkXdmfReaderActualGrid* grid = this->Internals->GetGrid(name);
  if ( !grid )
    {
    return;
    }
  
  if(grid->Enabled)
    {
    grid->Enabled = 0;
    --this->NumberOfEnabledActualGrids;
    this->PointDataArraySelection->RemoveAllArrays();
    this->CellDataArraySelection->RemoveAllArrays();

    this->Modified();
    this->UpdateInformation();
    }
}

//----------------------------------------------------------------------------
void vtkXdmfReader::DisableGrid(int idx)
{
  vtkDebugMacro("Disable grid \"" << idx << "\"");
  vtkXdmfReaderActualGrid* grid = this->Internals->GetGrid(idx);
  if ( !grid )
    {
    return;
    }
  
  if(grid->Enabled)
    {
    grid->Enabled = 0;
    --this->NumberOfEnabledActualGrids;
    this->PointDataArraySelection->RemoveAllArrays();
    this->CellDataArraySelection->RemoveAllArrays();
    this->Modified();
    this->UpdateInformation();
    }
}

//----------------------------------------------------------------------------
void vtkXdmfReader::DisableAllGrids()
{
  vtkDebugMacro("Disable all grids");
  vtkXdmfReaderInternal::MapOfActualGrids::iterator it;
  int changed=0;
  for ( it = this->Internals->ActualGrids.begin();
        it != this->Internals->ActualGrids.end();
        ++ it )
    {
    if(it->second.Enabled)
      {
      it->second.Enabled = 0;
      --this->NumberOfEnabledActualGrids;
      changed=1;
      }
    }
  if(changed)
    {
    this->PointDataArraySelection->RemoveAllArrays();
    this->CellDataArraySelection->RemoveAllArrays();
    this->Modified();
    this->UpdateInformation();
    }
}

//----------------------------------------------------------------------------
void vtkXdmfReader::RemoveAllGrids()
{
  vtkDebugMacro("Remove all grids");
  vtkXdmfReaderInternal::MapOfActualGrids::iterator it;
  this->Internals->ActualGrids.clear();
  this->NumberOfEnabledActualGrids = 0;
  this->GridsModified = 1;
  this->PointDataArraySelection->RemoveAllArrays();
  this->CellDataArraySelection->RemoveAllArrays();
  this->Modified();
  this->UpdateInformation();
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetGridSetting(const char* name)
{
  vtkXdmfReaderActualGrid* grid = this->Internals->GetGrid(name);
  if ( !grid )
    {
    return 0;
    }
  return grid->Enabled;
}

//----------------------------------------------------------------------------
int vtkXdmfReader::GetGridSetting(int idx)
{
  vtkXdmfReaderActualGrid* grid = this->Internals->GetGrid(idx);
  if ( !grid )
    {
    return 0;
    }
  return grid->Enabled;
}

//----------------------------------------------------------------------------
void vtkXdmfReader::UpdateUniformGrid(void *GridNode, char * CollectionName)
{

  XdmfConstString gridName = this->DOM->Get( (XdmfXmlNode)GridNode, "Name" );
  ostrstream str;
  if ( !gridName )
    {
    str << this->DOM->GetUniqueName("Grid") << ends;
    }
  else
    {
    str << gridName << ends;
    }
  gridName = str.str();
  vtkDebugMacro( << "Reading Light Data for " << gridName );
  XdmfConstString levelName = this->DOM->Get((XdmfXmlNode) GridNode, "Level" );

  vtkXdmfReaderGrid* grid = this->Internals->GetXdmfGrid(gridName, CollectionName,levelName);
  if ( !grid )
    {
    // Error happened
    str.rdbuf()->freeze(0);
    return;
    }
  if ( !grid->XMGrid )
    {
    grid->XMGrid = new XdmfGrid;
    }
  vtkDebugMacro(" .... Setting Grid Information");
  grid->XMGrid->SetDOM(this->DOM);
  grid->XMGrid->SetElement((XdmfXmlNode)GridNode);
  grid->XMGrid->UpdateInformation();
  str.rdbuf()->freeze(0);
  this->GridsModified = 0;
}

//----------------------------------------------------------------------------
void vtkXdmfReader::UpdateNonUniformGrid(void *GridNode, char * CollectionName)
{
  int done = 0;
  int NGrid;
  vtkIdType currentGrid;
  XdmfXmlNode gridNode = 0;

  NGrid = this->DOM->FindNumberOfElements("Grid", (XdmfXmlNode)GridNode);
  for ( currentGrid = 0; !done; currentGrid ++ )
    {
    // Find the first level grids under Domain
    gridNode = this->DOM->FindElement("Grid", 
                                      currentGrid, 
                                      (XdmfXmlNode)GridNode);
    if ( !gridNode )
      {
      break;
      }

    XdmfConstString gridName = this->DOM->Get( gridNode, "Name" );
    ostrstream str;
    if ( !gridName )
      {
      str << "Grid" << currentGrid << ends;
      }
    else
      {
      str << gridName << ends;
      }
    gridName = str.str();
    vtkDebugMacro( << "Reading Light Data for " << gridName );
    // What Type of Grid
    XdmfConstString gridType = this->DOM->Get(gridNode, "GridType");
    if(!gridType)
      {
      // Accept Old Style
      gridType = this->DOM->Get(gridNode, "Type");
      }
    if(XDMF_WORD_CMP(gridType, "Tree"))
      {
      vtkDebugMacro( << " Grid is a Tree ");
      this->UpdateNonUniformGrid(gridNode, CollectionName);
      } 
    else if(XDMF_WORD_CMP(gridType, "Collection"))
      {
      // Collection : collName is gridName
      vtkDebugMacro( << " Grid is a Collection");
      this->UpdateNonUniformGrid(gridNode, CollectionName);
      }
    else
      {
      // It's a Uniform Grid
      this->UpdateUniformGrid(gridNode, CollectionName);
      }
    str.rdbuf()->freeze(0);
    }
  this->GridsModified = 0;
}

//----------------------------------------------------------------------------
void vtkXdmfReader::UpdateGrids()
{
  int done = 0;
  int NGrid;
  vtkIdType currentGrid;
  XdmfXmlNode gridNode = 0;
  XdmfXmlNode domain = this->Internals->DomainPtr;

  if ( !domain )
    {
    return;
    }

  if( !this->GridsModified )
    {
    vtkDebugMacro( << "Skipping Grid Update : Not Modified");
    return;
    }

  NGrid = this->DOM->FindNumberOfElements("Grid", domain);
  for ( currentGrid = 0; !done; currentGrid ++ )
    {
    // Find the first level grids under Domain
    gridNode = this->DOM->FindElement("Grid", currentGrid, domain);
    if ( !gridNode )
      {
      break;
      }

    XdmfConstString gridName = this->DOM->Get( gridNode, "Name" );
    ostrstream str;
    if ( !gridName )
      {
      str << "Grid" << currentGrid << ends;
      }
    else
      {
      str << gridName << ends;
      }
    gridName = str.str();
    vtkDebugMacro( << "Reading Light Data for " << gridName );
    // Check for Old Style Collection
    XdmfConstString collectionName = this->DOM->Get( gridNode, "Collection" );
    // Copy collectionName because it is a pointer to an internal
    // string that will reused and overwritten when we will call Get
    // on "Level".
    char *collName=0;
    if(collectionName!=0)
      {
      collName=new char[strlen(collectionName)+1]; 
      strcpy(collName,collectionName);
      }
    // What Type of Grid
    XdmfConstString gridType = this->DOM->Get(gridNode, "GridType");
    if(!gridType)
      {
      // Accept Old Style
      gridType = this->DOM->Get(gridNode, "Type");
      }
    if(XDMF_WORD_CMP(gridType, "Tree"))
      {
      // Tree : collName is gridName
      vtkDebugMacro( << " Grid is a Tree ");
      if(collName) delete [] collName;
      collName=new char[strlen(gridName)+1]; 
      strcpy(collName,  gridName);
      this->UpdateNonUniformGrid(gridNode, collName);
      } 
    else if(XDMF_WORD_CMP(gridType, "Collection"))
      {
      // Collection : collName is gridName
      vtkDebugMacro( << " Grid is a Collection");
      if(collName) delete [] collName;
      collName=new char[strlen(gridName)+1]; 
      strcpy(collName,  gridName);
      this->UpdateNonUniformGrid(gridNode, collName);
      }
    else
      {
      // It's a Uniform Grid
      // All grids are treated the same so make it a collection
      // if(collName) delete [] collName;
      if(!collName)
        {
        collName=new char[strlen(gridName)+1]; 
        strcpy(collName,  gridName);
        }
      this->UpdateUniformGrid(gridNode, collName);
      }
    
    if(collName) delete [] collName;
    str.rdbuf()->freeze(0);
    }
  
  this->GridsModified = 0;
}

//----------------------------------------------------------------------------
void vtkXdmfReader::SetStride(int x, int y, int z)
{
  if ( x <= 0 || y <= 0 || z <= 0 )
    {
    vtkErrorMacro("Strides have to be greater than 0.");
    return;
    }
  vtkDebugMacro(<< this->GetClassName() << " (" << this 
                << "): setting Stride to (" << x << "," << y << "," << z << ")");
  if ((this->Stride[0] != x)||(this->Stride[1] != y)||(this->Stride[2] != z))
    {
    this->Stride[0] = x;
    this->Stride[1] = y;
    this->Stride[2] = z;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
const char * vtkXdmfReader::GetXdmfDOMHandle() 
{
  return( XdmfObjectToHandle( this->DOM ) );
}

//----------------------------------------------------------------------------
int vtkXdmfReader::CanReadFile(const char* fname)
{
  vtkXdmfReaderTester* tester = vtkXdmfReaderTester::New();
  tester->SetFileName(fname);
  int res = tester->TestReadFile();
  tester->Delete();
  return res;
}

//----------------------------------------------------------------------------
int vtkXdmfReader::ProcessRequest(vtkInformation* request,
                                  vtkInformationVector** inputVector,
                                  vtkInformationVector* outputVector)
{
  // create the output
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    return this->RequestDataObject(outputVector);
    }
  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkXdmfReader::RequestDataObject(vtkInformationVector *outputVector)
{
  // Set the number of outputs and the create the output data objects
  // needed because we may change the number of output ports
  // inside RequestDataObject. So we may change the output vector.
  vtkInformationVector *newOutputVector=outputVector;
  
  ////////////
  vtkIdType cc;
  XdmfConstString CurrentFileName;

  if ( !this->FileName )
    {
    vtkErrorMacro("File name not set");
    return 1;
    }
  // First make sure the file exists.  This prevents an empty file
  // from being created on older compilers.
  struct stat fs;
  if(stat(this->FileName, &fs) != 0)
    {
    vtkErrorMacro("Error opening file " << this->FileName);
    return 1;
    }
  if ( !this->DOM )
    {
    this->DOM = new XdmfDOM();
    }
  // this->DOM->GlobalDebugOn();
  if ( !this->Internals->DataItem )
    {
    this->Internals->DataItem = new XdmfDataItem();
    this->Internals->DataItem->SetDOM(this->DOM);
    }
  CurrentFileName = this->DOM->GetInputFileName();
  
  // Don't Re-Parse : Could Reset Parameters
  if((CurrentFileName == NULL) ||
     (STRCASECMP(CurrentFileName, this->FileName) != 0 ))
    {
    this->DOM->SetInputFileName(this->FileName);
    vtkDebugMacro( << ".!!............Preparing to Parse " << this->FileName);
    this->DOM->Parse();
    // Added By Jerry Clarke
    this->GridsModified = 1;
    }

  XdmfXmlNode domain = 0;
  int done = 0;
  this->Internals->DomainList.erase(this->Internals->DomainList.begin(),
                                    this->Internals->DomainList.end());
  for ( cc = 0; !done; cc ++ )
    {
    ostrstream str1, str2;
    domain = this->DOM->FindElement("Domain", cc);
    if ( !domain )
      {
      break;
      }
    XdmfConstString domainName = this->DOM->Get( domain, "Name" );
    ostrstream str;
    if ( !domainName )
      {
      str << "Domain" << cc << ends;
      domainName = str.str();
      }
    this->Internals->DomainList.push_back(domainName);
    str.rdbuf()->freeze(0);
    }
  if ( this->DomainName )
    {
    for ( cc = 0; !done; cc ++ )
      {
      domain = this->DOM->FindElement("Domain", cc);
      if ( !domain )
        {
        break;
        }
      XdmfConstString domainName = this->DOM->Get( domain, "Name" );
      ostrstream str;
      if ( !domainName )
        {
        str << "Domain" << cc << ends;
        domainName = str.str();
        }
      if( domainName && strcmp(domainName, this->DomainName) == 0)
        {
        str.rdbuf()->freeze(0);
        break;
        }      
      str.rdbuf()->freeze(0);
      }
    }

  if ( !domain )
    {
    domain = this->DOM->FindElement("Domain", 0); // 0 - domain index    
    }

  if ( !domain )
    {
    vtkErrorMacro("Cannot find any domain...");
    return 1;
    }

  this->Internals->DomainPtr = domain;
//  this->UpdateGrids();

  char* filename = 0;
  if ( this->FileName )
    {
    filename = new char[ strlen(this->FileName)+ 1];
    strcpy(filename, this->FileName);
    }
  int len = static_cast<int>(strlen(filename));
  for ( cc = len-1; cc >= 0; cc -- )
    {
    if ( filename[cc] != '/' && filename[cc] != '\\' )
      {
      filename[cc] = 0;
      }
    else
      {
      break;
      }
    }
  if ( filename[0] == 0 )
    {
    char buffer[1024];
    if ( GETCWD(buffer, 1023) )
      {
      delete [] filename;
      if ( buffer )
        {
        filename = new char[ strlen(buffer)+ 1];
        strcpy(filename, buffer);
        }
      }
    }
  this->DOM->SetWorkingDirectory(filename);
  delete [] filename;
////////////////
  
  
  this->UpdateGrids();

/*  
    if(1 !=this->GetNumberOfOutputPorts())
    {
    this->SetNumberOfOutputPorts(1);

    // We have to refresh the outputVector with this new number of ports.
    // The one in argument was given by the executive.
    // The only way to do that, is to ask the output vector from the executive.
    newOutputVector=this->GetExecutive()->GetOutputInformation();
    }
*/
  
  // for each output
  vtkXdmfReaderInternal::MapOfActualGrids::iterator currentGridIterator;
  currentGridIterator = this->Internals->ActualGrids.begin();

  
  int someOutputChanged=0;
  vtkInformation *jinfo=newOutputVector->GetInformationObject(0);
  vtkMultiGroupDataSet *output=vtkMultiGroupDataSet::SafeDownCast(jinfo->Get(vtkDataObject::DATA_OBJECT()));
  if(output==0)
    {
    someOutputChanged=1;
    output=vtkMultiGroupDataSet::New();
    output->SetPipelineInformation(jinfo);
    output->Delete();
    }
  // Collapse on second level of Hierarchy
  output->SetNumberOfGroups(this->NumberOfEnabledActualGrids);
  //
  //while(currentGridIterator != this->Internals->ActualGrids.end())
  if(someOutputChanged)
    {
    this->GetPointDataArraySelection()->RemoveAllArrays();
    this->GetCellDataArraySelection()->RemoveAllArrays();
    }
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkXdmfReader::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkDebugMacro("Execute");
  if ( !this->FileName )
    {
    vtkErrorMacro("File name not set");
    return 0;
    }

  if ( !this->DOM )
    {
    return 0;
    }
  
  int outputGrid = 0;
  vtkXdmfReaderInternal::MapOfActualGrids::iterator currentGridIterator;
  for ( currentGridIterator = this->Internals->ActualGrids.begin();
        currentGridIterator != this->Internals->ActualGrids.end();
        ++currentGridIterator )
    {
    if (currentGridIterator->second.Enabled )
      {
      this->Internals->RequestActualGridData(
        currentGridIterator->first.c_str(),&currentGridIterator->second,
        outputGrid, this->NumberOfEnabledActualGrids, outputVector);
      outputGrid++;
      this->UpdateProgress(1.0 * outputGrid / this->NumberOfEnabledActualGrids);
      }
    }
  
  return 1;
}

//-----------------------------------------------------------------------------
int vtkXdmfReader::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  vtkDebugMacro("ExecuteInformation");
  
  int numPorts = this->GetNumberOfOutputPorts();
  for (int i=0; i<numPorts; i++)
    {
    vtkInformation *info = outputVector->GetInformationObject(0);
    info->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);

    vtkMultiGroupDataInformation *compInfo=vtkMultiGroupDataInformation::New();
    compInfo->SetNumberOfGroups(this->NumberOfEnabledActualGrids);
    info->Set(vtkCompositeDataPipeline::COMPOSITE_DATA_INFORMATION(),compInfo);
    compInfo->Delete();
    }
  
  int outputGrid = 0;
  vtkXdmfReaderInternal::MapOfActualGrids::iterator currentGridIterator;
  for ( currentGridIterator = this->Internals->ActualGrids.begin();
        currentGridIterator != this->Internals->ActualGrids.end();
        ++currentGridIterator )
    {
    if ( currentGridIterator->second.Enabled )
      {
      vtkDebugMacro(<< "Processing grid: " 
                    << currentGridIterator->first.c_str() 
                    << " / " << outputGrid);
      if (this->Internals->RequestActualGridInformation(
            &currentGridIterator->second, 
            outputGrid, 
            this->NumberOfEnabledActualGrids, 
            outputVector) )
        {
        outputGrid ++;
        }
      }
    }
  return 1;
}

//----------------------------------------------------------------------------

int vtkXdmfReader::FillOutputPortInformation(int,
                                             vtkInformation *info)
{ 
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
void vtkXdmfReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CellDataArraySelection: " << this->CellDataArraySelection 
     << endl;
  os << indent << "PointDataArraySelection: " << this->PointDataArraySelection 
     << endl;
  os << indent << "Domain: " << this->DomainName << endl;
  int cc;
  os << indent << "Grids:" << endl;
  for ( cc = 0; cc < this->GetNumberOfGrids(); ++ cc )
    {
    os << indent << " " << cc << ": " << this->GetGridName(cc) << " - "
       << (this->GetGridSetting(cc)?"enabled":"disabled") << endl;
    }
  os << indent << "Outputs: " << this->GetNumberOfOutputPorts() << endl;
  for ( cc = 0; cc < this->GetNumberOfOutputPorts(); cc ++ )
    {
    //vtkIndent nindent = indent.GetNextIndent();
//    os << nindent << "Output " << cc << " " << this->GetOutput(cc)->GetClassName() << endl;
//    this->GetOutput(cc)->PrintSelf(os, nindent.GetNextIndent());
    }
}
