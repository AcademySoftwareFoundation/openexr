CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT (OpenEXR)

# Allow the developer to select if Dynamic or Static libraries are built
OPTION (BUILD_SHARED_LIBS "Build Shared Libraries" ON)
OPTION (USE_ZLIB_WINAPI   "Use ZLib Win API"       OFF)

ADD_DEFINITIONS ( -DHAVE_CONFIG_H -DILM_IMF_TEST_IMAGEDIR="${CMAKE_SOURCE_DIR}/openexr/IlmImfTest/" )

INCLUDE_DIRECTORIES (
  config
  IlmImf
  exrmaketiled
  exrenvmap
  exrmakepreview
  exrmultiview
  IlmImfFuzzTest
)

FIND_PACKAGE(ZLIB REQUIRED)
INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})

IF (NOT WIN32)
  SET ( PTHREAD_LIB pthread )
ENDIF()

INCLUDE_DIRECTORIES ( ${ILMBASE_PACKAGE_PREFIX}/include/OpenEXR )
LINK_DIRECTORIES ( ${ILMBASE_PACKAGE_PREFIX}/lib )
MESSAGE ( "ILMBASE_PACKAGE_PREFIX = " ${ILMBASE_PACKAGE_PREFIX})

SET (LIB_TYPE STATIC)
IF (BUILD_SHARED_LIBS)
  # User wants to build Dynamic Libraries, so change the LIB_TYPE variable to CMake keyword 'SHARED'
  SET (LIB_TYPE SHARED)
  ADD_DEFINITIONS(-DOPENEXR_DLL)
ENDIF ()

IF (USE_ZLIB_WINAPI)
  ADD_DEFINITIONS(-DZLIB_WINAPI)
ENDIF ()

ADD_SUBDIRECTORY ( IlmImf )
ADD_SUBDIRECTORY ( IlmImfTest )
ADD_SUBDIRECTORY ( IlmImfFuzzTest )
ADD_SUBDIRECTORY ( IlmImfExamples )

ADD_SUBDIRECTORY ( exrheader )
ADD_SUBDIRECTORY ( exrmaketiled )
ADD_SUBDIRECTORY ( exrstdattr )
ADD_SUBDIRECTORY ( exrmakepreview )
ADD_SUBDIRECTORY ( exrenvmap )
ADD_SUBDIRECTORY ( exrmultiview )
ADD_SUBDIRECTORY ( exr2aces )
ADD_SUBDIRECTORY ( exrmultipart )

##########################
# Installation
##########################

# Documentation
INSTALL ( FILES
  doc/TechnicalIntroduction.pdf
  doc/ReadingAndWritingImageFiles.pdf
  doc/OpenEXRFileLayout.pdf
  doc/MultiViewOpenEXR.pdf
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/share/doc/OpenEXR-2.0.0
  )

# Examples

INSTALL ( FILES
  IlmImfExamples/main.cpp
  IlmImfExamples/drawImage.cpp
  IlmImfExamples/rgbaInterfaceExamples.cpp
  IlmImfExamples/rgbaInterfaceTiledExamples.cpp
  IlmImfExamples/generalInterfaceExamples.cpp
  IlmImfExamples/lowLevelIoExamples.cpp
  IlmImfExamples/previewImageExamples.cpp
  IlmImfExamples/generalInterfaceTiledExamples.cpp
  IlmImfExamples/generalInterfaceTiledExamples.h
  IlmImfExamples/drawImage.h
  IlmImfExamples/rgbaInterfaceExamples.h
  IlmImfExamples/generalInterfaceExamples.h
  IlmImfExamples/rgbaInterfaceTiledExamples.h
  IlmImfExamples/lowLevelIoExamples.h
  IlmImfExamples/previewImageExamples.h
  DESTINATION
  ${CMAKE_INSTALL_PREFIX}/share/doc/OpenEXR-2.0.0/examples
  )
