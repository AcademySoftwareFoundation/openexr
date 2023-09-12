# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

if(NOT "${CMAKE_PROJECT_NAME}" STREQUAL "OpenEXR")
  cmake_minimum_required(VERSION 3.12)
  project(PyOpenEXR)
  find_package(OpenEXR)
endif()

add_library (PyOpenEXR SHARED OpenEXR.cpp)

set (Python_ADDITIONAL_VERSIONS 3)
find_package (PythonLibs REQUIRED)
find_package (PythonInterp REQUIRED)

include_directories ("${PYTHON_INCLUDE_DIRS}")

set_target_properties (PyOpenEXR PROPERTIES PREFIX "")
set_target_properties (PyOpenEXR PROPERTIES OUTPUT_NAME "OpenEXR")
set_target_properties (PyOpenEXR PROPERTIES SUFFIX ".so")

target_link_libraries (PyOpenEXR "${PYTHON_LIBRARIES}" OpenEXR::OpenEXR)

