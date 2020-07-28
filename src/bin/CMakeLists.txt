# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

# We require this to get object library link library support and
# combined python 2 + 3 support
if(OPENEXR_BUILD_BOTH_STATIC_SHARED)
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

# the Imath version matched with our release
find_package(Imath REQUIRED CONFIG)

##########################
# Binaries / Utilities
##########################
if(OPENEXR_BUILD_UTILS)
  add_subdirectory( exr2aces )
  add_subdirectory( exrheader )
  add_subdirectory( exrmaketiled )
  add_subdirectory( exrstdattr )
  add_subdirectory( exrmakepreview )
  add_subdirectory( exrenvmap )
  add_subdirectory( exrmultiview )
  add_subdirectory( exrmultipart )
  add_subdirectory( exrcheck )
endif()
