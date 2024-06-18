# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) Contributors to the OpenEXR Project.

add_executable(exrmetrics main.cpp exrmetrics.cpp)
target_link_libraries(exrmetrics OpenEXR::OpenEXR)
set_target_properties(exrmetrics PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
if(OPENEXR_INSTALL_TOOLS)
  install(TARGETS exrmetrics DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(WIN32 AND BUILD_SHARED_LIBS)
  target_compile_definitions(exrmetrics PRIVATE OPENEXR_DLL)
endif()
