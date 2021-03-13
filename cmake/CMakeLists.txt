# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckLibraryExists)
include(CheckStructHasMember)
include(CheckCXXSourceCompiles)

#
# Config headers and interface libs
#

# OpenEXRConfig.h and OpenEXRConfigInternal.h

# Settings related to computation of DWA compression lookup tables

if (NOT CMAKE_CROSSCOMPILING AND EXISTS "/proc/self/exe")
  set(OPENEXR_IMF_HAVE_LINUX_PROCFS TRUE)
endif()

check_cxx_source_compiles(
"
#include <unistd.h>
int main()
{
    sysconf(_SC_NPROCESSORS_ONLN);
}
" OPENEXR_IMF_HAVE_SYSCONF_NPROCESSORS_ONLN
)    

check_cxx_source_compiles(
"
int main()
{
#if defined(__SSE2__)
    int n   = 0;
    int eax = 0;
    int edx = 0;
    __asm__(
        \"xgetbv     ;\"
        \"vzeroupper  \"
        : \"=a\"(eax), \"=d\"(edx) : \"c\"(n) : );
#else
#    error No SSE support enabled to query AVX support
#endif
}
" OPENEXR_IMF_HAVE_GCC_INLINE_ASM_AVX
)

# This is no longer something that needs to be tested if we have C++14
# support.
set(OPENEXR_IMF_HAVE_COMPLETE_IOMANIP TRUE)

if(APPLE)
  set(OPENEXR_IMF_HAVE_DARWIN TRUE)
  if (${CMAKE_OSX_ARCHITECTURES} MATCHES arm64)
    set(OPENEXR_IMF_HAVE_GCC_INLINE_ASM_AVX FALSE)
  endif()
endif()

configure_file(OpenEXRConfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/OpenEXRConfig.h)
configure_file(OpenEXRConfigInternal.h.in ${CMAKE_CURRENT_BINARY_DIR}/OpenEXRConfigInternal.h)

if(OPENEXR_INSTALL)
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/OpenEXRConfig.h
  DESTINATION
    ${CMAKE_INSTALL_INCLUDEDIR}/${OPENEXR_OUTPUT_SUBDIR}
)
endif()

# IexConfig.h and IexConfigInternal.h

check_include_files(ucontext.h IEX_HAVE_UCONTEXT_H)
if(IEX_HAVE_UCONTEXT_H)
  # TODO: remove this once we cleanly have IEX_ prefix on all #defines
  set(HAVE_UCONTEXT_H ON)
  check_struct_has_member("struct _libc_fpstate" mxcsr ucontext.h IEX_HAVE_CONTROL_REGISTER_SUPPORT)
  if(NOT IEX_HAVE_CONTROL_REGISTER_SUPPORT)
    check_include_files(asm/sigcontext.h IEX_HAVE_SIGCONTEXT_H)
    if(IEX_HAVE_SIGCONTEXT_H)
      check_struct_has_member("struct _fpstate" mxcsr asm/sigcontext.h IEX_HAVE_SIGCONTEXT_CONTROL_REGISTER_SUPPORT)
    endif()
  endif()
endif()

if(OPENEXR_ENABLE_THREADING AND Threads_FOUND)
  set(ILMTHREAD_THREADING_ENABLED ON)
  if(CMAKE_HAVE_PTHREAD_H OR CMAKE_USE_PTHREADS_INIT OR CMAKE_HP_PTHREAD_INIT)
    set(ILMTHREAD_HAVE_PTHREAD ON)
  endif()

  # we have threads, but do we have posix semaphores for sem_init?
  # should be in pthreads
  if(NOT (APPLE OR (WIN32 AND (NOT MINGW))))
    if(MINGW)
      check_include_files("pthread_unistd.h;semaphore.h" ILMTHREAD_HAVE_SEMAPHORE_H)
    else()
      check_include_files(semaphore.h ILMTHREAD_HAVE_SEMAPHORE_H)
    endif()
    if(ILMTHREAD_HAVE_SEMAPHORE_H AND ILMTHREAD_HAVE_PTHREAD)
      # should just be in libc, so no need for check_library_exists
      set(CMAKE_REQUIRED_FLAGS ${CMAKE_THREAD_LIBS_INIT})
      check_library_exists(pthread sem_init "" ILMTHREAD_HAVE_POSIX_SEMAPHORES)
      set(CMAKE_REQUIRED_FLAGS)
    endif()
  endif()
