# "More Modern" CMake version
cmake_minimum_required(VERSION 3.12)

# this is for target_sources, new policy converts relative paths to absolute
cmake_policy(SET CMP0076 NEW)

# we include this first to parse configure.ac and extract the version
# numbers
include(config/ParseConfigure.cmake)

#set up our project
project(IlmBase VERSION ${ILMBASE_VERSION} LANGUAGES C CXX)

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

add_subdirectory( Half )
add_subdirectory( Iex )
add_subdirectory( IexMath )
add_subdirectory( Imath )
add_subdirectory( IlmThread )

# Can't seem to do EXCLUDE_FROM_ALL on the test executables
# since you can't then create a dependency on the internal
# "test" target such that it is auto built
include(CTest)
if(BUILD_TESTING)
  enable_testing()

  add_subdirectory( HalfTest )
  add_subdirectory( IexTest )
  add_subdirectory( ImathTest )
endif()
