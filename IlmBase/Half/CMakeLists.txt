# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

ilmbase_define_library(Half
  PRIV_EXPORT HALF_EXPORTS
  CURDIR ${CMAKE_CURRENT_SOURCE_DIR}
  CURBINDIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES
    half.cpp
  HEADERS
    half.h
    halfFunction.h
    halfExport.h
    halfLimits.h
)
