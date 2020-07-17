# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

# We require this to get object library link library support and
# combined python 2 + 3 support
if(ILMBASE_BUILD_BOTH_STATIC_SHARED)
  if (${CMAKE_VERSION} VERSION_LESS "3.12.0")
    message(FATAL_ERROR "CMake 3.12 or newer is required for object library support when building both static and shared libraries")
  endif()
  cmake_minimum_required(VERSION 3.12)
else()
  cmake_minimum_required(VERSION 3.10)
endif()

if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

# we include this first to parse configure.ac and extract the version
# numbers
include(config/ParseConfigure.cmake)

#set up our project
project(IlmBase VERSION ${ILMBASE_VERSION} LANGUAGES C CXX)

find_package(Imath)

if(NOT Imath_FOUND)
  message("Imath was not found. It is going to be cloned and installed.")
  
  include(ExternalProject)

  set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/Imath)

  ExternalProject_Add(Imath
    GIT_REPOSITORY https://github.com/AcademySoftwareFoundation/Imath.git
    GIT_TAG origin/master
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DPYTHON=${PYTHON}
    # Python bindings will be generated for Imath if PYTHON=ON
    )

  include_directories(${EXTERNAL_INSTALL_LOCATION}/include)
  link_directories(${EXTERNAL_INSTALL_LOCATION}/lib)
  
endif()


#######################################
#######################################
# This declares all the configuration variables visible
# in cmake-gui or similar and the rest of the global
# project setup
#
# Please look at this file to see what is configurable
#######################################
#######################################
include(config/IlmBaseSetup.cmake)

# generates config headers, package config files
add_subdirectory(config)

# utility function for the repeated boilerplate of defining
# the libraries
include(config/LibraryDefine.cmake)

add_subdirectory( Iex )
add_subdirectory( IexMath )
add_subdirectory( IlmThread )

# Can't seem to do EXCLUDE_FROM_ALL on the test executables
# since you can't then create a dependency on the internal
# "test" target such that it is auto built
include(CTest)
if(BUILD_TESTING)
  enable_testing()

  add_subdirectory( IexTest )
endif()
