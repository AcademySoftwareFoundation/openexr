# "More Modern" CMake version
cmake_minimum_required(VERSION 3.12)

# this is for target_sources, new policy converts relative paths to absolute
cmake_policy(SET CMP0076 NEW)

# we include this first to parse configure.ac and extract the version
# numbers
include(config/ParseConfigure.cmake)

project(OpenEXR VERSION ${OPENEXR_VERSION})

# Everyone depends on IlmBase, and we currently rely on
# the version matched with our release
find_package(IlmBase ${OPENEXR_VERSION} EXACT REQUIRED)

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

# generates config headers, package config files
add_subdirectory(config)

# utility function for the repeated boilerplate of defining
# the libraries
include(config/LibraryDefine.cmake)

add_subdirectory( IlmImf )
add_subdirectory( IlmImfUtil )
add_subdirectory( IlmImfExamples )

# Can't seem to do EXCLUDE_FROM_ALL on the test executables
# since you can't then create a dependency on the internal
# "test" target such that it is auto built
option(OPENEXR_ENABLE_TESTS "Enables building of tests" ON)
if(OPENEXR_ENABLE_TESTS)
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
