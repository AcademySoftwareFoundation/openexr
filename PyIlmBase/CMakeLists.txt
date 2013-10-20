CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT ( pyilmbase )

ENABLE_TESTING()

SET(CPACK_PACKAGE_VERSION_MAJOR "2")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "1")
SET(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}"
  )
set(CPACK_SOURCE_IGNORE_FILES
  "/.git*;/.cvs*;${CPACK_SOURCE_IGNORE_FILES}")
INCLUDE ( CPack )

OPTION (BUILD_SHARED_LIBS "Build Shared Libraries" ON)

SET (Python_ADDITIONAL_VERSIONS "2.7")
FIND_PACKAGE( PythonInterp REQUIRED)
FIND_PACKAGE( PythonLibs REQUIRED)
MESSAGE(STATUS "${PYTHONLIBS_VERSION_STRING}=${${PYTHONLIBS_VERSION_STRING}}")

set(Boost_USE_STATIC_LIBS       OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
FIND_PACKAGE ( Boost COMPONENTS python )

INCLUDE (FindPkgConfig)
PKG_CHECK_MODULES ( ILMBASE IlmBase )

IF ( NOT WIN32)
  ADD_DEFINITIONS ( -pthread )
ENDIF ()

IF (BUILD_SHARED_LIBS)
  IF (WIN32)
    ADD_DEFINITIONS(-DOPENEXR_DLL)
  ENDIF ()
ENDIF (BUILD_SHARED_LIBS)

INCLUDE_DIRECTORIES( PyIex
                     PyIexTest
                     PyImath
                     PyImathTest
                     PyImathNumpy
                     PyImathNumpyTest
                     ${ILMBASE_INCLUDE_DIRS}
                     ${PYTHON_INCLUDE_DIRS}
                     ${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES( ${ILMBASE_LIBRARY_DIRS})

ADD_SUBDIRECTORY ( PyIex )
ADD_SUBDIRECTORY ( PyImath )
ADD_SUBDIRECTORY ( PyImathNumpy )

SET_TARGET_PROPERTIES ( PyIex
  PROPERTIES
  VERSION 2.0.1
  SOVERSION 2
  )

SET_TARGET_PROPERTIES ( PyImath
  PROPERTIES
  VERSION 2.0.1
  SOVERSION 2
  )

ADD_SUBDIRECTORY ( PyIexTest )
ADD_SUBDIRECTORY ( PyImathTest )
ADD_SUBDIRECTORY ( PyImathNumpyTest )


FILE ( WRITE ${CMAKE_BINARY_DIR}/PyIlmBase.pc "prefix=${CMAKE_INSTALL_PREFIX}\n" )
FILE ( APPEND ${CMAKE_BINARY_DIR}/PyIlmBase.pc "exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include
PyIlmBase_includedir=\${prefix}/include/OpenEXR

Name: PyIlmBase
Description: Python bindings for the IlmBase libraries
Version: 2.0.1
Libs: -L\${libdir} -lImath-2_0 -lIexMath-2_0 -lHalf -lIex-2_0 -lIlmThread-2_0 -pthread
Cflags: -pthread -I\${PyIlmBase_includedir}
")

INSTALL ( FILES
  ${CMAKE_BINARY_DIR}/PyIlmBase.pc
  DESTINATION
  lib/pkgconfig
)
