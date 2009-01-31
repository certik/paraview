#-----------------------------------------------------------------------------
# Does Xdmf require support for 64 bit file systems
INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/CheckCXXSourceRuns.cmake)
FILE(READ "${CMAKE_CURRENT_SOURCE_DIR}/vtkRequireLargeFilesSupport.cxx"
  XDMF_REQUIRE_LARGE_FILE_SUPPORT_FILE)
CHECK_CXX_SOURCE_RUNS("${XDMF_REQUIRE_LARGE_FILE_SUPPORT_FILE}"
  CMAKE_REQUIRE_LARGE_FILE_SUPPORT "Xdmf support for 64 bit file systems")
SET(XDMF_REQUIRE_LARGE_FILE_SUPPORT ${CMAKE_REQUIRE_LARGE_FILE_SUPPORT})

SET( XdmfFormatSource
#  XdmfCharArray
  XdmfArray
  XdmfDOM
  XdmfDataDesc
  XdmfExpression
#  XdmfFormat
#  XdmfFormatMulti
#  XdmfFormatHDF
#  XdmfFormatXML
  XdmfValues
  XdmfValuesXML
  XdmfValuesHDF
  XdmfHeavyData
  XdmfHDF
  XdmfHDFSupport
  XdmfLightData
  XdmfInformation
  XdmfDataItem
  XdmfDataStructure
  XdmfDataTransform
  XdmfElement
  XdmfObject
#  XdmfParameter
#  XdmfXNode
#  XdmfTransform
#  XdmfXMLParser
  )

SET( XdmfExtraSource
#  XdmfTree
#  XdmfLlist
  XdmfExprLex
  XdmfExprYacc
  )

SET( XdmfModelSource
  XdmfRoot
  XdmfDomain
  XdmfAttribute
  XdmfGrid
  XdmfTopology
  XdmfGeometry
#  XdmfNDGM
#  XdmfRuntime
  )

IF(XDMF_BUILD_MPI)
    SET(XdmfModelSource ${XdmfModelSource} XdmfH5Driver)
ENDIF(XDMF_BUILD_MPI) 

SET( XdmfMpiSource
    XdmfDsm
    XdmfDsmBuffer
    XdmfDsmComm
    XdmfDsmMsg
    XdmfDsmCommMpi
    )

SET( XdmfNdgmSource
#  H5FDndgm
  )

SET( XdmfPythonSource
  XdmfPython
  )
SET( XdmfTclSource
#  XdmfTcl
  )

OPTION(XDMF_REGENERATE_WRAPPERS
  "Regenerate XdmfPython.cxx XdmfTcl.cxx and XdmfJava.cxx" OFF)
