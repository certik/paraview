/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkBoostConnectedComponents.cxx,v $

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
#include "vtkBoostConnectedComponents.h"

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkMath.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkFloatArray.h>
#include <vtkDataArray.h>

#include "vtkGraph.h"
#include "vtkGraphToBoostAdapter.h"
#include <boost/graph/strong_components.hpp>
#include <boost/vector_property_map.hpp>

using namespace boost;

vtkCxxRevisionMacro(vtkBoostConnectedComponents, "$Revision: 1.2 $");
vtkStandardNewMacro(vtkBoostConnectedComponents);

vtkBoostConnectedComponents::vtkBoostConnectedComponents()
{
}

vtkBoostConnectedComponents::~vtkBoostConnectedComponents()
{
}

int vtkBoostConnectedComponents::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkGraph *input = vtkGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkGraph *output = vtkGraph::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Send the data to output.
  output->ShallowCopy(input);

  // Compute connected components.
  if (input->GetDirected())
    {
    vtkBoostDirectedGraph g(output);
    vtkIntArray* comps = vtkIntArray::New();
    comps->SetName("component");
    vector_property_map<default_color_type> color;
    vector_property_map<vtkIdType> root;
    vector_property_map<vtkIdType> discoverTime;
    strong_components(g, comps, color_map(color).root_map(root).discover_time_map(discoverTime));
    output->GetVertexData()->AddArray(comps);
    comps->Delete();
    }
  else
    {
    vtkBoostUndirectedGraph g(output);
    vtkIntArray* comps = vtkIntArray::New();
    comps->SetName("component");
    vector_property_map<default_color_type> color;
    connected_components(g, comps, color_map(color));
    output->GetVertexData()->AddArray(comps);
    comps->Delete();
    }

  return 1;
}

void vtkBoostConnectedComponents::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

