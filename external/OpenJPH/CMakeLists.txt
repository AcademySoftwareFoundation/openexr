cmake_minimum_required(VERSION 3.12.0)

## Library name/version
include(ojph_version.cmake)

## project
project (openjph VERSION ${OPENJPH_VERSION} DESCRIPTION "Open source implementation of JPH" LANGUAGES C CXX)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

################################################################################################
# Building OpenJPH
################################################################################################

## Target architecture
# We use the target architecture to help with arranging files in "source_group" commands.
# The code does not use the results provided by target_arch.cmake, and relies, instead,
# on its own logic, which matches that in target_arch.cmake, to identify the architecture
include(target_arch.cmake)
target_architecture(OJPH_TARGET_ARCH)
message(STATUS "CPU Architecture is ${OJPH_TARGET_ARCH}")

## Building for multi-generation
# This is useful for when we are building a multi-architecture build, such as when using
# the -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" build configuration
if (CMAKE_OSX_ARCHITECTURES)
  list(FIND CMAKE_OSX_ARCHITECTURES "x86_64" x86_64_index)
  if (${x86_64_index} GREATER -1)
    set(MULTI_GEN_X86_64 TRUE)
  endif()
  unset(x86_64_index)
  list(FIND CMAKE_OSX_ARCHITECTURES "arm64" arm64_index)
  if (${arm64_index} GREATER -1)
    set(MULTI_GEN_ARM64 TRUE)
  endif()
  unset(arm64_index)
endif()

## options
option(OJPH_ENABLE_TIFF_SUPPORT "Enables input and output support for TIFF files" ON)
option(OJPH_BUILD_TESTS "Enables building test code" OFF)
option(OJPH_BUILD_EXECUTABLES "Enables building command line executables" ON)
option(OJPH_BUILD_STREAM_EXPAND "Enables building ojph_stream_expand executable" OFF)

option(OJPH_DISABLE_SIMD "Disables the use of SIMD instructions -- agnostic to architectures" OFF)
option(OJPH_DISABLE_SSE "Disables the use of SSE SIMD instructions and associated files" OFF)
option(OJPH_DISABLE_SSE2 "Disables the use of SSE2 SIMD instructions and associated files" OFF)
option(OJPH_DISABLE_SSSE3 "Disables the use of SSSE3 SIMD instructions and associated files" OFF)
option(OJPH_DISABLE_SSE4 "Disables the use of SSE4 SIMD instructions and associated files" OFF)
option(OJPH_DISABLE_AVX "Disables the use of AVX SIMD instructions and associated files" OFF)
option(OJPH_DISABLE_AVX2 "Disables the use of AVX2 SIMD instructions and associated files" OFF)
option(OJPH_DISABLE_AVX512 "Disables the use of AVX512 SIMD instructions and associated files" OFF)
option(OJPH_DISABLE_NEON "Disables the use of NEON SIMD instructions and associated files" OFF)

## options that are being deprecated
if (DEFINED OJPH_DISABLE_INTEL_SIMD)
  message(STATUS "OJPH_DISABLE_INTEL_SIMD is being deprecated. Instead, use \"OJPH_DISABLE_SIMD\", "
                 "which is architecture agnostic. If you do not specify any, the default is "
                 "OJPH_DISABLE_SIMD=OFF.")
  set(OJPH_DISABLE_SIMD ${OJPH_DISABLE_INTEL_SIMD})
  message(STATUS "OJPH_DISABLE_SIMD is set to ${OJPH_DISABLE_SIMD}")
  unset(OJPH_DISABLE_INTEL_SIMD)
endif()
if (DEFINED OJPH_ENABLE_INTEL_AVX512)
  message(STATUS "OJPH_ENABLE_INTEL_AVX512 is being deprecated, use \"OJPH_DISABLE_AVX512\" instead."
                 "If you do not specify any, the default is OJPH_DISABLE_AVX512=OFF.")
  if (OJPH_ENABLE_INTEL_AVX512)
    set(OJPH_DISABLE_AVX512 OFF)
  else()
    set(OJPH_DISABLE_AVX512 ON)
  endif()
  message(STATUS "OJPH_DISABLE_AVX512 is set to ${OJPH_DISABLE_AVX512}")
  unset(OJPH_ENABLE_INTEL_AVX512)
endif()

