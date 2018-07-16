cmake_minimum_required(VERSION 3.11.0)

# Version
set(OPENEXR_VERSION_MAJOR 2)
set(OPENEXR_VERSION_MINOR 3)
set(OPENEXR_VERSION_PATCH 0)
set(OPENEXR_VERSION ${OPENEXR_VERSION_MAJOR}.${OPENEXR_VERSION_MINOR}.${OPENEXR_VERSION_PATCH})
set(OPENEXR_VERSION_API ${OPENEXR_VERSION_MAJOR}_${OPENEXR_VERSION_MINOR})

project(OpenEXR VERSION ${OPENEXR_VERSION})

# User Options
option(BUILD_OPENEXR        "Build OpenEXR. IlmBase is always built." ON)
option(BUILD_PYTHON_LIBS    "Build the Python bindings" ON)
option(BUILD_VIEWERS        "Build the viewers"         OFF)

option(BUILD_SHARED_LIBS    "Build Shared Libraries"    ON)
option(BUILD_STATIC_LIBS    "Build Static Libraries"    OFF)
option(NAMESPACE_VERSIONING "Use Namespace Versioning"  ON)
option(FORCE_CXX03          "Force CXX03"               OFF)
option(ENABLE_TESTS         "Enable the tests"          ON)
set(PYTHON_VERSION "27" CACHE STRING "Boost >= 1.67 ? 27, 36. Otherwise 2 or 3")
# For more info on finding boost python:
# https://cmake.org/cmake/help/v3.11/module/FindBoost.html

if (BUILD_VIEWERS AND NOT BUILD_OPENEXR)
  message(ERROR, "Configuration error, enable BUILD_OPENEXR for BUILD_VIEWERS")
endif()

# Testing
if(ENABLE_TESTS)
  include(CTest)
  enable_testing()
endif()

# CPACK
set(CPACK_PROJECT_NAME             ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION          ${PROJECT_VERSION})
set(CPACK_SOURCE_IGNORE_FILES      "/.git*;/.cvs*;${CPACK_SOURCE_IGNORE_FILES}")
set(CPACK_SOURCE_GENERATOR         "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${OPENEXR_VERSION}" )
include(CPack)

# Configuration
if(FORCE_CXX03)
  ADD_DEFINITIONS ( -std=c++03 )
else(FORCE_CXX03)
  # VP18 switches to c++14, so let's do that by default
  set(CMAKE_CXX_STANDARD 14 CACHE STRING "C++ ISO Standard")
  # but switch gnu++14 or other extensions off for portability
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

add_definitions( -DHAVE_CONFIG_H )

if(NOT WIN32)
  add_definitions( -pthread )
endif()

if(WIN32)
  set(RUNTIME_DIR bin)
else()
  set(RUNTIME_DIR lib)
endif()

set(OPENEXR_LIBSUFFIX "")
set(ILMBASE_LIBSUFFIX "")
if(NAMESPACE_VERSIONING)
  set( OPENEXR_LIBSUFFIX "-${OPENEXR_VERSION_API}" )
  set( ILMBASE_LIBSUFFIX "-${OPENEXR_VERSION_API}" )
endif()

# MacOs/linux rpathing
set(CMAKE_MACOSX_RPATH 1)
set(BUILD_WITH_INSTALL_RPATH 1)

# Packages
find_package(ZLIB)
if(BUILD_OPENEXR AND NOT ZLIB_FOUND)
  message(WARNING "Make Zlib available to CMake's search path, and re-run configuration")
  message(FATAL_ERROR "Zlib is a required dependency when BUILD_OPENEXR is set")
endif()

if (BUILD_PYTHON_LIBS)
  find_package(PythonLibs)
  if(NOT PythonLibs_FOUND)
    message(WARNING "Make Python available to CMake's search path, and re-run configuration")
    message(FATAL_ERROR "Python is a required dependency when BUILD_PYTHON_LIBS is set")
  endif()

  find_package(Boost COMPONENTS python${PYTHON_VERSION})
  if(NOT Boost_PYTHON${PYTHON_VERSION}_FOUND)
    message(WARNING "Make boost Python${PYTHON_VERSION} available to CMake's search path, and re-run configuration")
    message(FATAL_ERROR "boost Python is a required dependency when BUILD_PYTHON_LIBS is set")
  endif()

  find_package(NumPy)
  if(NOT NumPy_FOUND)
    message(WARNING "Numpy not found, PyImathNumpy will not be built")
  endif()
endif()

if(BUILD_VIEWERS)
  message(WARNING "Viewers are currently out of order. Building anyway")
  SET (FLTK_SKIP_FLUID 1)
  find_package(FLTK)
  if(NOT FLTK_FOUND)
    message(WARNING "FLTK not found, exrdisplay will not be built")
  endif()
  find_package(OpenGL)
  if(NOT OpenGL_FOUND)
    message(WARNING "OpenGL not found, exrdisplay will not be built")
  endif()
endif()

# Perform the build
add_subdirectory(IlmBase)

if(BUILD_OPENEXR)
    add_subdirectory(OpenEXR)
else()
    message(INFO, "Skipping OpenEXR build")
endif()
if(BUILD_PYTHON_LIBS)
    add_subdirectory(PyIlmBase)
else()
    message(INFO, "Skipping Python bindings")
endif()
if(BUILD_VIEWERS AND FLTK_FOUND AND OpenGL_FOUND)
    add_subdirectory(OpenEXR_Viewers)
else()
    message(INFO, "Skipping OpenEXR Viewers")
endif()
