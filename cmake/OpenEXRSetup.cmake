# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

include(GNUInstallDirs)

if(NOT "${CMAKE_PROJECT_NAME}" STREQUAL "${PROJECT_NAME}")
  set(OPENEXR_IS_SUBPROJECT ON)
  message(STATUS "OpenEXR is configuring as a cmake subproject")
endif()

########################
## Target configuration

# What C++ standard to compile for. 17 by default
set(tmp 17)
if(CMAKE_CXX_STANDARD)
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
# When set to ON, will change the thread pool to use TBB for the
# global thread pool by default.
#
# Regardless of this setting, if you create your own additional thread
# pools, those will NOT use TBB by default, as it can easily cause
# recursive mutex deadlocks as TBB shares a single thread pool with
# multiple arenas
option(OPENEXR_USE_TBB "Switch internals of IlmThreadPool to use TBB by default" OFF)

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

option(OPENEXR_BUILD_OSS_FUZZ "Build the oss-fuzz fuzzers" OFF)
if (OPENEXR_BUILD_OSS_FUZZ)
  # If building the oss-fuzz fuzzers, accept the comiler/options from
  # the environment.
  set(CMAKE_CXX_COMPILER $ENV{CXX})
  set(CMAKE_CXX_FLAGS $ENV{CXX_FLAGS})
  set(CMAKE_C_COMPILER $ENV{CC})
  set(CMAKE_C_FLAGS $ENV{CC_FLAGS})
endif()

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
if (NOT DEFINED CMAKE_DEBUG_POSTFIX)
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
  if(OPENEXR_USE_TBB)
    find_package(TBB)
    if(NOT TBB_FOUND)
      message(FATAL_ERROR "Unable to find the OneTBB cmake library, disable with ILMTHREAD_USE_TBB=OFF or fix TBB install")
    endif()
  endif()
endif()
set (ILMTHREAD_USE_TBB ${OPENEXR_USE_TBB})

option(OPENEXR_FORCE_INTERNAL_DEFLATE "Force using an internal libdeflate" OFF)
set (OPENEXR_USE_INTERNAL_DEFLATE OFF)

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
      pkg_check_modules(deflate IMPORTED_TARGET GLOBAL QUIET libdeflate)
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
  message(STATUS "Using externally provided libdeflate: ${EXR_DEFLATE_VERSION}")
  # For OpenEXR.pc.in for static build
  set(EXR_DEFLATE_PKGCONFIG_REQUIRES "libdeflate >= ${EXR_DEFLATE_VERSION}")
else()
  # Using internal deflate
  if(OPENEXR_FORCE_INTERNAL_DEFLATE)
    message(STATUS "libdeflate forced internal, using vendored code")
  else()
    message(STATUS "libdeflate was not found, using vendored code")
  endif()

  set (OPENEXR_USE_INTERNAL_DEFLATE ON)
  set(EXR_DEFLATE_LIB)
endif()


#######################################
# Find or download OpenJPH
#######################################

option(OPENEXR_FORCE_INTERNAL_OPENJPH "Force downloading OpenJPH from a git repo" OFF)

if (NOT OPENEXR_FORCE_INTERNAL_OPENJPH)
  find_package(openjph CONFIG QUIET)
  if(openjph_FOUND)
    if(openjph_VERSION VERSION_LESS "0.21.0")
        message(FATAL_ERROR "OpenJPH >= 0.21.0 required, but found ${openjph_VERSION}")
    endif()

    message(STATUS "Using OpenJPH ${openjph_VERSION} from ${openjph_DIR}")
    set(EXR_OPENJPH_LIB openjph)
  else()
    # If not found, try pkgconfig
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
      include(FindPkgConfig)
      pkg_check_modules(openjph IMPORTED_TARGET GLOBAL QUIET openjph=0.21)
      if(openjph_FOUND)
        set(EXR_OPENJPH_LIB PkgConfig::openjph)
        message(STATUS "Using OpenJPH ${openjph_VERSION} from ${openjph_LINK_LIBRARIES}")
      endif()
    endif()
  endif()
endif()

if(EXR_OPENJPH_LIB)
  # Using external library
  # For OpenEXR.pc.in for static build
  set(EXR_OPENJPH_PKGCONFIG_REQUIRES "openjph >= 0.21.0")
