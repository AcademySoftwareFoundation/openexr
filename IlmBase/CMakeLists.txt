# yue.nicholas@gmail.com
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT ( ILMBase )

ENABLE_TESTING()

# Allow the developer to select if Dynamic or Static libraries are built
OPTION (BUILD_SHARED_LIBS "Build Shared Libraries" ON)

IF ( NOT WIN32)
  ADD_DEFINITIONS ( -pthread )
ENDIF ()

INCLUDE_DIRECTORIES ( Iex IexMath Imath Half config IlmThread IexTest ImathTest HalfTest )

SET (LIB_TYPE STATIC)
IF (BUILD_SHARED_LIBS)
  # User wants to build Dynamic Libraries, so change the LIB_TYPE variable to CMake keyword 'SHARED'
  SET (LIB_TYPE SHARED)
  IF (WIN32)
    ADD_DEFINITIONS(-DOPENEXR_DLL)
  ENDIF ()
ENDIF (BUILD_SHARED_LIBS)


ADD_SUBDIRECTORY ( Half )
ADD_SUBDIRECTORY ( Iex )
ADD_SUBDIRECTORY ( IexMath )
ADD_SUBDIRECTORY ( Imath )
ADD_SUBDIRECTORY ( IlmThread )

IF (WIN32)
  FILE ( COPY ${CMAKE_SOURCE_DIR}/config.windows/IlmBaseConfig.h
    DESTINATION ${CMAKE_SOURCE_DIR}/config
    )
ELSE ()
  FILE ( WRITE ${CMAKE_SOURCE_DIR}/config/IlmBaseConfig.h "#define HAVE_PTHREAD 1\n" )
  FILE ( APPEND ${CMAKE_SOURCE_DIR}/config/IlmBaseConfig.h "#define HAVE_POSIX_SEMAPHORES 1\n" )
  FILE ( APPEND ${CMAKE_SOURCE_DIR}/config/IlmBaseConfig.h "#define ILMBASE_VERSION_STRING \"2.0.1\"\n" )
  FILE ( APPEND ${CMAKE_SOURCE_DIR}/config/IlmBaseConfig.h "#define ILMBASE_PACKAGE_STRING \"IlmBase 2.0.1\"\n" )
ENDIF ()

SET_TARGET_PROPERTIES ( Half
  PROPERTIES
  VERSION 10.0.1
  SOVERSION 10
  )
SET_TARGET_PROPERTIES ( Iex
  PROPERTIES
  VERSION 10.0.1
  SOVERSION 10
  )
SET_TARGET_PROPERTIES ( Imath
  PROPERTIES
  VERSION 10.0.1
  SOVERSION 10
  )
SET_TARGET_PROPERTIES ( IlmThread
  PROPERTIES
  VERSION 10.0.1
  SOVERSION 10
  )
SET_TARGET_PROPERTIES ( IexMath
  PROPERTIES
  VERSION 10.0.1
  SOVERSION 10
  )

# Tests

ADD_SUBDIRECTORY ( HalfTest )
ADD_SUBDIRECTORY ( IexTest )
ADD_SUBDIRECTORY ( ImathTest )

# Installation

INSTALL ( FILES 
  config/IlmBaseConfig.h
  DESTINATION
  include/OpenEXR
)

FILE ( WRITE ${CMAKE_BINARY_DIR}/IlmBase.pc "prefix=${CMAKE_INSTALL_PREFIX}\n" )
FILE ( APPEND ${CMAKE_BINARY_DIR}/IlmBase.pc "exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include
OpenEXR_includedir=\${prefix}/include/OpenEXR

Name: IlmBase
Description: Base math and exception libraries
Version: 2.0.1
Libs: -L\${libdir} -lImath-2_0 -lIexMath-2_0 -lHalf -lIex-2_0 -lIlmThread-2_0 -pthread
Cflags: -pthread -I\${OpenEXR_includedir}
")

INSTALL ( FILES
  ${CMAKE_BINARY_DIR}/IlmBase.pc
  DESTINATION
  lib/pkgconfig
)
