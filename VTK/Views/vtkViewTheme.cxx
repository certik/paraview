/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkViewTheme.cxx,v $

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

#include "vtkViewTheme.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkViewTheme, "$Revision: 1.3 $");
vtkStandardNewMacro(vtkViewTheme);

vtkViewTheme::vtkViewTheme()
{
  this->PointColor[0] = this->PointColor[1] = this->PointColor[2] = 1;
  this->PointOpacity = 1;
  this->PointHueRange[0] = 0.667;
  this->PointHueRange[1] = 0;
  this->PointSaturationRange[0] = this->PointSaturationRange[1] = 1;
  this->PointValueRange[0] = this->PointValueRange[1] = 1;
  this->PointAlphaRange[0] = this->PointAlphaRange[1] = 1;

  this->CellColor[0] = this->CellColor[1] = this->CellColor[2] = 1;
  this->CellOpacity = 0.5;
  this->CellHueRange[0] = 0.667;
  this->CellHueRange[1] = 0;
  this->CellSaturationRange[0] = 0.5;
  this->CellSaturationRange[1] = 1;
  this->CellValueRange[0] = 0.5;
  this->CellValueRange[1] = 1;
  this->CellAlphaRange[0] = 0.5;
  this->CellAlphaRange[1] = 1;

  this->OutlineColor[0] = this->OutlineColor[1] = this->OutlineColor[2] = 0;

  this->SelectedPointColor[0] = this->SelectedPointColor[2] = 1;
  this->SelectedPointColor[1] = 0;
  this->SelectedPointOpacity = 1;
  this->SelectedCellColor[0] = this->SelectedCellColor[2] = 1;
  this->SelectedCellColor[1] = 0;
  this->SelectedCellOpacity = 1;

  this->BackgroundColor[0] = this->BackgroundColor[1] =
    this->BackgroundColor[2] = 0.3;
  this->VertexLabelColor[0] = this->VertexLabelColor[1] =
    this->VertexLabelColor[2] = 1;
  this->EdgeLabelColor[0] = this->EdgeLabelColor[1] =
    this->EdgeLabelColor[2] = 0.7;
}

vtkViewTheme::~vtkViewTheme()
{
}

vtkViewTheme* vtkViewTheme::CreateOceanTheme()
{
  vtkViewTheme* theme = vtkViewTheme::New();

  theme->SetBackgroundColor(1, 1, 1);
  theme->SetVertexLabelColor(0, 0, 0);
  theme->SetEdgeLabelColor(.2, .2, .2);

  theme->SetPointColor(0.5, 0.5, 0.5);
  theme->SetPointHueRange(0.667, 0);
  theme->SetPointSaturationRange(0.75, 0.75);
  theme->SetPointValueRange(0.75, 0.75);

  theme->SetCellColor(0.25, 0.25, 0.25);
  theme->SetCellOpacity(0.3);
  theme->SetCellHueRange(0.667, 0);
  theme->SetCellAlphaRange(0.3, 1);
  theme->SetCellValueRange(0.5, 1);
  theme->SetCellSaturationRange(0.5, 1);
  
  theme->SetOutlineColor(0,0,0);

  theme->SetSelectedPointColor(.8, .3, .8);
  theme->SetSelectedCellColor(.8, .3, .8);

  return theme;
}

vtkViewTheme* vtkViewTheme::CreateNeonTheme()
{
  vtkViewTheme* theme = vtkViewTheme::New();

  theme->SetBackgroundColor(0, 0, 0);
  theme->SetVertexLabelColor(1, 1, 1);
  theme->SetEdgeLabelColor(.7, .7, .7);

  theme->SetPointColor(0.5, 0.5, 0.5);
  theme->SetPointHueRange(0.667, 0);
  theme->SetPointSaturationRange(1, 1);
  theme->SetPointValueRange(1, 1);

  theme->SetCellColor(0.25, 0.25, 0.25);
  theme->SetCellOpacity(0.75);
  theme->SetCellHueRange(0.667, 0);
  theme->SetCellAlphaRange(0.75, 1);
  theme->SetCellValueRange(0.75, 1);
  theme->SetCellSaturationRange(1, 1);
  
  theme->SetOutlineColor(.5,.5,.5);

  theme->SetSelectedPointColor(1, 1, 1.0);
  theme->SetSelectedCellColor(1, 1, 1.0);

  return theme;
}

