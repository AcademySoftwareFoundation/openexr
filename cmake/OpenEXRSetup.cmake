# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

include(GNUInstallDirs)

if(NOT "${CMAKE_PROJECT_NAME}" STREQUAL "${PROJECT_NAME}")
  set(OPENEXR_IS_SUBPROJECT ON)
  message(STATUS "OpenEXR is configuring as a cmake subproject")
endif()

########################
## Target configuration

# What C++ standard to compile for
# VFX Platform 21 is c++17, so 21, 22, 23, 24 gives us 4+ years of 17
set(tmp 17)
if(CMAKE_CXX_STANDARD GREATER tmp)
  set(tmp ${CMAKE_CXX_STANDARD})
endif()
set(OPENEXR_CXX_STANDARD "${tmp}" CACHE STRING "C++ standard to compile against")
set(tmp)
message(STATUS "Building against C++ Standard: ${OPENEXR_CXX_STANDARD}")

set(OPENEXR_NAMESPACE_CUSTOM "0" CACHE STRING "Whether the namespace has been customized (so external users know)")
set(OPENEXR_INTERNAL_IMF_NAMESPACE "Imf_${OPENEXR_VERSION_API}" CACHE STRING "Real namespace for OpenEXR that will end up in compiled symbols")
set(OPENEXR_IMF_NAMESPACE "Imf" CACHE STRING "Public namespace alias for OpenEXR")
set(OPENEXR_PACKAGE_NAME "OpenEXR ${OPENEXR_VERSION}${OPENEXR_VERSION_RELEASE_TYPE}" CACHE STRING "Public string / label for displaying package")

# Namespace-related settings, allows one to customize the
# namespace generated, and to version the namespaces
set(ILMTHREAD_NAMESPACE_CUSTOM "0" CACHE STRING "Whether the namespace has been customized (so external users know)")
set(ILMTHREAD_INTERNAL_NAMESPACE "IlmThread_${OPENEXR_VERSION_API}" CACHE STRING "Real namespace for IlmThread that will end up in compiled symbols")
set(ILMTHREAD_NAMESPACE "IlmThread" CACHE STRING "Public namespace alias for IlmThread")

set(IEX_NAMESPACE_CUSTOM "0" CACHE STRING "Whether the namespace has been customized (so external users know)")
set(IEX_INTERNAL_NAMESPACE "Iex_${OPENEXR_VERSION_API}" CACHE STRING "Real namespace for Iex that will end up in compiled symbols")
set(IEX_NAMESPACE "Iex" CACHE STRING "Public namespace alias for Iex")

# Whether to generate and install a pkg-config file OpenEXR.pc
option(OPENEXR_INSTALL_PKG_CONFIG "Install OpenEXR.pc file" ON)

# Whether to enable threading. This can be disabled, although thread pool and tasks
# are still used, just processed immediately. Note that if this is disabled, the
# OpenEXR library may not be thread-safe and should only be used by a single thread.
option(OPENEXR_ENABLE_THREADING "Enables threaded processing of requests" ON)

option(OPENEXR_USE_DEFAULT_VISIBILITY "Makes the compile use default visibility (by default compiles tidy, hidden-by-default)"     OFF)

# This is primarily for the auto array that enables a stack
# object (if you enable this) that contains member to avoid double allocations
option(OPENEXR_ENABLE_LARGE_STACK "Enables code to take advantage of large stack support"     OFF)

########################
## Build related options

option(OPENEXR_INSTALL "Install OpenEXR libraries/binaries/bindings" ON)

# Whether to build & install the main libraries
option(OPENEXR_BUILD_LIBS "Enables building of main libraries" ON)

# Whether to build the various command line utility programs
option(OPENEXR_BUILD_TOOLS "Enables building of utility programs" ON)
option(OPENEXR_INSTALL_TOOLS "Install OpenEXR tools" ON)
option(OPENEXR_INSTALL_DEVELOPER_TOOLS "Install OpenEXR developer tools" OFF)

option(OPENEXR_BUILD_EXAMPLES "Build and install OpenEXR examples" ON)

option(OPENEXR_BUILD_PYTHON "Build python bindings" OFF)

option(OPENEXR_TEST_LIBRARIES "Run library tests" ON)
option(OPENEXR_TEST_TOOLS "Run tool tests" ON)
option(OPENEXR_TEST_PYTHON "Run python binding tests" ON)

# This is a variable here for use in controlling where include files are
# installed. Care must be taken when changing this, as many things
# probably assume this is OpenEXR
set(OPENEXR_OUTPUT_SUBDIR OpenEXR CACHE STRING "Destination sub-folder of the include path for install")

