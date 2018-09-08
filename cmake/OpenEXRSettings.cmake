#set(OPENEXR_LOCATION ${CMAKE_INSTALL_PREFIX})

#-------------------------------------------------------------------------------
# The following user options are cached. They are named with the OPENEXR
# prefix in order that they be grouped together in tools such as ccmake and cmake-gui.

option(OPENEXR_BUILD_ILMBASE        "Build IlmBase"              ON)
option(OPENEXR_BUILD_OPENEXR        "Build OpenEXR"              ON)
option(OPENEXR_BUILD_PYTHON_LIBS    "Build the Python bindings"  ON)
option(OPENEXR_BUILD_VIEWERS        "Build the viewers"          OFF)
option(OPENEXR_BUILD_TESTS          "Enable the tests"           ON)
# when enabled, adds the (long) running fuzz tests to the "make test" rule
# even when this is disabled, as long as OPENEXR_BUILD_TESTS is enabled, one
# can still run "make fuzz" (or equivalent)
option(OPENEXR_RUN_FUZZ_TESTS       "Run damaged-input tests"    OFF)
option(OPENEXR_BUILD_UTILS          "Build the utility programs" ON)

option(OPENEXR_BUILD_SHARED         "Build Shared Libraries"     ON)
option(OPENEXR_BUILD_STATIC         "Build Static Libraries"     OFF)
option(OPENEXR_NAMESPACE_VERSIONING "Use Namespace Versioning"   ON)
option(OPENEXR_FORCE_CXX03          "Force CXX03"                OFF)
set(OPENEXR_PYTHON_MAJOR "2" CACHE STRING "Python major version")
set(OPENEXR_PYTHON_MINOR "7" CACHE STRING "Python minor version")

# For more info on finding boost python:
# https://cmake.org/cmake/help/v3.11/module/FindBoost.html

# end of user options
#-------------------------------------------------------------------------------


if (OPENEXR_BUILD_VIEWERS AND NOT OPENEXR_BUILD_OPENEXR)
  message(ERROR, "Configuration error, enable OPENEXR_BUILD_OPENEXR for OPENEXR_BUILD_VIEWERS")
endif()

if (WIN32 AND OPENEXR_BUILD_ILMBASE AND OPENEXR_BUILD_OPENEXR AND OPENEXR_BUILD_SHARED)
    # necessary for building dwa lookup tables, and b44log tables on windows
    set(BUILD_ILMBASE_STATIC ON)
elseif (OPENEXR_BUILD_ILMBASE AND OPENEXR_BUILD_STATIC)
    set(BUILD_ILMBASE_STATIC ON)
else()
    set(BUILD_ILMBASE_STATIC OFF)
endif()

if (NOT OPENEXR_BUILD_SHARED)
  set(OPENEXR_TARGET_SUFFIX _static)
endif()

# Testing
set(ENABLE_TESTS ${OPENEXR_BUILD_TESTS})
if(ENABLE_TESTS)
  include(CTest)
  enable_testing()
endif()

# CPACK
set(CPACK_PROJECT_NAME             ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION          ${PROJECT_VERSION})
set(CPACK_SOURCE_IGNORE_FILES      "/.git*;/.cvs*;${CPACK_SOURCE_IGNORE_FILES}")
set(CPACK_SOURCE_GENERATOR         "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${OPENEXR_VERSION}" )
include(CPack)

# Configuration
if(OPENEXR_FORCE_CXX03)
  ADD_DEFINITIONS ( -std=c++03 )
else(OPENEXR_FORCE_CXX03)
  # VP18 switches to c++14, so let's do that by default
  set(CMAKE_CXX_STANDARD 14 CACHE STRING "C++ ISO Standard")
  # but switch gnu++14 or other extensions off for portability
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

add_definitions( -DHAVE_CONFIG_H )

if(NOT WIN32)
  add_definitions( -pthread )
endif()

if(WIN32)
  set(RUNTIME_DIR bin)
else()
  set(RUNTIME_DIR lib)
endif()

set(OPENEXR_LIBSUFFIX "")
set(ILMBASE_LIBSUFFIX "")
if(OPENEXR_NAMESPACE_VERSIONING)
  set( OPENEXR_LIBSUFFIX "-${OPENEXR_VERSION_API}" )
  set( ILMBASE_LIBSUFFIX "-${OPENEXR_VERSION_API}" )
endif()

# MacOs/linux rpathing
set(CMAKE_MACOSX_RPATH 1)
set(BUILD_WITH_INSTALL_RPATH 1)

# Set position independent code (mostly for static builds, but not a bad idea regardless)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
