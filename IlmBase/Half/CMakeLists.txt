# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

check_include_files(${CMAKE_CURRENT_BINARY_DIR}/eLut.h HAVE_ELUT_H)
check_include_files(${CMAKE_CURRENT_BINARY_DIR}/toFloat.h HAVE_TOFLOAT_H)

if((NOT HAVE_ELUT_H) OR (NOT HAVE_TOFLOAT_H))
  if(CMAKE_CROSSCOMPILING)
    message(STATUS "We're cross-compiling; will use native executables from a previous build to generate toFloat.h and eLut.h")
    set(NATIVE_ILMBASE_BUILD_DIR "NATIVE_ILMBASE-NOTFOUND" CACHE FILEPATH "Point it to the build folder of a native build.")
    if (NATIVE_ILMBASE_BUILD_DIR)
      set (toFloat_PATH "${NATIVE_ILMBASE_BUILD_DIR}/bin/toFloat")
      set (eLut_PATH "${NATIVE_ILMBASE_BUILD_DIR}/bin/eLut")
    else()
      message(FATAL_ERROR "Missing path to native build directory.")
    endif()
   else()
    add_executable(eLut eLut.cpp)
    target_compile_features(eLut PUBLIC cxx_std_${OPENEXR_CXX_STANDARD})
    set_target_properties(eLut PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    
    add_executable(toFloat toFloat.cpp)
    target_compile_features(toFloat PUBLIC cxx_std_${OPENEXR_CXX_STANDARD})
    set_target_properties(toFloat PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    set (toFloat_PATH "$<TARGET_FILE:toFloat>")
    set (eLut_PATH "$<TARGET_FILE:eLut>")
   endif()
    add_custom_command(
      OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/toFloat.h
        ${CMAKE_CURRENT_BINARY_DIR}/eLut.h
      COMMAND "${toFloat_PATH}" ARGS > "${CMAKE_CURRENT_BINARY_DIR}/toFloat.h"
      COMMAND "${eLut_PATH}" ARGS > "${CMAKE_CURRENT_BINARY_DIR}/eLut.h"
      DEPENDS $<$<NOT:$<BOOL:${CMAKE_CROSSCOMPILING}>>:eLut>
      DEPENDS $<$<NOT:$<BOOL:${CMAKE_CROSSCOMPILING}>>:toFloat>
    )
endif()

### Now define the real library

ilmbase_define_library(Half
  PRIV_EXPORT HALF_EXPORTS
  CURDIR ${CMAKE_CURRENT_SOURCE_DIR}
  CURBINDIR ${CMAKE_CURRENT_BINARY_DIR}
  SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/toFloat.h
    ${CMAKE_CURRENT_BINARY_DIR}/eLut.h
    half.cpp
  HEADERS
    half.h
    halfFunction.h
    halfExport.h
    halfLimits.h
)
