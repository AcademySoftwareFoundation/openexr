# "More Modern" CMake version
cmake_minimum_required(VERSION 3.12)

# we include this first to parse configure.ac and extract the version
# numbers
include(config/ParseConfigure.cmake)

project(OpenEXR VERSION ${OPENEXR_VERSION} LANGUAGES C CXX)

#######################################
#######################################
# This declares all the configuration variables visible
# in cmake-gui or similar and the rest of the global
# project setup
#
# Please look at this file to see what is configurable
#######################################
#######################################
include(config/OpenEXRSetup.cmake)

# Everyone depends on IlmBase, and we currently rely on
# the version matched with our release
find_package(IlmBase ${OPENEXR_VERSION} EXACT REQUIRED CONFIG)

# generates config headers, package config files
add_subdirectory(config)

# utility function for the repeated boilerplate of defining
# the libraries
include(config/LibraryDefine.cmake)

##########################

add_subdirectory( IlmImf )
add_subdirectory( IlmImfUtil )
add_subdirectory( IlmImfExamples )

##########################
# Tests
##########################
# Can't seem to do EXCLUDE_FROM_ALL on the test executables
# since you can't then create a dependency on the internal
# "test" target such that it is auto built
include(CTest)
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory( IlmImfTest )
  add_subdirectory( IlmImfUtilTest )
  add_subdirectory( IlmImfFuzzTest )
endif()

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
endif()

add_subdirectory(doc)
