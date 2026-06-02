# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.
#
# Minimal CMake build of vendored zstd for OpenEXR.
# Only common, compress, and decompress modules are included.

cmake_minimum_required(VERSION 3.12)

set(ZSTD_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)

file(GLOB ZSTD_COMMON_SOURCES ${ZSTD_LIB_DIR}/common/*.c)
file(GLOB ZSTD_COMPRESS_SOURCES ${ZSTD_LIB_DIR}/compress/*.c)
file(GLOB ZSTD_DECOMPRESS_SOURCES ${ZSTD_LIB_DIR}/decompress/*.c)

set(ZSTD_SOURCES
  ${ZSTD_COMMON_SOURCES}
  ${ZSTD_COMPRESS_SOURCES}
  ${ZSTD_DECOMPRESS_SOURCES}
)

set(ZSTD_COMPILE_OPTIONS "")

if (MSVC)
  list(APPEND ZSTD_COMPILE_OPTIONS -DZSTD_DISABLE_ASM)
else()
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|AMD64.*|x86_64.*|X86_64.*")
    enable_language(ASM)
    list(APPEND ZSTD_SOURCES ${ZSTD_LIB_DIR}/decompress/huf_decompress_amd64.S)
  else()
    list(APPEND ZSTD_COMPILE_OPTIONS -DZSTD_DISABLE_ASM)
  endif()
endif()

add_library(zstd STATIC ${ZSTD_SOURCES})

target_include_directories(zstd PUBLIC
  $<BUILD_INTERFACE:${ZSTD_LIB_DIR}>
)

if (ZSTD_COMPILE_OPTIONS)
  target_compile_options(zstd PRIVATE ${ZSTD_COMPILE_OPTIONS})
endif()

set_target_properties(zstd PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  C_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN YES
)
