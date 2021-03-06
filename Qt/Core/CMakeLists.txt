PROJECT(pqCore)
IF(COMMAND CMAKE_POLICY)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF(COMMAND CMAKE_POLICY)

INCLUDE_DIRECTORIES(
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${ParaView_SOURCE_DIR}/VTK/GUISupport/Qt
  ${QtWidgets_SOURCE_DIR}
  ${QtWidgets_BINARY_DIR}
  ${QtChart_SOURCE_DIR}
  ${QtChart_BINARY_DIR}
  ${QtTesting_SOURCE_DIR}
  ${QtTesting_BINARY_DIR}
  ${pqCore_BINARY_DIR}
  ${pqCore_SOURCE_DIR}
)

IF(QT_TESTING_WITH_PYTHON)
  SET(PYTHON_CORE_SRCS 
    pqPythonEventSourceImage.cxx
    pqPythonEventSourceImage.h
  )
  SET(PYTHON_MOC_SRCS 
    pqPythonEventSourceImage.h
  )
INCLUDE_DIRECTORIES(
  ${PYTHON_INCLUDE_PATH}
)
ENDIF(QT_TESTING_WITH_PYTHON)


# Reduce the number of dirs that get included on moc command line
# since it causes issues on Windows 2000.
GET_DIRECTORY_PROPERTY(include_dirs_tmp INCLUDE_DIRECTORIES)
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${MOC_INCLUDE_DIRS}")

QT4_WRAP_CPP(MOC_BUILT_SOURCES
  pq3DWidgetFactory.h
  pqAnimationCue.h
  pqAnimationScene.h
  pqApplicationCore.h
  pqBarChartRepresentation.h
  pqCommandServerStartup.h
  pqComparativeRenderView.h
  pqCoreTestUtility.h
  pqDataRepresentation.h
  pqDisplayPolicy.h
  pqFileDialog.h
  pqFileDialogEventTranslator.h
  pqFileDialogFavoriteModel.h
  pqFileDialogFilter.h
  pqFileDialogModel.h
  pqFlatTreeViewEventTranslator.h
  pqFormBuilder.h
  pqHistogramTableModel.h
  pqLineChartRepresentation.h
  pqLinksModel.h
  pqLinkViewWidget.h
  pqLookupTableManager.h
  pqManualServerStartup.h
  pqObjectBuilder.h
  pqOutputWindow.h
  pqOutputWindowAdapter.h
  pqOutputPort.h
  pqPendingDisplayManager.h
  pqPickHelper.h
  pqPipelineSource.h
  pqPipelineFilter.h
  pqPipelineRepresentation.h
  pqPlotView.h
  pqPlotViewHistogram.h
  pqPlotViewLineChart.h
  pqPluginManager.h
  pqProgressManager.h
  pqPropertyLinks.h
  pqPropertyManager.h
  pqProxy.h
  pqQVTKWidgetEventTranslator.h
  pqReaderFactory.h
  pqRenderView.h
  pqRepresentation.h
  pqRubberBandHelper.h
  pqScalarBarRepresentation.h
  pqScalarBarVisibilityAdaptor.h
  pqScalarsToColors.h
  pqServer.h
  pqServerManagerModel.h
  pqServerManagerModelItem.h
  pqServerManagerObserver.h
  pqServerManagerSelectionModel.h
  pqServerResources.h
  pqServerStartup.h
  pqServerStartups.h
  pqSettings.h
  pqSpreadSheetView.h
  pqSpreadSheetViewModel.h
  pqSpreadSheetViewSelectionModel.h
  pqStandardServerManagerModelInterface.h
  pqStandardViewModules.h
  pqTableView.h
  pqTextRepresentation.h
  pqTimeKeeper.h
  pqUndoStack.h
  pqView.h
  pqVTKHistogramModel.h
  pqWriterFactory.h
  ${PYTHON_MOC_SRCS}
  )
SET_DIRECTORY_PROPERTIES(PROPERTIES INCLUDE_DIRECTORIES "${include_dirs_tmp}")

SET(UI_FORMS
  Resources/UI/pqOutputWindow.ui
  Resources/UI/pqFileDialog.ui
)

