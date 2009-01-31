/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: TestBoostAdapter.cxx,v $

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
#include "vtkGraph.h"
#include "vtkGraphToBoostAdapter.h"
#include "vtkIntArray.h"
#include "vtkIdTypeArray.h"
#include "vtkMath.h"
#include "vtkTimerLog.h"
#include "vtkTreeToBoostAdapter.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <vtksys/stl/vector>
#include <vtksys/stl/map>
#include <vtksys/stl/utility>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/visitors.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/transitive_closure.hpp>
#include <boost/graph/sequential_vertex_coloring.hpp>
#include <boost/property_map.hpp>
#include <boost/vector_property_map.hpp>

using namespace boost;
using namespace vtksys_stl;

template <typename Graph>
void TestTraversal(Graph g, int repeat, int& vtkNotUsed(errors))
{
  typedef typename graph_traits<Graph>::edge_descriptor Edge;
  typedef typename graph_traits<Graph>::vertex_descriptor Vertex;
  
  VTK_CREATE(vtkTimerLog, timer);
  typename graph_traits<Graph>::vertex_iterator vi, viEnd;

  // Traverse the edge list of each vertex
  timer->StartTimer();
  int count = 0;
  for (int r = 0; r < repeat; r++)
    {
    for (tie(vi, viEnd) = vertices(g); vi != viEnd; ++vi)
      {
      typename graph_traits<Graph>::out_edge_iterator oi, oiEnd;
      tie(oi, oiEnd) = out_edges(*vi, g);
      count++;
      }
    }
  timer->StopTimer();
  double time_out_edges = timer->GetElapsedTime();
  cerr << "getting out edges: " << time_out_edges / count << " sec." << endl;

  Edge e = *(edges(g).first);
  Vertex v = *(vertices(g).first);
  vector<Edge> edge_vec;
  vector<Vertex> vert_vec;

  timer->StartTimer();
  count = 0;
  for (int r = 0; r < repeat; r++)
    {
    for (tie(vi, viEnd) = vertices(g); vi != viEnd; ++vi)
      {
      typename graph_traits<Graph>::out_edge_iterator oi, oiEnd;
      for (tie(oi, oiEnd) = out_edges(*vi, g); oi != oiEnd; ++oi)
        {
        count++;
        }
      }
    }
  timer->StopTimer();
  double time_inc = timer->GetElapsedTime();
  cerr << "+increment: " << time_inc / count << " sec." << endl;
  cerr << "  just increment: " << (time_inc - time_out_edges) / count << " sec." << endl;

  timer->StartTimer();
  count = 0;
  for (int r = 0; r < repeat; r++)
    {
    edge_vec.clear();
    vert_vec.clear();
    for (tie(vi, viEnd) = vertices(g); vi != viEnd; ++vi)
      {
      typename graph_traits<Graph>::out_edge_iterator oi, oiEnd;
      for (tie(oi, oiEnd) = out_edges(*vi, g); oi != oiEnd; ++oi)
        {
        edge_vec.push_back(e);
        vert_vec.push_back(v);
        count++;
        }
      }
    }
  timer->StopTimer();
  double time_push_back = timer->GetElapsedTime();
  cerr << "+push_back: " << time_push_back / count << " sec." << endl;
  cerr << "  just push_back: " << (time_push_back - time_inc) / count << " sec." << endl;

  timer->StartTimer();
  count = 0;
  for (int r = 0; r < repeat; r++)
    {
    edge_vec.clear();
    vert_vec.clear();
    for (tie(vi, viEnd) = vertices(g); vi != viEnd; ++vi)
      {
      typename graph_traits<Graph>::out_edge_iterator oi, oiEnd;
      for (tie(oi, oiEnd) = out_edges(*vi, g); oi != oiEnd; ++oi)
        {
        Edge e1 = *oi;
        edge_vec.push_back(e1);
        vert_vec.push_back(v);
        count++;
        }
      }
    }
  timer->StopTimer();
  double time_deref = timer->GetElapsedTime();
  cerr << "+dereference: " << time_deref / count << " sec." << endl;
  cerr << "  just dereference: " << (time_deref - time_push_back) / count << " sec." << endl;

  timer->StartTimer();
  count = 0;
  for (int r = 0; r < repeat; r++)
    {
    edge_vec.clear();
    vert_vec.clear();
    for (tie(vi, viEnd) = vertices(g); vi != viEnd; ++vi)
      {
      typename graph_traits<Graph>::out_edge_iterator oi, oiEnd;
      for (tie(oi, oiEnd) = out_edges(*vi, g); oi != oiEnd; ++oi)
        {
        Edge e1 = *oi;
        edge_vec.push_back(e1);
        Vertex v1 = target(e1, g);
        vert_vec.push_back(v1);
        count++;
        }
      }
    }
  timer->StopTimer();
  double time_target = timer->GetElapsedTime();
  cerr << "+target: " << time_target / count << " sec." << endl;
  cerr << "  just target: " << (time_target - time_deref) / count << " sec." << endl;
}

