package require vtk
package require vtkinteraction
package require vtktesting

#define a Single Cube
vtkFloatArray Scalars
    Scalars InsertNextValue 1.0
    Scalars InsertNextValue 1.0
    Scalars InsertNextValue 0.0
    Scalars InsertNextValue 0.0
    Scalars InsertNextValue 0.0
    Scalars InsertNextValue 0.0
    Scalars InsertNextValue 0.0
    Scalars InsertNextValue 0.0

vtkPoints Points
    Points InsertNextPoint 0 0 0
    Points InsertNextPoint 1 0 0
    Points InsertNextPoint 1 1 0
    Points InsertNextPoint 0 1 0
    Points InsertNextPoint 0 0 1
    Points InsertNextPoint 1 0 1
    Points InsertNextPoint 1 1 1
    Points InsertNextPoint 0 1 1

vtkIdList Ids
    Ids InsertNextId 0
    Ids InsertNextId 1
    Ids InsertNextId 2
    Ids InsertNextId 3
    Ids InsertNextId 4
    Ids InsertNextId 5
    Ids InsertNextId 6
    Ids InsertNextId 7

vtkUnstructuredGrid Grid
    Grid Allocate 10 10
    Grid InsertNextCell 12 Ids
    Grid SetPoints Points
    [Grid GetPointData] SetScalars Scalars

#Clip the hex
vtkClipDataSet clipper
    clipper SetInput Grid
    clipper SetValue 0.5

# build tubes for the triangle edges
#
vtkExtractEdges tetEdges
    tetEdges SetInputConnection [clipper GetOutputPort]
vtkTubeFilter tetEdgeTubes
    tetEdgeTubes SetInputConnection [tetEdges GetOutputPort]
    tetEdgeTubes SetRadius .005
    tetEdgeTubes SetNumberOfSides 6
    tetEdgeTubes UseDefaultNormalOn
    tetEdgeTubes SetDefaultNormal .577 .577 .577
vtkPolyDataMapper tetEdgeMapper
    tetEdgeMapper SetInputConnection [tetEdgeTubes GetOutputPort]
    tetEdgeMapper ScalarVisibilityOff
vtkActor tetEdgeActor
    tetEdgeActor SetMapper tetEdgeMapper
    eval [tetEdgeActor GetProperty] SetDiffuseColor $lamp_black
    [tetEdgeActor GetProperty] SetSpecular .4
    [tetEdgeActor GetProperty] SetSpecularPower 10

#shrink the triangles so we can see each one
vtkShrinkFilter aShrinker
    aShrinker SetShrinkFactor 1
    aShrinker SetInputConnection [clipper GetOutputPort]
vtkDataSetMapper aMapper
    aMapper ScalarVisibilityOff
    aMapper SetInputConnection [aShrinker GetOutputPort]
vtkActor Tets
    Tets SetMapper aMapper
    eval [Tets GetProperty] SetDiffuseColor $banana

#build a model of the cube
vtkCubeSource CubeModel
    CubeModel SetCenter .5 .5 .5
vtkExtractEdges Edges
    Edges SetInputConnection [CubeModel GetOutputPort]
vtkTubeFilter Tubes
    Tubes SetInputConnection [Edges GetOutputPort]
    Tubes SetRadius .01
    Tubes SetNumberOfSides 6
    Tubes UseDefaultNormalOn
    Tubes SetDefaultNormal .577 .577 .577
vtkPolyDataMapper TubeMapper
    TubeMapper SetInputConnection [Tubes GetOutputPort]
vtkActor CubeEdges
    CubeEdges SetMapper TubeMapper
    eval [CubeEdges GetProperty] SetDiffuseColor $khaki
    [CubeEdges GetProperty] SetSpecular .4
    [CubeEdges GetProperty] SetSpecularPower 10

# build the vertices of the cube
#
vtkSphereSource Sphere
    Sphere SetRadius 0.04
    Sphere SetPhiResolution 20
    Sphere SetThetaResolution 20
vtkThresholdPoints ThresholdIn
    ThresholdIn SetInput Grid
    ThresholdIn ThresholdByUpper .5
vtkGlyph3D Vertices
    Vertices SetInputConnection [ThresholdIn GetOutputPort]
    Vertices SetSource [Sphere GetOutput]
vtkPolyDataMapper SphereMapper
    SphereMapper SetInputConnection [Vertices GetOutputPort]
    SphereMapper ScalarVisibilityOff
vtkActor CubeVertices
    CubeVertices SetMapper SphereMapper
    eval [CubeVertices GetProperty] SetDiffuseColor $tomato
    eval [CubeVertices GetProperty] SetDiffuseColor $tomato

#define the text for the labels
vtkVectorText caseLabel
    caseLabel SetText "Case 2"

vtkTransform aLabelTransform
    aLabelTransform Identity
    aLabelTransform Translate  -.2 0 1.25
    aLabelTransform Scale .05 .05 .05

vtkTransformPolyDataFilter labelTransform
    labelTransform SetTransform aLabelTransform
    labelTransform SetInputConnection [caseLabel GetOutputPort]
  
vtkPolyDataMapper labelMapper
    labelMapper SetInputConnection [labelTransform GetOutputPort];
 
vtkActor labelActor
    labelActor SetMapper labelMapper
 
#define the base 
vtkCubeSource baseModel
    baseModel SetXLength 1.5
    baseModel SetYLength .01
    baseModel SetZLength 1.5
vtkPolyDataMapper baseMapper
    baseMapper SetInputConnection [baseModel GetOutputPort]
vtkActor base
    base SetMapper baseMapper

# Create the RenderWindow, Renderer and both Actors
#
vtkRenderer ren1
vtkRenderWindow renWin
    renWin AddRenderer ren1
vtkRenderWindowInteractor iren
    iren SetRenderWindow renWin

# position the base
base SetPosition .5 -.09 .5

ren1 AddActor tetEdgeActor
ren1 AddActor base
ren1 AddActor labelActor
ren1 AddActor CubeEdges
ren1 AddActor CubeVertices
ren1 AddActor Tets
eval ren1 SetBackground $slate_grey
iren AddObserver UserEvent {wm deiconify .vtkInteract}

case2 Scalars 1 0
renWin SetSize 400 400

ren1 ResetCamera
[ren1 GetActiveCamera] Dolly 1.2
[ren1 GetActiveCamera] Azimuth 30
[ren1 GetActiveCamera] Elevation 20
ren1 ResetCameraClippingRange

renWin Render
iren Initialize

wm withdraw .


