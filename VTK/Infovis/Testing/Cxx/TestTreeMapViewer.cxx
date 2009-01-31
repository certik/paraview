/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: TestTreeMapViewer.cxx,v $

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
#include "vtkTestUtilities.h"
#include "vtkTreeMapViewer.h"
#include "vtkXMLTreeReader.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

int TestTreeMapViewer(int argc, char* argv[])
{
  int imode = 0; // Interactive mode
  char *file = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                    "Data/treetest.xml");
  cout << "Filename: " << file << endl;
  const char* labelField = "name";
  const char* sizeField = "size";
  for(int i = 1; i < argc; i++)
    {
    if (!strcmp(argv[i], "-I")) 
      {
      imode = 1;
      continue;
      }

    if (!strcmp(argv[i], "-f")) 
      {
      i++;
      file = argv[i];
      continue;
      }

    if (!strcmp(argv[i], "-l")) 
      {
      i++;
      labelField = argv[i];
      continue;
      }

    if (!strcmp(argv[i], "-s")) 
      {
      i++;
      sizeField = argv[i];
      continue;
      }

    if (!strcmp(argv[i], "-D") ||
        !strcmp(argv[i], "-T") ||
        !strcmp(argv[i], "-V"))
      {
      i++;
      continue;
      }

    cerr << argv[0] << " Options:\n  " 
      << " -h (prints this message)\n  "
      << " -I (run interactively)\n  "
      << " -f filename (default is VTKData\\Data\\treetest.xml)\n  "
      << " -l label field\n  "
      << " -s size field\n  ";
    return 0;
    }

  vtkXMLTreeReader* reader = vtkXMLTreeReader::New();

  reader->SetFileName(file);
  reader->Update();

  delete [] file;

  vtkTreeMapViewer* viewer = vtkTreeMapViewer::New();

  vtkRenderWindow* win = vtkRenderWindow::New();
  vtkRenderWindowInteractor* iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(win);
  viewer->SetRenderWindow(win);

  viewer->SetInput(reader->GetOutput());
  if (strcmp(sizeField, "0"))
    {
    viewer->SetAggregationFieldName(sizeField);
    }
  if (strcmp(labelField, "0"))
    {
    viewer->SetLabelFieldName(labelField);
    }
  viewer->SetLayoutStrategy(vtkTreeMapViewer::SQUARIFY_LAYOUT);

  int retVal = vtkRegressionTestImage(win);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    win->Render();
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
    }

  win->Delete();
  iren->Delete();
  reader->Delete();
  viewer->Delete();

  return !retVal;
}
