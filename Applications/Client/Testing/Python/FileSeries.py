#/usr/bin/env python

import QtTesting
import QtTestingImage

object1 = 'MainWindow/menubar/menuFile'
QtTesting.playCommand(object1, 'activate', 'actionFileOpen')
object2 = 'MainWindow/FileOpenDialog'
QtTesting.playCommand(object2, 'filesSelected', '$PARAVIEW_DATA_ROOT/Data/FileSeries/blow..vtk')
object3 = 'MainWindow/objectInspectorDock/1pqProxyTabWidget0/qt_tabwidget_stackedwidget/objectInspector/Accept'
QtTesting.playCommand(object3, 'activate', '')
object4 = 'MainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/0/Viewport'
QtTesting.playCommand(object4, 'mousePress', '(0.539658,0.641618,1,1,0)')
QtTesting.playCommand(object4, 'mouseMove', '(0.225505,0.603083,1,0,0)')
QtTesting.playCommand(object4, 'mouseRelease', '(0.225505,0.603083,1,0,0)')
object5 = 'MainWindow/axesToolbar/1QToolButton0'
QtTesting.playCommand(object5, 'set_boolean', 'false')
object6 = 'MainWindow/variableToolbar/displayColor/Variables'
QtTesting.playCommand(object6, 'set_string', 'cellNormals')
QtTesting.playCommand(object6, 'set_string', 'thickness')
object7 = 'MainWindow/commonFilters/1QToolButton8'
QtTesting.playCommand(object7, 'activate', '')
QtTesting.playCommand(object3, 'activate', '')
#object8 = 'MainWindow/menubar/menuTools'
#QtTesting.playCommand(object8, 'activate', 'actionToolsRecordTestScreenshot')
#object9 = 'MainWindow/RecordTestScreenshotDialog'
#QtTesting.playCommand(object9, 'filesSelected', '$PARAVIEW_DATA_ROOT/Baseline/FileSeries1.png')
snapshotWidget = 'MainWindow/MultiViewManager/SplitterFrame/MultiViewSplitter/0/Viewport'
QtTestingImage.compareImage(snapshotWidget, 'FileSeries1.png', 300, 300);
object10 = 'MainWindow/currentTimeToolbar/qt_toolbar_ext_button'
QtTesting.playCommand(object10, 'activate', '')
QtTesting.playCommand(object10, 'activate', '')
QtTesting.playCommand(object10, 'activate', '')
QtTesting.playCommand(object10, 'activate', '')
object11 = 'MainWindow/currentTimeToolbar/1QToolBarHandle0'
QtTesting.playCommand(object11, 'mousePress', '1,1,0,10,18')
QtTesting.playCommand(object11, 'mouseRelease', '1,0,0,10,18')
QtTesting.playCommand(object11, 'mousePress', '1,1,0,8,17')
QtTesting.playCommand(object11, 'mouseMove', '1,0,0,8,16')
QtTesting.playCommand(object11, 'mouseRelease', '1,0,0,8,16')
object12 = 'MainWindow/currentTimeToolbar/CurrentTimeIndex'
QtTesting.playCommand(object12, 'key', '16777219')
QtTesting.playCommand(object12, 'key', '16777219')
QtTesting.playCommand(object12, 'set_int', '3')
QtTesting.playCommand(object12, 'key', '16777220')
#QtTesting.playCommand(object8, 'activate', 'actionToolsRecordTestScreenshot')
#QtTesting.playCommand(object9, 'filesSelected', '$PARAVIEW_DATA_ROOT/Baseline/FileSeries2.png')
QtTestingImage.compareImage(snapshotWidget, 'FileSeries2.png', 300, 300);
