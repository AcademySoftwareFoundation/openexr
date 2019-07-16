
include(GNUInstallDirs)

########################
## Target configuration

# TODO: Right now, we support compiling for multiple pythons at once
set(PYILMBASE_OVERRIDE_PYTHON2_INSTALL_DIR "" CACHE STRING "Override the install location for any python 2.x modules compiled")
set(PYILMBASE_OVERRIDE_PYTHON3_INSTALL_DIR "" CACHE STRING "Override the install location for any python 3.x modules compiled")

########################
## Build related options

# This option builds the python modules with an extra library layer
# for the various modules. This was originally done for a larger
# internal system, but is unlikely to work on systems such as
# windows, or with compiling for multiple python platforms at once,
# without larger intervention and as such is disabled.
option(PYILMBASE_BUILD_SUPPORT_LIBRARIES "Build python modules with library layer (see comments before enabling)" OFF)
# Suffix to append to root name, this helps with version management
# but can be turned off if you don't care, or otherwise customized
# 
set(PYILMBASE_LIB_SUFFIX "-${PYILMBASE_VERSION_API}" CACHE STRING "String added to the end of all the libraries (if on)")
set(PYILMBASE_LIB_PYTHONVER_ROOT "_Python" CACHE STRING "String added as a root to the python version in the libraries (if libs are on)")

# This is a variable here for use in install lines when creating
# libraries (otherwise ignored). Care must be taken when changing this,
# as many things assume this is OpenEXR
set(PYILMBASE_OUTPUT_SUBDIR OpenEXR CACHE STRING "Destination sub-folder of the include path for install")

# This does not seem to be available as a per-target property,
# but is pretty harmless to set globally
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# rpath related setup
#
# NB: This is global behavior. This can be made to be
# set on a per-target basis, but that places a larger burden
# on the cmake add_library / add_executable call sites
#
# make sure we force an rpath to the rpath we're compiling
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
# adds the automatically determined parts of the rpath
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
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
      set(CMAKE_INSTALL_RPATH "\\\$ORIGIN/../${tmpSysPath}:${CMAKE_INSTALL_FULL_LIBDIR}")
    elseif(APPLE)
      set(CMAKE_INSTALL_RPATH "@rpath:${CMAKE_INSTALL_FULL_LIBDIR}")
    else()
      set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}")
    endif()
  endif()
  set(isSystemDir)
endif()

########################

# Code check related features
option(PYILMBASE_USE_CLANG_TIDY "Check if clang-tidy is available, and enable that" OFF)
if(PYILMBASE_USE_CLANG_TIDY)
  find_program(PYILMBASE_CLANG_TIDY_BIN clang-tidy)
  if(PYILMBASE_CLANG_TIDY_BIN-NOTFOUND)
    message(FATAL_ERROR "clang-tidy processing requested, but no clang-tidy found")
  endif()
  # TODO: Need to define the list of valid checks and add a file with said list
  set(CMAKE_CXX_CLANG_TIDY
    ${PYILMBASE_CLANG_TIDY_BIN};
    -header-filter=.;
    -checks=*;
  )
endif()
