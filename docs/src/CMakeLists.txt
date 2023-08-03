# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

add_executable(docs_src all.cpp)
target_link_libraries(docs_src OpenEXR::OpenEXR)
set_target_properties(docs_src PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(docs_src PRIVATE OPENEXR_DLL)
endif()