# This does not seem to be available as a per-target property,
# but is pretty harmless to set globally
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Suffix for debug configuration libraries
# (if you should choose to install those)
# Don't override if the user has set it and don't save it in the cache
if (NOT CMAKE_DEBUG_POSTFIX)
  set(CMAKE_DEBUG_POSTFIX "_d")
endif()

if(NOT OPENEXR_IS_SUBPROJECT)
  # Usual cmake option to build shared libraries or not, only overridden if OpenEXR is a top level project,
  # in general this setting should be explicitly configured by the end user
  option(BUILD_SHARED_LIBS "Build shared library" ON)
endif()

# Suffix to append to root name, this helps with version management
# but can be turned off if you don't care, or otherwise customized
set(OPENEXR_LIB_SUFFIX "-${OPENEXR_VERSION_API}" CACHE STRING "string added to the end of all the libraries")
# when building both dynamic and static, the additional string to
# add to the library name, such that to get static linkage, you
# would use -lOpenEXR_static (or target_link_libraries(xxx OpenEXR::OpenEXR_static))
set(OPENEXR_STATIC_LIB_SUFFIX "_static" CACHE STRING "When building both static and shared, name to append to static library (in addition to normal suffix)")

# rpath related setup
# make sure we force an rpath to the rpath we're compiling
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
# adds the automatically determined parts of the rpath
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
if(APPLE)
  set(CMAKE_MACOSX_RPATH ON)
endif()
# if the user sets an install rpath
# then just use that, or otherwise set one for them
if(NOT CMAKE_INSTALL_RPATH)
  list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
  if("${isSystemDir}" STREQUAL "-1")
    if("${CMAKE_SYSTEM}" MATCHES "Linux")
      get_filename_component(tmpSysPath "${CMAKE_INSTALL_FULL_LIBDIR}" NAME)
      if(NOT tmpSysPath)
        set(tmpSysPath "lib")
      endif()
      set(CMAKE_INSTALL_RPATH "\\\$ORIGIN/../${tmpSysPath};${CMAKE_INSTALL_FULL_LIBDIR}")
      set(tmpSysPath)
	elseif(APPLE)
      set(CMAKE_INSTALL_RPATH "@loader_path/../lib;@executable_path/../lib;${CMAKE_INSTALL_FULL_LIBDIR}")
    else()
      set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}")
    endif()
  endif()
  set(isSystemDir)
endif()

########################

# set a default build type if not set
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Code check related features
option(OPENEXR_USE_CLANG_TIDY "Check if clang-tidy is available, and enable that" OFF)
if(OPENEXR_USE_CLANG_TIDY)
  find_program(OPENEXR_CLANG_TIDY_BIN clang-tidy)
  if(OPENEXR_CLANG_TIDY_BIN-NOTFOUND)
    message(FATAL_ERROR "clang-tidy processing requested, but no clang-tidy found")
  endif()
  # TODO: Need to define the list of valid checks and add a file with said list
  set(CMAKE_CXX_CLANG_TIDY
    ${OPENEXR_CLANG_TIDY_BIN};
    -header-filter=.;
    -checks=*;
  )
endif()

if (NOT OPENEXR_BUILD_LIBS)
  return()
endif()

###############################
# Dependent libraries

# so we know how to add the thread stuff to the pkg-config package
# which is the only (but good) reason.
if(OPENEXR_ENABLE_THREADING)
  if(NOT TARGET Threads::Threads)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads)
    if(NOT Threads_FOUND)
      message(FATAL_ERROR "Unable to find a threading library, disable with OPENEXR_ENABLE_THREADING=OFF")
    endif()
  endif()
endif()

option(OPENEXR_FORCE_INTERNAL_DEFLATE "Force using an internal libdeflate" OFF)
set(OPENEXR_DEFLATE_REPO "https://github.com/ebiggers/libdeflate.git" CACHE STRING "Repo path for libdeflate source")
set(OPENEXR_DEFLATE_TAG "v1.18" CACHE STRING "Tag to use for libdeflate source repo (defaults to primary if empty)")

