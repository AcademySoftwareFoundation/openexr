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
find_package(Python COMPONENTS Interpreter Development)
if(NOT TARGET Python::Interpreter AND NOT TARGET Python::Python)
  message(WARNING ": Unable to find any python interpreter or libraries, disabling PyIlmBase")
  return()
endif()

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
  message(WARNING ": Unable to find python development libraries for python 2 or 3")
  return()
endif()

# Now that we know what versions of python we have, let's look
# for our other dependency - boost.
# Boost Python has some .. annoyances in that the python module
# has version names attached to it
if (TARGET Python2::Python)
  set(PYILMBASE_BOOST_PY2_COMPONENT "python${Python2_VERSION_MAJOR}${Python2_VERSION_MINOR}")
  message(STATUS "Found Python2 libraries: ${Python2_VERSION_MAJOR}${Python2_VERSION_MINOR}")
  # ${Python2_SITELIB}
endif()
if (TARGET Python3::Python)
  set(PYILMBASE_BOOST_PY3_COMPONENT "python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}")
  message(STATUS "Found Python3 libraries: ${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}")
endif()
# different flavors of O.S. have multiple versions of python
# some of them have both. Then for boost, some versions of boost
# have just a python component, some it's by major version (python2/python3)
# and still others have maj/min (python27)
# let's run a search and see what we get instead of making it
# an explicit required. The older names are not portable, but
# we'll do the best we can
find_package(Boost OPTIONAL_COMPONENTS
  python
  python2
  ${PYILMBASE_BOOST_PY2_COMPONENT}
  python3
  ${PYILMBASE_BOOST_PY3_COMPONENT})
set(_pyilmbase_have_perver_boost)
if(PYILMBASE_BOOST_PY2_COMPONENT)
  string(TOUPPER ${PYILMBASE_BOOST_PY2_COMPONENT} PYILMBASE_PY2_UPPER)
endif()
if(PYILMBASE_BOOST_PY3_COMPONENT)
  string(TOUPPER ${PYILMBASE_BOOST_PY3_COMPONENT} PYILMBASE_PY3_UPPER)
endif()
if(Boost_PYTHON2_FOUND OR Boost_${PYILMBASE_PY2_UPPER}_FOUND)
  set(_pyilmbase_have_perver_boost TRUE)
  if(NOT Boost_${PYILMBASE_PY2_UPPER}_FOUND)
    message(WARNING "Legacy Boost python2 found, but does not include minor version, this is an old configuration and may not be portable")
    set(PYILMBASE_BOOST_PY2_COMPONENT python2)
  endif()
endif()
if(Boost_PYTHON3_FOUND OR Boost_${PYILMBASE_PY3_UPPER}_FOUND)
  set(_pyilmbase_have_perver_boost TRUE)
  if(NOT Boost_${PYILMBASE_PY3_UPPER}_FOUND)
    message(WARNING "Legacy Boost python3 found, but does not include minor version, this is an old configuration and may not be portable")
    set(PYILMBASE_BOOST_PY3_COMPONENT python3)
  endif()
endif()

if(Boost_PYTHON_FOUND AND NOT _pyilmbase_have_perver_boost)
  # old boost case, I guess we just warn and assume it is python2 (likely)
  message(WARNING "Ambiguous boost python module found, assuming python 2")
  set(PYILMBASE_BOOST_PY2_COMPONENT python)
  # set it to a bogus string but not empty so later we don't test against a namespace only target
  set(PYILMBASE_BOOST_PY3_COMPONENT pythonIgnore)
elseif(NOT _pyilmbase_have_perver_boost)
  message(WARNING "Unable to find boost::python library, disabling PyIlmBase")
  return()
else()
  if(TARGET Boost::${PYILMBASE_BOOST_PY2_COMPONENT})
    message(NOTICE "Found Python 2 boost: Boost::${PYILMBASE_BOOST_PY2_COMPONENT}")
  elseif(Boost_PYTHON2_FOUND OR Boost_${PYILMBASE_PY2_UPPER}_FOUND)
    message(WARNING "Found boost for python 2, but FindBoost did not create an import library")
    return()
  endif()
  if(TARGET Boost::${PYILMBASE_BOOST_PY3_COMPONENT})
    message(NOTICE "Found Python 3 boost: Boost::${PYILMBASE_BOOST_PY3_COMPONENT}")
  elseif(Boost_PYTHON3_FOUND OR Boost_${PYILMBASE_PY3_UPPER}_FOUND)
    message(WARNING "Found boost for python 3, but FindBoost did not create an import library")
    return()
  endif()
endif()
set(PYILMBASE_PY2_UPPER)
set(PYILMBASE_PY3_UPPER)
set(_pyilmbase_have_perver_boost)

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
