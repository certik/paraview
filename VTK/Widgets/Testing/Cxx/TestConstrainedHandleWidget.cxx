/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: TestConstrainedHandleWidget.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the vtkHandleWidget with a 2D representation

// First include the required header files for the VTK classes we are using.
#include "vtkHandleWidget.h"
#include "vtkConstrainedPointHandleRepresentation.h"
#include "vtkCoordinate.h"
#include "vtkActor2D.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkRegressionTestImage.h"
#include "vtkDebugLeaks.h"
#include "vtkImageActor.h"
#include "vtkVolume16Reader.h"
#include "vtkImageShiftScale.h"
#include "vtkTestUtilities.h"
#include "vtkImageData.h"
#include "vtkProperty2D.h"
#include "vtkCamera.h"
#include "vtkPlane.h"

int TestConstrainedHandleWidget( int argc, char *argv[] )
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");
   
  vtkVolume16Reader* v16 = vtkVolume16Reader::New();
  v16->SetDataDimensions(64, 64);
  v16->SetDataByteOrderToLittleEndian();
  v16->SetImageRange(1, 93);
  v16->SetDataSpacing(3.2, 3.2, 1.5);
  v16->SetFilePrefix(fname);
  v16->ReleaseDataFlagOn();
  v16->SetDataMask(0x7fff);
  v16->Update();
  delete[] fname;
    
  double range[2];
  v16->GetOutput()->GetScalarRange(range);

  vtkImageShiftScale* shifter = vtkImageShiftScale::New();
  shifter->SetShift(-1.0*range[0]);
  shifter->SetScale(255.0/(range[1]-range[0]));
  shifter->SetOutputScalarTypeToUnsignedChar();
  shifter->SetInputConnection(v16->GetOutputPort());
  shifter->ReleaseDataFlagOff();
  shifter->Update();
  
  vtkImageActor* imageActor = vtkImageActor::New();
  imageActor->SetInput(shifter->GetOutput());
  imageActor->VisibilityOn();
//  imageActor->SetDisplayExtent(0, 63, 0, 63, 46, 46);
  imageActor->SetDisplayExtent(0, 63, 30, 30, 0, 92);
  imageActor->InterpolateOn();
    
  // Create the RenderWindow, Renderer and both Actors
  //
  vtkRenderer *ren1 = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->AddRenderer(ren1);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  vtkConstrainedPointHandleRepresentation *handleRep = vtkConstrainedPointHandleRepresentation::New();
  handleRep->ActiveRepresentationOn();

  vtkHandleWidget *handleWidget = vtkHandleWidget::New();
  handleWidget->SetInteractor(iren);
  handleWidget->SetRepresentation(handleRep);

  ren1->AddActor(imageActor);

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  handleRep->SetPosition(imageActor->GetCenter());
  handleRep->SetProjectionNormalToYAxis();
  handleRep->SetProjectionPosition(imageActor->GetCenter()[1]);

  double bounds[6];
  imageActor->GetBounds( bounds );
  
  vtkPlane *p1 = vtkPlane::New();
  p1->SetOrigin( bounds[0], bounds[2], bounds[4] );
  p1->SetNormal( 1.0, 0.0, 0.0 );

  vtkPlane *p2 = vtkPlane::New();
  p2->SetOrigin( bounds[0], bounds[2], bounds[4] );
  p2->SetNormal( 0.0, 0.0, 1.0 );

  vtkPlane *p3 = vtkPlane::New();
  p3->SetOrigin( bounds[1], bounds[3], bounds[5] );
  p3->SetNormal( -1.0, 0.0, 0.0 );

  vtkPlane *p4 = vtkPlane::New();
  p4->SetOrigin( bounds[1], bounds[3], bounds[5] );
  p4->SetNormal( 0.0, 0.0, -1.0 );

  handleRep->AddBoundingPlane(p1);
  handleRep->AddBoundingPlane(p2);
  handleRep->AddBoundingPlane(p3);
  handleRep->AddBoundingPlane(p4);
  
    
  // render the image
  //
  ren1->GetActiveCamera()->SetPosition(0,0,0);
  ren1->GetActiveCamera()->SetFocalPoint(0,1,0);  
  ren1->GetActiveCamera()->SetViewUp(0,0,1);
  ren1->ResetCamera();
  iren->Initialize();
  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  v16->Delete();
  shifter->Delete();
  imageActor->Delete();
  handleRep->Delete();
  handleWidget->Off();
  handleWidget->Delete();
  iren->Delete();
  renWin->Delete();
  ren1->Delete();
  p1->Delete();
  p2->Delete();
  p3->Delete();
  p4->Delete();
  
  return !retVal;

}