if(NOT OPENEXR_FORCE_INTERNAL_DEFLATE)
  #TODO: ^^ Release should not clone from main, this is a place holder
  set(CMAKE_IGNORE_PATH "${CMAKE_CURRENT_BINARY_DIR}/_deps/deflate-src/config;${CMAKE_CURRENT_BINARY_DIR}/_deps/deflate-build/config")
  # First try cmake config
  find_package(libdeflate CONFIG QUIET)
  if(libdeflate_FOUND)
    if(TARGET libdeflate::libdeflate_shared)
      set(EXR_DEFLATE_LIB libdeflate::libdeflate_shared)
    else()
      set(EXR_DEFLATE_LIB libdeflate::libdeflate_static)
    endif()
    set(EXR_DEFLATE_VERSION ${libdeflate_VERSION})
    message(STATUS "Using libdeflate from ${libdeflate_DIR}")
  else()
    # If not found, try pkgconfig
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
      include(FindPkgConfig)
      pkg_check_modules(deflate IMPORTED_TARGET GLOBAL libdeflate)
      if(deflate_FOUND)
        set(EXR_DEFLATE_LIB PkgConfig::deflate)
        set(EXR_DEFLATE_VERSION ${deflate_VERSION})
        message(STATUS "Using libdeflate from ${deflate_LINK_LIBRARIES}")
      endif()
    endif()
  endif()
  set(CMAKE_IGNORE_PATH)
endif()

if(EXR_DEFLATE_LIB)
  # Using external library
  set(EXR_DEFLATE_SOURCES)
  set(EXR_DEFLATE_INCLUDE_DIR)
  # For OpenEXR.pc.in for static build
  set(EXR_DEFLATE_PKGCONFIG_REQUIRES "libdeflate >= ${EXR_DEFLATE_VERSION}")
else()
  # Using internal deflate
  if(OPENEXR_FORCE_INTERNAL_DEFLATE)
    message(STATUS "libdeflate forced internal, installing from ${OPENEXR_DEFLATE_REPO} (${OPENEXR_DEFLATE_TAG})")
  else()
    message(STATUS "libdeflate was not found, installing from ${OPENEXR_DEFLATE_REPO} (${OPENEXR_DEFLATE_TAG})")
  endif()
  include(FetchContent)
  # Fetch deflate but exclude it from the "all" target.
  # This prevents the library from being built.
  FetchContent_Declare(Deflate
    GIT_REPOSITORY "${OPENEXR_DEFLATE_REPO}"
    GIT_TAG "${OPENEXR_DEFLATE_TAG}"
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    )

  FetchContent_GetProperties(Deflate)
  if(NOT deflate_POPULATED)
    if(CMAKE_VERSION VERSION_LESS "3.30")
      # CMake 3.30 deprecated this single argument version of
      # FetchContent_Populate():
      #   https://cmake.org/cmake/help/latest/policy/CMP0169.html
      # Prior to CMake 3.28, passing the EXCLUDE_FROM_ALL option to
      # FetchContent_Declare() does *not* have the desired effect of
      # excluding the fetched content from the build when
      # FetchContent_MakeAvailable() is called.
      # Ideally we could "manually" set the EXCLUDE_FROM_ALL property on the
      # deflate SOURCE_DIR and BINARY_DIR, but a bug that was only fixed as of
      # CMake 3.20.3 prevents that from properly excluding the directories:
      #   https://gitlab.kitware.com/cmake/cmake/-/issues/22234
      # To support the full range of CMake versions without overly
      # complicating the logic here with workarounds, we continue to use
      # Populate for CMake versions before 3.30, and switch to MakeAvailable
      # for CMake 3.30 and later.
      FetchContent_Populate(Deflate)
    else()
      FetchContent_MakeAvailable(Deflate)
    endif()
  endif()

  # Rather than actually compile something, just embed the sources
  # into exrcore. This could in theory cause issues when compiling as
  # a static library into another application which also uses
  # libdeflate but we switch the export symbol to hidden which should
  # hide the symbols when linking...
  set(EXR_DEFLATE_SOURCES
    lib/arm/cpu_features.c
    lib/x86/cpu_features.c
    lib/utils.c
    lib/deflate_compress.c
    lib/deflate_decompress.c
    lib/adler32.c
    lib/zlib_compress.c
    lib/zlib_decompress.c)
  # don't need these
  # lib/crc32.c
  # lib/gzip_compress.c
  # lib/gzip_decompress.c
  file(READ ${deflate_SOURCE_DIR}/lib/lib_common.h DEFLATE_HIDE)
  string(REPLACE "visibility(\"default\")" "visibility(\"hidden\")" DEFLATE_HIDE_NEW "${DEFLATE_HIDE}")
  string(REPLACE "__declspec(dllexport)" "/**/" DEFLATE_HIDE_NEW "${DEFLATE_HIDE_NEW}")

  string(COMPARE EQUAL "${DEFLATE_HIDE}" "${DEFLATE_HIDE_NEW}" DEFLATE_HIDE_SAME)
  if (NOT DEFLATE_HIDE_SAME)
    message(STATUS "libdeflate visibility changed, updating ${deflate_SOURCE_DIR}/lib/lib_common.h")
    file(WRITE ${deflate_SOURCE_DIR}/lib/lib_common.h "${DEFLATE_HIDE_NEW}")
  endif()

  # cmake makes fetch content name lowercase for the properties (to deflate)
  list(TRANSFORM EXR_DEFLATE_SOURCES PREPEND ${deflate_SOURCE_DIR}/)
  set(EXR_DEFLATE_INCLUDE_DIR ${deflate_SOURCE_DIR})
  set(EXR_DEFLATE_LIB)
