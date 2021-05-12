# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

# We require this to get object library link library support
cmake_minimum_required(VERSION 3.12)

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

project(OpenEXR VERSION 3.0.2 LANGUAGES C CXX)

set(OPENEXR_VERSION_EXTRA "" CACHE STRING "Extra version tag string for OpenEXR build")

# See https://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
set(OPENEXR_SOVERSION 28)
set(OPENEXR_SOAGE 0) 
set(OPENEXR_SOREVISION 1) 
set(OPENEXR_LIB_VERSION "${OPENEXR_SOVERSION}.${OPENEXR_SOAGE}.${OPENEXR_SOREVISION}")

set(OPENEXR_VERSION_MAJOR ${CMAKE_PROJECT_VERSION_MAJOR})
set(OPENEXR_VERSION_MINOR ${CMAKE_PROJECT_VERSION_MINOR})
set(OPENEXR_VERSION_PATCH ${CMAKE_PROJECT_VERSION_PATCH})
set(OPENEXR_VERSION ${CMAKE_PROJECT_VERSION})
if (NOT OPENEXR_VERSION_EXTRA STREQUAL "")
  set(OPENEXR_VERSION "${CMAKE_PROJECT_VERSION}-${OPENEXR_VERSION_EXTRA}")
endif()
set(OPENEXR_VERSION_API "${OPENEXR_VERSION_MAJOR}_${OPENEXR_VERSION_MINOR}")

message(STATUS "Configure OpenEXR Version: ${OPENEXR_VERSION} Lib API: ${OPENEXR_LIB_VERSION}")

include(cmake/LibraryDefine.cmake)
include(cmake/OpenEXRSetup.cmake)
add_subdirectory(cmake)

# Hint: This can be set to enable custom find_package
# search paths, probably best to set it when configuring
# on the command line to cmake instead of setting it
# here.
###set(CMAKE_PREFIX_PATH "/prefix")

#######################################
# Add all source in subdirectories
#######################################

# Include these two modules without enable/disable options
add_subdirectory(src/lib)
add_subdirectory(src/bin)

# Tell CMake where to find the OpenEXRConfig.cmake file. Makes it posible to call 
# find_package(OpenEXR) in downstream projects
set(OpenEXR_DIR "${CMAKE_CURRENT_BINARY_DIR}/cmake" CACHE PATH "" FORCE)
# Add an empty OpenEXRTargets.cmake file for the config to use. 
# Can be empty since we already defined the targets in add_subdirectory
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/cmake/OpenEXRTargets.cmake" "# Dummy file")

option(OPENEXR_INSTALL_EXAMPLES "Install OpenEXR examples" ON)
if(OPENEXR_INSTALL_EXAMPLES)
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

if(BUILD_TESTING AND NOT OPENEXR_IS_SUBPROJECT)
  enable_testing()
  add_subdirectory(src/test)
endif()

# Including this module will add a `clang-format` target to the build if
# the clang-format executable can be found. Only do this if we are top level
if(NOT OPENEXR_IS_SUBPROJECT)
  include(cmake/clang-format.cmake)
endif()
