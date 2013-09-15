CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT (OpenEXR)

ENABLE_TESTING()

# Allow the developer to select if Dynamic or Static libraries are built
OPTION (BUILD_SHARED_LIBS "Build Shared Libraries" ON)
OPTION (USE_ZLIB_WINAPI   "Use ZLib Win API"       OFF)

ADD_DEFINITIONS ( -DHAVE_CONFIG_H -DILM_IMF_TEST_IMAGEDIR="${CMAKE_SOURCE_DIR}/IlmImfTest/" )

INCLUDE (FindPkgConfig)
PKG_CHECK_MODULES ( ILMBASE IlmBase )

FIND_PACKAGE(ZLIB REQUIRED)

IF (NOT WIN32)
  SET ( PTHREAD_LIB pthread )
ENDIF()

INCLUDE_DIRECTORIES (
  config
  IlmImf
  exrmaketiled
  exrenvmap
  exrmakepreview
  exrmultiview
  IlmImfFuzzTest
  ${ILMBASE_INCLUDE_DIRS}
  ${ZLIB_INCLUDE_DIR}
)
LINK_DIRECTORIES( ${ILMBASE_LIBRARY_DIRS})

IF (BUILD_SHARED_LIBS)
  IF (WIN32)
    ADD_DEFINITIONS(-DOPENEXR_DLL)
  ENDIF ()
ENDIF (BUILD_SHARED_LIBS)

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

SET_TARGET_PROPERTIES ( IlmImf
  PROPERTIES
  VERSION 20.0.1
  SOVERSION 20
  )

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

FILE ( WRITE ${CMAKE_BINARY_DIR}/OpenEXR.pc "prefix=${CMAKE_INSTALL_PREFIX}\n" )
FILE ( APPEND ${CMAKE_BINARY_DIR}/OpenEXR.pc "exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include
OpenEXR_includedir=\${prefix}/include/OpenEXR

Name: OpenEXR
Description: OpenEXR image library
Version: 2.0.1

Libs: -L\${libdir} -lIlmImf-2_0
Cflags: -I\${OpenEXR_includedir}
Requires: IlmBase
Libs.private: -lz
")

INSTALL ( FILES
  ${CMAKE_BINARY_DIR}/OpenEXR.pc
  DESTINATION
  lib/pkgconfig
)
