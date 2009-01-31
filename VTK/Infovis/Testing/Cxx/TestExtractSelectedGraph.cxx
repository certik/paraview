/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: TestExtractSelectedGraph.cxx,v $

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
#include "vtkActor.h"
#include "vtkCircularLayoutStrategy.h"
#include "vtkDoubleArray.h"
#include "vtkExtractSelectedGraph.h"
#include "vtkGlyph3D.h"
#include "vtkGlyphSource2D.h"
#include "vtkGraphLayout.h"
#include "vtkGraphToPolyData.h"
#include "vtkIdTypeArray.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelection.h"
#include "vtkTestUtilities.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

void RenderGraph(vtkAlgorithm* alg, vtkRenderer* ren, double r, double g, double b, double z, float size)
{
  VTK_CREATE(vtkGraphToPolyData, graphToPoly);
  graphToPoly->SetInputConnection(alg->GetOutputPort());
  VTK_CREATE(vtkPolyDataMapper, edgeMapper);
  edgeMapper->SetInputConnection(graphToPoly->GetOutputPort());
  VTK_CREATE(vtkActor, edgeActor);
  edgeActor->SetMapper(edgeMapper);
  edgeActor->GetProperty()->SetColor(r, g, b);
  edgeActor->GetProperty()->SetLineWidth(size/2);
  edgeActor->SetPosition(0, 0, z);
  VTK_CREATE(vtkGlyphSource2D, vertex);
  vertex->SetGlyphTypeToVertex();
  VTK_CREATE(vtkGlyph3D, glyph);
  glyph->SetInputConnection(0, graphToPoly->GetOutputPort());
  glyph->SetInputConnection(1, vertex->GetOutputPort());
  VTK_CREATE(vtkPolyDataMapper, vertMapper);
  vertMapper->SetInputConnection(glyph->GetOutputPort());
  VTK_CREATE(vtkActor, vertActor);
  vertActor->SetMapper(vertMapper);
  vertActor->GetProperty()->SetColor(r, g, b);
  vertActor->GetProperty()->SetPointSize(size);
  vertActor->SetPosition(0, 0, z);
  ren->AddActor(edgeActor);
  ren->AddActor(vertActor);
}

int TestExtractSelectedGraph(int argc, char* argv[])
{
  VTK_CREATE(vtkRenderer, ren);
  
  cerr << "Creating test graph..." << endl;
  VTK_CREATE(vtkGraph, graph);
  graph->AddVertex();
  graph->AddVertex();
  graph->AddVertex();
  graph->AddVertex();
  graph->AddVertex();
  graph->AddEdge(0, 1);
  graph->AddEdge(1, 2);
  graph->AddEdge(2, 3);
  graph->AddEdge(3, 4);
  graph->AddEdge(4, 0);
  VTK_CREATE(vtkDoubleArray, valueArr);
  valueArr->InsertNextValue(-0.5);
  valueArr->InsertNextValue(0.0);
  valueArr->InsertNextValue(0.5);
  valueArr->InsertNextValue(1.0);
  valueArr->InsertNextValue(1.5);
  valueArr->SetName("value");
  graph->GetVertexData()->AddArray(valueArr);
  VTK_CREATE(vtkGraphLayout, layout);
  layout->SetInput(graph);
  VTK_CREATE(vtkCircularLayoutStrategy, circular);
  layout->SetLayoutStrategy(circular);
  RenderGraph(layout, ren, 1, 1, 1, 0.01, 2.0f);
  cerr << "...done." << endl;
    
  cerr << "Testing threshold selection..." << endl;
  VTK_CREATE(vtkSelection, threshold);
  threshold->SetContentType(vtkSelection::THRESHOLDS);
  threshold->SetFieldType(vtkSelection::POINT);
  VTK_CREATE(vtkDoubleArray, thresholdArr);
  thresholdArr->SetName("value");
  thresholdArr->InsertNextValue(0.0);
  thresholdArr->InsertNextValue(1.0);
  threshold->SetSelectionList(thresholdArr); 
  
  VTK_CREATE(vtkExtractSelectedGraph, extractThreshold);
  extractThreshold->SetInputConnection(0, layout->GetOutputPort());
  extractThreshold->SetInput(1, threshold);
  RenderGraph(extractThreshold, ren, 1, 0, 0, -0.01, 5.0f);
  cerr << "...done." << endl;
  
  cerr << "Testing indices selection..." << endl;
  VTK_CREATE(vtkSelection, indices);
  indices->SetContentType(vtkSelection::INDICES);
  indices->SetFieldType(vtkSelection::POINT);
  VTK_CREATE(vtkIdTypeArray, indicesArr);
  indicesArr->InsertNextValue(0);
  indicesArr->InsertNextValue(2);
  indicesArr->InsertNextValue(4);
  indices->SetSelectionList(indicesArr);
  
  VTK_CREATE(vtkExtractSelectedGraph, extractIndices);
  extractIndices->SetInputConnection(0, layout->GetOutputPort());
  extractIndices->SetInput(1, indices);
  RenderGraph(extractIndices, ren, 0, 1, 0, -0.02, 9.0f);
  cerr << "...done." << endl;
  
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  VTK_CREATE(vtkRenderWindow, win);
  win->AddRenderer(ren);
  win->SetInteractor(iren);

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    win->Render();
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
    }
  return !retVal;
}