## Setting some of the options if EMSCRIPTEN is the compiler
# In previous releases, the cmake script used to produce both non-SIMD and
# SIMD builds in one go.  At the time of this writing, all interpreters and
# compilers of WASM code, such as web-browser and node, support SIMD, therefore
# it is time to make the SIMD build the default.  In other words, this cmake
# script builds only WASM SIMD code by default, if desired, a non-SIMD build
# can be generated using the OJPH_DISABLE_SIMD option (in this case, the
# WASM SIMD code is not generated).
# It is worth remembering that the SIMD/non-SIMD issue arose because it is
# NOT possible to have multiple execution paths in the code, one for non-SIMD
# and one for SIMD, as we do for CPUs, letting the program select, at run-time,
# the best path to follow.
if(EMSCRIPTEN)
  set(BUILD_SHARED_LIBS OFF)
  set(OJPH_ENABLE_TIFF_SUPPORT OFF)
  set(OJPH_BUILD_STREAM_EXPAND OFF)
  if (OJPH_DISABLE_SIMD)
    set(OJPH_ENABLE_WASM_SIMD OFF)
  else()
    set(OJPH_ENABLE_WASM_SIMD ON)
  endif()
endif()

# This is related to how the timestamp is set for URL downloaded files.
# Set DOWNLOAD_EXTRACT_TIMESTAMP
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0")
  if(POLICY CMP0065)
    cmake_policy(SET CMP0135 NEW)
  endif()
endif()

## Added by Michael Smith
set(CMAKE_CXX_FLAGS_ASAN
  "-fsanitize=address -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -fno-omit-frame-pointer -g -O1"
  CACHE STRING "Flags used by the C++ compiler during AddressSanitizer builds."
  FORCE)

## Build type
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
  message(STATUS "To use AddressSanitizer, use \"cmake .. -DCMAKE_BUILD_TYPE=asan\"" )
endif()
message(STATUS "Building ${CMAKE_BUILD_TYPE}")

## C++ version and flags
# C11 is needed for aligned_alloc
if (NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 11)
endif()
message(STATUS "C Standard is set to ${CMAKE_C_STANDARD}")
# C++14 is needed for gtest, otherwise, C++11 is sufficient for the library
if (NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()
message(STATUS "C++ Standard is set to ${CMAKE_CXX_STANDARD}")

if (MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(
  -fexceptions
  -Wall
  -Wextra
  -Wconversion
  -Wunused-parameter
  )
endif()
if (EMSCRIPTEN)
  add_compile_options(-fexceptions)
  if(OJPH_ENABLE_WASM_SIMD)
    add_compile_options(-DOJPH_ENABLE_WASM_SIMD -msimd128)
  endif()
endif()

## Enhanced instruction options
if (OJPH_DISABLE_SIMD)
  add_compile_definitions(OJPH_DISABLE_SIMD)
else()
  if(OJPH_DISABLE_SSE)
    add_compile_definitions(OJPH_DISABLE_SSE)
  endif()
  if(OJPH_DISABLE_SSE2)
    add_compile_definitions(OJPH_DISABLE_SSE2)
  endif()
  if(OJPH_DISABLE_SSSE3)
    add_compile_definitions(OJPH_DISABLE_SSSE3)
  endif()
  if(OJPH_DISABLE_SSE4)
    add_compile_definitions(OJPH_DISABLE_SSE4)
  endif()
  if(OJPH_DISABLE_AVX)
    add_compile_definitions(OJPH_DISABLE_AVX)
  endif()
  if(OJPH_DISABLE_AVX2)
    add_compile_definitions(OJPH_DISABLE_AVX2)
  endif()
  if(OJPH_DISABLE_AVX512)
    add_compile_definitions(OJPH_DISABLE_AVX512)
  endif()
  if(OJPH_DISABLE_NEON)
    add_compile_definitions(OJPH_DISABLE_NEON)
  endif()
endif()

## Build library and applications
include(GNUInstallDirs)
add_subdirectory(src/core)
if (OJPH_BUILD_EXECUTABLES)
  add_subdirectory(src/apps)
endif()

################################################################################################
# Install
################################################################################################

install(EXPORT openjph-targets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/openjph
)

include(CMakePackageConfigHelpers)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/src/openjph-config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/openjph-config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/openjph
)

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/openjph-config-version.cmake
                                 COMPATIBILITY SameMinorVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/openjph-config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/openjph-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/openjph
)

if(IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
  set(PKG_CONFIG_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}")
else()
  set(PKG_CONFIG_INCLUDEDIR "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
endif()

if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
  set(PKG_CONFIG_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
else()
  set(PKG_CONFIG_LIBDIR "\${prefix}/${CMAKE_INSTALL_LIBDIR}")
endif()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/openjph.pc.in"
  "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc"
  @ONLY
)

install(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

################################################################################################
# Testing (OJPH_BUILD_TESTS)
################################################################################################

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND OJPH_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
