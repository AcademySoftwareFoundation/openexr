
include(GNUInstallDirs)

########################
## Target configuration

# This is primarily for the halfFunction code that enables a stack
# object (if you enable this) that contains a LUT of the function
option(ILMBASE_ENABLE_LARGE_STACK "Enables code to take advantage of large stack support"     OFF)

# What C++ standard to compile for
# VFX Platform 18 is c++14, so let's enable that by default
set(ILMBASE_CXX_STANDARD "14" CACHE STRING "C++ standard to compile against")

# Namespace-related settings, allows one to customize the
# namespace generated, and to version the namespaces
set(ILMBASE_NAMESPACE_CUSTOM "0" CACHE STRING "Whether the namespace has been customized (so external users know)")
set(ILMBASE_INTERNAL_IMATH_NAMESPACE "Imath_${ILMBASE_VERSION_API}" CACHE STRING "Real namespace for Imath that will end up in compiled symbols")
set(ILMBASE_INTERNAL_IEX_NAMESPACE "Iex_${ILMBASE_VERSION_API}" CACHE STRING "Real namespace for Iex that will end up in compiled symbols")
set(ILMBASE_INTERNAL_ILMTHREAD_NAMESPACE "IlmThread_${ILMBASE_VERSION_API}" CACHE STRING "Real namespace for IlmThread that will end up in compiled symbols")
set(ILMBASE_IMATH_NAMESPACE "Imath" CACHE STRING "Public namespace alias for Imath")
set(ILMBASE_IEX_NAMESPACE "Iex" CACHE STRING "Public namespace alias for Iex")
set(ILMBASE_ILMTHREAD_NAMESPACE "IlmThread" CACHE STRING "Public namespace alias for IlmThread")
set(ILMBASE_PACKAGE_NAME "IlmBase ${ILMBASE_VERSION}" CACHE STRING "Public string / label for displaying package")

########################
## Build related options

# This is a variable here for use in install lines. Care must be taken
# when changing this, as many things assume this is OpenEXR
set(ILMBASE_OUTPUT_SUBDIR OpenEXR CACHE STRING "Destination sub-folder of the include path for install")

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
option(ILMBASE_BUILD_BOTH_STATIC_SHARED  "Build both static and shared libraries in one step (otherwise follows BUILD_SHARED_LIBS)" OFF)
if (ILMBASE_BUILD_BOTH_STATIC_SHARED)
  set(BUILD_SHARED_LIBS ON)
endif()
# Suffix to append to root name, this helps with version management
# but can be turned off if you don't care, or otherwise customized
set(ILMBASE_LIB_SUFFIX "-${ILMBASE_VERSION_API}" CACHE STRING "string added to the end of all the libraries")
# when building both dynamic and static, the additional string to
# add to the library name, such that to get static linkage, you
# would use -lImath_static (or target_link_libraries(xxx IlmBase::Imath_static))
set(ILMBASE_STATIC_LIB_SUFFIX "_static" CACHE STRING "When building both static and shared, name to append to static library (in addition to normal suffix)")

if(APPLE)
  # TODO: Right now, this will make a framework for every library
  # is that desired? Or should the framework be the set of libraries?
  # or trigger some compile-all-as-one-big-.so?
  set(CMAKE_MACOSX_RPATH ON)
  option(ILMBASE_BUILD_FRAMEWORK "Built Framework bundle" OFF)
  if(ILMBASE_BUILD_FRAMEWORK)
    set(BUILD_SHARED_LIBS ON)
  endif()
endif()

########################

# Code check related features
option(ILMBASE_USE_CLANG_TIDY "Check if clang-tidy is available, and enable that" OFF)
if(ILMBASE_USE_CLANG_TIDY)
  find_program(ILMBASE_CLANG_TIDY_BIN clang-tidy)
  if(ILMBASE_CLANG_TIDY_BIN-NOTFOUND)
    message(FATAL_ERROR "clang-tidy processing requested, but no clang-tidy found")
  endif()
  # TODO: Need to define the list of valid checks and add a file with said list
  set(CMAKE_CXX_CLANG_TIDY
    ${ILMBASE_CLANG_TIDY_BIN};
    -header-filter=.;
    -checks=*;
  )
endif()
