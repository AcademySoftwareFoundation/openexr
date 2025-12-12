# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenEXR Project.

add_openexr_bin_program(exrinfo SOURCES main.c)

if (OPENEXR_CORE_USE_NAMESPACE)
  set_source_files_properties(main.c PROPERTIES LANGUAGE CXX)
endif()
