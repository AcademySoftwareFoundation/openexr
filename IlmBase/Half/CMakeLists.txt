# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

ilmbase_define_library(Half
  PRIV_EXPORT HALF_EXPORTS
  CURDIR ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    toFloat.h
    eLut.h
    half.cpp
  HEADERS
    half.h
    halfFunction.h
    halfExport.h
    halfLimits.h
)
