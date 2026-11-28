# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

###########################################################################
# Code coverage (gcov / gcovr)
#
# Configure with -DOPENEXR_ENABLE_COVERAGE=ON to instrument the build,
# then:
#
#   cmake --build <build> --target coverage
#
# which runs ctest and writes an HTML report to
# <build>/coverage/index.html, plus coverage.xml (Cobertura) and
# summary.json. Other targets:
#
#   coverage-report  regenerate the report from existing results
#                    without re-running the tests
#   coverage-clean   delete accumulated results (*.gcda) to start over
#
# Requires gcovr (pip install gcovr). Which files are measured is
# configured in gcovr.cfg at the top of the source tree.
#
# Coverage should be measured with an unoptimized build; for best
# results configure with -DCMAKE_BUILD_TYPE=Debug.

option(OPENEXR_ENABLE_COVERAGE "Instrument the build for gcov coverage and add 'coverage' targets" OFF)
set(OPENEXR_GCOV_EXECUTABLE "" CACHE STRING
    "gcov program for gcovr to use (default: matched to the compiler)")

if(NOT OPENEXR_ENABLE_COVERAGE)
  return()
endif()

if(OPENEXR_IS_SUBPROJECT)
  message(WARNING "OPENEXR_ENABLE_COVERAGE is ignored when OpenEXR is a subproject")
  return()
endif()

if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU|Clang" OR
   NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  message(FATAL_ERROR "OPENEXR_ENABLE_COVERAGE requires GCC or Clang")
endif()

add_compile_options(--coverage -O0 -g)
add_link_options(--coverage)

# The tests are multi-threaded; with the default non-atomic counter updates,
# racing threads corrupt the counts (negative or wrapped hits in gcovr).
if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-fprofile-update=atomic)
endif()

find_program(GCOVR_EXE NAMES gcovr DOC "Path to gcovr executable")
if(NOT GCOVR_EXE)
  message(WARNING
    "gcovr not found, the coverage targets will not be available "
    "(pip install gcovr)")
  return()
endif()

# gcovr needs a gcov that matches the compiler that produced the data:
# llvm-cov for clang, the gcov next to gcc otherwise.
if(OPENEXR_GCOV_EXECUTABLE)
  set(_gcov "${OPENEXR_GCOV_EXECUTABLE}")
else()
  get_filename_component(_ccdir "${CMAKE_CXX_COMPILER}" DIRECTORY)
endif()

if(OPENEXR_GCOV_EXECUTABLE)
  # already set
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  string(REGEX MATCH "[0-9]+" _ccmajor "${CMAKE_CXX_COMPILER_VERSION}")
  find_program(_gcov_prog NAMES gcov-${_ccmajor} gcov HINTS "${_ccdir}" NO_DEFAULT_PATH)
  if(NOT _gcov_prog)
    find_program(_gcov_prog NAMES gcov-${_ccmajor} gcov)
  endif()
  set(_gcov "${_gcov_prog}")
else()
  find_program(_llvm_cov NAMES llvm-cov
               HINTS "${_ccdir}" /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin)
  if(NOT _llvm_cov)
    execute_process(COMMAND xcrun --find llvm-cov
                    OUTPUT_VARIABLE _llvm_cov OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET)
  endif()
  set(_gcov "${_llvm_cov} gcov")
endif()

if(NOT _gcov OR _gcov STREQUAL " gcov")
  message(WARNING
    "Could not find a gcov matching the compiler; set OPENEXR_GCOV_EXECUTABLE")
  return()
endif()

set(OPENEXR_COVERAGE_DIR "${CMAKE_BINARY_DIR}/coverage")

# gcovr is run from the source directory so that gcovr.cfg is picked up
# and the filters in it are relative to the source tree.
add_custom_target(coverage-report
  COMMAND ${CMAKE_COMMAND} -E make_directory "${OPENEXR_COVERAGE_DIR}"
  COMMAND ${GCOVR_EXE}
          --gcov-executable "${_gcov}"
          --html-details "${OPENEXR_COVERAGE_DIR}/index.html"
          --xml "${OPENEXR_COVERAGE_DIR}/coverage.xml"
          --json-summary "${OPENEXR_COVERAGE_DIR}/summary.json"
          --print-summary
          "${CMAKE_BINARY_DIR}"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  COMMENT "Generating coverage report in ${OPENEXR_COVERAGE_DIR}"
  VERBATIM)

add_custom_target(coverage
  COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CMAKE_BINARY_DIR}" --output-on-failure
  COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --target coverage-report
  COMMENT "Running tests and generating coverage report"
  VERBATIM)

add_custom_target(coverage-clean
  COMMAND ${CMAKE_COMMAND}
          -DDIR=${CMAKE_BINARY_DIR} -P "${CMAKE_CURRENT_LIST_DIR}/coverage-clean.cmake"
  COMMAND ${CMAKE_COMMAND} -E rm -rf "${OPENEXR_COVERAGE_DIR}"
  COMMENT "Deleting coverage results"
  VERBATIM)

message(STATUS "Coverage enabled: build the 'coverage' target (gcov: ${_gcov})")