MARK_AS_ADVANCED(XDMF_REGENERATE_WRAPPERS)
IF(XDMF_REGENERATE_WRAPPERS)
  FIND_PROGRAM(SWIG_EXECUTABLE 
    NAMES swig-1.3 swig
    PATHS /usr/bin
    DOC "Swig executable for generating wrapped files")
  MARK_AS_ADVANCED(SWIG_EXECUTABLE)
  IF(SWIG_EXECUTABLE)
    SET(XDMF_SWIG_INCLUDES 
      -I${Xdmf_SOURCE_DIR} 
      -I${Xdmf_SOURCE_DIR}/libsrc 
      -I${Xdmf_BINARY_DIR}/libsrc 
      -I${Xdmf_SOURCE_DIR}/Ice/libsrc
      -I${Xdmf_BINARY_DIR}/Ice/libsrc
      )
    GET_FILENAME_COMPONENT(SWIG_EXE_PATH ${SWIG_EXECUTABLE}
      PATH)
    SET(SWIG_LIB_PATH "${SWIG_EXE_PATH}/Lib")
    MESSAGE("SWIG_LIB_PATH ${SWIG_LIB_PATH}")
    IF(EXISTS "${SWIG_LIB_PATH}")
      SET(XDMF_SWIG_INCLUDES
        ${XDMF_SWIG_INCLUDES}
        -I${SWIG_LIB_PATH}
        -I${SWIG_LIB_PATH}/tcl
        -I${SWIG_LIB_PATH}/python
        )
    ENDIF(EXISTS "${SWIG_LIB_PATH}")
    FOREACH(LANGUAGE Tcl Python Java)
      STRING(TOLOWER ${LANGUAGE} lang)
      SET(swigtarget ${Xdmf_BINARY_DIR}/libsrc/Xdmf${LANGUAGE}.cxx)
      IF(LANGUAGE MATCHES "Tcl")
        SET(swig_extra_args -prefix Xdmf -namespace)
      ENDIF(LANGUAGE MATCHES "Tcl")
      ADD_CUSTOM_COMMAND(
        OUTPUT ${swigtarget}
        DEPENDS ${Xdmf_SOURCE_DIR}/libsrc/Xdmf.i
        COMMAND ${SWIG_EXECUTABLE}
        ARGS -v -c++ -make_default -includeall "-${lang}" ${swig_extra_args} ${XDMF_SWIG_INCLUDES} -o "${swigtarget}" Xdmf.i)

    ENDFOREACH(LANGUAGE)
    ADD_CUSTOM_TARGET(RerunSwig DEPENDS 
      ${Xdmf_BINARY_DIR}/libsrc/XdmfTcl.cxx
      ${Xdmf_BINARY_DIR}/libsrc/XdmfPython.cxx
      ${Xdmf_BINARY_DIR}/libsrc/XdmfJava.cxx
      )
  ENDIF(SWIG_EXECUTABLE)
ENDIF(XDMF_REGENERATE_WRAPPERS)

OPTION(XDMF_REGENERATE_YACCLEX
  "Regenerate XdmfExprYacc.cxx and XdmfExprLex.cxx" OFF)
MARK_AS_ADVANCED(XDMF_REGENERATE_YACCLEX)
IF(XDMF_REGENERATE_YACCLEX)
  FIND_PROGRAM(YACC_EXECUTABLE
    NAMES yacc bison
    PATHS /usr/bin
    DOC "Yacc or Bison executable")
  FIND_PROGRAM(FLEX_EXECUTABLE
    NAMES flex
    PATHS /usr/bin
    DOC "Flex executable")
  MARK_AS_ADVANCED(YACC_EXECUTABLE FLEX_EXECUTABLE)
  IF(YACC_EXECUTABLE)
    SET(BISON_FLAGS)
    IF(YACC_EXECUTABLE MATCHES "bison")
      SET(BISON_FLAGS "-y")
    ENDIF(YACC_EXECUTABLE MATCHES "bison")
    ADD_CUSTOM_COMMAND(
      OUTPUT ${Xdmf_BINARY_DIR}/libsrc/XdmfExprYacc.tab.c
      DEPENDS ${Xdmf_SOURCE_DIR}/libsrc/XdmfExpr.y
      COMMAND ${YACC_EXECUTABLE}
      ARGS ${BISON_FLAGS} -d -b ${Xdmf_BINARY_DIR}/libsrc/XdmfExprYacc
      -p dice_yy
      ${Xdmf_SOURCE_DIR}/libsrc/XdmfExpr.y
      )
    ADD_CUSTOM_COMMAND(
      OUTPUT ${Xdmf_BINARY_DIR}/libsrc/XdmfExprYacc.tab.h
      DEPENDS ${Xdmf_SOURCE_DIR}/libsrc/XdmfExpr.y
      COMMAND ${YACC_EXECUTABLE}
      ARGS ${BISON_FLAGS} -d -b ${Xdmf_BINARY_DIR}/libsrc/XdmfExprYacc
      -p dice_yy
      ${Xdmf_SOURCE_DIR}/libsrc/XdmfExpr.y
      )
    ADD_CUSTOM_COMMAND(
      OUTPUT ${Xdmf_BINARY_DIR}/libsrc/XdmfExprYacc.cxx
      DEPENDS ${Xdmf_BINARY_DIR}/libsrc/XdmfExprYacc.tab.c
      COMMAND ${CMAKE_COMMAND}
      ARGS -E copy ${Xdmf_BINARY_DIR}/libsrc/XdmfExprYacc.tab.c
      ${Xdmf_BINARY_DIR}/libsrc/XdmfExprYacc.cxx
      )
    ADD_CUSTOM_TARGET(RerunYacc DEPENDS
      ${Xdmf_BINARY_DIR}/libsrc/XdmfExprYacc.cxx)
  ENDIF(YACC_EXECUTABLE)
  IF(FLEX_EXECUTABLE)
    ADD_CUSTOM_COMMAND(
      OUTPUT ${Xdmf_BINARY_DIR}/libsrc/XdmfExprLex.cxx
      DEPENDS ${Xdmf_SOURCE_DIR}/libsrc/XdmfExpr.l
      COMMAND ${FLEX_EXECUTABLE}
      ARGS -Pdice_yy -o${Xdmf_BINARY_DIR}/libsrc/XdmfExprLex.cxx 
      ${Xdmf_SOURCE_DIR}/libsrc/XdmfExpr.l)
    ADD_CUSTOM_TARGET(RerunLex DEPENDS
      ${Xdmf_BINARY_DIR}/libsrc/XdmfExprLex.cxx)
  ENDIF(FLEX_EXECUTABLE)

