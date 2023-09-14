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
# VFX Platform 18 is c++14, so let's enable that by default
set(tmp 14)
if(CMAKE_CXX_STANDARD)
  set(tmp ${CMAKE_CXX_STANDARD})
endif()
set(OPENEXR_CXX_STANDARD "${tmp}" CACHE STRING "C++ standard to compile against")
set(tmp)

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
# are still used, just processed immediately
option(OPENEXR_ENABLE_THREADING "Enables threaded processing of requests" ON)

option(OPENEXR_USE_DEFAULT_VISIBILITY "Makes the compile use default visibility (by default compiles tidy, hidden-by-default)"     OFF)

# This is primarily for the auto array that enables a stack
# object (if you enable this) that contains member to avoid double allocations
option(OPENEXR_ENABLE_LARGE_STACK "Enables code to take advantage of large stack support"     OFF)

########################
## Build related options

# Whether to build & install the various command line utility programs
option(OPENEXR_BUILD_TOOLS "Enables building of utility programs" ON)

# This is a variable here for use in controlling where include files are 
# installed. Care must be taken when changing this, as many things
# probably assume this is OpenEXR
set(OPENEXR_OUTPUT_SUBDIR OpenEXR CACHE STRING "Destination sub-folder of the include path for install")

# This does not seem to be available as a per-target property,
# but is pretty harmless to set globally
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Suffix for debug configuration libraries
# (if you should choose to install those)
set(CMAKE_DEBUG_POSTFIX "_d" CACHE STRING "Suffix for debug builds")

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
  include(FindPkgConfig)
  pkg_check_modules(deflate IMPORTED_TARGET GLOBAL libdeflate)
  set(CMAKE_IGNORE_PATH)
  if (deflate_FOUND)
    message(STATUS "Using libdeflate from ${deflate_LINK_LIBRARIES}")
  endif()
endif()

if(NOT TARGET PkgConfig::deflate AND NOT deflate_FOUND)
  if(OPENEXR_FORCE_INTERNAL_DEFLATE)
    message(STATUS "libdeflate forced internal, installing from ${OPENEXR_DEFLATE_REPO} (${OPENEXR_DEFLATE_TAG})")
  else()
    message(STATUS "libdeflate was not found, installing from ${OPENEXR_DEFLATE_REPO} (${OPENEXR_DEFLATE_TAG})")
  endif()
  include(FetchContent)
  FetchContent_Declare(Deflate
    GIT_REPOSITORY "${OPENEXR_DEFLATE_REPO}"
    GIT_TAG "${OPENEXR_DEFLATE_TAG}"
    GIT_SHALLOW ON
    )

  FetchContent_GetProperties(Deflate)
  if(NOT Deflate_POPULATED)
    FetchContent_Populate(Deflate)
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
  string(REPLACE "visibility(\"default\")" "visibility(\"hidden\")" DEFLATE_HIDE "${DEFLATE_HIDE}")
  string(REPLACE "__declspec(dllexport)" "/**/" DEFLATE_HIDE "${DEFLATE_HIDE}")
  file(WRITE ${deflate_SOURCE_DIR}/lib/lib_common.h "${DEFLATE_HIDE}")
  
  # cmake makes fetch content name lowercase for the properties (to deflate)
  list(TRANSFORM EXR_DEFLATE_SOURCES PREPEND ${deflate_SOURCE_DIR}/)
  set(EXR_DEFLATE_INCLUDE_DIR ${deflate_SOURCE_DIR})
  set(EXR_DEFLATE_LIB)
else()
  set(EXR_DEFLATE_INCLUDE_DIR)
  set(EXR_DEFLATE_LIB ${deflate_LIBRARIES})
  # set EXR_DEFATE_LDFLAGS for OpenEXR.pc.in for static build
  if (BUILD_SHARED_LIBS)
    set(EXR_DEFLATE_LDFLAGS "")
  else()
    set(EXR_DEFLATE_LDFLAGS "-l${deflate_LIBRARIES}")
  endif()
  set(EXR_DEFLATE_SOURCES)
endif()

#######################################
# Find or install Imath
#######################################

option(OPENEXR_FORCE_INTERNAL_IMATH "Force using an internal imath" OFF)
# Check to see if Imath is installed outside of the current build directory.
set(OPENEXR_IMATH_REPO "https://github.com/AcademySoftwareFoundation/Imath.git" CACHE STRING
    "Repo for auto-build of Imath")
set(OPENEXR_IMATH_TAG "main" CACHE STRING
  "Tag for auto-build of Imath (branch, tag, or SHA)")
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
    FetchContent_Populate(Imath)

    # Propagate OpenEXR's setting for pkg-config generation to Imath:
    # If OpenEXR is generating it, the internal Imath should, too.
    set(IMATH_INSTALL_PKG_CONFIG ${OPENEXR_INSTALL_PKG_CONFIG}) 

    # hrm, cmake makes Imath lowercase for the properties (to imath)
    add_subdirectory(${imath_SOURCE_DIR} ${imath_BINARY_DIR})
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
