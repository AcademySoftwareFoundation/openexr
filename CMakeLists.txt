# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

# We require this to get object library link library support
cmake_minimum_required(VERSION 3.14)

if(POLICY CMP0074)
  # enable find_package(<Package>) to use <Package>_ROOT as a hint
  cmake_policy(SET CMP0074 NEW)
endif()

if(POLICY CMP0077)
  # enable variables set outside to override options
  cmake_policy(SET CMP0077 NEW)
endif()

#######################################
# Create project and include cmake
# configuration files
#######################################

file(READ "src/lib/OpenEXRCore/openexr_version.h" VERSION_H)
string(REGEX MATCH "VERSION_MAJOR ([0-9]*)" _ ${VERSION_H})
set(OPENEXR_VERSION_MAJOR ${CMAKE_MATCH_1})
string(REGEX MATCH "VERSION_MINOR ([0-9]*)" _ ${VERSION_H})
set(OPENEXR_VERSION_MINOR ${CMAKE_MATCH_1})
string(REGEX MATCH "VERSION_PATCH ([0-9]*)" _ ${VERSION_H})
set(OPENEXR_VERSION_PATCH ${CMAKE_MATCH_1})

project(OpenEXR VERSION ${OPENEXR_VERSION_MAJOR}.${OPENEXR_VERSION_MINOR}.${OPENEXR_VERSION_PATCH} LANGUAGES C CXX)

set(OPENEXR_VERSION_RELEASE_TYPE "-dev" CACHE STRING "Extra version tag string for OpenEXR build, such as -dev, -beta1, etc.")

set(OPENEXR_VERSION ${OpenEXR_VERSION})
set(OPENEXR_VERSION_API "${OpenEXR_VERSION_MAJOR}_${OpenEXR_VERSION_MINOR}")

# The SOVERSION (i.e. numerical component of SONAME) tracks the ABI
# version. Increment this number whenever, and only when, the ABI changes in
# non-backwards-compatible ways.
#
# The OpenEXR project policy is to append the library version 
# "major.minor.patch" to the SONAME to form the real shared library name.  
# For example, in "libOpenEXR.so.31.3.2.0", "libOpenEXR.so.31" is the SONAME
# and ".3.2.0" identifies the corresponding library release.

set(OPENEXR_LIB_SOVERSION 32)
set(OPENEXR_LIB_VERSION "${OPENEXR_LIB_SOVERSION}.${OPENEXR_VERSION}") # e.g. "31.3.2.0"

option(OPENEXR_INSTALL "Install OpenEXR libraries" ON)
option(OPENEXR_INSTALL_TOOLS "Install OpenEXR tools" ON)
option(OPENEXR_INSTALL_DEVELOPER_TOOLS "Install OpenEXR developer tools" OFF)

if(OPENEXR_INSTALL OR OPENEXR_INSTALL_TOOLS OR OPENEXR_INSTALL_DEVELOPER_TOOLS)
  # uninstall target
  if(NOT TARGET uninstall)
    configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
      "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
      IMMEDIATE @ONLY)
    add_custom_target(uninstall
      COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
  endif()
endif()

include(cmake/LibraryDefine.cmake)
include(cmake/OpenEXRSetup.cmake)
add_subdirectory(cmake)
message(STATUS "Configure ${OPENEXR_PACKAGE_NAME}, library API version: ${OPENEXR_LIB_VERSION}")

# Hint: This can be set to enable custom find_package
# search paths, probably best to set it when configuring
# on the command line to cmake instead of setting it
# here.
###set(CMAKE_PREFIX_PATH "/prefix")

#######################################
# Add all source in subdirectories
#######################################

if(BUILD_TESTING AND NOT OPENEXR_IS_SUBPROJECT)
  # Enable testing *before* adding any subdirectories that may include tests
  enable_testing()
endif()

# Include these two modules without enable/disable options
if (OPENEXR_BUILD_LIBS)
  add_subdirectory(src/lib)
endif()

if(OPENEXR_BUILD_TOOLS AND OPENEXR_BUILD_LIBS)
  add_subdirectory(src/bin)
endif()

# Tell CMake where to find the OpenEXRConfig.cmake file. Makes it possible to call 
# find_package(OpenEXR) in downstream projects
set(OpenEXR_DIR "${CMAKE_CURRENT_BINARY_DIR}/cmake" CACHE PATH "" FORCE)
# Add an empty OpenEXRTargets.cmake file for the config to use. 
# Can be empty since we already defined the targets in add_subdirectory
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cmake/OpenEXRTargets.cmake" "# Dummy file")

if(OPENEXR_BUILD_EXAMPLES AND OPENEXR_BUILD_LIBS)
  add_subdirectory( src/examples )
endif()

# If you want to use ctest to configure, build and
# upload the results, cmake has builtin support for
# submitting to CDash, or any server who speaks the
# same protocol
# 
# These settings will need to be set for your environment,
# and then a script such as the example in
#
# cmake/SampleCTestScript.cmake
#
# edited and placed into the CI system, then run:
#
# cmake -S cmake/SampleCTestScript.cmake
#
# [or whatever you name the file you edit]
# 
#set(CTEST_PROJECT_NAME "OpenEXR")
#set(CTEST_NIGHTLY_START_TIME "01:01:01 UTC")
#set(CTEST_DROP_METHOD "http") # there are others...
#set(CTEST_DROP_SITE "open.cdash.org")
#set(CTEST_DROP_LOCATION "/submit.php?project=MyProject")
#set(CTEST_DROP_SITE_CDASH TRUE)
include(CTest)

if(BUILD_TESTING AND OPENEXR_BUILD_LIBS AND NOT OPENEXR_IS_SUBPROJECT)
  add_subdirectory(src/test)
endif()

# Including this module will add a `clang-format` target to the build if
# the clang-format executable can be found. Only do this if we are top level
if(NOT OPENEXR_IS_SUBPROJECT)
  include(cmake/clang-format.cmake)
endif()

option(OPENEXR_INSTALL_DOCS "Set ON to install tool manpages")
if (OPENEXR_INSTALL_DOCS AND NOT OPENEXR_IS_SUBPROJECT)
  add_subdirectory(docs)
endif()

option(BUILD_WEBSITE "Set ON to build website source")
if (BUILD_WEBSITE AND NOT OPENEXR_IS_SUBPROJECT)
  add_subdirectory(website)
endif()

if (OPENEXR_BUILD_LIBS AND NOT OPENEXR_IS_SUBPROJECT)
  # Even if not building the website, still make sure the website example code compiles.
  add_subdirectory(website/src)
endif()

if (OPENEXR_BUILD_PYTHON AND OPENEXR_BUILD_LIBS AND NOT OPENEXR_IS_SUBPROJECT)
  add_subdirectory(src/wrappers/python)
endif()