vtkViewTheme* vtkViewTheme::CreateMellowTheme()
{
  vtkViewTheme* theme = vtkViewTheme::New();

  theme->SetBackgroundColor(0.6, 0.6, 0.5); // Tan
  theme->SetVertexLabelColor(1, 1, 1);
  theme->SetEdgeLabelColor(.7, .7, 1);

  theme->SetPointColor(0.0, 0.0, 1.0);
  theme->SetPointHueRange(0.667, 0);

  theme->SetCellColor(0.25, 0.25, 0.25);
  theme->SetCellOpacity(0.4);
  theme->SetCellHueRange(0.667, 0);
  theme->SetCellAlphaRange(0.4, 1);
  theme->SetCellValueRange(0.5, 1);
  theme->SetCellSaturationRange(0.5, 1);
  
  theme->SetOutlineColor(0, 0, 0);

  theme->SetSelectedPointColor(1, 1, 1);
  theme->SetSelectedCellColor(0, 0, 0);

  return theme;
}

void vtkViewTheme::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PointColor: " 
     << this->PointColor[0] << "," 
     << this->PointColor[1] << "," 
     << this->PointColor[2] << endl;
  os << indent << "PointOpacity: " << this->PointOpacity << endl;
  os << indent << "PointHueRange: "
     << this->PointHueRange[0] << ","
     << this->PointHueRange[1] << endl;
  os << indent << "PointSaturationRange: "
     << this->PointSaturationRange[0] << ","
     << this->PointSaturationRange[1] << endl;
  os << indent << "PointValueRange: "
     << this->PointValueRange[0] << ","
     << this->PointValueRange[1] << endl;
  os << indent << "PointAlphaRange: "
     << this->PointAlphaRange[0] << ","
     << this->PointAlphaRange[1] << endl;
  os << indent << "CellColor: " 
     << this->CellColor[0] << "," 
     << this->CellColor[1] << "," 
     << this->CellColor[2] << endl;
  os << indent << "CellOpacity: " << this->CellOpacity << endl;
  os << indent << "CellHueRange: "
     << this->CellHueRange[0] << ","
     << this->CellHueRange[1] << endl;
  os << indent << "CellSaturationRange: "
     << this->CellSaturationRange[0] << ","
     << this->CellSaturationRange[1] << endl;
  os << indent << "CellValueRange: "
     << this->CellValueRange[0] << ","
     << this->CellValueRange[1] << endl;
  os << indent << "CellAlphaRange: "
     << this->CellAlphaRange[0] << ","
     << this->CellAlphaRange[1] << endl;
  os << indent << "OutlineColor: " 
     << this->OutlineColor[0] << "," 
     << this->OutlineColor[1] << "," 
     << this->OutlineColor[2] << endl;
  os << indent << "SelectedPointColor: " 
     << this->SelectedPointColor[0] << "," 
     << this->SelectedPointColor[1] << "," 
     << this->SelectedPointColor[2] << endl;
  os << indent << "SelectedPointOpacity: " << this->SelectedPointOpacity << endl;
  os << indent << "SelectedCellColor: " 
     << this->SelectedCellColor[0] << "," 
     << this->SelectedCellColor[1] << "," 
     << this->SelectedCellColor[2] << endl;
  os << indent << "SelectedCellOpacity: " << this->SelectedCellOpacity << endl;
  os << indent << "BackgroundColor: " 
     << this->BackgroundColor[0] << "," 
     << this->BackgroundColor[1] << "," 
     << this->BackgroundColor[2] << endl;
  os << indent << "VertexLabelColor: " 
     << this->VertexLabelColor[0] << "," 
     << this->VertexLabelColor[1] << "," 
     << this->VertexLabelColor[2] << endl;
  os << indent << "EdgeLabelColor: " 
     << this->EdgeLabelColor[0] << "," 
     << this->EdgeLabelColor[1] << "," 
     << this->EdgeLabelColor[2] << endl;
}