else()
  # Using internal openjph

  # extract the openjph version variables from ojph_version.h
  set(openjph_SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/openjph")
  set(openjph_version "${openjph_SOURCE_DIR}/src/core/openjph/ojph_version.h")
  if(EXISTS "${openjph_version}")
    file(STRINGS "${openjph_version}" _openjph_major REGEX "#define OPENJPH_VERSION_MAJOR")
    file(STRINGS "${openjph_version}" _openjph_minor REGEX "#define OPENJPH_VERSION_MINOR")
    file(STRINGS "${openjph_version}" _openjph_patch REGEX "#define OPENJPH_VERSION_PATCH")
    string(REGEX REPLACE ".*OPENJPH_VERSION_MAJOR[ \t]+([0-9]+).*" "\\1" openjph_VERSION_MAJOR "${_openjph_major}")
    string(REGEX REPLACE ".*OPENJPH_VERSION_MINOR[ \t]+([0-9]+).*" "\\1" openjph_VERSION_MINOR "${_openjph_minor}")
    string(REGEX REPLACE ".*OPENJPH_VERSION_PATCH[ \t]+([0-9]+).*" "\\1" openjph_VERSION_PATCH "${_openjph_patch}")
  endif()

  set(openjph_VERSION "${openjph_VERSION_MAJOR}.${openjph_VERSION_MINOR}.${openjph_VERSION_PATCH}")
  
  if(OPENEXR_FORCE_INTERNAL_OPENJPH)
    message(STATUS "openjph forced internal, using vendored code, version ${openjph_VERSION}")
  else()
    message(STATUS "openjph was not found, using vendored code, version ${openjph_VERSION}")
  endif()

endif()

#######################################
# Find or install Imath
#######################################

option(OPENEXR_FORCE_INTERNAL_IMATH "Force using an internal imath" OFF)
# Check to see if Imath is installed outside of the current build directory.
set(OPENEXR_IMATH_REPO "https://github.com/AcademySoftwareFoundation/Imath.git" CACHE STRING "Repo for auto-build of Imath")
set(OPENEXR_IMATH_TAG "main" CACHE STRING "Tag for auto-build of Imath (branch, tag, or SHA)")
if(NOT OPENEXR_FORCE_INTERNAL_IMATH)
  #TODO: ^^ Release should not clone from main, this is a place holder
  set(CMAKE_IGNORE_PATH "${CMAKE_CURRENT_BINARY_DIR}/_deps/imath-src/config;${CMAKE_CURRENT_BINARY_DIR}/_deps/imath-build/config")
  find_package(Imath 3.1 CONFIG QUIET)
  set(CMAKE_IGNORE_PATH)
endif()

if(NOT TARGET Imath::Imath AND NOT Imath_FOUND)
  if(OPENEXR_FORCE_INTERNAL_IMATH)
    message(STATUS "Imath forced internal, fetching from ${OPENEXR_IMATH_REPO} @ ${OPENEXR_IMATH_TAG}")
  else()
    message(STATUS "Imath was not found, fetching from ${OPENEXR_IMATH_REPO} @ ${OPENEXR_IMATH_TAG}")
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

  # extract the imath version variables from ImathConfig.h
  set(_imath_cfg "${Imath_BINARY_DIR}/config/ImathConfig.h")
  if(EXISTS "${_imath_cfg}")
    file(STRINGS "${_imath_cfg}" _ver_line REGEX "IMATH_LIB_VERSION_STRING")
    string(REGEX REPLACE ".*IMATH_LIB_VERSION_STRING[ \t]+\"([0-9.]+)\".*" "\\1" IMATH_LIB_VERSION_STRING "${_ver_line}")
    string(REPLACE "." ";" _ver_list "${IMATH_LIB_VERSION_STRING}")
    list(GET _ver_list 0 Imath_SOVERSION)
    list(GET _ver_list 1 Imath_VERSION_MAJOR)
    list(GET _ver_list 2 Imath_VERSION_MINOR)
    list(GET _ver_list 3 Imath_VERSION_PATCH)
  endif()
  
  # the install creates this but if we're using the library locally we
  # haven't installed the header files yet, so need to extract those
  # and make a variable for header only usage
  if(NOT TARGET Imath::ImathConfig)
    get_target_property(imathinc Imath INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(imathconfinc ImathConfig INTERFACE_INCLUDE_DIRECTORIES)
    list(APPEND imathinc ${imathconfinc})
    set(IMATH_HEADER_ONLY_INCLUDE_DIRS ${imathinc})
  endif()
else()
  message(STATUS "Using Imath ${Imath_VERSION} from ${Imath_DIR}")
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
