# "More Modern" CMake version
cmake_minimum_required(VERSION 3.12)

# this is for target_sources, new policy converts relative paths to absolute
cmake_policy(SET CMP0076 NEW)

# we include this first to parse configure.ac and extract the version
# numbers
include(config/ParseConfigure.cmake)

project(PyIlmBase VERSION ${PYILMBASE_VERSION} LANGUAGES C CXX)

#######################################
#######################################
# This declares all the configuration variables visible
# in cmake-gui or similar and the rest of the global
# project setup
#
# Please look at this file to see what is configurable
#######################################
#######################################
include(config/PyIlmBaseSetup.cmake)

# we have a strong dependence on IlmBase being an exact match
find_package(IlmBase ${OPENEXR_VERSION} EXACT REQUIRED CONFIG)

# we are building a python extension, so of course we depend on
# python as well. Excepth we don't know which version...
# cmake 3.14 can also search for numpy, but we only depend on 3.12
# in the rest of OpenEXR right now...
#find_package(Python2 COMPONENTS Development NumPy)
find_package(Python2 COMPONENTS Interpreter Development)
find_package(Python3 COMPONENTS Interpreter Development)
if(TARGET Python2::Python AND TARGET Python3::Python)
  message(NOTICE ": Found Python ${Python2_VERSION} and ${Python3_VERSION}")
elseif(TARGET Python2::Python)
  message(NOTICE ": Found Python ${Python2_VERSION}")
elseif(TARGET Python3::Python)
  message(NOTICE ": Found Python ${Python3_VERSION}")
else()
  message(FATAL_ERROR ": Unable to find any python development libraries")
endif()

# Now that we know what versions of python we have, let's look
# for our other dependency - boost.
# Boost Python has some .. annoyances in that the python module
# has version names attached to it
if (TARGET Python2::Python)
set(PYILMBASE_BOOST_PY2_COMPONENT "python${Python2_VERSION_MAJOR}${Python2_VERSION_MINOR}")
set(PYILMBASE_BOOST_NUMPY2_COMPONENT "numpy${Python2_VERSION_MAJOR}${Python2_VERSION_MINOR}")
message(": Python2 site lib: ${Python2_SITELIB}")
endif()
if (TARGET Python3::Python)
set(PYILMBASE_BOOST_PY3_COMPONENT "python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}")
set(PYILMBASE_BOOST_NUMPY3_COMPONENT "numpy${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}")
message(": Python3 site lib: ${Python3_SITELIB}")
endif()

find_package(Boost COMPONENTS ${PYILMBASE_BOOST_PY2_COMPONENT} ${PYILMBASE_BOOST_NUMPY2_COMPONENT} ${PYILMBASE_BOOST_PY3_COMPONENT} ${PYILMBASE_BOOST_NUMPY3_COMPONENT})
##########################

add_subdirectory( PyIex )
#add_subdirectory( PyImath )
#add_subdirectory( PyImathNumpy )

##########################
# Tests
##########################
#include(CTest)
#if(BUILD_TESTING)
#  enable_testing()
#  add_subdirectory( PyIexTest )
#  add_subdirectory( PyImathTest )
#  add_subdirectory( PyImathNumpyTest )
#endif()
