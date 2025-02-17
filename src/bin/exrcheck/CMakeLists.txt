# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

add_executable(exrcheck main.cpp)
target_link_libraries(exrcheck OpenEXR::OpenEXR OpenEXR::OpenEXRUtil)
set_target_properties(exrcheck PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# exrcheck is considered a "developer" tool, so only install it
# when installation of developer tools is enabled.
if(OPENEXR_INSTALL_DEVELOPER_TOOLS)
  install(TARGETS exrcheck DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(WIN32 AND (BUILD_SHARED_LIBS OR OPENEXR_BUILD_BOTH_STATIC_SHARED))
  target_compile_definitions(exrcheck PRIVATE OPENEXR_DLL)
endif()
