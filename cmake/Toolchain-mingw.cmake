# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

# This is only an example of cross compiling, but to compile for windows
# under linux, have the relevant bits installed, and
# validate the find path below
#
# Then to run, do something like
# mkdir win32_cross
# cd win32_cross
# cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-mingw.cmake -DCMAKE_INSTALL_PREFIX=/tmp/win32 ..
# or for static libs only
# cmake -DBUILD_SHARED_LIBS=OFF -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-mingw.cmake -DCMAKE_INSTALL_PREFIX=/tmp/win32 ..
# make
#

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR i686)
set(TOOLCHAIN_PREFIX i686-w64-mingw32)

set(CMAKE_CROSSCOMPILING_EMULATOR wine)

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_RC_COMPILER ${TOOLCHAIN_PREFIX}-windres)
set(CMAKE_MC_COMPILER ${TOOLCHAIN_PREFIX}-windmc)
set(CMAKE_AR:FILEPATH ${TOOLCHAIN_PREFIX}-ar)
set(CMAKE_RANLIB:FILEPATH ${TOOLCHAIN_PREFIX}-ranlib)

set(CMAKE_EXE_LINKER_FLAGS "-m32 -static-libgcc -static-libstdc++ -static" CACHE STRING "link flags")
set(CMAKE_SHARED_LINKER_FLAGS "-m32 -static-libgcc -static-libstdc++ -static" CACHE STRING "slink flags")

set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_C_FLAGS_INIT "-m32 -mfpmath=sse -msse2 -Wl,--large-address-aware $ENV{CFLAGS} ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "-m32 -mfpmath=sse -msse2 -Wl,--large-address-aware $ENV{CFLAGS} ${CMAKE_CXX_FLAGS}")
unset (CMAKE_C_FLAGS CACHE)
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS_INIT}" CACHE STRING
  "Flags used by the compiler during all build types.")
unset (CMAKE_CXX_FLAGS CACHE)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_INIT}" CACHE STRING
  "Flags used by the compiler during all build types.")
