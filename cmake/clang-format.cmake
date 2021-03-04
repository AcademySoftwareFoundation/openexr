# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.
#
# This module was partially adapted from OpenImageIO, also under the
# same BSD-3-Clause license.

###########################################################################
# clang-format options
#
# clang-format is a source code reformatter that is part of the LLVM tools.
# It can be used to check adherence to project code formatting rules and
# correct any deviations. If clang-format is found on the system, a
# "clang-format" build target will trigger a reformatting.
#
set (CLANG_FORMAT_ROOT "" CACHE PATH "clang-format executable's directory (will search if not specified")
set (CLANG_FORMAT_INCLUDES "*.h" "*.cpp"
     CACHE STRING "Glob patterns to include for clang-format")
set (CLANG_FORMAT_EXCLUDES ""
     CACHE STRING "Glob patterns to exclude for clang-format")

# Look for clang-format. If not in the ordinary execution path, you can
# hint at it with either CLANG_FORMAT_ROOT or LLVM_ROOT (as a CMake variable
# or as an environment variable).
find_program (CLANG_FORMAT_EXE
              NAMES clang-format bin/clang-format
              HINTS ${CLANG_FORMAT_ROOT}
                    ENV CLANG_FORMAT_ROOT
                    LLVM_ROOT
                    ENV LLVM_ROOT
              DOC "Path to clang-format executable")

# If clang-format was found, set up a custom `clang-format` target that will
# reformat all source code with filenames patterns matching
# CLANG_FORMAT_INCLUDES and excluding CLANG_FORMAT_EXCLUDES.
if (CLANG_FORMAT_EXE)
    message (STATUS "clang-format found: ${CLANG_FORMAT_EXE}")
    # Start with the list of files to include when formatting...
    file (GLOB_RECURSE FILES_TO_FORMAT ${CLANG_FORMAT_INCLUDES})
    # ... then process any list of excludes we are given
    foreach (_pat ${CLANG_FORMAT_EXCLUDES})
        file (GLOB_RECURSE _excl ${_pat})
        list (REMOVE_ITEM FILES_TO_FORMAT ${_excl})
    endforeach ()
    #message (STATUS "clang-format file list: ${FILES_TO_FORMAT}")
    file (COPY ${CMAKE_CURRENT_SOURCE_DIR}/.clang-format
          DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    add_custom_target (clang-format
        COMMAND "${CLANG_FORMAT_EXE}" -i -style=file ${FILES_TO_FORMAT} )
else ()
    message (STATUS "clang-format not found.")
endif ()
