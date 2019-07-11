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
option(ILMBASE_ENABLE_TESTS "Enables building of tests" ON)
if(ILMBASE_ENABLE_TESTS)
  enable_testing()
  # TODO:
  #include(CTest)
  #set(CTEST_PROJECT_NAME "OpenEXR - IlmBase")
  #set(CTEST_NIGHTLY_START_TIME "01:01:01 UTC")
  #set(CTEST_DROP_METHOD "http") # there are others...
  #set(CTEST_DROP_SITE "open.cdash.org")
  #set(CTEST_DROP_LOCATION "/submit.php?project=MyProject")
  #set(CTEST_DROP_SITE_CDASH TRUE)
  #
  # Then can make a ctest script that has something like the following
  # in it:
  # set(CTEST_SOURCE_DIRECTORY "/source")
  # set(CTEST_BINARY_DIRECTORY "/binary")
  #
  # set(ENV{CXXFLAGS} "--coverage")
  # set(CTEST_CMAKE_GENERATOR "Ninja")
  # set(CTEST_USE_LAUNCHERS 1)
  # set(CTEST_COVERAGE_COMMAND "gcov")
  # set(CTEST_MEMORYCHECK_COMMAND "valgrind")
  # #set(CTEST_MEMORYCHECK_TYPE "ThreadSanitizer")
  # ctest_start("Continuous")
  # ctest_configure()
  # ctest_build()
  # ctest_test()
  # ctest_coverage()
  # ctest_memcheck()
  # ctest_submit()

  add_subdirectory( HalfTest )
  add_subdirectory( IexTest )
  add_subdirectory( ImathTest )
endif()
