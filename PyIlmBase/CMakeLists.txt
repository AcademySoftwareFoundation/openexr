# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

# We require this to get object library link library support and
# combined python 2 + 3 support
cmake_minimum_required(VERSION 3.12)

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
# python as well. Except we don't know which version...
# cmake 3.14 can also search for numpy, but we only depend on 3.12
# in the rest of OpenEXR right now...

# first make sure we find *some* python
find_package(Python REQUIRED)

# now determine which (or both), and compile for both
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
  message(STATUS "Found Python2 libraries: ${Python2_VERSION_MAJOR}${Python2_VERSION_MINOR}")
  # ${Python2_SITELIB}
endif()
if (TARGET Python3::Python)
  set(PYILMBASE_BOOST_PY3_COMPONENT "python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}")
  set(PYILMBASE_BOOST_NUMPY3_COMPONENT "numpy${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}")
  message(STATUS "Found Python3 libraries: ${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}")
endif()
# different flavors of O.S. have multiple versions of python
# some of them have both. Then for boost, some versions of boost
# have just a python component, some it's by major version (python2/python3)
# and still others have maj/min (python27)
# let's run a search and see what we get instead of making it
# an explicit required. The older names are not portable, but
# we'll do the best we can
find_package(Boost REQUIRED OPTIONAL_COMPONENTS
  python
  python2
  ${PYILMBASE_BOOST_PY2_COMPONENT}
  python3
  ${PYILMBASE_BOOST_PY3_COMPONENT})
if(Boost_python_FOUND AND NOT(
    Boost_python2_FOUND OR
    Boost_${PYILMBASE_BOOST_PY2_COMPONENT}_FOUND OR
    Boost_python3_FOUND OR
    Boost_${PYILMBASE_BOOST_PY3_COMPONENT}_FOUND))
  # old boost case, I guess we just warn and assume it is python2 (likely)
  message(WARNING "Ambiguous boost python module found, assuming python 2")
  set(PYILMBASE_BOOST_PY2_COMPONENT python)
  set(PYILMBASE_BOOST_PY3_COMPONENT pythonIgnore)
else()
  if(Boost_python2_FOUND AND NOT Boost_${PYILMBASE_BOOST_PY2_COMPONENT}_FOUND)
    message(WARNING "Legacy Boost python2 found, but does not include minor version, this is an old configuration and may not be portable")
    set(PYILMBASE_BOOST_PY2_COMPONENT python2)
  endif()
  if(Boost_python3_FOUND AND NOT Boost_${PYILMBASE_BOOST_PY3_COMPONENT}_FOUND)
    message(WARNING "Legacy Boost python3 found, but does not include minor version, this is an old configuration and may not be portable")
    set(PYILMBASE_BOOST_PY3_COMPONENT python3)
  endif()
endif()

# unfortunately, we can't use the boost numpy stuff, as that requires a
# version of boost that is newer than is mandated by many active versions
# of the VFX reference platform (numpy became active in 1.63 of boost).
# rather than make this an "official" find package thing
include(config/NumPyLocate.cmake)


# utility function for the repeated boilerplate of defining
# the libraries and/or python modules
include(config/ModuleDefine.cmake)

##########################
add_subdirectory(config)

add_subdirectory( PyIex )
add_subdirectory( PyImath )
if(TARGET Python2::IlmBaseNumPy OR TARGET Python3::IlmBaseNumPy)
  add_subdirectory( PyImathNumpy )
endif()

##########################
# Tests
##########################
include(CTest)
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory( PyIexTest )
  add_subdirectory( PyImathTest )
  if(TARGET Python2::IlmBaseNumPy OR TARGET Python3::IlmBaseNumPy)
    add_subdirectory( PyImathNumpyTest )
  endif()
endif()
