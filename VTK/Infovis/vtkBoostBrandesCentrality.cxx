/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkBoostBrandesCentrality.cxx,v $

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
#include "vtkBoostBrandesCentrality.h"

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkMath.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>
#include <vtkDataArray.h>
#include <vtkStringArray.h>

#include "vtkGraphToBoostAdapter.h"
#include "vtkGraph.h"
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/betweenness_centrality.hpp>
#include <boost/vector_property_map.hpp>

using namespace boost;

vtkCxxRevisionMacro(vtkBoostBrandesCentrality, "$Revision: 1.2 $");
vtkStandardNewMacro(vtkBoostBrandesCentrality);




// Constructor/Destructor
vtkBoostBrandesCentrality::vtkBoostBrandesCentrality()
{

}

vtkBoostBrandesCentrality::~vtkBoostBrandesCentrality()
{

}


int vtkBoostBrandesCentrality::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkGraph *input = vtkGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkGraph *output = vtkGraph::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Send the data to output.
  output->ShallowCopy(input);

  // Compute betweenness centrality
  vtkFloatArray* cMap = vtkFloatArray::New();
  cMap->SetName("centrality");
  identity_property_map imap;
  
  // Is the graph directed or undirected
  if (output->GetDirected())
    {
    vtkBoostDirectedGraph g(output);
    brandes_betweenness_centrality(g, centrality_map(cMap).vertex_index_map(imap));
    }
  else
    {
    vtkBoostUndirectedGraph g(output);
    brandes_betweenness_centrality(g, centrality_map(cMap).vertex_index_map(imap));
    }
    
  output->GetVertexData()->AddArray(cMap);
  cMap->Delete();

  return 1;
}

void vtkBoostBrandesCentrality::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}