endif()

#######################################
# Find or install Imath
#######################################

option(OPENEXR_FORCE_INTERNAL_IMATH "Force using an internal imath" OFF)
# Check to see if Imath is installed outside of the current build directory.
set(OPENEXR_IMATH_REPO "https://github.com/AcademySoftwareFoundation/Imath.git" CACHE STRING "Repo for auto-build of Imath")
set(OPENEXR_IMATH_TAG "v3.1.12" CACHE STRING "Tag for auto-build of Imath (branch, tag, or SHA)")
if(NOT OPENEXR_FORCE_INTERNAL_IMATH)
  #TODO: ^^ Release should not clone from main, this is a place holder
  set(CMAKE_IGNORE_PATH "${CMAKE_CURRENT_BINARY_DIR}/_deps/imath-src/config;${CMAKE_CURRENT_BINARY_DIR}/_deps/imath-build/config")
  find_package(Imath 3.1)
  set(CMAKE_IGNORE_PATH)
endif()

if(NOT TARGET Imath::Imath AND NOT Imath_FOUND)
  if(OPENEXR_FORCE_INTERNAL_IMATH)
    message(STATUS "Imath forced internal, installing from ${OPENEXR_IMATH_REPO} (${OPENEXR_IMATH_TAG})")
  else()
    message(STATUS "Imath was not found, installing from ${OPENEXR_IMATH_REPO} (${OPENEXR_IMATH_TAG})")
  endif()
  include(FetchContent)
  FetchContent_Declare(Imath
    GIT_REPOSITORY "${OPENEXR_IMATH_REPO}"
    GIT_TAG "${OPENEXR_IMATH_TAG}"
    GIT_SHALLOW ON
      )
  FetchContent_GetProperties(Imath)
  if(NOT Imath_POPULATED)
    FetchContent_MakeAvailable(Imath)

    # Propagate OpenEXR's install setting to Imath
    set(IMATH_INSTALL ${OPENEXR_INSTALL})

    # Propagate OpenEXR's setting for pkg-config generation to Imath:
    # If OpenEXR is generating it, the internal Imath should, too.
    set(IMATH_INSTALL_PKG_CONFIG ${OPENEXR_INSTALL_PKG_CONFIG})
  endif()
  # the install creates this but if we're using the library locally we
  # haven't installed the header files yet, so need to extract those
  # and make a variable for header only usage
  if(NOT TARGET Imath::ImathConfig)
    get_target_property(imathinc Imath INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(imathconfinc ImathConfig INTERFACE_INCLUDE_DIRECTORIES)
    list(APPEND imathinc ${imathconfinc})
    set(IMATH_HEADER_ONLY_INCLUDE_DIRS ${imathinc})
    message(STATUS "Imath interface dirs ${IMATH_HEADER_ONLY_INCLUDE_DIRS}")
  endif()
else()
  message(STATUS "Using Imath from ${Imath_DIR}")
  # local build
  # add_subdirectory(${IMATH_ROOT} Imath)
  # add_subdirectory(${OPENEXR_ROOT} OpenEXR)
  if(NOT TARGET Imath::ImathConfig AND TARGET Imath AND TARGET ImathConfig)
    get_target_property(imathinc Imath INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(imathconfinc ImathConfig INTERFACE_INCLUDE_DIRECTORIES)
    list(APPEND imathinc ${imathconfinc})
    set(IMATH_HEADER_ONLY_INCLUDE_DIRS ${imathinc})
    message(STATUS "Imath interface dirs ${IMATH_HEADER_ONLY_INCLUDE_DIRS}")
  endif()
endif()

###########################################
# Check if we need to emulate vld1q_f32_x2
###########################################

if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
  include(CheckCSourceCompiles)
  check_c_source_compiles("#include <arm_neon.h>
int main() {
  float a[] = {1.0, 1.0};
  vld1q_f32_x2(a);
  return 0;
}" HAS_VLD1)

  if(NOT HAS_VLD1)
    set(OPENEXR_MISSING_ARM_VLD1 TRUE)
  endif()
endif()
