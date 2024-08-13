# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

message(STATUS "Building OpenEXR python bindings")

if(NOT "${CMAKE_PROJECT_NAME}" STREQUAL "OpenEXR")
  cmake_minimum_required(VERSION 3.12)
  project(PyOpenEXR)
  find_package(OpenEXR)
endif()

find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
find_package(pybind11 CONFIG REQUIRED)

python_add_library (PyOpenEXR MODULE PyOpenEXR.cpp PyOpenEXR_old.cpp)

target_link_libraries (PyOpenEXR PRIVATE "${Python_LIBRARIES}" OpenEXR::OpenEXR pybind11::headers)

# The python module should be called "OpenEXR.so", not "PyOpenEXR.so",
# but "OpenEXR" is taken as a library name by the main lib, so specify
# the name explicitly here.

set_target_properties (PyOpenEXR PROPERTIES OUTPUT_NAME "OpenEXR")

configure_file(Imath.py ${CMAKE_CURRENT_BINARY_DIR}/Imath.py COPYONLY)

set(PYTHON_INSTALL_DIR "python/OpenEXR")
if(SKBUILD)
  set(PYTHON_INSTALL_DIR ${SKBUILD_PLATLIB_DIR})
endif()

install(TARGETS PyOpenEXR DESTINATION ${PYTHON_INSTALL_DIR} COMPONENT python)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/Imath.py DESTINATION ${PYTHON_INSTALL_DIR} COMPONENT python)

if(BUILD_TESTING AND OPENEXR_TEST_PYTHON)

  add_test(OpenEXR.PyOpenEXR pytest ${CMAKE_CURRENT_SOURCE_DIR}/tests)

  set_tests_properties(OpenEXR.PyOpenEXR PROPERTIES
    ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}"
  )

endif()
