# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

include(GNUInstallDirs)

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
set(OPENEXR_INTERNAL_IMF_NAMESPACE "Imf_${OPENEXR_VERSION_API}" CACHE STRING "Real namespace for Imath that will end up in compiled symbols")
set(OPENEXR_IMF_NAMESPACE "Imf" CACHE STRING "Public namespace alias for OpenEXR")
set(OPENEXR_PACKAGE_NAME "OpenEXR ${OPENEXR_VERSION}" CACHE STRING "Public string / label for displaying package")

# Namespace-related settings, allows one to customize the
# namespace generated, and to version the namespaces
set(ILMBASE_NAMESPACE_CUSTOM "0" CACHE STRING "Whether the namespace has been customized (so external users know)")
set(ILMBASE_INTERNAL_IEX_NAMESPACE "Iex_${ILMBASE_VERSION_API}" CACHE STRING "Real namespace for Iex that will end up in compiled symbols")
set(ILMBASE_INTERNAL_ILMTHREAD_NAMESPACE "IlmThread_${ILMBASE_VERSION_API}" CACHE STRING "Real namespace for IlmThread that will end up in compiled symbols")
set(ILMBASE_IEX_NAMESPACE "Iex" CACHE STRING "Public namespace alias for Iex")
set(ILMBASE_ILMTHREAD_NAMESPACE "IlmThread" CACHE STRING "Public namespace alias for IlmThread")
set(ILMBASE_PACKAGE_NAME "IlmBase ${ILMBASE_VERSION}" CACHE STRING "Public string / label for displaying package")

# Whether to generate and install a pkg-config file OpenEXR.pc and IlmBase.pc
if (WIN32)
option(OPENEXR_INSTALL_PKG_CONFIG "Install OpenEXR.pc file" OFF)
else()
option(OPENEXR_INSTALL_PKG_CONFIG "Install OpenEXR.pc file" ON)
endif()

# Whether to enable threading. This can be disabled, although thread pool and tasks
# are still used, just processed immediately
option(OPENEXR_ENABLE_THREADING "Enables threaded processing of requests" ON)

# This is primarily for the auto array that enables a stack
# object (if you enable this) that contains member to avoid double allocations
option(OPENEXR_ENABLE_LARGE_STACK "Enables code to take advantage of large stack support"     OFF)

########################
## Build related options

# Whether to build & install the various command line utility programs
option(OPENEXR_BUILD_UTILS "Enables building of utility programs" ON)

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

# Usual cmake option to build shared libraries or not
option(BUILD_SHARED_LIBS "Build shared library" ON)
# This allows a "double library" setup, where we compile both
# a dynamic and shared library
option(OPENEXR_BUILD_BOTH_STATIC_SHARED  "Build both static and shared libraries in one step (otherwise follows BUILD_SHARED_LIBS)" OFF)
if (OPENEXR_BUILD_BOTH_STATIC_SHARED)
  set(BUILD_SHARED_LIBS ON)
endif()
# Suffix to append to root name, this helps with version management
# but can be turned off if you don't care, or otherwise customized
set(OPENEXR_LIB_SUFFIX "-${OPENEXR_VERSION_API}" CACHE STRING "string added to the end of all the libraries")
# when building both dynamic and static, the additional string to
# add to the library name, such that to get static linkage, you
# would use -lImath_static (or target_link_libraries(xxx IlmBase::Imath_static))
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
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads)
    if(NOT Threads_FOUND)
      message(FATAL_ERROR "Unable to find a threading library which is required for OpenEXR")
    endif()
  endif()
endif()

option(OPENEXR_FORCE_INTERNAL_ZLIB "Force using an internal zlib" OFF)
if (NOT OPENEXR_FORCE_INTERNAL_ZLIB)
  find_package(ZLIB)
endif()
if(OPENEXR_FORCE_INTERNAL_ZLIB OR NOT ZLIB_FOUND)
  set(zlib_VER "1.2.11")
  if(OPENEXR_FORCE_INTERNAL_ZLIB)
    message(NOTICE "Compiling internal copy of zlib version ${zlib_VER}")
  else()
    message(NOTICE "ZLIB library not found, compiling ${zlib_VER}")
  endif()
  include(ExternalProject)

  set(cmake_cc_arg)
  if (CMAKE_CROSSCOMPILING)
    set(cmake_cc_arg -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
  endif ()

  ExternalProject_Add(zlib_external
    PREFIX "${CMAKE_BINARY_DIR}"
    GIT_REPOSITORY "https://github.com/madler/zlib.git"
    GIT_SHALLOW ON
    GIT_TAG "v${zlib_VER}"
    SOURCE_DIR zlib-src
    BINARY_DIR zlib-build
    INSTALL_DIR zlib-install
    BUILD_BYPRODUCTS "${CMAKE_BINARY_DIR}/zlib-install/lib/${CMAKE_STATIC_LIBRARY_PREFIX}z${CMAKE_STATIC_LIBRARY_SUFFIX}"
    CMAKE_ARGS
      -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -D CMAKE_POSITION_INDEPENDENT_CODE=ON
      -D CMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
      -D CMAKE_GENERATOR:STRING=${CMAKE_GENERATOR}
      ${cmake_cc_arg}
      )

  set(zlib_INTERNAL_DIR "${CMAKE_BINARY_DIR}/zlib-install" CACHE PATH "zlib dir" FORCE)
  file(MAKE_DIRECTORY "${zlib_INTERNAL_DIR}")
  file(MAKE_DIRECTORY "${zlib_INTERNAL_DIR}/include")
  file(MAKE_DIRECTORY "${zlib_INTERNAL_DIR}/lib")

  add_library(zlib_static STATIC IMPORTED)
  add_dependencies(zlib_static zlib_external)

  target_include_directories(zlib_static INTERFACE "${zlib_INTERNAL_DIR}/include")
  set_property(TARGET zlib_static PROPERTY
    IMPORTED_LOCATION "${zlib_INTERNAL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}z${CMAKE_STATIC_LIBRARY_SUFFIX}"
  )
  add_library(ZLIB::ZLIB ALIAS zlib_static)
endif()

#######################################
# Find or install Imath
#######################################
# Check to see if Imath is installed outside of the current build directory.
set(CMAKE_IGNORE_PATH "${CMAKE_CURRENT_BINARY_DIR}/_deps/imath-src/config/") 
find_package(Imath QUIET)
set(CMAKE_IGNORE_PATH)

if(NOT Imath_FOUND)
  if (${CMAKE_VERSION} VERSION_LESS "3.11.0")
    message(FATAL_ERROR "CMake 3.11 or newer is required for FetchContent, you must manually install Imath if you are using an earlier version of CMake")
  endif()
  cmake_minimum_required(VERSION 3.11)
  
  message(STATUS "Imath was not found. It is going to be cloned and installed.")
  
  include(FetchContent)

  FetchContent_Declare(Imath
    GIT_REPOSITORY https://github.com/AcademySoftwareFoundation/Imath.git
    GIT_TAG origin/master #TODO: Release should not clone from master, this is a place holder
    CMAKE_ARGS
      -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -D BUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
      -D CMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
      -D CMAKE_PREFIX_PATH:PATH=<INSTALL_DIR>
      -D PYTHON=${PYTHON}
      # Python bindings will be generated for Imath if PYTHON=ON
      )
    
  FetchContent_GetProperties(Imath)
  if(NOT Imath_POPULATED)
    FetchContent_Populate(Imath)
    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/_deps/imath-src
                     ${CMAKE_CURRENT_BINARY_DIR}/_deps/imath-src)
  endif()

endif()