QT4_WRAP_UI(UI_BUILT_SOURCES
  ${UI_FORMS}
)

SET(UI_RESOURCES
  Resources/pqCore.qrc
)

QT4_ADD_RESOURCES(RCS_SOURCES
  ${UI_RESOURCES}
)


SOURCE_GROUP("Resources" FILES
  ${UI_RESOURCES}
  ${UI_FORMS}
)

SOURCE_GROUP("Generated" FILES
  ${MOC_BUILT_SOURCES}
  ${RCS_SOURCES}
  ${UI_BUILT_SOURCES}
)

ADD_LIBRARY(pqCore
  pq3DWidgetFactory.cxx
  pq3DWidgetFactory.h
  pqAnimationCue.cxx
  pqAnimationCue.h
  pqAnimationScene.cxx
  pqAnimationScene.h
  pqAnimationSceneImageWriter.cxx
  pqAnimationSceneImageWriter.h
  pqApplicationCore.cxx
  pqApplicationCore.h
  pqBarChartRepresentation.cxx
  pqBarChartRepresentation.h
  pqCommandServerStartup.cxx
  pqCommandServerStartup.h
  pqComparativeRenderView.cxx
  pqComparativeRenderView.h
  pqCoreExport.h
  pqCoreInit.cxx
  pqCoreInit.h
  pqCoreTestUtility.cxx
  pqCoreTestUtility.h
  pqDataRepresentation.cxx
  pqDataRepresentation.h
  pqDisplayPolicy.cxx
  pqDisplayPolicy.h
  pqFileDialog.cxx
  pqFileDialog.h
  pqFileDialogEventPlayer.cxx
  pqFileDialogEventPlayer.h
  pqFileDialogEventTranslator.cxx
  pqFileDialogEventTranslator.h
  pqFileDialogFavoriteModel.cxx
  pqFileDialogFavoriteModel.h
  pqFileDialogFilter.cxx
  pqFileDialogFilter.h
  pqFileDialogModel.cxx
  pqFileDialogModel.h
  pqFlatTreeViewEventPlayer.cxx
  pqFlatTreeViewEventPlayer.h
  pqFlatTreeViewEventTranslator.cxx
  pqFlatTreeViewEventTranslator.h
  pqFormBuilder.cxx
  pqFormBuilder.h
  pqHelperProxyRegisterUndoElement.cxx
  pqHelperProxyRegisterUndoElement.h
  pqHistogramTableModel.cxx
  pqHistogramTableModel.h
  pqImageUtil.cxx
  pqImageUtil.h
  pqLineChartRepresentation.cxx
  pqLineChartRepresentation.h
  pqLinksModel.cxx
  pqLinksModel.h
  pqLinkViewWidget.cxx
  pqLinkViewWidget.h
  pqLookupTableManager.cxx
  pqLookupTableManager.h
  pqMain.cxx
  pqMain.h
  pqManualServerStartup.cxx
  pqManualServerStartup.h
  pqNameCount.cxx
  pqNameCount.h
  pqOptions.cxx
  pqOptions.h
  pqObjectBuilder.cxx
  pqObjectBuilder.h
  pqOutputWindow.cxx
  pqOutputWindow.h
  pqOutputWindowAdapter.cxx
  pqOutputWindowAdapter.h
  pqOutputPort.cxx
  pqOutputPort.h
  pqPendingDisplayManager.cxx
  pqPendingDisplayManager.h
  pqPendingDisplayUndoElement.cxx
  pqPendingDisplayUndoElement.h
  pqPickHelper.cxx
  pqPickHelper.h
  pqPipelineFilter.cxx
  pqPipelineFilter.h
  pqPipelineSource.cxx
  pqPipelineSource.h
  pqPipelineRepresentation.cxx
  pqPipelineRepresentation.h
  pqPlotView.cxx
  pqPlotView.h
  pqPlotViewHistogram.cxx
  pqPlotViewHistogram.h
  pqPlotViewLineChart.cxx
  pqPlotViewLineChart.h
  pqPluginManager.cxx
  pqPluginManager.h
  pqProcessModuleGUIHelper.cxx
  pqProcessModuleGUIHelper.h
  pqProgressManager.cxx
  pqProgressManager.h
  pqPropertyLinks.cxx
  pqPropertyLinks.h
  pqPropertyManager.cxx
  pqPropertyManager.h
  pqProxy.cxx
  pqProxy.h
  pqProxyUnRegisterUndoElement.cxx
  pqProxyUnRegisterUndoElement.h
  pqQVTKWidgetEventPlayer.cxx
  pqQVTKWidgetEventPlayer.h
  pqQVTKWidgetEventTranslator.cxx
  pqQVTKWidgetEventTranslator.h
  pqReaderFactory.cxx
  pqReaderFactory.h
  pqRenderView.cxx
  pqRenderView.h
  pqRepresentation.cxx
  pqRepresentation.h
  pqRubberBandHelper.cxx
  pqRubberBandHelper.h
  pqSMAdaptor.cxx
  pqSMAdaptor.h
  pqSMProxy.cxx
  pqSMProxy.h
  pqScalarBarVisibilityAdaptor.cxx
  pqScalarBarVisibilityAdaptor.h
  pqScalarsToColors.cxx
  pqScalarsToColors.h
  pqServer.cxx
  pqServer.h
  pqServerManagerModel.cxx
  pqServerManagerModel.h
  pqServerManagerModelItem.cxx
  pqServerManagerModelItem.h
  pqServerManagerObserver.cxx
  pqServerManagerObserver.h
  pqServerManagerSelectionModel.cxx
  pqServerManagerSelectionModel.h
  pqServerResource.cxx
  pqServerResource.h
  pqServerResources.cxx
  pqServerResources.h
  pqServerStartup.h
  pqServerStartups.cxx
  pqServerStartups.h
  pqSettings.cxx
  pqSettings.h
  pqScalarBarRepresentation.cxx
  pqScalarBarRepresentation.h
  pqSpreadSheetView.cxx
  pqSpreadSheetView.h
  pqSpreadSheetViewModel.cxx
  pqSpreadSheetViewModel.h
  pqSpreadSheetViewSelectionModel.cxx
  pqSpreadSheetViewSelectionModel.h
  pqStandardServerManagerModelInterface.cxx
  pqStandardServerManagerModelInterface.h
  pqStandardViewModules.cxx
  pqStandardViewModules.h
  pqTableView.cxx
  pqTableView.h
  pqTextRepresentation.cxx
  pqTextRepresentation.h
  pqTimeKeeper.cxx
  pqTimeKeeper.h
  pqUndoStack.cxx
  pqUndoStack.h
  pqView.cxx
  pqView.h
  pqVTKHistogramColor.cxx
  pqVTKHistogramColor.h
  pqVTKHistogramModel.cxx
  pqVTKHistogramModel.h
  pqVTKLineChartSeries.cxx
  pqVTKLineChartSeries.h
  pqWriterFactory.cxx
  pqWriterFactory.h
  pqXMLUtil.cxx
  pqXMLUtil.h

  ${MOC_BUILT_SOURCES}
  ${RCS_SOURCES}
  ${UI_BUILT_SOURCES}

  ${PYTHON_CORE_SRCS}

  vtkPVAxesWidget.cxx
  vtkPVAxesWidget.h
  vtkPVAxesActor.cxx
  vtkPVAxesActor.h
)

TARGET_LINK_LIBRARIES(pqCore
  QtChart
  QtTesting
  pqWidgets
  QVTK
  vtkPVServerManager
)

IF(VTK_USE_INFOVIS)
  TARGET_LINK_LIBRARIES(pqCore
    vtkInfovisCS
  )
ENDIF(VTK_USE_INFOVIS)

IF(QT_TESTING_WITH_PYTHON)
  TARGET_LINK_LIBRARIES(pqCore
    ${PYTHON_LIBRARIES}
  )
ENDIF(QT_TESTING_WITH_PYTHON)

CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/pqCoreExport.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/pqCoreExport.h")

INSTALL(TARGETS pqCore
  RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR_CM24} COMPONENT Runtime
  LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Runtime
  ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Development)

IF(BUILD_TESTING)
  ADD_SUBDIRECTORY(Testing)
ENDIF(BUILD_TESTING)

