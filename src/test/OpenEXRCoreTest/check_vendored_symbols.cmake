# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

#
# Check that a static OpenEXRCore built with the vendored OpenJPH defines
# none of OpenJPH's symbols under their own names, so that they can't
# clash with another OpenJPH linked into the same program. The vendored
# OpenJPH's namespace and C functions are renamed with OpenEXR's internal
# namespace as a prefix (see the vendored OpenJPH section of
# src/lib/OpenEXRCore/CMakeLists.txt); this catches anything the rename
# misses, e.g. a C function added in an OpenJPH update.
#
# Usage: cmake -DNM=<nm> -DLIB=<libOpenEXRCore.a> -P check_vendored_symbols.cmake
#

execute_process(
  COMMAND "${NM}" -g "${LIB}"
  OUTPUT_VARIABLE _out
  ERROR_VARIABLE _err
  RESULT_VARIABLE _rv)
if (NOT _rv EQUAL 0)
  message(FATAL_ERROR "${NM} -g ${LIB} failed: ${_err}")
endif()

string(REPLACE "\n" ";" _lines "${_out}")
set(_bad)
foreach(_line IN LISTS _lines)
  # nm prints "<address> <type> <name>" for a defined symbol; undefined
  # ones (type U) have no address and are only references.
  if (NOT _line MATCHES "^[0-9A-Fa-f]+ +[A-TV-Za-tv-z] +(.+)$")
    continue()
  endif()
  set(_name "${CMAKE_MATCH_1}")
  # A C function ojph_*, with Mach-O's leading underscore if any, or a
  # C++ symbol in namespace ojph: in the mangled name, the identifier ojph
  # is "4ojph", e.g. _ZN4ojph..., not preceded by another digit (which
  # would make it a longer identifier that merely ends in "ojph").
  if (_name MATCHES "^_?ojph_" OR _name MATCHES "(^|[^0-9])4ojph")
    list(APPEND _bad "${_name}")
  endif()
endforeach()

if (_bad)
  list(REMOVE_DUPLICATES _bad)
  list(LENGTH _bad _n)
  list(JOIN _bad "\n    " _list)
  message(FATAL_ERROR
    "${LIB} defines ${_n} vendored OpenJPH symbol(s) under their own names, "
    "which would clash with another OpenJPH in the same program:\n"
    "    ${_list}\n"
    "Check the vendored OpenJPH rename in src/lib/OpenEXRCore/CMakeLists.txt.")
endif()
message(STATUS "No unrenamed vendored OpenJPH symbols in ${LIB}")