endif()

if (OPENEXR_ENABLE_LARGE_STACK)
  set(OPENEXR_HAVE_LARGE_STACK ON)
endif()

configure_file(IexConfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/IexConfig.h)
configure_file(IexConfigInternal.h.in ${CMAKE_CURRENT_BINARY_DIR}/IexConfigInternal.h)
configure_file(IlmThreadConfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/IlmThreadConfig.h)

#
# Install public config headers
#

if(OPENEXR_INSTALL)
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/OpenEXRConfig.h
    ${CMAKE_CURRENT_BINARY_DIR}/IexConfig.h
    ${CMAKE_CURRENT_BINARY_DIR}/IlmThreadConfig.h
  DESTINATION
    ${CMAKE_INSTALL_INCLUDEDIR}/${OPENEXR_OUTPUT_SUBDIR}
)
endif()

#
# OpenEXRConfig
#
add_library(OpenEXRConfig INTERFACE)
target_include_directories(OpenEXRConfig INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${OPENEXR_OUTPUT_SUBDIR}>)
if(OPENEXR_INSTALL)
  install(TARGETS OpenEXRConfig EXPORT ${PROJECT_NAME})
endif()

add_library(${PROJECT_NAME}::Config ALIAS OpenEXRConfig)

#
# IexConfig
#
add_library(IexConfig INTERFACE)
target_include_directories(IexConfig INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${OPENEXR_OUTPUT_SUBDIR}>)
if(OPENEXR_INSTALL)
  install(TARGETS IexConfig EXPORT ${PROJECT_NAME})
endif()

add_library(${PROJECT_NAME}::Config ALIAS IexConfig)

#
# IlmThreadConfig
#
add_library(IlmThreadConfig INTERFACE)
target_include_directories(IlmThreadConfig INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${OPENEXR_OUTPUT_SUBDIR}>)

if(OPENEXR_INSTALL)
  install(TARGETS IlmThreadConfig EXPORT ${PROJECT_NAME})
endif()

add_library(${PROJECT_NAME}::Config ALIAS IlmThreadConfig)

#
# Install pkg-config file if requested
#
if(OPENEXR_INSTALL_PKG_CONFIG)
  # use a helper function to avoid variable pollution, but pretty simple
  function(openexr_pkg_config_help pcinfile)
    set(prefix ${CMAKE_INSTALL_PREFIX})
    set(exec_prefix "\${prefix}")
    set(libdir "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
    set(includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
    set(LIB_SUFFIX_DASH ${OPENEXR_LIB_SUFFIX})
    if(OPENEXR_ENABLE_THREADING AND TARGET Threads::Threads)
      # hrm, can't use properties as they end up as generator expressions
      # which don't seem to evaluate
      if(THREADS_HAVE_PTHREAD_ARG)
        set(PTHREAD_CFLAGS "-pthread")
      endif()
      set(PTHREAD_LIBS ${CMAKE_THREAD_LIBS_INIT})
    endif()
    if(NOT zlib_INTERNAL_DIR)
      set(zlib_link "-lz")
    endif()
    string(REPLACE ".in" "" pcout ${pcinfile})
    configure_file(${pcinfile} ${CMAKE_CURRENT_BINARY_DIR}/${pcout} @ONLY)
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/${pcout}
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    )
  endfunction()
  openexr_pkg_config_help(OpenEXR.pc.in)
else()
  message(STATUS "-- pkg-config generation disabled")
endif()

# The main export of the configuration: this is the cmake equivalent
# of a pkg-config file and replaces the Find*.cmake of the "old" cmake.

include(CMakePackageConfigHelpers)

if(OPENEXR_INSTALL)
configure_package_config_file(${PROJECT_NAME}Config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

write_basic_package_version_file("${PROJECT_NAME}ConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

install(EXPORT ${PROJECT_NAME}
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  FILE ${PROJECT_NAME}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::
  EXPORT_LINK_INTERFACE_LIBRARIES
)
endif()