ENDIF(XDMF_REGENERATE_YACCLEX)

IF (VTK_USE_MPI)
  # Needed for mpich 2
  ADD_DEFINITIONS("-DMPICH_IGNORE_CXX_SEEK")
ENDIF (VTK_USE_MPI)

INCLUDE (${CMAKE_ROOT}/Modules/CheckTypeSize.cmake)
IF(WIN32)
  CHECK_TYPE_SIZE(__int64        SIZEOF___INT64)
ENDIF(WIN32)
CHECK_TYPE_SIZE("long long"    SIZEOF_LONG_LONG)

INCLUDE (${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
CHECK_INCLUDE_FILE(malloc.h XDMF_HAVE_MALLOC_H)

## From Old IceConfig.h.in CMakeLists.txt

# Platform configuration tests.
# INCLUDE(${CMAKE_ROOT}/Modules/CMakeBackwardCompatibilityC.cmake)
# INCLUDE(${CMAKE_ROOT}/Modules/CMakeBackwardCompatibilityCXX.cmake)

# System to Build
STRING(REGEX REPLACE "-" "_" XDMF_SYSTEM ${CMAKE_SYSTEM_NAME})

SET(ARCH_TO_BUILD ${XDMF_SYSTEM} CACHE INTERNAL "Host Arcitecture : Linux IRIXN32 IRIX64 AIX CYGWIN")
# Allow the user to customize their build with some local options
#
ADD_DEFINITIONS(-D${ARCH_TO_BUILD})

MESSAGE(STATUS "Finding Atomic Data Sizes")
SET(XDMF_SIZEOF_CHAR   ${CMAKE_SIZEOF_CHAR})
SET(XDMF_SIZEOF_DOUBLE ${CMAKE_SIZEOF_DOUBLE})
SET(XDMF_SIZEOF_FLOAT  ${CMAKE_SIZEOF_FLOAT})
SET(XDMF_SIZEOF_INT    ${CMAKE_SIZEOF_INT})
SET(XDMF_SIZEOF_LONG   ${CMAKE_SIZEOF_LONG})
SET(XDMF_SIZEOF_SHORT  ${CMAKE_SIZEOF_SHORT})
INCLUDE (${CMAKE_ROOT}/Modules/CheckTypeSize.cmake)
INCLUDE (${CMAKE_ROOT}/Modules/CheckFunctionExists.cmake)
MESSAGE(STATUS "Finding a 64 Bit Integer")
IF(WIN32)
  CHECK_TYPE_SIZE(__int64        SIZEOF___INT64)
ENDIF(WIN32)
CHECK_TYPE_SIZE("long long"    SIZEOF_LONG_LONG)
CHECK_FUNCTION_EXISTS(strtoll HAVE_STRTOLL)

# Don't Really Need to check these ...
SET(XDMF_VOID_VALID 1)
SET(XDMF_VOID "void")
SET(XDMF_PTR_VALID 1)
SET(XDMF_PTR "void *")
SET(XDMF_CHAR_VALID 1)
SET(XDMF_CHAR "char")
SET(XDMF_8_INT_VALID 1)
SET(XDMF_8_INT  "char")
SET(XDMF_8_U_INT  "unsigned char")
SET(XDMF_16_INT_VALID 1)
SET(XDMF_16_INT "short")
SET(XDMF_16_U_INT "unsigned short")
# These should be Valid
IF(${XDMF_SIZEOF_FLOAT} MATCHES 4)
  SET(XDMF_32_FLOAT_VALID 1)
  SET(XDMF_FLOAT "float")
  SET(XDMF_32_FLOAT "float")
ELSE (${XDMF_SIZEOF_FLOAT} MATCHES 4)
  MESSAGE(SEND_ERROR "Can't Find a 32 Bit Float")
ENDIF (${XDMF_SIZEOF_FLOAT} MATCHES 4)
IF(${XDMF_SIZEOF_DOUBLE} MATCHES 8)
  SET(XDMF_64_FLOAT_VALID 1)
  SET(XDMF_DOUBLE "double")
  SET(XDMF_64_FLOAT "double")
ELSE (${XDMF_SIZEOF_DOUBLE} MATCHES 8)
  MESSAGE(SEND_ERROR "Can't Find a 64 Bit Float")
ENDIF (${XDMF_SIZEOF_DOUBLE} MATCHES 8)
# These are sometimes different
IF(${XDMF_SIZEOF_INT} MATCHES 4)
  SET(XDMF_32_INT_VALID 1)
  SET(XDMF_32_INT "int")
  SET(XDMF_32_U_INT "unsigned int")
  SET(XDMF_32_S_INT "int")
ELSE (${XDMF_SIZEOF_INT} MATCHES 4)
  IF(${XDMF_SIZEOF_LONG} MATCHES 4)
    SET(XDMF_32_INT_VALID 1)
    SET(XDMF_32_INT "long")
    SET(XDMF_32_U_INT "unsigned long")
    SET(XDMF_32_S_INT "long")
  ELSE(${XDMF_SIZEOF_LONG} MATCHES 4)
    MESSAGE(SEND_ERROR "Can't Find a 32 Bit Integer")
  ENDIF(${XDMF_SIZEOF_LONG} MATCHES 4)
ENDIF(${XDMF_SIZEOF_INT} MATCHES 4)

INCLUDE(${CMAKE_ROOT}/Modules/TestForANSIStreamHeaders.cmake)
INCLUDE(${Xdmf_SOURCE_DIR}/CMake/CheckFor64BitStreams.cmake)
CHECK_FOR_64BIT_STREAMS(XDMF_HAVE_64BIT_STREAMS)

# Find include files
FIND_PATH(XDMF_HAVE_FCNTL fcntl.h /usr/include /usr/include/sys)
FIND_PATH(XDMF_HAVE_NETINET in.h /usr/include/netinet /usr/include /usr/include/sys)
FIND_PATH(XDMF_HAVE_MMAN mman.h /usr/include/sys /usr/include)
MARK_AS_ADVANCED(XDMF_HAVE_FCNTL XDMF_HAVE_NETINET XDMF_HAVE_MMAN)


##
CONFIGURE_FILE(${Xdmf_SOURCE_DIR}/libsrc/XdmfConfig.h.in
  ${Xdmf_BINARY_DIR}/libsrc/XdmfConfig.h)

# IF(NOT XDMF_EXPAT_HEADER)
#   IF(XDMF_BUILD_VTK)
#     SET(XDMF_EXPAT_HEADER "vtk_expat.h")
#   ELSE(XDMF_BUILD_VTK)
#     SET(XDMF_EXPAT_HEADER "expat.h")
#   ENDIF(XDMF_BUILD_VTK)
# ENDIF(NOT XDMF_EXPAT_HEADER)

# CONFIGURE_FILE(${Xdmf_SOURCE_DIR}/libsrc/xdmf_expat.h.in
#   ${Xdmf_BINARY_DIR}/libsrc/xdmf_expat.h)

IF(NOT WIN32)
  ADD_DEFINITIONS(-D_HPUX_SOURCE)
ENDIF(NOT WIN32)

SET(XdmfSources ${XdmfFormatSource} ${XdmfExtraSource} ${XdmfModelSource})
## MPI ###
IF(XDMF_BUILD_MPI)
    SET(XdmfSources ${XdmfSources} ${XdmfMpiSource})
ELSE(XDMF_BUILD_MPI)
    ADD_DEFINITIONS("-DXDMF_NO_MPI")
ENDIF(XDMF_BUILD_MPI)

IF(XDMF_HAS_NDGM)
  SET(XdmfSources ${XdmfSources} ${XdmfNdgmSource})
ENDIF(XDMF_HAS_NDGM)

IF(VTK_BUILD_SHARED_LIBS OR BUILD_SHARED_LIBS)
  SET(LIBTYPE SHARED)
ELSE(VTK_BUILD_SHARED_LIBS OR BUILD_SHARED_LIBS)
  SET(LIBTYPE STATIC)
ENDIF(VTK_BUILD_SHARED_LIBS OR BUILD_SHARED_LIBS)

ADD_LIBRARY(Xdmf ${LIBTYPE} ${XdmfSources})

IF(XDMF_BUILD_MPI)
  IF(MPI_EXTRA_LIBRARY)
    TARGET_LINK_LIBRARIES(Xdmf ${MPI_EXTRA_LIBRARY})
  ENDIF(MPI_EXTRA_LIBRARY)
  TARGET_LINK_LIBRARIES(Xdmf ${MPI_LIBRARY} )
ENDIF(XDMF_BUILD_MPI)

IF(XDMF_HAS_NDGM)
  TARGET_LINK_LIBRARIES(Xdmf ${NDGM_LIBRARY} ${NDGM_SERVER_LIBRARY} ${HDF5_LIBRARY} ${XDMF_MPI_LIBRARIES} ${LIBXML2_LIBRARY} ${XDMF_EXPAT_LIBRARIES} ${XDMF_ZLIB_LIBRARIES})
ELSE(XDMF_HAS_NDGM)
  TARGET_LINK_LIBRARIES(Xdmf ${HDF5_LIBRARY} ${XDMF_MPI_LIBRARIES} ${LIBXML2_LIBRARY} ${XDMF_EXPAT_LIBRARIES} ${XDMF_ZLIB_LIBRARIES})
ENDIF(XDMF_HAS_NDGM)

OPTION(XDMF_WRAP_PYTHON "Build a Python wrapping interface" OFF)
IF(XDMF_WRAP_PYTHON OR VTK_WRAP_PYTHON)
  FIND_PACKAGE(PythonExecutable)
  FIND_PACKAGE(PythonLibs)
ENDIF(XDMF_WRAP_PYTHON OR VTK_WRAP_PYTHON)

IF(XDMF_WRAP_PYTHON)
  IF(NOT BUILD_SHARED_LIBS)
    MESSAGE("Python wrapping only works on shared libraries")
  ENDIF(NOT BUILD_SHARED_LIBS)
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
  ADD_LIBRARY(_Xdmf SHARED ${XdmfPythonSource})
  IF(XDMF_HAS_NDGM)
    TARGET_LINK_LIBRARIES(_Xdmf ${NDGM_LIBRARY} ${NDGM_SERVER_LIBRARY})
  ENDIF(XDMF_HAS_NDGM)
  TARGET_LINK_LIBRARIES(_Xdmf Xdmf ${HDF5_LIBRARY} ${EXPAT_LIBRARY} ${XDMF_ZLIB_LIBRARIES})
  IF(WIN32)
    TARGET_LINK_LIBRARIES(_Xdmf ${PYTHON_LIBRARY})
  ENDIF(WIN32)
  SET_TARGET_PROPERTIES(_Xdmf PROPERTIES PREFIX "")
  ADD_DEPENDENCIES(_Xdmf Xdmf)
ENDIF(XDMF_WRAP_PYTHON)

OPTION(XDMF_WRAP_TCL "Build a TCL wrapping interface" OF)
IF(XDMF_WRAP_TCL)
  IF(NOT BUILD_SHARED_LIBS)
    MESSAGE("Tcl wrapping only works on shared libraries")
  ENDIF(NOT BUILD_SHARED_LIBS)
  INCLUDE_DIRECTORIES(${TCL_INCLUDE_PATH})
  INCLUDE (${CMAKE_ROOT}/Modules/FindTCL.cmake)
  ADD_LIBRARY(XdmfTCL SHARED ${XdmfTclSource})
  IF(XDMF_HAS_NDGM)
    TARGET_LINK_LIBRARIES(XdmfTCL ${NDGM_LIBRARY} ${NDGM_SERVER_LIBRARY})
  ENDIF(XDMF_HAS_NDGM)
  TARGET_LINK_LIBRARIES(XdmfTCL Xdmf ${TCL_LIBRARY} ${HDF5_LIBRARY} ${EXPAT_LIBRARY} ${XDMF_ZLIB_LIBRARIES})
  ADD_EXECUTABLE(DummyTCL dummy.cxx)
  TARGET_LINK_LIBRARIES(DummyTCL XdmfTCL)
  ADD_DEPENDENCIES(XdmfTCL Xdmf)
ENDIF(XDMF_WRAP_TCL)

IF(XDMF_BUILD_TESTING)
  SUBDIRS(Testing)
ENDIF(XDMF_BUILD_TESTING)

IF(NOT XDMF_INSTALL_NO_DEVELOPMENT)
  INSTALL_FILES(${XDMF_INSTALL_INCLUDE_DIR} .h
    XdmfConfig
    Xdmf
    XdmfArrayCopyMacro
    XdmfDice
    XdmfExport
    xdmf_expat
    XdmfExpr
    ${XdmfFormatSource}
    ${XdmfModelSource})
  IF(XDMF_HAS_NDGM)
    INSTALL_FILES(${XDMF_INSTALL_INCLUDE_DIR} .h
      ${XdmfNdgmSource})
  ENDIF(XDMF_HAS_NDGM)
ENDIF(NOT XDMF_INSTALL_NO_DEVELOPMENT)

IF(NOT XDMF_INSTALL_NO_LIBRARIES)
  IF(PV_INSTALL_HAS_CMAKE_24)
    INSTALL(TARGETS Xdmf
      RUNTIME DESTINATION ${PV_INSTALL_BIN_DIR_CM24} COMPONENT Runtime
      LIBRARY DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Runtime
      ARCHIVE DESTINATION ${PV_INSTALL_LIB_DIR_CM24} COMPONENT Development)
  ELSE(PV_INSTALL_HAS_CMAKE_24)
    INSTALL_TARGETS(${XDMF_INSTALL_LIB_DIR} Xdmf)
  ENDIF(PV_INSTALL_HAS_CMAKE_24)
  IF(XDMF_WRAP_PYTHON)
    INSTALL_TARGETS(${XDMF_INSTALL_LIB_DIR} _Xdmf)
  ENDIF(XDMF_WRAP_PYTHON)
  IF(XDMF_WRAP_TCL)
    INSTALL_TARGETS(${XDMF_INSTALL_LIB_DIR} XdmfTCL)
  ENDIF(XDMF_WRAP_TCL)
ENDIF(NOT XDMF_INSTALL_NO_LIBRARIES)