template <typename Graph>
void TestGraph(Graph g, vtkIdType numVertices, vtkIdType numEdges, int repeat, int& errors)
{
  typedef typename graph_traits<Graph>::edge_descriptor Edge;
  typedef typename graph_traits<Graph>::vertex_descriptor Vertex;

  VTK_CREATE(vtkTimerLog, timer);
  
  vector<Vertex> graphVerts;
  vector<Edge> graphEdges;
  typename graph_traits<Graph>::vertex_iterator vi, viEnd;

  // Create a graph
  timer->StartTimer();
  for (int i = 0; i < numVertices; ++i)
    {
    add_vertex(g);
    }
  timer->StopTimer();
  cerr << "vertex insertion: " << timer->GetElapsedTime() / numVertices  << " sec." << endl;
  
  if (static_cast<int>(num_vertices(g)) != numVertices)
    {
    cerr << "ERROR: Number of vertices (" << num_vertices(g) 
         << ") not as expected (" << numVertices << ")." << endl;
    errors++;
    }

  for (tie(vi, viEnd) = vertices(g); vi != viEnd; ++vi)
    {
    graphVerts.push_back(*vi);
    }

  timer->StartTimer();
  for (int i = 0; i < numEdges; ++i)
    {
    int u = static_cast<int>(vtkMath::Random(0, numVertices));
    int v = static_cast<int>(vtkMath::Random(0, numVertices));
    add_edge(graphVerts[u], graphVerts[v], g);
    }
  timer->StopTimer();
  cerr << "edge insertion: " << timer->GetElapsedTime() / numEdges  << " sec." << endl;

  if (static_cast<int>(num_edges(g)) != numEdges)
    {
    cerr << "ERROR: Number of edges (" << num_edges(g) 
         << ") not as expected (" << numEdges << ")." << endl;
    errors++;
    }

  typename graph_traits<Graph>::edge_iterator ei, eiEnd;
  for (tie(ei, eiEnd) = edges(g); ei != eiEnd; ++ei)
    {
    graphEdges.push_back(*ei);
    }
  
  TestTraversal(g, repeat, errors);
  
  timer->StartTimer();
  while (num_edges(g) > 0)
    {
    remove_edge(*(edges(g).first), g);
    }
  timer->StopTimer();
  cerr << "edge deletion: " << timer->GetElapsedTime() / numEdges  << " sec." << endl;

  // Perform edge deletions followed by accesses
  timer->StartTimer();
  while (num_vertices(g) > 0)
    {
    remove_vertex(*(vertices(g).first), g);
    }
  timer->StopTimer();
  cerr << "vertex deletion: " << timer->GetElapsedTime() / numVertices  << " sec." << endl;
}

int TestBoostAdapter(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int errors = 0;
  int repeat = 100;
  vtkIdType numVertices = 1000;
  vtkIdType numEdges = 2000;

  cerr << "Testing boost list graph..." << endl;
  typedef adjacency_list<listS, listS, directedS, property<vertex_index_t, unsigned int>, property<edge_index_t, unsigned int>, no_property, listS> ListGraph;
  ListGraph listGraph;
  TestGraph(listGraph, numVertices, numEdges, repeat, errors);
  cerr << "...done." << endl << endl;
  
  cerr << "Testing boost vector graph..." << endl;
  typedef adjacency_list<vecS, vecS, directedS, property<vertex_index_t, unsigned int>, property<edge_index_t, unsigned int>, no_property, vecS> VectorGraph;
  VectorGraph vectorGraph;
  TestGraph(vectorGraph, numVertices, numEdges, repeat, errors);
  cerr << "...done." << endl << endl;

  cerr << "Testing undirected graph adapter..." << endl;
  vtkGraph* ug = vtkGraph::New();
  ug->SetDirected(false);
  vtkBoostUndirectedGraph ugBoost(ug);
  TestGraph(ugBoost, numVertices, numEdges, repeat, errors);
  ug->Delete();
  cerr << "...done." << endl << endl;

  cerr << "Testing directed graph adapter..." << endl;
  vtkGraph* dg = vtkGraph::New();
  dg->SetDirected(true);
  vtkBoostDirectedGraph dgBoost(dg);
  TestGraph(dgBoost, numVertices, numEdges, repeat, errors);
  dg->Delete();
  cerr << "...done." << endl << endl;
  
  cerr << "Testing tree adapter..." << endl;
  vtkTree* t = vtkTree::New();
  t->AddRoot();
  for (vtkIdType i = 1; i < numVertices; i++)
    {
    t->AddChild(static_cast<vtkIdType>(vtkMath::Random(0, i)));
    }
  TestTraversal(t, repeat, errors);
  t->Delete();
  cerr << "...done." << endl << endl;
  
  return errors;
}
